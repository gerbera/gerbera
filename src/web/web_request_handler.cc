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
#include "config/config_manager.h"
#include "content_manager.h"
#include "iohandler/mem_io_handler.h"
#include "util/tools.h"
#include "web/pages.h"
#include <ctime>
#include <util/headers.h>
#include "util/xml_to_json.h"

namespace web {

WebRequestHandler::WebRequestHandler(std::shared_ptr<ConfigManager> config,
    std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content,
    std::shared_ptr<SessionManager> sessionManager)
    : RequestHandler(config, storage)
    , content(content)
    , sessionManager(sessionManager)
    , checkRequestCalled(false)
{
}

int WebRequestHandler::intParam(std::string name, int invalid)
{
    std::string value = param(name);
    if (!string_ok(value))
        return invalid;
    else
        return std::stoi(value);
}

bool WebRequestHandler::boolParam(std::string name)
{
    std::string value = param(name);
    return string_ok(value) && (value == "1" || value == "true");
}

void WebRequestHandler::check_request(bool checkLogin)
{
    // we have a minimum set of parameters that are "must have"

    // check if the session parameter was supplied and if we have
    // a session with that id

    checkRequestCalled = true;

    std::string sid = param("sid");
    if (sid.empty())
        throw SessionException("no session id given");

    if ((session = sessionManager->getSession(sid)) == nullptr)
        throw SessionException("invalid session id");

    if (checkLogin && !session->isLoggedIn())
        throw LoginException("not logged in");
    session->access();
}

std::string WebRequestHandler::renderXMLHeader()
{
    return std::string(R"(<?xml version="1.0" encoding=")") + DEFAULT_INTERNAL_CHARSET + "\"?>\n";
}

void WebRequestHandler::getInfo(const char *filename, UpnpFileInfo *info)
{
    this->filename = filename;

    std::string path, parameters;
    splitUrl(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

    dict_decode(parameters, &params);

    UpnpFileInfo_set_FileLength(info, -1); // length is unknown

    UpnpFileInfo_set_LastModified(info, 0);
    UpnpFileInfo_set_IsDirectory(info, 0);
    UpnpFileInfo_set_IsReadable(info, 1);

    std::string contentType;

    std::string mimetype;
    std::string returnType = param("return_type");

    if (string_ok(returnType) && returnType == "xml")
        mimetype = MIMETYPE_XML;
    else
        mimetype = MIMETYPE_JSON;

    contentType = mimetype + "; charset=" + DEFAULT_INTERNAL_CHARSET;

    UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString(contentType.c_str()));
    Headers headers;
    headers.addHeader(std::string{"Cache-Control"}, std::string{"no-cache, must-revalidate"});
    headers.writeHeaders(info);
}

std::unique_ptr<IOHandler> WebRequestHandler::open(enum UpnpOpenFileMode mode)
{
    xmlDoc = std::make_shared<pugi::xml_document>();
    auto decl = xmlDoc->prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    auto root = xmlDoc->append_child("root");

    xml2JsonHints = std::make_shared<Xml2Json::Hints>();

    std::string error;
    int error_code = 0;

    std::string output;
    // processing page, creating output
    try {
        if (!config->getBoolOption(CFG_SERVER_UI_ENABLED)) {
            log_warning("The UI is disabled in the configuration file. See README.");
            error = "The UI is disabled in the configuration file. See README.";
            error_code = 900;
        } else {
            process();

            if (checkRequestCalled) {
                // add current task
                appendTask(content->getCurrentTask(), &root);

                handleUpdateIDs();
            }
        }
    } catch (const LoginException& e) {
        error = e.what();
        error_code = 300;
    } catch (const ObjectNotFoundException& e) {
        error = e.what();
        error_code = 200;
    } catch (const SessionException& e) {
        error = e.what();
        error_code = 400;
    } catch (const StorageException& e) {
        error = e.getUserMessage();
        error_code = 500;
    } catch (const std::runtime_error& e) {
        error = std::string{"Error: "} + e.what();
        error_code = 800;
    }

    if (!string_ok(error)) {
        root.append_attribute("success") = true;
    } else {
        root.append_attribute("success") = false;

        auto errorEl = root.append_child("error");
        errorEl.append_attribute("text") = error.c_str();

        if (error_code == 0)
            error_code = 899;
        errorEl.append_attribute("code") = error_code;

        log_warning("Web Error: {} {}", error_code, error);
    }

    std::string returnType = param("return_type");
    if (string_ok(returnType) && returnType == "xml") {
#ifdef TOMBDEBUG
        try {
            // make sure we can generate JSON w/o exceptions
            std::stringstream buf;
            xmlDoc->print(buf, "    ");
            output = Xml2Json::getJson(root, xml2JsonHints.get());
            log_debug("JSON-----------------------{}", output);
        } catch (const std::runtime_error& e) {
            log_error("Exception: {}", e.what());
        }
#endif
        std::stringstream buf;
        xmlDoc->print(buf, "  ");
        output = buf.str();
    } else {
        try {
#if 0
            // debug helper
            std::stringstream buf;
            xmlDoc->print(buf, "    ");
            output = buf.str();
            log_debug("XML-----------------------{}", output);
#endif
            output = Xml2Json::getJson(root, xml2JsonHints.get());
        } catch (const std::runtime_error& e) {
            log_error("Exception: {}", e.what());
        }
    }

    log_debug("output-----------------------{}", output);

    auto io_handler = std::make_unique<MemIOHandler>(output);
    io_handler->open(mode);
    return io_handler;
}

std::unique_ptr<IOHandler> WebRequestHandler::open(const char* filename,
    enum UpnpOpenFileMode mode,
    std::string range)
{
    this->filename = filename;
    this->mode = mode;

    std::string path, parameters;
    splitUrl(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

    dict_decode(parameters, &params);

    return open(mode);
}

void WebRequestHandler::handleUpdateIDs()
{
    // session will be filled by check_request
    std::string updates = param("updates");
    if (string_ok(updates)) {
        auto root = xmlDoc->document_element();
        auto updateIDs = root.append_child("update_ids");

        if (updates == "check") {
            updateIDs.append_attribute("pending") = session->hasUIUpdateIDs();
        } else if (updates == "get") {
            addUpdateIDs(session, &updateIDs);
        }
    }
}

void WebRequestHandler::addUpdateIDs(std::shared_ptr<Session> session, pugi::xml_node* updateIDsEl)
{
    std::string updateIDs = session->getUIUpdateIDs();
    if (string_ok(updateIDs)) {
        log_debug("UI: sending update ids: {}", updateIDs.c_str());
        updateIDsEl->append_attribute("ids") = updateIDs.c_str();
        updateIDsEl->append_attribute("updates") = true;
    }
}

void WebRequestHandler::appendTask(std::shared_ptr<GenericTask> task, pugi::xml_node* parent)
{
    if (task == nullptr || parent == nullptr)
        return;
    auto taskEl = parent->append_child("task");
    taskEl.append_attribute("id") = task->getID();
    taskEl.append_attribute("cancellable") = task->isCancellable();
    taskEl.append_attribute("text") = task->getDescription().c_str();
}

std::string WebRequestHandler::mapAutoscanType(int type)
{
    if (type == 1)
        return "ui";
    else if (type == 2)
        return "persistent";
    else
        return "none";
}

} // namespace web
