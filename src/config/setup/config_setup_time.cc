/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_time.cc - this file is part of Gerbera.

    Copyright (C) 2023-2025 Gerbera Contributors

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

/// @file config/setup/config_setup_time.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_time.h" // API

#include "config/config_options.h"
#include "exceptions.h"
#include "util/grb_time.h"

#include <pugixml.hpp>

LongOptionType ConfigTimeSetup::getXmlContent(
    const pugi::xml_node& root,
    const std::shared_ptr<Config>& config)
{
    auto optValue = ConfigSetup::getXmlContent(root, config, true);
    LongOptionType result;
    if (!parseTime(result, optValue, type)) {
        throw_std_runtime_error("Invalid {} time format '{}'", xpath, optValue);
    }
    return result;
}

void ConfigTimeSetup::makeOption(
    const pugi::xml_node& root,
    const std::shared_ptr<Config>& config,
    const std::map<std::string, std::string>* arguments)
{
    auto optValue = ConfigSetup::getXmlContent(root, config, true);
    newOption(optValue);
    setOption(config);
}

void ConfigTimeSetup::makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    newOption(optValue);
    setOption(config);
}

std::shared_ptr<ConfigOption> ConfigTimeSetup::newOption(std::string& optValue)
{
    auto pathValue = optValue;
    LongOptionType result;
    if (!parseTime(result, optValue, type)) {
        throw_std_runtime_error("Invalid {} time format '{}'", xpath, optValue);
    }
    if (minValue > -1 && result < minValue) {
        throw_std_runtime_error("Time Value {} too small {} < {}", xpath, result, minValue);
    }
    if (maxValue > -1 && result > maxValue) {
        throw_std_runtime_error("Time Value {} too small {} < {}", xpath, maxValue, result);
    }
    optionValue = std::make_shared<LongOption>(result, optValue);
    return optionValue;
}

LongOptionType ConfigTimeSetup::checkTimeValue(std::string& optValue)
{
    LongOptionType result;
    if (!parseTime(result, optValue, type)) {
        throw_std_runtime_error("Invalid {} time format '{}'", xpath, optValue);
    }
    if (minValue > -1 && result < minValue) {
        throw_std_runtime_error("Time Value {} too small {} < {}", xpath, result, minValue);
    }
    if (maxValue > -1 && result > maxValue) {
        throw_std_runtime_error("Time Value {} too small {} < {}", xpath, maxValue, result);
    }
    return result;
}
