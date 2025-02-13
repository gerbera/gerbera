/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/web_request_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

/// \file web/web_request_handler.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "web_request_handler.h" // API

#include "cds/cds_enums.h"
#include "cds/cds_objects.h"
#include "cds/cds_resource.h"
#include "common.h"
#include "config/config.h"
#include "config/config_val.h"
#include "content/content.h"
#include "context.h"
#include "exceptions.h"
#include "iohandler/mem_io_handler.h"
#include "server.h"
#include "session_manager.h"
#include "upnp/compat.h"
#include "upnp/headers.h"
#include "upnp/quirks.h"
#include "util/generic_task.h"
#include "util/url_utils.h"
#include "util/xml_to_json.h"
#include "web/pages.h"

namespace Web {

WebRequestHandler::WebRequestHandler(const std::shared_ptr<Content>& content,
    std::shared_ptr<Server> server,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
    const std::shared_ptr<Quirks>& quirks)
    : RequestHandler(content, xmlBuilder, quirks)
    , sessionManager(this->content->getContext()->getSessionManager())
    , server(std::move(server))
{
}

std::string WebRequestHandler::getGroup() const
{
    return quirks ? quirks->getGroup() : DEFAULT_CLIENT_GROUP;
}

int WebRequestHandler::intParam(const std::string& name, int invalid) const
{
    std::string value = param(name);
    return !value.empty() ? std::stoi(value) : invalid;
}

bool WebRequestHandler::boolParam(const std::string& name) const
{
    std::string value = param(name);
    return !value.empty() && (value == "1" || value == "true");
}

void WebRequestHandler::checkRequest(bool checkLogin)
{
    // we have a minimum set of parameters that are "must have"

    // check if the session parameter was supplied and if we have
    // a session with that id

    checkRequestCalled = true;

    std::string sid = param(SID);
    if (sid.empty())
        throw SessionException("no session id given");

    if (!(session = sessionManager->getSession(sid)))
        throw SessionException("invalid session id");

    if (checkLogin && !session->isLoggedIn())
        throw LoginException("not logged in");
    session->access();
}

bool WebRequestHandler::getInfo(const char* filename, UpnpFileInfo* info)
{
    this->filename = filename;
    auto&& parameters = URLUtils::getQuery(this->filename);
    auto decodedParams = URLUtils::dictDecode(parameters);
    if (params.empty()) {
        params = std::move(decodedParams);
    } else {
        params.merge(decodedParams);
    }

    UpnpFileInfo_set_FileLength(info, -1); // length is unknown

    UpnpFileInfo_set_LastModified(info, currentTime().count()); // always use current time
    UpnpFileInfo_set_IsDirectory(info, 0);
    UpnpFileInfo_set_IsReadable(info, 1);

    std::string contentType = "application/json; charset=UTF-8";

    Headers headers;
    headers.addHeader("Cache-Control", "no-cache, must-revalidate");
    headers.addHeader("SameSite", "Lax");
    GrbUpnpFileInfoSetContentType(info, contentType);
#ifdef UPNP_NEEDS_CORS
    headers.addHeader("Access-Control-Allow-Origin", fmt::format("{}", fmt::join(server->getCorsHosts(), " ")));
#endif

    if (quirks)
        quirks->updateHeaders(headers);
    headers.writeHeaders(info);
    return quirks && quirks->getClient();
}

std::unique_ptr<IOHandler> WebRequestHandler::open(const char* filename, const std::shared_ptr<Quirks>& quirks, enum UpnpOpenFileMode mode)
{
    this->filename = filename;
    auto&& parameters = URLUtils::getQuery(this->filename);
    auto decodedParams = URLUtils::dictDecode(parameters);
    if (params.empty()) {
        params = std::move(decodedParams);
    } else {
        params.merge(decodedParams);
    }

    xmlDoc = std::make_unique<pugi::xml_document>();
    auto decl = xmlDoc->prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    auto root = xmlDoc->append_child("root");

    xml2Json = std::make_unique<Xml2Json>();

    std::string error;
    int errorCode = 0;

    std::string output;
    // processing page, creating output
    try {
        if (!config->getBoolOption(ConfigVal::SERVER_UI_ENABLED)) {
            log_warning("The UI is disabled in the configuration file. See README.");
            error = "The UI is disabled in the configuration file. See README.";
            errorCode = 900;
        } else {
            process();

            if (checkRequestCalled) {
                // add current task
                appendTask(content->getCurrentTask(), root);

                handleUpdateIDs();
            }
        }
    } catch (const LoginException& e) {
        error = e.what();
        errorCode = 300;
    } catch (const ObjectNotFoundException& e) {
        error = e.what();
        errorCode = 200;
    } catch (const SessionException& e) {
        error = e.what();
        errorCode = 400;
    } catch (const DatabaseException& e) {
        error = e.getUserMessage();
        errorCode = 500;
    } catch (const std::runtime_error& e) {
        error = fmt::format("Error: {}", e.what());
        errorCode = 800;
    }

    if (error.empty()) {
        root.append_attribute("success") = true;
    } else {
        root.append_attribute("success") = false;

        auto errorEl = root.append_child("error");
        errorEl.append_attribute("text") = error.c_str();

        if (errorCode == 0)
            errorCode = 899;
        errorEl.append_attribute("code") = errorCode;

        log_warning("Web Error: {} {}", errorCode, error);
    }

    try {
        output = xml2Json->getJson(root);
    } catch (const std::runtime_error& e) {
        log_error("Web marshalling error: {}", e.what());
    }

    log_debug("output-----------------------{}", output);

    auto ioHandler = std::make_unique<MemIOHandler>(output);
    ioHandler->open(mode);
    return ioHandler;
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
            addUpdateIDs(session, updateIDs);
        }
    }
}

void WebRequestHandler::addUpdateIDs(const std::shared_ptr<Session>& session, pugi::xml_node& updateIDsEl)
{
    std::string updateIDs = session->getUIUpdateIDs();
    if (!updateIDs.empty()) {
        log_debug("UI: sending update ids: {}", updateIDs);
        updateIDsEl.append_attribute("ids") = updateIDs.c_str();
        updateIDsEl.append_attribute("updates") = true;
    }
}

void WebRequestHandler::appendTask(const std::shared_ptr<GenericTask>& task, pugi::xml_node& parent)
{
    if (!task || !parent)
        return;
    auto taskEl = parent.append_child("task");
    taskEl.append_attribute("id") = task->getID();
    taskEl.append_attribute("cancellable") = task->isCancellable();
    taskEl.append_attribute("text") = task->getDescription().c_str();
}

std::string_view WebRequestHandler::mapAutoscanType(AutoscanType type)
{
    switch (type) {
    case AutoscanType::Ui:
        return "ui";
    case AutoscanType::Config:
        return "persistent";
    default:
        return "none";
    }
}

bool WebRequestHandler::readResources(const std::shared_ptr<CdsObject>& object)
{
    std::string resources = param("resources"); // contains list of changed resource properties separated by |
    bool result = false;
    if (!resources.empty()) {
        auto resourceList = splitString(resources, '|');
        std::map<int, std::map<std::string, std::string>> resMap;
        for (auto&& resValue : resourceList) {
            auto propList = splitString(resValue, '.'); // resource property is formatted : resource.<index>.<propertyName>
            if (propList.size() >= 3) {
                std::string prop = param(resValue);
                auto resIndex = stoiString(propList[1]);
                if (resMap.find(resIndex) == resMap.end()) {
                    resMap[resIndex] = {};
                }
                resMap[resIndex][propList[2]] = prop;
            }
        }
        for (auto&& [resIndex, resDef] : resMap) {
            std::shared_ptr<CdsResource> resource = object->getResource(resIndex);
            if (resDef.find("purpose") != resDef.end() || !resource) {
                auto purpose = EnumMapper::mapPurpose(resDef.at("purpose"));
                if (!resource) {
                    resource = std::make_shared<CdsResource>(ContentHandler::EXTURL, purpose);
                    object->addResource(resource);
                } else {
                    resource->setPurpose(purpose);
                }
                result = true;
            }
            if (resDef.find("protocolInfo") != resDef.end()) {
                std::string protocolInfo = resDef.at("protocolInfo");
                resource->addAttribute(ResourceAttribute::PROTOCOLINFO, protocolInfo);
                result = true;
            } else if (resDef.find("mime-type") != resDef.end() && resDef.find("protocol") != resDef.end()) {
                std::string resMime = resDef.at("mime-type");
                if (resMime.empty())
                    resMime = MIMETYPE_DEFAULT;
                std::string resProtocol = resDef.at("protocol");
                std::string protocolInfo;
                if (!resProtocol.empty())
                    protocolInfo = renderProtocolInfo(resMime, resProtocol);
                else
                    protocolInfo = renderProtocolInfo(resMime);
                resource->addAttribute(ResourceAttribute::PROTOCOLINFO, protocolInfo);
                result = true;
            }
            if (resDef.find("location") != resDef.end()) {
                std::string resLocation = resDef.at("location");
                resource->addAttribute(ResourceAttribute::RESOURCE_FILE, resLocation);
                result = true;
            } else if (resDef.find("resFile") != resDef.end()) {
                std::string resLocation = resDef.at("resFile");
                resource->addAttribute(ResourceAttribute::RESOURCE_FILE, resLocation);
                result = true;
            }
        }
    }
    return result;
}

} // namespace Web
