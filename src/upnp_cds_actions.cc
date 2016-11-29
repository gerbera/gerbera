/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    upnp_cds_actions.cc - this file is part of MediaTomb.
    
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

/// \file upnp_cds_actions.cc

#include "upnp_cds.h"
#include "server.h"
#include "storage.h"

using namespace zmm;
using namespace mxml;

void ContentDirectoryService::upnp_action_Browse(Ref<ActionRequest> request)
{
    log_debug("start\n");
    Ref<Storage> storage = Storage::getInstance();
   
    Ref<Element> req = request->getRequest();
   
    String objID = req->getChildText(_("ObjectID"));
    int objectID;
    String BrowseFlag = req->getChildText(_("BrowseFlag"));
    //String Filter; // not yet supported
    String StartingIndex = req->getChildText(_("StartingIndex"));
    String RequestedCount = req->getChildText(_("RequestedCount"));
    // String SortCriteria; // not yet supported

    //log_debug("Browse received parameters: ObjectID [%s] BrowseFlag [%s] StartingIndex [%s] RequestedCount [%s]\n",
//            ObjectID.c_str(), BrowseFlag.c_str(), StartingIndex.c_str(), RequestedCount.c_str());
   

    if (objID == nil)
        throw UpnpException(UPNP_E_NO_SUCH_ID, _("empty object id"));
    else
        objectID = objID.toInt();
    
    unsigned int flag = BROWSE_ITEMS | BROWSE_CONTAINERS | BROWSE_EXACT_CHILDCOUNT;
    
    if(BrowseFlag == "BrowseDirectChildren")
        flag |= BROWSE_DIRECT_CHILDREN;
    else if (BrowseFlag != "BrowseMetadata")
        throw UpnpException(UPNP_SOAP_E_INVALID_ARGS,
                            _("invalid browse flag: ") + BrowseFlag);

    Ref<CdsObject> parent = storage->loadObject(objectID);
    if ((parent->getClass() == UPNP_DEFAULT_CLASS_MUSIC_ALBUM) ||
        (parent->getClass() == UPNP_DEFAULT_CLASS_PLAYLIST_CONTAINER))
        flag |= BROWSE_TRACK_SORT;

    if (ConfigManager::getInstance()->getBoolOption(CFG_SERVER_HIDE_PC_DIRECTORY))
         flag |= BROWSE_HIDE_FS_ROOT;

    Ref<BrowseParam> param(new BrowseParam(objectID, flag));

    param->setStartingIndex(StartingIndex.toInt());
    param->setRequestedCount(RequestedCount.toInt());
    
    Ref<Array<CdsObject> > arr;
   
    try
    {
        arr = storage->browse(param);
    }
    catch (Exception e)
    {
        throw UpnpException(UPNP_E_NO_SUCH_ID, _("no such object"));
    }

    Ref<Element> didl_lite (new Element(_("DIDL-Lite")));
    didl_lite->setAttribute(_(XML_NAMESPACE_ATTR), 
                            _(XML_DIDL_LITE_NAMESPACE));
    didl_lite->setAttribute(_(XML_DC_NAMESPACE_ATTR), 
                            _(XML_DC_NAMESPACE));
    didl_lite->setAttribute(_(XML_UPNP_NAMESPACE_ATTR), 
                            _(XML_UPNP_NAMESPACE));

    Ref<ConfigManager> cfg = ConfigManager::getInstance();

#ifdef EXTEND_PROTOCOLINFO
    if (cfg->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO_SM_HACK))
    {
        didl_lite->setAttribute(_(XML_SEC_NAMESPACE_ATTR),
                                _(XML_SEC_NAMESPACE));
    }
#endif

    for(int i = 0; i < arr->size(); i++)
    {
        Ref<CdsObject> obj = arr->get(i);
        if (cfg->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED) &&
            obj->getFlag(OBJECT_FLAG_PLAYED))
        {
            String title = obj->getTitle();
            if (cfg->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND))
                title = cfg->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING) + title;
            else
                title = title + cfg->getOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING);

            obj->setTitle(title);
        }

        Ref<Element> didl_object = UpnpXML_DIDLRenderObject(obj, false, stringLimit);

        didl_lite->appendElementChild(didl_object);
    }

    Ref<Element> response;
    response = UpnpXML_CreateResponse(request->getActionName(), serviceType);

    response->appendTextChild(_("Result"), didl_lite->print());
    response->appendTextChild(_("NumberReturned"), String::from(arr->size()));
    response->appendTextChild(_("TotalMatches"), String::from(param->getTotalMatches()));
    response->appendTextChild(_("UpdateID"), String::from(systemUpdateID));

    request->setResponse(response);
    log_debug("end\n");
}

void ContentDirectoryService::upnp_action_GetSearchCapabilities(Ref<ActionRequest> request)
{
    log_debug("start\n");

    Ref<Element> response;
    response = UpnpXML_CreateResponse(request->getActionName(), serviceType);
    response->appendTextChild(_("SearchCaps"), _(""));
            
    request->setResponse(response);

    log_debug("end\n");
}

void ContentDirectoryService::upnp_action_GetSortCapabilities(Ref<ActionRequest> request)
{
    log_debug("start\n");

    Ref<Element> response;
    response = UpnpXML_CreateResponse(request->getActionName(), serviceType);
    response->appendTextChild(_("SortCaps"), _(""));
            
    request->setResponse(response);

    log_debug("end\n");
}

void ContentDirectoryService::upnp_action_GetSystemUpdateID(Ref<ActionRequest> request)
{
    log_debug("start\n");

    Ref<Element> response;
    response = UpnpXML_CreateResponse(request->getActionName(), serviceType);
    response->appendTextChild(_("Id"), String::from(systemUpdateID));

    request->setResponse(response);
    
    log_debug("end\n");
}

void ContentDirectoryService::process_action_request(Ref<ActionRequest> request)
{
    log_debug("start\n");

    if (request->getActionName() == "Browse")
    {
        upnp_action_Browse(request);
    }
    else if (request->getActionName() == "GetSearchCapabilities")
    {
        upnp_action_GetSearchCapabilities(request);
    }
    else if (request->getActionName() == "GetSortCapabilities")
    {
        upnp_action_GetSortCapabilities(request);
    }
    else if (request->getActionName() == "GetSystemUpdateID")
    {
        upnp_action_GetSystemUpdateID(request);
    }
    else
    {
        // invalid or unsupported action
        log_debug("unrecognized action %s\n",
                request->getActionName().c_str());
        request->setErrorCode(UPNP_E_INVALID_ACTION);
    //    throw UpnpException(UPNP_E_INVALID_ACTION, _("unrecognized action"));
    }

    log_debug("ContentDirectoryService::process_action_request: end\n");
}
