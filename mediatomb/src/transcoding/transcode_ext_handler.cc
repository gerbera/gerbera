/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    transcode_ext_handler.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef EXTERNAL_TRANSCODING

#include "transcode_ext_handler.h"
#include "server.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include "common.h"
#include "storage.h"
#include "cds_objects.h"
#include "process.h"
#include "update_manager.h"
#include "session_manager.h"
#include "ixml.h"
#include "process_io_handler.h"
#include "buffered_io_handler.h"
#include "dictionary.h"
#include "metadata_handler.h"
#include "tools.h"
#include "file_io_handler.h"
#include "transcoding_process_executor.h"
#include "io_handler_chainer.h"

#ifdef HAVE_CURL
    #include "curl_io_handler.h"
#endif

using namespace zmm;

TranscodeExternalHandler::TranscodeExternalHandler() : TranscodeHandler()
{
}

Ref<IOHandler> TranscodeExternalHandler::open(Ref<TranscodingProfile> profile, 
                                              String location, 
                                              int objectType, 
                                              struct File_Info *info)
{
    bool isURL = false;
//    bool is_srt = false;

    log_debug("start\n");
    char fifo_template[]="mt_transcode_XXXXXX";

    if (profile == nil)
        throw _Exception(_("Transcoding of file ") + location +
                           "requested but no profile given");
    
    isURL = (IS_CDS_ITEM_INTERNAL_URL(objectType) ||
            IS_CDS_ITEM_EXTERNAL_URL(objectType));

    String mimeType = profile->getTargetMimeType();

    info->content_type = ixmlCloneDOMString(mimeType.c_str());
    
    info->file_length = UNKNOWN_CONTENT_LENGTH;

    Ref<ConfigManager> cfg = ConfigManager::getInstance();
   
    String fifo_name = tempName(cfg->getOption(CFG_SERVER_TMPDIR), 
                                fifo_template);
    String arguments;
    String temp;
    String command;
    Ref<Array<StringBase> > arglist;
    Ref<Array<ProcListItem> > proc_list = nil; 

    if (isURL && (!profile->acceptURL()))
    {
#ifdef HAVE_CURL
        String url = location;
        strcpy(fifo_template, "mt_transcode_XXXXXX");
        location = tempName(cfg->getOption(CFG_SERVER_TMPDIR), fifo_template);
        log_debug("creating reader fifo: %s\n", location.c_str());
        if (mkfifo(location.c_str(), O_RDWR) == -1)
        {
            log_error("Failed to create fifo for the remote content reading thread: %s\n", strerror(errno));
            throw _Exception(_("Could not create reader fifo!\n"));
        }

        try
        {
            chmod(location.c_str(), S_IWUSR | S_IRUSR);

            Ref<IOHandler> c_ioh(new CurlIOHandler(url, NULL, 
                  cfg->getIntOption(CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE),
                  cfg->getIntOption(CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE)));
            
            Ref<IOHandler> p_ioh(new ProcessIOHandler(location, nil));
            Ref<Executor> ch(new IOHandlerChainer(c_ioh, p_ioh, 16384));
            proc_list = Ref<Array<ProcListItem> >(new Array<ProcListItem>(1));
            Ref<ProcListItem> pr_item(new ProcListItem(ch));
            proc_list->append(pr_item);
        }
        catch (Exception ex)
        {
            unlink(location.c_str());
            throw ex;
        }
#else
        throw _Exception(_("MediaTomb was compiled without libcurl support,"
                           "data proxying is not available"));
#endif

    }

    log_debug("creating fifo: %s\n", fifo_name.c_str());
    if (mkfifo(fifo_name.c_str(), O_RDWR) == -1) 
    {
        log_error("Failed to create fifo for the transcoding process!: %s\n", strerror(errno));
        throw _Exception(_("Could not create fifo!\n"));
    }
        
    chmod(fifo_name.c_str(), S_IWUSR | S_IRUSR);
   
    log_debug("Arguments: %s\n", profile->getArguments().c_str());
    arglist = parseCommandLine(profile->getArguments(), location, fifo_name);

    Ref<TranscodingProcessExecutor> main_proc(new TranscodingProcessExecutor(profile->getCommand(), arglist));
    main_proc->removeFile(fifo_name);
    if (isURL && (!profile->acceptURL()))
    {
        main_proc->removeFile(location);
    }
    
    Ref<IOHandler> io_handler(new BufferedIOHandler(Ref<IOHandler> (new ProcessIOHandler(fifo_name, RefCast(main_proc, Executor), proc_list)), profile->getBufferSize(), profile->getBufferChunkSize(), profile->getBufferInitialFillSize()));

    io_handler->open(UPNP_READ);
    return io_handler;
}

#endif//EXTERNAL_TRANSCODING
