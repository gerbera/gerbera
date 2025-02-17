/*GRB*

    Gerbera - https://gerbera.io/

    web/config_save.cc - this file is part of Gerbera.

    Copyright (C) 2020-2025 Gerbera Contributors

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

/// \file web/config_save.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "config/config.h"
#include "config/config_definition.h"
#include "config/config_setup.h"
#include "config/result/autoscan.h"
#include "content/content.h"
#include "context.h"
#include "database/database.h"
#include "upnp/client_manager.h"
#include "util/xml_to_json.h"

const std::string Web::ConfigSave::PAGE = "config_save";

Web::ConfigSave::ConfigSave(std::shared_ptr<Context> context,
    const std::shared_ptr<Content>& content,
    const std::shared_ptr<Server>& server,
    const std::shared_ptr<UpnpXMLBuilder>& xmlBuilder,
    const std::shared_ptr<Quirks>& quirks)
    : PageRequest(content, server, xmlBuilder, quirks)
    , context(std::move(context))
    , definition(this->content->getContext()->getDefinition())
{
}

/// \brief: process config_save request
void Web::ConfigSave::processPageAction(pugi::xml_node& element)
{
    std::string action = param("action");

    int count = 0;
    try {
        count = std::stoi(param("changedCount"));
    } catch (const std::invalid_argument& ex) {
        log_error(ex.what());
    }
    if (action == "clear") {
        database->removeConfigValue("*"); // remove all
    }

    // go through all config items from UI
    for (int i = 0; i < count; i++) {
        try {
            auto key = fmt::format("data[{}][{}]", i, "id");
            auto item = fmt::format("data[{}][{}]", i, "item");
            auto status = fmt::format("data[{}][{}]", i, "status");
            std::shared_ptr<ConfigSetup> cs;
            std::string parItem = param(item);
            log_debug("save item {}='{}' {}", parItem, param(key), param(status));
            if (!param(key).empty() && param(key) != "-1") {
                auto option = static_cast<ConfigVal>(std::stoi(param(key)));
                cs = definition->findConfigSetup(option, true);
            } else if (!parItem.empty()) {
                cs = definition->findConfigSetupByPath(parItem, true);
            } else {
                log_error("{} has empty value", item);
                continue;
            }

            if (cs) {
                bool success = false;
                auto value = fmt::format("data[{}][{}]", i, "value");
                auto orig = fmt::format("data[{}][{}]", i, "origValue");
                log_debug("found option to update {}", cs->getUniquePath());

                if (!param(orig).empty()) {
                    config->setOrigValue(parItem, param(orig));
                }
                std::string parValue = param(value);
                auto statusList = splitString(param(status), ',');
                std::string parStatus = statusList[0];
                bool update = parStatus == STATUS_CHANGED;
                std::map<std::string, std::string> arguments { { "status", param(status) } };
                if (parStatus == STATUS_KILLED || statusList.back() == STATUS_KILLED) {
                    // remove added value
                    database->removeConfigValue(parItem);
                    log_debug("Dropping {}", parItem);
                    parValue.clear();
                    update = false;
                    success = true;
                    if (!cs->updateDetail(parItem, parValue, config, &arguments)) {
                        log_debug("unhandled {} option {} != {}", parStatus, parItem, cs->getUniquePath());
                    }
                } else if (parStatus == STATUS_RESET || statusList.back() == STATUS_RESET) {
                    // remove value and restore original
                    database->removeConfigValue(parItem);
                    if (config->hasOrigValue(parItem)) {
                        parValue = config->getOrigValue(parItem);
                        log_debug("reset {} to '{}'", parItem, parValue);
                        success = false;
                        update = true;
                    } else if (parItem.substr(parItem.length() - 1) == "]") {
                        parValue.clear();
                        log_debug("dropping {}", parItem);
                        success = false;
                        update = true;
                    } else {
                        success = true;
                    }
                } else if (parStatus == STATUS_ADDED || parStatus == STATUS_MANUAL) {
                    // add new value
                    database->updateConfigValue(cs->getUniquePath(), parItem, parValue, std::string(STATUS_MANUAL));
                    log_debug("added {}", parItem);
                    success = false;
                    update = true;
                } else if (parStatus == STATUS_REMOVED) {
                    // remove value
                    cs->updateDetail(parItem, parValue, config, &arguments);
                    database->updateConfigValue(cs->getUniquePath(), parItem, parValue, parStatus);
                    log_debug("removed {}", parItem);
                    success = true;
                }
                if (update) {
                    // save option to database
                    if (parItem == cs->xpath) {
                        cs->makeOption(parValue, config);
                        success = true;
                    } else {
                        if (!cs->updateDetail(parItem, parValue, config, &arguments)) {
                            log_error("unhandled {} option {} != {}", parStatus, parItem, cs->getUniquePath());
                        } else {
                            success = true;
                        }
                    }
                    if (parStatus == STATUS_CHANGED && success) {
                        database->updateConfigValue(cs->getUniquePath(), parItem, param(value), param(status));
                    }
                }
            }
        } catch (const std::runtime_error& e) {
            log_error("error setting option {}. Exception {}", i, e.what());
        }
    }

    auto taskEl = element.append_child("task");
    if (action == "clear")
        taskEl.append_attribute("text") = "Removed all config values from database";
    else if (action != "rescan")
        taskEl.append_attribute("text") = fmt::format("Successfully updated {} items", count).c_str();

    // trigger rescan of database after update
    std::string target = param("target");
    if (action == "rescan" && !target.empty()) {
        if (target != "--all") {
            fs::path targetPath(target);
            std::shared_ptr<AutoscanDirectory> autoscan;
            while (targetPath != "/" && !autoscan) {
                autoscan = content->getAutoscanDirectory(targetPath);
                targetPath = targetPath.parent_path();
            }
            int objectID = database->findObjectIDByPath(target);
            if (objectID > 0 && autoscan) {
                content->rescanDirectory(autoscan, objectID, target);
                taskEl.append_attribute("text") = fmt::format("Rescanning directory {}", target).c_str();
                log_info("Rescanning directory {}", target);
            } else {
                log_error("No such autoscan or dir: {} ({})", target, objectID);
            }
        } else {
            auto autoScans = content->getAutoscanDirectories();
            for (auto&& autoscan : autoScans) {
                content->rescanDirectory(autoscan, autoscan->getObjectID());
                log_info("Rescanning directory {}", autoscan->getLocation().c_str());
            }
        }
    }

    context->getClients()->refresh();
}
