/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    transcode_request_handler.cc - this file is part of MediaTomb.
    
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
    
    $Id: transcode_request_handler.cc $
*/

/// \file transcode_request_handler.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef TRANSCODING

#include "server.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "storage.h"
#include "cds_objects.h"
#include "process.h"
#include "update_manager.h"
#include "session_manager.h"
#include "ixml.h"
#include "transcode_process_io_handler.h"
#include "dictionary.h"
#include "transcode_request_handler.h"
#include "metadata_handler.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

TranscodeRequestHandler::TranscodeRequestHandler() : RequestHandler()
{
}

void TranscodeRequestHandler::get_info(IN const char *filename, OUT struct File_Info *info)
{
    log_debug("start\n");

    String mimeType;
    int objectID;

    struct stat statbuf;
    int ret = 0;
    bool is_srt = false;

    String url_path, parameters;
    split_url(filename, URL_PARAM_SEPARATOR, url_path, parameters);
    
    Ref<Dictionary> dict(new Dictionary());
    dict->decode(parameters);

    log_debug("full url (filename): %s, url_path: %s, parameters: %s\n",
               filename, url_path.c_str(), parameters.c_str());
    
    String objID = dict->get(_("object_id"));
    if (objID == nil)
    {
        //log_error("object_id not found in url\n");
        throw _Exception(_("get_info: object_id not found"));
    }
    else
        objectID = objID.toInt();

    //log_debug("got ObjectID: [%s]\n", object_id.c_str());

    Ref<Storage> storage = Storage::getInstance();

    Ref<CdsObject> obj = storage->loadObject(objectID);

    int objectType = obj->getObjectType();

    if (!IS_CDS_ITEM(objectType))
    {
        throw _Exception(_("get_info: object is not an item"));
    }
   
    Ref<CdsItem> item = RefCast(obj, CdsItem);

    String path = item->getLocation();

    String p_name = dict->get(_(URL_PARAM_TRANSCODE_PROFILE_NAME));
    if (!string_ok(p_name))
        throw _Exception(_("Transcoding of file ") + path +
                         " requested, but no profile specified");

    /// \todo make sure that blind .srt requests are processed correctly 
    /// in the transcode handler

    Ref<TranscodingProfile> tp = ConfigManager::getInstance()->getTranscodingProfileListOption(
                        CFG_TRANSCODING_PROFILE_LIST)->getByName(p_name);
    if (tp == nil)
        throw _Exception(_("Transcoding of file ") + path +
                           " but no profile matching the name " +
                           p_name + " found");
    
    ret = stat(path.c_str(), &statbuf);
    if (ret != 0)
    {
            throw _Exception(_("Failed to open ") + path + " - " + strerror(errno));

    }


    if (access(path.c_str(), R_OK) == 0)
    {
        info->is_readable = 1;
    }
    else
    {
        info->is_readable = 0;
    }

    String header;
    log_debug("path: %s\n", path.c_str());
    /// \todo what's up with the content disposition header for transcoded streams?
/*
    int slash_pos = path.rindex(DIR_SEPARATOR);
    if (slash_pos >= 0)
    {
        if (slash_pos < path.length()-1)
        {
            slash_pos++;


            header = _("Content-Disposition: attachment; filename=\"") + 
                     path.substring(slash_pos) + _("\"");
        }
    }
*/
    info->http_header = NULL;
    mimeType = tp->getTargetMimeType();
      
    info->file_length = -1;
    info->last_modified = statbuf.st_mtime;
    info->is_directory = S_ISDIR(statbuf.st_mode);
    info->content_type = ixmlCloneDOMString(mimeType.c_str());

   //    log_debug("get_info: Requested %s, ObjectID: %s, Location: %s\n, MimeType: %s\n",
//          filename, object_id.c_str(), path.c_str(), info->content_type);

    log_debug("web_get_info(): end\n");
}

Ref<IOHandler> TranscodeRequestHandler::open(IN const char *filename, OUT struct File_Info *info, IN enum UpnpOpenFileMode mode)
{
    int objectID;
    String mimeType;
    int ret;
//    bool is_srt = false;

    log_debug("start\n");
    struct stat statbuf;

    // Currently we explicitly do not support UPNP_WRITE
    // due to security reasons.
    if (mode != UPNP_READ)
        throw _Exception(_("UPNP_WRITE unsupported"));

    String url_path, parameters;
    split_url(filename, URL_PARAM_SEPARATOR, url_path, parameters);

    Ref<Dictionary> dict(new Dictionary());
    dict->decode(parameters);
    log_debug("full url (filename): %s, url_path: %s, parameters: %s\n",
               filename, url_path.c_str(), parameters.c_str());

    String objID = dict->get(_("object_id"));
    if (objID == nil)
    {
        throw _Exception(_("object_id not found"));
    }
    else
        objectID = objID.toInt();

    log_debug("Opening media file with object id %d\n", objectID);
    Ref<Storage> storage = Storage::getInstance();

    Ref<CdsObject> obj = storage->loadObject(objectID);

    int objectType = obj->getObjectType();

    if (!IS_CDS_ITEM(objectType))
    {
        throw _Exception(_("object is not an item"));
    }

    // update item info by running action
    if (IS_CDS_ACTIVE_ITEM(objectType)) // check - if thumbnails, then no action, just show
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);

        Ref<Element> inputElement = UpnpXML_DIDLRenderObject(obj, true);

        inputElement->setAttribute(_(XML_DC_NAMESPACE_ATTR), _(XML_DC_NAMESPACE));
        inputElement->setAttribute(_(XML_UPNP_NAMESPACE_ATTR), _(XML_UPNP_NAMESPACE));
        String action = aitem->getAction();
        String input = inputElement->print();
        String output;

        log_debug("Script input: %s\n", input.c_str());
        if(strncmp(action.c_str(), "http://", 7))
        {
#ifdef LOG_TOMBDEBUG
            struct timespec before;
            getTimespecNow(&before);
#endif
            output = run_simple_process(action, _("run"), input);
#ifdef LOG_TOMBDEBUG
            long delta = getDeltaMillis(&before);
            log_debug("script executed in %ld milliseconds\n", delta);
#endif
        }
        else
        {
            /// \todo actually fetch...
            log_debug("fetching %s\n", action.c_str());
            output = input;
        }
        log_debug("Script output: %s\n", output.c_str());

        Ref<CdsObject> clone = CdsObject::createObject(objectType);
        aitem->copyTo(clone);

        UpnpXML_DIDLUpdateObject(clone, output);

        if (! aitem->equals(clone, true)) // check for all differences
        {
            Ref<UpdateManager> um = UpdateManager::getInstance();
            Ref<SessionManager> sm = SessionManager::getInstance();

            log_debug("Item changed, updating database\n");
            int containerChanged = INVALID_OBJECT_ID;
            storage->updateObject(clone, &containerChanged);
            um->containerChanged(containerChanged);
            sm->containerChangedUI(containerChanged);

            if (! aitem->equals(clone)) // check for visible differences
            {
                log_debug("Item changed visually, updating parent\n");
                um->containerChanged(clone->getParentID(), FLUSH_ASAP);
            }
            obj = clone;
        }
        else
        {
            log_debug("Item untouched...\n");
        }
    }

    Ref<CdsItem> item = RefCast(obj, CdsItem);

    String path = item->getLocation();

    // ok, that's a little tricky... we are doing transcoding, so the
    // resource id that we receive in the URL is not in the database
    // 
    // instead of using the res_id we have to look up the profile name in 
    // the configuration and see if we can find a match, we will then put
    // together our option string, setup the location of the data and launch
    // the transoder
    String p_name = dict->get(_(URL_PARAM_TRANSCODE_PROFILE_NAME));
    if (!string_ok(p_name))
        throw _Exception(_("Transcoding of file ") + path + 
                         " requested, but no profile specified");

    /// \todo make sure that blind .srt requests are processed correctly 
    /// in the transcode handler

    Ref<TranscodingProfile> tp = ConfigManager::getInstance()->getTranscodingProfileListOption(
                        CFG_TRANSCODING_PROFILE_LIST)->getByName(p_name);
    if (tp == nil)
        throw _Exception(_("Transcoding of file ") + path +
                           " but no profile matching the name " +
                           p_name + " found");

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0)
        throw _Exception(_("Failed to open ") + path + " - " + strerror(errno));

    if (access(path.c_str(), R_OK) == 0)
    {
        info->is_readable = 1;
    }
    else
    {
        info->is_readable = 0;
    }

//    String header;

    info->last_modified = statbuf.st_mtime;
    info->is_directory = S_ISDIR(statbuf.st_mode);

    log_debug("path: %s\n", path.c_str());
    /// \todo what's up with content disposition header for transcoded streams?
/*    int slash_pos = path.rindex(DIR_SEPARATOR);
    if (slash_pos >= 0)
    {
        if (slash_pos < path.length()-1)
        {
            slash_pos++;


            header = _("Content-Disposition: attachment; filename=\"") + 
                     path.substring(slash_pos) + _("\"");
        }
    }
    */

    info->http_header = NULL;
    mimeType = tp->getTargetMimeType();

    info->content_type = ixmlCloneDOMString(mimeType.c_str());
    info->file_length = -1;


    // the real deal should proabbyl be here...
    // I think we can use the FileIOHandler to read from the fifo
    /// \todo define architecture for transcoding

    String fifo_name = String(tempnam("/tmp/", "mt_tr"));
    String arguments;
    String temp;
    String command;
#define MAX_ARGS 128
    char *argv[MAX_ARGS];
    Ref<Array<StringBase> > arglist;
    int i;
    int apos = 0;
    printf("creating fifo: %s\n", fifo_name.c_str());
    if (mkfifo(fifo_name.c_str(), O_RDWR) == -1) 
    {
        log_error("Failed to create fifo for the transcoding process!: %s\n", strerror(errno));
        throw _Exception(_("Could not create fifo!\n"));
    }

    chmod(fifo_name.c_str(), S_IWOTH | S_IWGRP | S_IWUSR | S_IRUSR);

    transcoding_process = fork();
    switch (transcoding_process)
    {
        case -1:
            throw _Exception(_("Fork failed when launching transcoding process!"));
        case 0:
            // "safe" version, code below does not work yet
            // still checking in so Leo can have a look
            printf("FORKING NOW!!!!!!!!!!!\n");
            /// \todo this is only test code, to be removed !!
            arglist = parseCommandLine(tp->getArguments(), path, fifo_name);
            command = tp->getCommand();
            argv[0] = command.c_str();
            apos = 0;

                for (i = 0; i < arglist->size(); i++)
                {
                    argv[++apos] = arglist->get(i)->data; 
                    if (apos >= MAX_ARGS-1)
                        break;
                }

            argv[++apos] = NULL;

            i = 0;
            printf("ARGLIST: ");
            do
            {
                printf("%s ", argv[i]);
                i++;
            }
            while (argv[i] != NULL);
          
            printf("\n");
            printf("EXECUTING COMMAND!\n");
            execvp(command.c_str(), argv);
        default:
            break;
    }

    printf("TRANSCODING PROCESS: %d\n", transcoding_process);
    Ref<IOHandler> io_handler(new TranscodeProcessIOHandler(fifo_name, 
                transcoding_process));
    io_handler->open(mode);
    return io_handler;
}

#endif//TRANSCODING
