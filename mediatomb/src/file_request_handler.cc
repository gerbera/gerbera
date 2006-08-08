/*  file_request_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "server.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "storage.h"
#include "cds_objects.h"
#include "process.h"
#include "update_manager.h"
#include "ixml.h"
#include "file_io_handler.h"
#include "dictionary.h"
#include "file_request_handler.h"
#include "metadata_handler.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

FileRequestHandler::FileRequestHandler() : RequestHandler()
{
}

void FileRequestHandler::get_info(IN const char *filename, OUT struct File_Info *info)
{
    log_info(("FileRequestHandler::get_info start\n"));

    int objectID;

    struct stat statbuf;
    int ret = 0;

    String url_path, parameters;
    split_url(filename, url_path, parameters);
    
    Ref<Dictionary> dict(new Dictionary());
    dict->decode(parameters);

    log_debug(("full url (filename): %s, url_path: %s, parameters: %s\n",
               filename, url_path.c_str(), parameters.c_str()));
    
    String objID = dict->get(_("object_id"));
    if (objID == nil)
    {
        //log_info(("object_id not found in url\n"));
        throw Exception(_("get_info: object_id not found"));
    }
    else
        objectID = objID.toInt();

    //log_info(("got ObjectID: [%s]\n", object_id.c_str()));

    Ref<Storage> storage = Storage::getInstance();

    Ref<CdsObject> obj = storage->loadObject(objectID);

    int objectType = obj->getObjectType();

    if (!IS_CDS_ITEM(objectType))
    {
        throw Exception(_("get_info: object is not an item"));
    }

    // update item info by running action
    if (IS_CDS_ACTIVE_ITEM(objectType)) // check - if thumbnails, then no action, just show
    {
        Ref<CdsActiveItem> aitem = RefCast(obj, CdsActiveItem);

        Ref<Element> inputElement = UpnpXML_DIDLRenderObject(obj, true);


        String action = aitem->getAction();
        String input = inputElement->print();
        String output;

        log_info(("Script input: %s\n", input.c_str()));
        if(strncmp(action.c_str(), "http://", 7))
        {
            long before = getMillis();
            output = run_process(action, _("run"), input);
            long after = getMillis();
            log_info(("script executed in %ld milliseconds\n", after - before));
        }
        else
        {
            log_info(("fetching %s\n", action.c_str()));
            output = input;
        }
        log_info(("Script output: %s\n", output.c_str()));

        Ref<CdsObject> clone = CdsObject::createObject(objectType);
        aitem->copyTo(clone);

        UpnpXML_DIDLUpdateObject(clone, output);

        if (! aitem->equals(clone, true)) // check for all differences
        {
            log_info(("Item changed, updating database\n"));
            storage->updateObject(clone);
            if (! aitem->equals(clone)) // check for visible differences
            {
                log_info(("Item changed visually, updating parent\n"));
                Ref<UpdateManager> um = UpdateManager::getInstance();
                um->containerChanged(clone->getParentID());
                um->flushUpdates(FLUSH_ASAP);
            }
            obj = clone;
        }
        else
        {
            log_info(("Item untouched...\n"));
        }
    }

    Ref<CdsItem> item = RefCast(obj, CdsItem);

    String path = item->getLocation();

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0)
    {
        throw Exception(_("Failed to stat ") + path);
    }


    if (access(path.c_str(), R_OK) == 0)
    {
        info->is_readable = 1;
    }
    else
    {
        info->is_readable = 0;
    }

    String mimeType;
    /* determining which resource to serve */

    int res_id = 0;
    String s_res_id = dict->get(_(URL_RESOURCE_ID));
    if (s_res_id != nil)
        res_id = s_res_id.toInt();

    log_info(("FileRequestHandler: fetching resource id %d\n", res_id));
    if ((res_id > 0) && (res_id < item->getResourceCount()))
    {
        // http-get:*:image/jpeg:*
        String protocolInfo = item->getResource(res_id)->getAttributes()->get(_("protocolInfo"));
        if (protocolInfo != nil)
        {
            Ref<Array<StringBase> > parts = split_string(protocolInfo, ':');
            mimeType = parts->get(2);
        }
      
        log_info(("FileRequestHandler: setting content length to unknown\n"));
        /// \todo we could figure out the content length...
        info->file_length = 0;
        info->is_file_length_known = 0;
    }
    else
    {
        mimeType = item->getMimeType();
        info->file_length = statbuf.st_size;
        info->is_file_length_known = 1;
        //log_debug(("sizeof off_t %d, statbuf.st_size %d\n", sizeof(off_t), sizeof(statbuf.st_size)));
        //log_info(("FileRequestHandler: get_info: file_length: " LARGEFILE_SPRINTF "\n", statbuf.st_size));
    }
        
    info->last_modified = statbuf.st_mtime;
    info->is_directory = S_ISDIR(statbuf.st_mode);
   
    info->content_type = ixmlCloneDOMString(mimeType.c_str());

//    log_info(("get_info: Requested %s, ObjectID: %s, Location: %s\n, MimeType: %s\n",
//          filename, object_id.c_str(), path.c_str(), info->content_type));

//    log_info(("web_get_info(): end\n"));
}

Ref<IOHandler> FileRequestHandler::open(IN const char *filename, IN enum UpnpOpenFileMode mode)
{
    int objectID;

 //   log_info(("FileIOHandler web_open(): start\n"));

    // Currently we explicitly do not support UPNP_WRITE
    // due to security reasons.
    if (mode != UPNP_READ)
        throw Exception(_("UPNP_WRITE unsupported"));

    String url_path, parameters;
    split_url(filename, url_path, parameters);

    Ref<Dictionary> dict(new Dictionary());
    dict->decode(parameters);

    String objID = dict->get(_("object_id"));
    if (objID == nil)
    {
        throw Exception(_("object_id not found"));
    }
    else
        objectID = objID.toInt();

    Ref<Storage> storage = Storage::getInstance();

    log_info(("FileIOHandler web_open(): Opening media file with object id %d\n", objectID));

    Ref<CdsObject> obj = storage->loadObject(objectID);

    if (!IS_CDS_ITEM(obj->getObjectType()))
    {
        throw Exception(_("web_open: unsuitable object type: ") + obj->getObjectType());
    }

    Ref<CdsItem> item = RefCast(obj, CdsItem);

    String path = item->getLocation();

    /* determining which resource to serve */
    int res_id = 0;
    String s_res_id = dict->get(_(URL_RESOURCE_ID));
    if (s_res_id != nil)
        res_id = s_res_id.toInt();

    // Per default and in case of a bad resource ID, serve the file
    // itself

    if ((res_id > 0) && (res_id < item->getResourceCount()))
    {
        Ref<CdsResource> resource = item->getResource(res_id);
        Ref<MetadataHandler> h = MetadataHandler::createHandler(resource->getHandlerType());
        Ref<IOHandler> io_handler = h->serveContent(item, res_id);
        io_handler->open(mode);
        return io_handler;
    }
    else
    {
        Ref<IOHandler> io_handler(new FileIOHandler(path));
        io_handler->open(mode);
        return io_handler;
    }
}

