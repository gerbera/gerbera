/*  browse.cc - this file is part of MediaTomb.
                                                                                
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
#include <stdio.h>
#include "common.h"
#include "storage.h"
#include "cds_objects.h"
#include "dictionary.h"
#include "pages.h"
#include "session_manager.h"
#include "config_manager.h"

using namespace zmm;
using namespace mxml;

web::browse::browse() : WebRequestHandler()
{
    pagename = _("browse");
}

/*
static Ref<Element> addAction(String action_type, String action_name)
{
    Ref<Element> action (new Element(action_type));
    action->addAttribute("req_type", action_name);
    return action;
}
*/

void web::browse::process()
{
    check_request();
    
    String type = param(_("type"));
    if (type == nil)
        type = _("database");
    root->addAttribute(_("type"), type);

/*
    int flag;

    Ref<Storage> storage; // storage instance, will be chosen depending on the driver
    Ref<Session> session; // current session
    session_data_t sd;    // the session carries two independand fields for storing data
            
    String objID = param("object_id");
    String browse_flag = param("browse_flag");
    String starting_index = param("starting_index");
    String requested_count = param("requested_count");
    String driver = param("driver");
    String sid = param("sid");

    // and check if they are ok, try to use default
    // values if something is missing
    
    session = SessionManager::getInstance()->getSession(sid);
  
    storage = Storage::getInstance();
    sd = PRIMARY;

    int objectID;
    
    // first check if an object_id was specified
    // -1 means: get it from the session
    if ((objID == nil) || (objID == "-1"))
    {
        objID = session->getFrom(sd, "object_id");
        if (objID == nil)
            objectID = 0;
        else
            objectID = objID.toInt();
    } 
    else
        objectID = objID.toInt();
  
    // if a valid object id was given, we save it
    // we will also be saving the other parameters
    // (index, count and so on)
    // the idea is, that when you switch between the
    // two drivers, you return back to where you 
    // you were before switching
    session->putTo(sd, "object_id", objID);

    // by default we use BROWSE_DIRECT_CHILDREN
    if (browse_flag == "BrowseMetadata")
        flag = BROWSE_METADATA;
    else 
        flag = BROWSE_DIRECT_CHILDREN;

    if ((starting_index == nil) || (starting_index == ""))
    {
        starting_index = session->getFrom(sd, "starting_index");
        if (starting_index == nil) starting_index = "0";
    } 
 
    // our xsl ui should handle this correctly... bit i did not 
    // figure that out yet.. anyway, lets check, just in case...
    if (starting_index.toInt() < 0)
        starting_index = "0";
   
    session->putTo(sd, "starting_index", starting_index);

    if ((requested_count == nil) || (requested_count == ""))
    {
        requested_count = session->getFrom(sd, "requested_count");
        if (requested_count == nil) requested_count = "0";
    }

    if (requested_count.toInt() < 0)
        requested_count = "0";
    
    session->putTo(sd, "requested_count", requested_count);

//    log_info(("browse:open - browsing object with id [%s]\n", object_id.c_str()));

    // ok, we validated all the parameters, now we are ready tobrowse
    Ref<BrowseParam> param(new BrowseParam(objectID, flag));
    param->setStartingIndex(starting_index.toInt());
    param->setRequestedCount(requested_count.toInt());
    
    Ref<Array<CdsObject> > arr = storage->browse(param);

    // we keep the browse result in the DIDL-Lite tag in our xml
    Ref<Element> didl_lite (new Element("DIDL-Lite"));

    for (int i = 0; i < arr->size(); i++)
    {
        Ref<CdsObject> obj = arr->get(i);
        // this has to be adjusted: the resourced for browsing are different
        Ref<Element> didl_object = UpnpXML_DIDLRenderObject(obj);

        didl_lite->appendChild(didl_object);
    }
   
    // and here comes all the additional stuff
    Ref<Element> current_browse (new Element("current_browse"));
    // object that we are currently browsing
    Ref<CdsObject> current = storage->loadObject(objectID);

    Ref<Element> didl_object = UpnpXML_DIDLRenderObject(current);

    // the path section will contain the "way" to the current container
    Ref<Element> path (new Element("path"));
    Ref<Array<CdsObject> > full_path = storage->getObjectPath(objectID);
    
    for (int i = 0; i < full_path->size(); i++)
    {
        Ref<CdsObject> obj = full_path->get(i);
        Ref<Element> container (new Element("container"));
        container->addAttribute("id", String::from(obj->getID()));
        container->appendTextChild("dc:title", obj->getTitle());
        path->appendChild(container);
    }
    
    current_browse->appendChild(didl_object);
    current_browse->appendChild(path);
    // give the ui back the driver that we are currently
    // and the session id, the ui then will return this data
    // back to us upon a request
    current_browse->appendTextChild("driver", driver);
    current_browse->appendTextChild("sid", sid);

    // this section is used for going to next/prev page
    Ref<Element> page (new Element("page"));
    page->appendTextChild("NumberReturned", _("") + arr->size());
    page->appendTextChild("TotalMatches", _("") + param->getTotalMatches());
    page->appendTextChild("LastStartingIndex", starting_index);
    page->appendTextChild("LastRequestedCount", requested_count);
    page->appendTextChild("CurrentIndex",  _("") + (starting_index.toInt() + arr->size())); 

    current_browse->appendChild(page);
    
    // all actions that are available will be stored here
    // the actions section has three subsections:
    // - current : actions available for the container that we are now browsing
    // - common : actions that are available for both - items and containers
    // - items  : actions that are available only for items
    // - containers : actions taht are available only for containers
    Ref<Element> actions (new Element("actions"));

    // depending on the driver we have different actions
    if (driver == "1") // browsing the server database 
    {
        actions->appendChild(addAction("current", "new"));
        if (objectID != 0)
            actions->appendChild(addAction("current", "edit_ui"));

        actions->appendChild(addAction("common", "remove"));
        actions->appendChild(addAction("common", "edit_ui"));

        // then add the stuff that is different.. currently we do not
        // have anything here
    }
    else // browsing the filesystem
    {

        actions->appendChild(addAction("current", "refresh"));
        actions->appendChild(addAction("common", "add"));
        // containers will have an option to add them with as watch
        // folders, but fam support is not yet implemented
    }

    current_browse->appendChild(actions);

    root->addAttribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
    root->addAttribute("xmlns:upnp", "urn:schemas-upnp-org:metadata-1-0/upnp/");
    
    root->appendChild(current_browse);
    root->appendChild(didl_lite);

    */
}

