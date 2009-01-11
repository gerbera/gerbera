/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    web_request_handler.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2009 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file web_request_handler.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include <time.h>
#include "mem_io_handler.h"
#include "web_request_handler.h"
#include "config_manager.h"
#include "content_manager.h"
#include "web/pages.h"
#include "tools.h"
#include "hash.h"

using namespace zmm;
using namespace mxml;

WebRequestHandler::WebRequestHandler() : RequestHandler()
{
    checkRequestCalled = false;
    params = Ref<Dictionary>(new Dictionary());
}

int WebRequestHandler::intParam(String name, int invalid)
{
    String value = param(name);
    if (!string_ok(value))
        return invalid;
    else
        return value.toInt();
}

bool WebRequestHandler::boolParam(zmm::String name)
{
    String value = param(name);
    return string_ok(value) && (value == "1" || value == "true");
}

void WebRequestHandler::check_request(bool checkLogin)
{
    // we have a minimum set of parameters that are "must have"
    
    // check if the session parameter was supplied and if we have
    // a session with that id
    
    checkRequestCalled = true;
    
    String sid = param(_("sid"));
    if (sid == nil)
        throw SessionException(_("no session id given"));
    
    if ((session = SessionManager::getInstance()->getSession(sid)) == nil)
        throw SessionException(_("invalid session id"));
    
    if (checkLogin && ! session->isLoggedIn())
        throw LoginException(_("not logged in"));
    session->access();
}

String WebRequestHandler::renderXMLHeader()
{
    return _("<?xml version=\"1.0\" encoding=\"") +
            DEFAULT_INTERNAL_CHARSET +"\"?>\n";
}

void WebRequestHandler::get_info(IN const char *filename, OUT struct File_Info *info)
{
    info->file_length = -1; // length is unknown
    info->last_modified = time(NULL);
    info->is_directory = 0;
    info->is_readable = 1;
    
    String contentType;
    
    String mimetype;
    String returnType = param(_("return_type"));
    if (string_ok(returnType) && returnType == "xml")
        mimetype = _(MIMETYPE_XML);
    else
        mimetype = _(MIMETYPE_JSON);
    
    contentType = mimetype + "; charset=" + DEFAULT_INTERNAL_CHARSET;
    
    info->content_type = ixmlCloneDOMString(contentType.c_str());
    info->http_header = ixmlCloneDOMString("Cache-Control: no-cache, must-revalidate");
}

Ref<IOHandler> WebRequestHandler::open(IN enum UpnpOpenFileMode mode)
{
    root = Ref<Element>(new Element(_("root")));
    out = Ref<StringBuffer>(new StringBuffer());
    
    String error = nil;
    int error_code = 0;
    
    String output;
    // processing page, creating output
    try
    {
        if(!ConfigManager::getInstance()->getBoolOption(CFG_SERVER_UI_ENABLED))
        {
            log_warning("The UI is disabled in the configuration file. See README.\n");
            error = _("The UI is disabled in the configuration file. See README.");
            error_code = 900;
        }
        else
        {
            process();
            
            if (checkRequestCalled)
            {
                // add current task
                appendTask(root, ContentManager::getInstance()->getCurrentTask());
                
                handleUpdateIDs();
            }
        }
    }
    catch (LoginException e)
    {
        error = e.getMessage();
        error_code = 300;
    }
    catch (ObjectNotFoundException e)
    {
        error = e.getMessage();;
        error_code = 200;
    }
    catch (SessionException e)
    {
        error = e.getMessage();
        error_code = 400;
    }
    catch (StorageException e)
    {
        error = e.getUserMessage();
        error_code = 500;
        e.printStackTrace();
    }
    catch (Exception e)
    {
        error = _("Error: ") + e.getMessage();
        error_code = 800;
        e.printStackTrace();
    }
    
    if (! string_ok(error))
    {
        root->setAttribute(_("success"), _("1"), mxml_bool_type);
    }
    else
    {
        root->setAttribute(_("success"), _("0"), mxml_bool_type);
        Ref<Element> errorEl(new Element(_("error")));
        errorEl->setTextKey(_("text"));
        errorEl->setText(error);
        
        if (error_code == 0)
            error_code = 899;
        errorEl->setAttribute(_("code"), String::from(error_code));
        root->appendElementChild(errorEl);
    }
    
    String returnType = param(_("return_type"));
    if (string_ok(returnType) && returnType == "xml")
    {
#ifdef TOMBDEBUG
        try
        {
            // make sure we can generate JSON w/o exceptions
            XML2JSON::getJSON(root);
            //log_debug("JSON-----------------------\n\n\n%s\n\n\n\n", XML2JSON::getJSON(root).c_str());
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }
#endif
            output = renderXMLHeader() + root->print();
    }
    else
    {
        try
        {
            output = XML2JSON::getJSON(root);
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }
    }
    
    /*
    try
    {
        printf("%s\n", output.c_str());
        String json = XML2JSON::getJSON(root);
        printf("%s\n",json.c_str());
    }
    catch (Exception e)
    {
        e.printStackTrace();
    }
    */
    
    //root = nil;
    
    Ref<MemIOHandler> io_handler(new MemIOHandler(output));
    io_handler->open(mode);
    return RefCast(io_handler, IOHandler);
}

Ref<IOHandler> WebRequestHandler::open(IN const char *filename,
                                       OUT struct File_Info *info,
                                       IN enum UpnpOpenFileMode mode)
{
    log_debug("request: %s\n", filename);
    this->filename = String((char *)filename);
    this->mode = mode;

    String path, parameters;
    split_url(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

    params->decode(parameters);
    
    get_info(NULL, info);
    return open(mode);
}

void WebRequestHandler::handleUpdateIDs()
{
    // session will be filled by check_request
    
    String updates = param(_("updates"));
    if (string_ok(updates))
    {
        Ref<Element> updateIDs(new Element(_("update_ids")));
        root->appendElementChild(updateIDs);
        if (updates == "check")
        {
            updateIDs->setAttribute(_("pending"), session->hasUIUpdateIDs() ? _("1") : _("0"), mxml_bool_type);
        }
        else if (updates == "get")
        {
            addUpdateIDs(updateIDs, session);
        }
    }
}

void WebRequestHandler::addUpdateIDs(Ref<Element> updateIDsEl, Ref<Session> session)
{
    String updateIDs = session->getUIUpdateIDs();
    if (string_ok(updateIDs))
    {
        log_debug("UI: sending update ids: %s\n", updateIDs.c_str());
        updateIDsEl->setTextKey(_("ids"));
        updateIDsEl->setText(updateIDs);
        updateIDsEl->setAttribute(_("updates"), _("1"), mxml_bool_type);
    }
}

void WebRequestHandler::appendTask(Ref<Element> el, Ref<CMTask> task)
{
    if (task == nil || el == nil)
        return;
    Ref<Element> taskEl (new Element(_("task")));
    taskEl->setAttribute(_("id"), String::from(task->getID()), mxml_int_type);
    taskEl->setAttribute(_("cancellable"), task->isCancellable() ? _("1") : _("0"), mxml_bool_type);
    taskEl->setTextKey(_("text"));
    taskEl->setText(task->getDescription());
    el->appendElementChild(taskEl);
}

String WebRequestHandler::mapAutoscanType(int type)
{
    if (type == 1)
        return _("ui");
    else if (type == 2)
        return _("persistent");
    else
        return _("none");
}

int WebRequestHandler::remapAutoscanType(String type)
{
    if (type == "ui")
        return 1;
    else if (type == "persistent")
        return 2;
    else
        return 0;
}
