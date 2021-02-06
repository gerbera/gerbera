/*GRB*

    Gerbera - https://gerbera.io/

    config_save.cc - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

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

#include <filesystem> // for path, operator!=
#include <fmt/format.h> // for format
#include <map> // for map
#include <memory> // for shared_ptr, __shared_ptr_access
#include <pugixml.hpp> // for xml_node, xml_attribute, xml_do...
#include <stdexcept> // for invalid_argument, runtime_error
#include <string> // for string, basic_string, operator==
#include <string_view> // for operator==
#include <utility> // for move
#include <vector> // for vector

#include "config/config.h" // for Config, config_option_t, CFG_MAX
#include "config/config_manager.h" // for ConfigManager
#include "config/config_setup.h" // for ConfigSetup, STATUS_CHANGED
#include "content/autoscan.h" // for AutoscanDirectory
#include "content/content_manager.h" // for ContentManager
#include "database/database.h" // for Database
#include "pages.h" // for configSave
#include "util/logger.h" // for log_debug, log_error, log_info
#include "web/web_request_handler.h" // for WebRequestHandler

web::configSave::configSave(std::shared_ptr<ContentManager> content)
    : WebRequestHandler(std::move(content))
{
}

void web::configSave::process()
{
    check_request();
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

    for (int i = 0; i < count; i++) {
        try {
            auto key = fmt::format("data[{}][{}]", i, "id");
            auto item = fmt::format("data[{}][{}]", i, "item");
            auto status = fmt::format("data[{}][{}]", i, "status");
            bool success = false;
            std::shared_ptr<ConfigSetup> cs = nullptr;
            log_debug("save item {}='{}' {}", param(item), param(key), param(status));
            if (!param(key).empty() && param(key) != "-1") {
                config_option_t option = CFG_MAX;
                option = config_option_t(std::stoi(param(key)));
                cs = ConfigManager::findConfigSetup(option, true);
            } else if (!param(item).empty()) {
                cs = ConfigManager::findConfigSetupByPath(param(item), true);
            } else {
                log_error("{} has empty value", item);
                continue;
            }

            if (cs != nullptr) {
                auto value = fmt::format("data[{}][{}]", i, "value");
                auto orig = fmt::format("data[{}][{}]", i, "origValue");
                log_debug("found option to update {}", cs->getUniquePath());

                if (!param(orig).empty()) {
                    config->setOrigValue(param(item), param(orig));
                }
                std::string parValue = param(value);
                std::string parStatus = param(status);
                bool update = param(status) == STATUS_CHANGED;
                std::map<std::string, std::string> arguments = { { "status", parStatus } };
                if (parStatus == STATUS_RESET || parStatus == STATUS_KILLED) {
                    database->removeConfigValue(param(item));
                    if (config->hasOrigValue(param(item))) {
                        parValue = config->getOrigValue(param(item));
                        log_debug("reset {} to '{}'", param(item), parValue);
                        success = false;
                        update = true;
                    } else if (param(item).substr(param(item).length() - 1) == "]") {
                        parValue = "";
                        log_debug("dropping {}", param(item));
                        success = false;
                        update = true;
                    } else {
                        success = true;
                    }
                } else if (parStatus == STATUS_ADDED || parStatus == STATUS_MANUAL) {
                    database->updateConfigValue(cs->getUniquePath(), param(item), parValue, STATUS_MANUAL);
                    log_debug("added {}", param(item));
                    success = false;
                    update = true;
                } else if (parStatus == STATUS_REMOVED) {
                    cs->updateDetail(param(item), parValue, config, &arguments);
                    database->updateConfigValue(cs->getUniquePath(), param(item), parValue, parStatus);
                    log_debug("removed {}", param(item));
                    success = true;
                }
                if (update) {
                    if (param(item) == cs->xpath) {
                        cs->makeOption(parValue, config);
                        success = true;
                    } else {
                        if (!cs->updateDetail(param(item), parValue, config, &arguments)) {
                            log_error("unhandled option {} != {}", param(item), cs->getUniquePath());
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

    std::string target = param("target");
    if (action == "rescan" && !target.empty()) {
        if (target != "--all") {
            fs::path targetPath(target);
            std::shared_ptr<AutoscanDirectory> autoscan = nullptr;
            while (targetPath != "/" && autoscan == nullptr) {
                autoscan = content->getAutoscanDirectory(targetPath);
                targetPath = targetPath.parent_path();
            }
            int objectID = database->findObjectIDByPath(target);
            if (objectID > 0 && autoscan != nullptr) {
                content->rescanDirectory(autoscan, objectID, target);
                auto taskEl = root.append_child("task");
                taskEl.append_attribute("text") = fmt::format("Rescanning directory {}", target).c_str();
                log_info("Rescanning directory {}", target);
            } else {
                log_error("No such autoscan or dir: {} ({})", target.c_str(), objectID);
            }
        } else {
            auto autoScans = content->getAutoscanDirectories();
            for (const auto& autoscan : autoScans) {
                content->rescanDirectory(autoscan, autoscan->getObjectID());
                log_info("Rescanning directory {}", autoscan->getLocation().c_str());
            }
        }
    }
}
