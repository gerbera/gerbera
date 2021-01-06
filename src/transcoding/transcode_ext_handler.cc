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
#include <filesystem>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "content_manager.h"
#include "database/database.h"
#include "iohandler/buffered_io_handler.h"
#include "iohandler/file_io_handler.h"
#include "iohandler/io_handler_chainer.h"
#include "iohandler/process_io_handler.h"
#include "metadata/metadata_handler.h"
#include "server.h"
#include "transcoding_process_executor.h"
#include "update_manager.h"
#include "util/process.h"
#include "util/tools.h"
#include "web/session_manager.h"

#ifdef HAVE_CURL
#include "iohandler/curl_io_handler.h"
#endif

TranscodeExternalHandler::TranscodeExternalHandler(std::shared_ptr<Config> config,
    std::shared_ptr<ContentManager> content)
    : TranscodeHandler(std::move(config), std::move(content))
{
}

std::unique_ptr<IOHandler> TranscodeExternalHandler::serveContent(std::shared_ptr<TranscodingProfile> profile,
    std::string location,
    std::shared_ptr<CdsObject> obj,
    const std::string& range)
{
    log_debug("Start transcoding file: {}", location.c_str());

    if (profile == nullptr)
        throw_std_runtime_error("Transcoding of file " + location + "requested but no profile given");

    bool isURL = obj->isExternalItem();

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

    char fifo_template[] = "grb_transcode_XXXXXX";
    fs::path fifo_name = tempName(config->getOption(CFG_SERVER_TMPDIR), fifo_template);
    std::string arguments;
    std::string temp;
    std::string command;
    std::vector<std::string> arglist;
    std::vector<std::shared_ptr<ProcListItem>> proc_list;

#ifdef SOPCAST
    service_type_t service = OS_None;
    if (obj->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
        service = static_cast<service_type_t>(std::stoi(obj->getAuxData(ONLINE_SERVICE_AUX_ID)));
    }

    if (service == OS_SopCast) {
        std::vector<std::string> sop_args;
        int p1 = find_local_port(45000, 65500);
        int p2 = find_local_port(45000, 65500);
        sop_args = populateCommandLine(location + " " + std::to_string(p1) + " " + std::to_string(p2));
        auto spsc = std::make_shared<ProcessExecutor>("sp-sc-auth", sop_args);
        auto pr_item = std::make_shared<ProcListItem>(spsc);
        proc_list.push_back(pr_item);
        location = "http://localhost:" + std::to_string(p2) + "/tv.asf";

        //FIXME: #warning check if socket is ready
        sleep(15);
    }
    //FIXME: #warning check if we can use "accept url" with sopcast
    else {
#endif
        if (isURL && (!profile->acceptURL())) {
#ifdef HAVE_CURL
            std::string url = location;
            strcpy(fifo_template, "mt_transcode_XXXXXX");
            location = tempName(config->getOption(CFG_SERVER_TMPDIR), fifo_template);
            log_debug("creating reader fifo: {}", location.c_str());
            if (mkfifo(location.c_str(), O_RDWR) == -1) {
                log_error("Failed to create fifo for the remote content reading thread: {}", strerror(errno));
                throw_std_runtime_error("Could not create reader fifo");
            }

            try {
                chmod(location.c_str(), S_IWUSR | S_IRUSR);

                std::unique_ptr<IOHandler> c_ioh = std::make_unique<CurlIOHandler>(url, nullptr,
                    config->getIntOption(CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE),
                    config->getIntOption(CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE));
                std::unique_ptr<IOHandler> p_ioh = std::make_unique<ProcessIOHandler>(content, location, nullptr);
                auto ch = std::make_shared<IOHandlerChainer>(c_ioh, p_ioh, 16384);
                auto pr_item = std::make_shared<ProcListItem>(ch);
                proc_list.push_back(pr_item);
            } catch (const std::runtime_error& ex) {
                unlink(location.c_str());
                throw ex;
            }
#else
        throw_std_runtime_error("Compiled without libcurl support, data proxying is not available");
#endif
        }
#ifdef SOPCAST
    }
#endif

    fs::path check;
    if (profile->getCommand().is_absolute()) {
        std::error_code ec;
        if (!isRegularFile(profile->getCommand(), ec))
            throw_std_runtime_error("Could not find transcoder: " + profile->getCommand().string());

        check = profile->getCommand();
    } else {
        check = findInPath(profile->getCommand());

        if (check.empty())
            throw_std_runtime_error("Could not find transcoder " + profile->getCommand().string() + " in $PATH");
    }

    int err = 0;
    if (!isExecutable(check, &err))
        throw_std_runtime_error("Transcoder " + profile->getCommand().string() + " is not executable: " + strerror(err));

    log_debug("creating fifo: {}", fifo_name.c_str());
    if (mkfifo(fifo_name.c_str(), O_RDWR) == -1) {
        log_error("Failed to create fifo for the transcoding process!: {}", strerror(errno));
        throw_std_runtime_error("Could not create fifo");
    }

    chmod(fifo_name.c_str(), S_IWUSR | S_IRUSR);

    arglist = populateCommandLine(profile->getArguments(), location, fifo_name, range, obj->getTitle());

    log_debug("Command: {}", profile->getCommand().c_str());
    log_debug("Arguments: {}", profile->getArguments().c_str());
    auto main_proc = std::make_shared<TranscodingProcessExecutor>(profile->getCommand(), arglist);
    main_proc->removeFile(fifo_name);
    if (isURL && (!profile->acceptURL())) {
        main_proc->removeFile(location);
    }

    content->triggerPlayHook(obj);

    std::unique_ptr<IOHandler> u_ioh = std::make_unique<ProcessIOHandler>(content, fifo_name, main_proc, proc_list);
    auto io_handler = std::make_unique<BufferedIOHandler>(
        u_ioh,
        profile->getBufferSize(), profile->getBufferChunkSize(), profile->getBufferInitialFillSize());
    return io_handler;
}
