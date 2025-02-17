/*GRB*

    Gerbera - https://gerbera.io/

    web/page_request.cc - this file is part of Gerbera.

    Copyright (C) 2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file web/page_request.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "page_request.h" // API

#include "cds/cds_objects.h"
#include "cds/cds_resource.h"
#include "content/content.h"
#include "exceptions.h"
#include "util/generic_task.h"
#include "util/logger.h"
#include "util/xml_to_json.h"
#include "web/session_manager.h"

void Web::PageRequest::process(pugi::xml_node& root)
{
    log_debug("start {}", getPage());
    checkRequest();
    auto element = xmlDoc->document_element();
    processPageAction(element);
    // add current task
    appendTask(content->getCurrentTask(), root);
    handleUpdateIDs(element);
    log_debug("end {}", getPage());
}

void Web::PageRequest::checkRequest(bool checkLogin)
{
    // we have a minimum set of parameters that are "must have"

    // check if the session parameter was supplied and if we have
    // a session with that id

    std::string sid = param(SID);
    if (sid.empty())
        throw SessionException("no session id given");

    session = sessionManager->getSession(sid);
    if (!session)
        throw SessionException("invalid session id");

    if (checkLogin && !session->isLoggedIn())
        throw LoginException("not logged in");
    session->access();
}

void Web::PageRequest::addUpdateIDs(
    const std::shared_ptr<Session>& session,
    pugi::xml_node& updateIDsEl)
{
    std::string updateIDs = session->getUIUpdateIDs();
    if (!updateIDs.empty()) {
        log_debug("UI: sending update ids: {}", updateIDs);
        updateIDsEl.append_attribute("ids") = updateIDs.c_str();
        updateIDsEl.append_attribute("updates") = true;
    }
}

void Web::PageRequest::handleUpdateIDs(pugi::xml_node& element)
{
    // session will be filled by check_request
    std::string updates = param("updates");
    if (!updates.empty()) {
        auto updateIDs = element.append_child("update_ids");

        if (updates == "check") {
            updateIDs.append_attribute("pending") = session->hasUIUpdateIDs();
        } else if (updates == "get") {
            addUpdateIDs(session, updateIDs);
        }
    }
}

void Web::PageRequest::appendTask(
    const std::shared_ptr<GenericTask>& task,
    pugi::xml_node& parent)
{
    if (!task || !parent)
        return;
    auto taskEl = parent.append_child("task");
    taskEl.append_attribute("id") = task->getID();
    taskEl.append_attribute("cancellable") = task->isCancellable();
    taskEl.append_attribute("text") = task->getDescription().c_str();
}

int Web::PageRequest::intParam(const std::string& name, int invalid) const
{
    std::string value = param(name);
    return !value.empty() ? std::stoi(value) : invalid;
}

bool Web::PageRequest::boolParam(const std::string& name) const
{
    std::string value = param(name);
    return !value.empty() && (value == "1" || value == "true");
}

bool Web::PageRequest::readResources(const std::shared_ptr<CdsObject>& object)
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
