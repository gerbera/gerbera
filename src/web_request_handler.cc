/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    web_request_handler.cc - this file is part of MediaTomb.
    
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

/// \file web_request_handler.cc

#include "web_request_handler.h"
#include "config_manager.h"
#include "content_manager.h"
#include "mem_io_handler.h"
#include "tools.h"
#include "web/pages.h"
#include <ctime>
#include <handler/headers.h>

using namespace zmm;
using namespace mxml;

WebRequestHandler::WebRequestHandler()
    : RequestHandler()
    , checkRequestCalled(false)
{
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
    if (sid == nullptr)
        throw SessionException(_("no session id given"));

    if ((session = SessionManager::getInstance()->getSession(sid)) == nullptr)
        throw SessionException(_("invalid session id"));

    if (checkLogin && !session->isLoggedIn())
        throw LoginException(_("not logged in"));
    session->access();
}

String WebRequestHandler::renderXMLHeader()
{
    return _("<?xml version=\"1.0\" encoding=\"") + DEFAULT_INTERNAL_CHARSET + "\"?>\n";
}

void WebRequestHandler::getInfo(IN const char *filename, OUT UpnpFileInfo *info)
{
    this->filename = filename;

    String path, parameters;
    splitUrl(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

    params->decode(parameters);

    UpnpFileInfo_set_FileLength(info, -1); // length is unknown

    UpnpFileInfo_set_LastModified(info, 0);
    UpnpFileInfo_set_IsDirectory(info, 0);
    UpnpFileInfo_set_IsReadable(info, 1);

    String contentType;

    String mimetype;
    String returnType = param(_("return_type"));

    if (string_ok(returnType) && returnType == "xml")
        mimetype = _(MIMETYPE_XML);
    else
        mimetype = _(MIMETYPE_JSON);

    contentType = mimetype + "; charset=" + DEFAULT_INTERNAL_CHARSET;

    UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString(contentType.c_str()));
    Headers headers;
    headers.addHeader(std::string{"Cache-Control: no-cache, must-revalidate"});
    headers.writeHeaders(info);
}

Ref<IOHandler> WebRequestHandler::open(IN enum UpnpOpenFileMode mode)
{
    root = Ref<Element>(new Element(_("root")));

    String error = nullptr;
    int error_code = 0;

    String output;
    // processing page, creating output
    try {
        if (!ConfigManager::getInstance()->getBoolOption(CFG_SERVER_UI_ENABLED)) {
            log_warning("The UI is disabled in the configuration file. See README.\n");
            error = _("The UI is disabled in the configuration file. See README.");
            error_code = 900;
        } else {
            process();

            if (checkRequestCalled) {
                // add current task
                appendTask(root, ContentManager::getInstance()->getCurrentTask());

                handleUpdateIDs();
            }
        }
    } catch (const LoginException& e) {
        error = e.getMessage();
        error_code = 300;
    } catch (const ObjectNotFoundException& e) {
        error = e.getMessage();
        error_code = 200;
    } catch (const SessionException& e) {
        error = e.getMessage();
        error_code = 400;
    } catch (const StorageException& e) {
        error = e.getUserMessage();
        error_code = 500;
        e.printStackTrace();
    } catch (const Exception& e) {
        error = _("Error: ") + e.getMessage();
        error_code = 800;
        e.printStackTrace();
    }

    if (!string_ok(error)) {
        root->setAttribute(_("success"), _("1"), mxml_bool_type);
    } else {
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
    if (string_ok(returnType) && returnType == "xml") {
#ifdef TOMBDEBUG
        try {
            // make sure we can generate JSON w/o exceptions
            XML2JSON::getJSON(root);
            //log_debug("JSON-----------------------\n\n\n%s\n\n\n\n", XML2JSON::getJSON(root).c_str());
        } catch (const Exception& e) {
            e.printStackTrace();
        }
#endif
        output = renderXMLHeader() + root->print();
    } else {
        try {
            output = XML2JSON::getJSON(root);
        } catch (const Exception& e) {
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
    catch (const Exception e)
    {
        e.printStackTrace();
    }
    */

    //root = nullptr;

    Ref<MemIOHandler> io_handler(new MemIOHandler(output));
    io_handler->open(mode);
    return RefCast(io_handler, IOHandler);
}

Ref<IOHandler> WebRequestHandler::open(IN const char* filename,
    IN enum UpnpOpenFileMode mode,
    IN String range)
{
    this->filename = filename;
    this->mode = mode;

    String path, parameters;
    splitUrl(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

    params->decode(parameters);

    return open(mode);
}

void WebRequestHandler::handleUpdateIDs()
{
    // session will be filled by check_request

    String updates = param(_("updates"));
    if (string_ok(updates)) {
        Ref<Element> updateIDs(new Element(_("update_ids")));
        root->appendElementChild(updateIDs);
        if (updates == "check") {
            updateIDs->setAttribute(_("pending"), session->hasUIUpdateIDs() ? _("1") : _("0"), mxml_bool_type);
        } else if (updates == "get") {
            addUpdateIDs(updateIDs, session);
        }
    }
}

void WebRequestHandler::addUpdateIDs(Ref<Element> updateIDsEl, Ref<Session> session)
{
    String updateIDs = session->getUIUpdateIDs();
    if (string_ok(updateIDs)) {
        log_debug("UI: sending update ids: %s\n", updateIDs.c_str());
        updateIDsEl->setTextKey(_("ids"));
        updateIDsEl->setText(updateIDs);
        updateIDsEl->setAttribute(_("updates"), _("1"), mxml_bool_type);
    }
}

void WebRequestHandler::appendTask(Ref<Element> el, Ref<GenericTask> task)
{
    if (task == nullptr || el == nullptr)
        return;
    Ref<Element> taskEl(new Element(_("task")));
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
