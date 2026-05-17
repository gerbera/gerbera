/*MT*

    MediaTomb - http://www.mediatomb.cc/

    transcode_ext_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file transcoding/transcode_ext_handler.cc
#define GRB_LOG_FAC GrbLogFacility::transcoding

#include "transcode_ext_handler.h" // API

#include "cds/cds_objects.h"
#include "config/config.h"
#include "config/config_val.h"
#include "config/result/transcoding.h"
#include "content/content.h"
#include "exceptions.h"
#include "iohandler/buffered_io_handler.h"
#include "iohandler/io_handler_chainer.h"
#include "iohandler/process_io_handler.h"
#include "util/process_executor.h"
#include "util/tools.h"
#include "web/session_manager.h"

#ifdef HAVE_CURL
#include "iohandler/curl_io_handler.h"
#endif

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

std::unique_ptr<IOHandler> TranscodeExternalHandler::serveContent(
    const std::shared_ptr<TranscodingProfile>& profile,
    const fs::path& location,
    const std::shared_ptr<CdsObject>& obj,
    const std::string& group,
    const std::string& range)
{
    log_debug("Start transcoding file: {}", location.c_str());
    if (!profile)
        throw_std_runtime_error("Transcoding of file {} requested but no profile given", location.c_str());

    std::vector<ProcListItem> procList;
    fs::path inLocation = location;

    bool isURL = obj->isExternalItem();
    if (isURL && !profile->getAcceptURL()) {
#ifdef HAVE_CURL
        inLocation = openCurlFifo(location, procList);
#else
        throw_std_runtime_error("Compiled without libcurl support, data proxying for profile {} is not available", profile->getName());
#endif
    }

    checkTranscoder(profile);
    fs::path fifoName = makeFifo();

    std::vector<std::string> arglist = populateCommandLine(profile->agent.getArguments(), inLocation, fifoName, range, obj->getTitle());

    log_debug("Running profile command: '{}', arguments: '{}'", profile->agent.getCommand().c_str(), fmt::to_string(fmt::join(arglist, " ")));

    std::vector<fs::path> tempFiles;
    tempFiles.push_back(fifoName);
    if (isURL && !profile->getAcceptURL()) {
        tempFiles.push_back(std::move(inLocation));
    }
    auto mainProc = std::make_shared<ProcessExecutor>(profile->agent.getCommand(), arglist, profile->getEnviron(), tempFiles);

    content->triggerPlayHook(group, obj);

    auto processIoHandler = std::make_unique<ProcessIOHandler>(content, std::move(fifoName), std::move(mainProc), profile->buffer.getTimeout(), profile->buffer.getRetryCount(), std::move(procList));
    return std::make_unique<BufferedIOHandler>(config, std::move(processIoHandler), profile->buffer.getSize(), profile->buffer.getChunkSize(), profile->buffer.getInitialFillSize());
}

fs::path TranscodeExternalHandler::makeFifo()
{
    fs::path tmpDir = config->getOption(ConfigVal::SERVER_TMPDIR);
    auto fifoPath = tmpDir / fmt::format("grb-tr-{}", generateRandomId());

    log_debug("Creating FIFO: {}", fifoPath.string());
    int err = mkfifo(fifoPath.c_str(), O_RDWR);
    if (err != 0) {
        throw_fmt_system_error("Failed to create FIFO for the transcoding process!");
    }

    err = chmod(fifoPath.c_str(), S_IWUSR | S_IRUSR);
    if (err != 0) {
        log_error("Failed to change location permissions: {}", std::strerror(errno));
    }
    return fifoPath;
}

void TranscodeExternalHandler::checkTranscoder(const std::shared_ptr<TranscodingProfile>& profile)
{
    GrbFile check(profile->agent.getCommand());
    if (check.isAbsolute()) {
        std::error_code ec;
        if (!isRegularFile(check.getPath(), ec))
            throw_std_runtime_error("Could not find transcoder: {}", profile->agent.getCommand().c_str());
    } else {
        if (!check.findInPath())
            throw_std_runtime_error("Could not find transcoder {} in $PATH", profile->agent.getCommand().c_str());
    }

    int err = 0;
    if (!check.isExecutable(err))
        throw_std_runtime_error("Transcoder {} is not executable: {}", profile->agent.getCommand().c_str(), std::strerror(err));
}

std::vector<std::string> TranscodeExternalHandler::populateCommandLine(
    const std::string& line,
    const std::string& in,
    const std::string& out,
    const std::string& range,
    const std::string& title)
{
    log_debug("Template: '{}', in: '{}', out: '{}', range: '{}', title: '{}'", line, in, out, range, title);
    std::vector<std::string> params = splitString(line, ' ', '"');
    if (in.empty() && out.empty())
        return params;

    for (auto&& param : params) {
        auto inPos = param.find("%in");
        if (inPos != std::string::npos) {
            std::string newParam = param.replace(inPos, 3, in);
        }

        auto outPos = param.find("%out");
        if (outPos != std::string::npos) {
            std::string newParam = param.replace(outPos, 4, out);
        }

        auto rangePos = param.find("%range");
        if (rangePos != std::string::npos) {
            std::string newParam = param.replace(rangePos, 6, range);
        }

        auto titlePos = param.find("%title");
        if (titlePos != std::string::npos) {
            std::string newParam = param.replace(titlePos, 6, title);
        }
    }
    return params;
}

#ifdef HAVE_CURL
fs::path TranscodeExternalHandler::openCurlFifo(const fs::path& location, std::vector<ProcListItem>& procList)
{
    std::string url = location;
    log_debug("creating reader fifo: {}", location.c_str());
    auto ret = makeFifo();

    try {
        auto cIoh = std::make_unique<CurlIOHandler>(config, url,
            config->getIntOption(ConfigVal::EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE),
            config->getIntOption(ConfigVal::EXTERNAL_TRANSCODING_CURL_FILL_SIZE),
            std::chrono::seconds(config->getLongOption(ConfigVal::URL_REQUEST_CURL_CONNECT_TIMEOUT)),
            std::chrono::seconds(config->getLongOption(ConfigVal::URL_REQUEST_CURL_TIMEOUT)));
        auto pIoh = std::make_unique<ProcessIOHandler>(content, ret, nullptr,
            std::chrono::seconds(config->getLongOption(ConfigVal::EXTERNAL_TRANSCODING_CURL_BUFFER_TIMEOUT)),
            config->getUIntOption(ConfigVal::EXTERNAL_TRANSCODING_CURL_BUFFER_RETRY_COUNT));
        auto ch = std::make_unique<IOHandlerChainer>(std::move(cIoh), std::move(pIoh), config->getUIntOption(ConfigVal::EXTERNAL_TRANSCODING_CURL_CHUNK_SIZE));
        procList.emplace_back(std::move(ch));
    } catch (const std::runtime_error&) {
        unlink(ret.c_str());
        throw;
    }

    return ret;
}
#endif
