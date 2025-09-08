/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_string.cc - this file is part of Gerbera.

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

/// @file config/setup/config_setup_string.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_string.h" // API

#include "common.h"
#include "config/config_options.h"
#include "exceptions.h"
#include "util/logger.h"
#include "util/string_converter.h"
#include "util/tools.h"

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

bool ConfigStringSetup::CheckCharset(std::string& value)
{
    try {
        auto conv = StringConverter(value, DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        log_error("Error in config file: unsupported charset specified: {}\n{}", value, e.what());
        return false;
    }
    return true;
}

bool ConfigStringSetup::MergeContentSecurityPolicy(std::string& value)
{
    replaceAllString(value, "\n", ";");
    replaceAllString(value, "\t", " ");
    replaceAllString(value, "  ", " ");
    replaceAllString(value, ";;", ";");
    return true;
}
