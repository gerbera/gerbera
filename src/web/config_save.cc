/*GRB*

    Gerbera - https://gerbera.io/

    config_save.cc - this file is part of Gerbera.

    Copyright (C) 2020-2024 Gerbera Contributors

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

/// \file config_load.cc
#define LOG_FAC log_facility_t::web

#include "pages.h" // API

#include "config/client_config.h"
#include "config/config.h"
#include "config/config_definition.h"
#include "config/config_setup.h"
#include "content/autoscan.h"
#include "content/content_manager.h"
#include "database/database.h"
#include "metadata/metadata_handler.h"
#include "util/upnp_clients.h"
#include "util/xml_to_json.h"

Web::ConfigSave::ConfigSave(std::shared_ptr<Context> context, const std::shared_ptr<ContentManager>& content)
    : WebRequestHandler(content)
    , context(std::move(context))
{
}

/// \brief: process config_save request
void Web::ConfigSave::process()
{
    checkRequest();
    auto root = xmlDoc->document_element();
    log_debug("configSave");
    std::string action = param("action");

    int count = 0;
    try {
        count = std::stoi(param("changedCount"));
    } catch (const std::invalid_argument& ex) {
        log_error(ex.what());
    }
    if (count == -1 && action == "clear") {
        database->removeConfigValue("*"); // remove all
        auto taskEl = root.append_child("task");
        taskEl.append_attribute("text") = "Removed all config values from database";
        return;
    }

    // go through all config items from UI
    for (int i = 0; i < count; i++) {
        try {
            auto key = fmt::format("data[{}][{}]", i, "id");
            auto item = fmt::format("data[{}][{}]", i, "item");
            auto status = fmt::format("data[{}][{}]", i, "status");
            std::shared_ptr<ConfigSetup> cs;
            log_debug("save item {}='{}' {}", param(item), param(key), param(status));
            if (!param(key).empty() && param(key) != "-1") {
                auto option = config_option_t(std::stoi(param(key)));
                cs = ConfigDefinition::findConfigSetup(option, true);
            } else if (!param(item).empty()) {
                cs = ConfigDefinition::findConfigSetupByPath(param(item), true);
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
                    config->setOrigValue(param(item), param(orig));
                }
                std::string parValue = param(value);
                std::string parStatus = param(status);
                bool update = param(status) == STATUS_CHANGED;
                std::map<std::string, std::string> arguments { { "status", parStatus } };
                if (parStatus == STATUS_RESET || parStatus == STATUS_KILLED) {
                    // remove value and restore original
                    database->removeConfigValue(param(item));
                    if (config->hasOrigValue(param(item))) {
                        parValue = config->getOrigValue(param(item));
                        log_debug("reset {} to '{}'", param(item), parValue);
                        success = false;
                        update = true;
                    } else if (param(item).substr(param(item).length() - 1) == "]") {
                        parValue.clear();
                        log_debug("dropping {}", param(item));
                        success = false;
                        update = true;
                    } else {
                        success = true;
                    }
                } else if (parStatus == STATUS_ADDED || parStatus == STATUS_MANUAL) {
                    // add new value
                    database->updateConfigValue(cs->getUniquePath(), param(item), parValue, std::string(STATUS_MANUAL));
                    log_debug("added {}", param(item));
                    success = false;
                    update = true;
                } else if (parStatus == STATUS_REMOVED) {
                    // remove value
                    cs->updateDetail(param(item), parValue, config, &arguments);
                    database->updateConfigValue(cs->getUniquePath(), param(item), parValue, parStatus);
                    log_debug("removed {}", param(item));
                    success = true;
                }
                if (update) {
                    // save option to database
                    if (param(item) == cs->xpath) {
                        cs->makeOption(parValue, config);
                        success = true;
                    } else {
                        if (!cs->updateDetail(param(item), parValue, config, &arguments)) {
                            log_error("unhandled {} option {} != {}", parStatus, param(item), cs->getUniquePath());
                        } else {
                            success = true;
                        }
                    }
                    if (parStatus == STATUS_CHANGED && success) {
                        database->updateConfigValue(cs->getUniquePath(), param(item), param(value));
                    }
                }
            }
        } catch (const std::runtime_error& e) {
            log_error("error setting option {}. Exception {}", i, e.what());
        }

        auto taskEl = root.append_child("task");
        taskEl.append_attribute("text") = fmt::format("Successfully updated {} items", count).c_str();
    }

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
                auto taskEl = root.append_child("task");
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

    context->getClients()->refresh(context->getConfig());
}
