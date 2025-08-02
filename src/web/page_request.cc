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
#include "util/generic_task.h"
#include "util/logger.h"
#include "web/session_manager.h"

void Web::PageRequest::process()
{
    log_debug("start {}", getPage());
    if (doCheck)
        checkRequest();
    std::string action = param("action");
    log_debug("action: {}", action);
    if (processPageAction(jsonDoc, action)) {
        // add current task
        auto task = content->getCurrentTask();
        Json::Value taskEl;
        appendTask(task, taskEl);
        jsonDoc["task"] = taskEl;
        handleUpdateIDs(jsonDoc);
    }
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
    Json::Value& updateIDsEl)
{
    std::string updateIDs = session->getUIUpdateIDs();
    if (!updateIDs.empty()) {
        log_debug("UI: sending update ids: {}", updateIDs);
        updateIDsEl["ids"] = updateIDs;
        updateIDsEl["updates"] = true;
    }
}

void Web::PageRequest::handleUpdateIDs(Json::Value& element)
{
    // session will be filled by check_request
    std::string updates = param("updates");
    if (!updates.empty()) {
        Json::Value updateIDs;

        if (updates == "check") {
            updateIDs["pending"] = session->hasUIUpdateIDs();
        } else if (updates == "get") {
            addUpdateIDs(session, updateIDs);
        }
        element["update_ids"] = updateIDs;
    }
}

void Web::PageRequest::appendTask(
    const std::shared_ptr<GenericTask>& task,
    Json::Value& taskEl)
{
    if (task) {
        taskEl["id"] = task->getID();
        taskEl["cancellable"] = task->isCancellable();
        taskEl["text"] = task->getDescription();
    }
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
        std::map<int, std::map<std::string, std::string>> resMap;
        for (auto&& resValue : splitString(resources, '|')) {
            auto propList = splitString(resValue, '.'); // resource property is formatted : resource.<index>.<propertyName>
            if (propList.size() >= 3) {
                std::string prop = param(resValue);
                auto& innerMap = resMap.try_emplace(stoiString(propList[1])).first->second;
                innerMap[propList[2]] = prop;
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
