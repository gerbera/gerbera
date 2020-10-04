/*GRB*

    Gerbera - https://gerbera.io/

    config_save.cc - this file is part of Gerbera.

    Copyright (C) 2020 Gerbera Contributors

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

#include <numeric>

#include "pages.h" // API

#include "autoscan.h"
#include "config/client_config.h"
#include "config/config.h"
#include "config/config_manager.h"
#include "config/config_options.h"
#include "config/config_setup.h"
#include "metadata/metadata_handler.h"
#include "storage/storage.h"
#include "transcoding/transcoding.h"
#include "upnp_xml.h"
#include "util/upnp_clients.h"

web::configSave::configSave(std::shared_ptr<Config> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(std::move(config), std::move(storage), std::move(content), std::move(sessionManager))
{
}

void web::configSave::process()
{
    check_request();
    //auto root = xmlDoc->document_element();
    log_debug("configSave");

    int count = std::stoi(param("changedCount"));

    for (int i = 0; i < count; i++) {
        try {
            auto key = fmt::format("data[{}][{}]", i, "id");
            auto item = fmt::format("data[{}][{}]", i, "item");
            auto status = fmt::format("data[{}][{}]", i, "status");
            bool success = false;
            std::shared_ptr<ConfigSetup> cs = nullptr;
            log_info("id {}='{}'", param(item), param(key));
            if (!param(key).empty() && param(key) != "-1") {
                config_option_t option = CFG_MAX;
                option = static_cast<config_option_t>(std::stoi(param(key)));
                cs = ConfigManager::findConfigSetup(option, true);
                log_info("{} = {}", key, option);
            } else if (!param(item).empty()) {
                cs = ConfigManager::findConfigSetupByPath(param(item), true);
                //option = cs->option;
                log_info("{} = '{}' status {}", param(item), cs != nullptr ? cs->xpath : "", param(status));
            } else {
                log_error("{} has empty value", item);
                continue;
            }

            if (cs != nullptr) {
                auto value = fmt::format("data[{}][{}]", i, "value");
                auto orig = fmt::format("data[{}][{}]", i, "origValue");
                log_debug("found option to update {}", cs->xpath);

                config->setOrigValue(param(item), param(orig));
                std::string parValue = param(value);
                std::string parStatus = param(status);
                bool update = param(status) == "changed";
                if (parStatus == "reset") {
                    storage->removeConfigValue(param(item));
                    parValue = config->getOrigValue(param(item));
                    log_info("reset {}", param(item));
                    success = false;
                    update = true;
                } else if (parStatus == "added") {
                    log_info("added {}", param(item));
                    success = false;
                } else if (parStatus == "removed") {
                    log_info("removed {}", param(item));
                    success = false;
                }
                if (update) {
                    if (param(item) == cs->xpath) {
                        cs->makeOption(parValue, config);
                        success = true;
                    } else {
                        if (!cs->updateDetail(param(item), parValue, config)) {
                            log_error("unhandled option {} != {}", param(item), cs->xpath);
                        } else {
                            success = true;
                        }
                    }
                    if (parStatus == "changed" && success) {
                        storage->updateConfigValue(cs->getUniquePath(), param(item), param(value));
                    }
                }
            }
        } catch (const std::runtime_error& e) {
            log_error("error setting option {}. Exception {}", i, e.what());
        }
    }
}
