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

#include "transcode_ext_handler.h"
#include "server.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ixml.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <csignal>
#include <climits>
#include "common.h"
#include "storage.h"
#include "cds_objects.h"
#include "util/process.h"
#include "update_manager.h"
#include "session_manager.h"
#include "iohandler/process_io_handler.h"
#include "iohandler/buffered_io_handler.h"
#include "iohandler/file_io_handler.h"
#include "iohandler/io_handler_chainer.h"
#include "zmm/dictionary.h"
#include "metadata_handler.h"
#include "util/tools.h"
#include "transcoding_process_executor.h"
#include "play_hook.h"

#ifdef HAVE_CURL
    #include "iohandler/curl_io_handler.h"
#endif

using namespace zmm;

TranscodeExternalHandler::TranscodeExternalHandler() : TranscodeHandler()
{
}

Ref<IOHandler> TranscodeExternalHandler::open(Ref<TranscodingProfile> profile, 
                                              std::string location, 
                                              Ref<CdsObject> obj,
                                              std::string range)
{
    bool isURL = false;
//    bool is_srt = false;

    log_debug("start transcoding file: %s\n", location.c_str());
    char fifo_template[]="mt_transcode_XXXXXX";

    if (profile == nullptr)
        throw _Exception("Transcoding of file " + location +
                         "requested but no profile given");
    
    isURL = (IS_CDS_ITEM_INTERNAL_URL(obj->getObjectType()) ||
            IS_CDS_ITEM_EXTERNAL_URL(obj->getObjectType()));

    std::string mimeType = profile->getTargetMimeType();

    if (IS_CDS_ITEM(obj->getObjectType()))
    {
        Ref<CdsItem> it = RefCast(obj, CdsItem);
        Ref<Dictionary> mappings = ConfigManager::getInstance()->getDictionaryOption(
                CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);

        if (mappings->get(mimeType) == CONTENT_TYPE_PCM)
        {
            std::string freq = it->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY));
            std::string nrch = it->getResource(0)->getAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS));

            if (string_ok(freq)) 
                mimeType = mimeType + ";rate=" + freq;
            if (string_ok(nrch))
                mimeType = mimeType + ";channels=" + nrch;
        }
    }

    /* Upstream, move to getinfo?
    info->content_type = ixmlCloneDOMString(mimeType.c_str());
#ifdef EXTEND_PROTOCOLINFO
    std::string header;
    header = header + "TimeSeekRange.dlna.org: npt=" + range;

    log_debug("Adding TimeSeekRange response HEADERS: %s\n", header.c_str());
    header = getDLNAtransferHeader(mimeType, header);
    if (string_ok(header))
        info->http_header = ixmlCloneDOMString(header.c_str());
#endif

    info->file_length = UNKNOWN_CONTENT_LENGTH;
    info->force_chunked = (int)profile->getChunked();
    */

    Ref<ConfigManager> cfg = ConfigManager::getInstance();
   
    std::string fifo_name = normalizePath(tempName(cfg->getOption(CFG_SERVER_TMPDIR),
                                     fifo_template));
    std::string arguments;
    std::string temp;
    std::string command;
    std::vector<std::string> arglist;
    Ref<Array<ProcListItem> > proc_list = nullptr;

#ifdef SOPCAST
    service_type_t service = OS_None;
    if (obj->getFlag(OBJECT_FLAG_ONLINE_SERVICE))
    {
        service = (service_type_t)std::stoi(obj->getAuxData(ONLINE_SERVICE_AUX_ID));
    }
    
    if (service == OS_SopCast)
    {
        std::vector<std::string> sop_args;
        int p1 = find_local_port(45000,65500);
        int p2 = find_local_port(45000,65500);
        sop_args = parseCommandLine(location + " " + std::to_string(p1) + " " +
                   std::to_string(p2), nullptr, nullptr, nullptr);
        Ref<ProcessExecutor> spsc(new ProcessExecutor("sp-sc-auth", 
                                                      sop_args));
        proc_list = Ref<Array<ProcListItem> >(new Array<ProcListItem>(1));
        Ref<ProcListItem> pr_item(new ProcListItem(RefCast(spsc, Executor)));
        proc_list->append(pr_item);
        location = "http://localhost:" + std::to_string(p2) + "/tv.asf";

//FIXME: #warning check if socket is ready
        sleep(15); 
    }
//FIXME: #warning check if we can use "accept url" with sopcast
    else
    {
#endif
        if (isURL && (!profile->acceptURL()))
        {
#ifdef HAVE_CURL
            std::string url = location;
            strcpy(fifo_template, "mt_transcode_XXXXXX");
            location = normalizePath(tempName(cfg->getOption(CFG_SERVER_TMPDIR), fifo_template));
            log_debug("creating reader fifo: %s\n", location.c_str());
            if (mkfifo(location.c_str(), O_RDWR) == -1)
            {
                log_error("Failed to create fifo for the remote content "
                          "reading thread: %s\n", strerror(errno));
                throw _Exception("Could not create reader fifo!\n");
            }

            try
            {
                chmod(location.c_str(), S_IWUSR | S_IRUSR);

                Ref<IOHandler> c_ioh(new CurlIOHandler(url, nullptr, 
                   cfg->getIntOption(CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE),
                   cfg->getIntOption(CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE)));

                Ref<IOHandler> p_ioh(new ProcessIOHandler(location, nullptr));
                Ref<Executor> ch(new IOHandlerChainer(c_ioh, p_ioh, 16384));
                proc_list = Ref<Array<ProcListItem> >(new Array<ProcListItem>(1));
                Ref<ProcListItem> pr_item(new ProcListItem(ch));
                proc_list->append(pr_item);
            }
            catch (const Exception & ex)
            {
                unlink(location.c_str());
                throw ex;
            }
#else
            throw _Exception("MediaTomb was compiled without libcurl support,"
                             "data proxying is not available");
#endif

        }
#ifdef SOPCAST
    }
#endif

    std::string check;
    if (startswith(profile->getCommand(), _DIR_SEPARATOR))
    {
        if (!check_path(profile->getCommand()))
            throw _Exception("Could not find transcoder: " + 
                    profile->getCommand());

        check = profile->getCommand();
    }
    else
    {
        check = find_in_path(profile->getCommand());

        if (!string_ok(check))
            throw _Exception("Could not find transcoder " + 
                        profile->getCommand() + " in $PATH");

    }

    int err = 0;
    if (!is_executable(check, &err))
        throw _Exception("Transcoder " + profile->getCommand() + 
                " is not executable: " + strerror(err));

    log_debug("creating fifo: %s\n", fifo_name.c_str());
    if (mkfifo(fifo_name.c_str(), O_RDWR) == -1) 
    {
        log_error("Failed to create fifo for the transcoding process!: %s\n", strerror(errno));
        throw _Exception("Could not create fifo!\n");
    }
        
    chmod(fifo_name.c_str(), S_IWUSR | S_IRUSR);
   
    arglist = parseCommandLine(profile->getArguments(), location, fifo_name, range);

    log_debug("Command: %s\n", profile->getCommand().c_str());
    log_debug("Arguments: %s\n", profile->getArguments().c_str());
    Ref<TranscodingProcessExecutor> main_proc(new TranscodingProcessExecutor(profile->getCommand(), arglist));
    main_proc->removeFile(fifo_name);
    if (isURL && (!profile->acceptURL()))
    {
        main_proc->removeFile(location);
    }
    Ref<IOHandler> io_handler(new BufferedIOHandler(Ref<IOHandler> (new ProcessIOHandler(fifo_name, RefCast(main_proc, Executor), proc_list)), profile->getBufferSize(), profile->getBufferChunkSize(), profile->getBufferInitialFillSize()));

    io_handler->open(UPNP_READ);
    PlayHook::getInstance()->trigger(obj);
    return io_handler;
}

