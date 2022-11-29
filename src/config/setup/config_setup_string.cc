/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_string.cc - this file is part of Gerbera.
    Copyright (C) 2020-2022 Gerbera Contributors

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

/// \file config_setup_string.cc

#include "config/config_setup.h" // API

#include "config/client_config.h"
#include "config/config_definition.h"
#include "config/config_options.h"
#include "content/autoscan.h"

#include <array>

void ConfigStringSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    bool trim = true;
    if (arguments && arguments->find("trim") != arguments->end()) {
        trim = arguments->at("trim") == "true";
    }
    newOption(getXmlContent(root, trim));
    setOption(config);
}

std::shared_ptr<ConfigOption> ConfigStringSetup::newOption(const std::string& optValue)
{
    if (notEmpty && optValue.empty()) {
        throw_std_runtime_error("Invalid {} empty value '{}'", xpath, optValue);
    }
    optionValue = std::make_shared<Option>(optValue);
    return optionValue;
}

static constexpr auto sqliteJournalModes = std::array<std::string_view, 6> { "DELETE", "TRUNCATE", "PERSIST", "MEMORY", "WAL", "OFF" };

bool ConfigStringSetup::CheckSqlJournalMode(std::string& value)
{
    value.assign(toUpper(value));
    return std::find(sqliteJournalModes.begin(), sqliteJournalModes.end(), value) != sqliteJournalModes.end();
}
