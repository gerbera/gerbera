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

#include "web_request_handler.h" // API

#include <ctime>
#include <utility>

#include "config/config_manager.h"
#include "content_manager.h"
#include "iohandler/mem_io_handler.h"
#include "util/tools.h"
#include "util/upnp_headers.h"
#include "util/xml_to_json.h"
#include "web/pages.h"

namespace web {

WebRequestHandler::WebRequestHandler(std::shared_ptr<ContentManager> content)
    : RequestHandler(std::move(content))
    , sessionManager(this->content->getSessionManager())
    , checkRequestCalled(false)
{
}

int WebRequestHandler::intParam(const std::string& name, int invalid)
{
    std::string value = param(name);
    return !value.empty() ? std::stoi(value) : invalid;
}

bool WebRequestHandler::boolParam(const std::string& name)
{
    std::string value = param(name);
    return !value.empty() && (value == "1" || value == "true");
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

void WebRequestHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    this->filename = filename;

    std::string path, parameters;
    splitUrl(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

    dictDecode(parameters, &params);

    UpnpFileInfo_set_FileLength(info, -1); // length is unknown

    UpnpFileInfo_set_LastModified(info, 0);
    UpnpFileInfo_set_IsDirectory(info, 0);
    UpnpFileInfo_set_IsReadable(info, 1);

    std::string returnType = param("return_type");
    std::string mimetype = (returnType == "xml") ? MIMETYPE_XML : MIMETYPE_JSON;
    std::string contentType = mimetype + "; charset=" + DEFAULT_INTERNAL_CHARSET;

#if defined(USING_NPUPNP)
    UpnpFileInfo_set_ContentType(info, contentType);
#else
    UpnpFileInfo_set_ContentType(info, ixmlCloneDOMString(contentType.c_str()));
#endif
    Headers headers;
    headers.addHeader(std::string { "Cache-Control" }, std::string { "no-cache, must-revalidate" });
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
    } catch (const DatabaseException& e) {
        error = e.getUserMessage();
        error_code = 500;
    } catch (const std::runtime_error& e) {
        error = std::string { "Error: " } + e.what();
        error_code = 800;
    }

    if (error.empty()) {
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
    if (returnType == "xml") {
#ifdef TOMBDEBUG
        try {
            // make sure we can generate JSON w/o exceptions
            std::ostringstream buf;
            xmlDoc->print(buf, "    ");
            output = Xml2Json::getJson(root, *xml2JsonHints);
            log_debug("JSON-----------------------{}", output);
        } catch (const std::runtime_error& e) {
            log_error("Exception: {}", e.what());
        }
#endif
        std::ostringstream buf;
        xmlDoc->print(buf, "  ");
        output = buf.str();
    } else {
        try {
#if 0
            // debug helper
            std::ostringstream buf;
            xmlDoc->print(buf, "    ");
            output = buf.str();
            log_debug("XML-----------------------{}", output);
#endif
            output = Xml2Json::getJson(root, *xml2JsonHints);
        } catch (const std::runtime_error& e) {
            log_error("Exception: {}", e.what());
        }
    }

    log_debug("output-----------------------{}", output);

    auto io_handler = std::make_unique<MemIOHandler>(output);
    io_handler->open(mode);
    return io_handler;
}

std::unique_ptr<IOHandler> WebRequestHandler::open(const char* filename, enum UpnpOpenFileMode mode)
{
    this->filename = filename;
    this->mode = mode;

    std::string path, parameters;
    splitUrl(filename, URL_UI_PARAM_SEPARATOR, path, parameters);

    dictDecode(parameters, &params);

    return open(mode);
}

void WebRequestHandler::handleUpdateIDs()
{
    // session will be filled by check_request
    std::string updates = param("updates");
    if (!updates.empty()) {
        auto root = xmlDoc->document_element();
        auto updateIDs = root.append_child("update_ids");

        if (updates == "check") {
            updateIDs.append_attribute("pending") = session->hasUIUpdateIDs();
        } else if (updates == "get") {
            addUpdateIDs(session, &updateIDs);
        }
    }
}

void WebRequestHandler::addUpdateIDs(const std::shared_ptr<Session>& session, pugi::xml_node* updateIDsEl)
{
    std::string updateIDs = session->getUIUpdateIDs();
    if (!updateIDs.empty()) {
        log_debug("UI: sending update ids: {}", updateIDs.c_str());
        updateIDsEl->append_attribute("ids") = updateIDs.c_str();
        updateIDsEl->append_attribute("updates") = true;
    }
}

void WebRequestHandler::appendTask(const std::shared_ptr<GenericTask>& task, pugi::xml_node* parent)
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
    switch (type) {
    case 1:
        return "ui";
    case 2:
        return "persistent";
    default:
        return "none";
    }
}

} // namespace web
