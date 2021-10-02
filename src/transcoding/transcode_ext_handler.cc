/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    transcode_ext_handler.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/// \file transcode_ext_handler.cc

#include "transcode_ext_handler.h" // API

#include <climits>
#include <csignal>
#include <cstdio>
#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "iohandler/buffered_io_handler.h"
#include "iohandler/file_io_handler.h"
#include "iohandler/io_handler_chainer.h"
#include "iohandler/process_io_handler.h"
#include "metadata/metadata_handler.h"
#include "transcoding_process_executor.h"
#include "util/process.h"
#include "util/tools.h"
#include "web/session_manager.h"

#ifdef HAVE_CURL
#include "iohandler/curl_io_handler.h"
#endif

TranscodeExternalHandler::TranscodeExternalHandler(std::shared_ptr<ContentManager> content)
    : TranscodeHandler(std::move(content))
{
}

std::unique_ptr<IOHandler> TranscodeExternalHandler::serveContent(std::shared_ptr<TranscodingProfile> profile,
    std::string location, std::shared_ptr<CdsObject> obj, std::string range)
{
    log_debug("Start transcoding file: {}", location);
    if (!profile)
        throw_std_runtime_error("Transcoding of file {} requested but no profile given", location);

#if 0
    std::string mimeType = profile->getTargetMimeType();
    if (obj->isItem()) {
        auto item = std::static_pointer_cast<CdsItem>(obj);
        auto mappings = config->getDictionaryOption(
            CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

        if (getValueOrDefault(mappings, mimeType) == CONTENT_TYPE_PCM) {
            std::string freq = item->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY));
            std::string nrch = item->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS));

            if (!freq.empty())
                mimeType = mimeType + ";rate=" + freq;
            if (!nrch.empty())
                mimeType = mimeType + ";channels=" + nrch;
        }
    }
#endif

    std::vector<std::shared_ptr<ProcListItem>> procList;
    bool isConnected = false;
#ifdef SOPCAST
    isConnected = startSopcastConnector(obj, location, procList);
#endif

    bool isURL = obj->isExternalItem();
    if (!isConnected && isURL && (!profile->acceptURL())) {
        //FIXME: #warning check if we can use "accept url" with sopcast
#ifdef HAVE_CURL
        openCurlFifo(location, procList);
#else
        throw_std_runtime_error("Compiled without libcurl support, data proxying for profile {} is not available", profile->getName());
#endif
    }

    checkTranscoder(profile);
    fs::path fifoName = makeFifo();

    std::vector<std::string> arglist = populateCommandLine(profile->getArguments(), location, fifoName, range, obj->getTitle());

    log_debug("Running profile command: '{}', arguments: '{}'", profile->getCommand().c_str(), fmt::to_string(fmt::join(arglist, " ")));
    auto mainProc = std::make_shared<TranscodingProcessExecutor>(profile->getCommand(), arglist);
    mainProc->removeFile(fifoName);
    if (isURL && (!profile->acceptURL())) {
        mainProc->removeFile(location);
    }

    content->triggerPlayHook(obj);

    auto uIoh = std::make_unique<ProcessIOHandler>(std::move(content), std::move(fifoName), std::move(mainProc), std::move(procList));
    return std::make_unique<BufferedIOHandler>(
        config, std::move(uIoh),
        profile->getBufferSize(), profile->getBufferChunkSize(), profile->getBufferInitialFillSize());
}

fs::path TranscodeExternalHandler::makeFifo()
{
    char fifoTemplate[] = "grb_transcode_XXXXXX";
    fs::path fifoName = tempName(config->getOption(CFG_SERVER_TMPDIR), fifoTemplate);
    log_debug("creating fifo: {}", fifoName.c_str());
    int err = mkfifo(fifoName.c_str(), O_RDWR);
    if (err != 0) {
        log_error("Failed to create fifo for the transcoding process!: {}", std::strerror(errno));
        throw_std_runtime_error("Could not create fifo");
    }

    err = chmod(fifoName.c_str(), S_IWUSR | S_IRUSR);
    if (err != 0) {
        log_error("Failed to change location permissions: {}", std::strerror(errno));
    }
    return fifoName;
}

void TranscodeExternalHandler::checkTranscoder(const std::shared_ptr<TranscodingProfile>& profile)
{
    fs::path check;
    if (profile->getCommand().is_absolute()) {
        std::error_code ec;
        if (!isRegularFile(profile->getCommand(), ec))
            throw_std_runtime_error("Could not find transcoder: {}", profile->getCommand().c_str());

        check = profile->getCommand();
    } else {
        check = findInPath(profile->getCommand());

        if (check.empty())
            throw_std_runtime_error("Could not find transcoder {} in $PATH", profile->getCommand().c_str());
    }

    int err = 0;
    if (!isExecutable(check, &err))
        throw_std_runtime_error("Transcoder {} is not executable: {}", profile->getCommand().c_str(), std::strerror(err));
}

#ifdef SOPCAST
bool TranscodeExternalHandler::startSopcastConnector(const std::shared_ptr<CdsObject>& obj, std::string& location, std::vector<std::shared_ptr<ProcListItem>>& procList)
{
    service_type_t service = OS_None;
    if (obj->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
        service = service_type_t(std::stoi(obj->getAuxData(ONLINE_SERVICE_AUX_ID)));
    }

    if (service == OS_SopCast) {
        std::vector<std::string> sopArgs;
        int p1 = find_local_port(45000, 65500);
        int p2 = find_local_port(45000, 65500);
        sopArgs = populateCommandLine(fmt::format("{} {} {}", location.c_str(), p1, p2));
        auto spsc = std::make_shared<ProcessExecutor>("sp-sc-auth", sopArgs);
        procList.push_back(std::make_shared<ProcListItem>(std::move(spsc)));
        location = fmt::format("http://localhost:{}/tv.asf", p2);

        //FIXME: #warning check if socket is ready
        sleep(15);
        return true;
    }
    return false;
}
#endif

#ifdef HAVE_CURL
void TranscodeExternalHandler::openCurlFifo(std::string& location, std::vector<std::shared_ptr<ProcListItem>>& procList)
{
    std::string url = location;
    char fifoTemplate[] = "grb_curl_transcode_XXXXXX";
    location = tempName(config->getOption(CFG_SERVER_TMPDIR), fifoTemplate);
    log_debug("creating reader fifo: {}", location.c_str());
    auto r = mkfifo(location.c_str(), O_RDWR);
    if (r != 0) {
        log_error("Failed to create fifo {} for the remote content reading thread: {}", location, std::strerror(errno));
        throw_std_runtime_error("Could not create reader fifo");
    }

    try {
        auto ret = chmod(location.c_str(), S_IWUSR | S_IRUSR);
        if (ret != 0) {
            log_error("Failed to change location {} permissions: {}", location, std::strerror(errno));
        }

        auto cIoh = std::make_unique<CurlIOHandler>(config, url, nullptr,
            config->getIntOption(CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE),
            config->getIntOption(CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE));
        auto pIoh = std::make_unique<ProcessIOHandler>(content, location, nullptr);
        auto ch = std::make_shared<IOHandlerChainer>(std::move(cIoh), std::move(pIoh), 16384);
        procList.push_back(std::make_shared<ProcListItem>(std::move(ch)));
    } catch (const std::runtime_error& ex) {
        unlink(location.c_str());
        throw ex;
    }
}
#endif
