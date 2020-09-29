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
            bool success = false;
            config_option_t option = static_cast<config_option_t>(std::stoi(param(key)));
            log_info("{} = {}", key, option);
            auto cs = ConfigManager::findConfigSetup(option, true);

            if (cs != nullptr) {
                auto value = fmt::format("data[{}][{}]", i, "value");
                auto item = fmt::format("data[{}][{}]", i, "item");
                auto status = fmt::format("data[{}][{}]", i, "status");
                log_debug("found option to update {}", cs->xpath);

                if (param(item) == cs->xpath) {
                    cs->makeOption(param(value), config);
                    success = true;
                } else {
                    std::string parValue = param(value);
                    if (param(status) == "changed") {
                        if (!cs->updateDetail(param(item), parValue, config)) {
                            log_error("unhandled option {} != {}", param(item), cs->xpath);
                        } else {
                            success = true;
                        }
                    } else if (param(status) == "added") {
                        success = true;
                    } else if (param(status) == "removed") {
                        success = true;
                    }
                }
                if (success) {
                    storage->updateConfigValue(cs->getUniquePath(), param(item), param(value));
                }
            }
        } catch (const std::runtime_error& e) {
            log_error("error setting option {}. Exception {}", i, e.what());
        }
    }
}
