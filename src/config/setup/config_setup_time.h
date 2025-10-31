/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_time.h - this file is part of Gerbera.

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

/// @file config/setup/config_setup_time.h
/// @brief Definitions of the ConfigTimeSetup classes.

#ifndef __CONFIG_SETUP_TIME_H__
#define __CONFIG_SETUP_TIME_H__

#include "config/config_setup.h"

#include "util/grb_time.h"

class ConfigTimeSetup : public ConfigSetup {
protected:
    GrbTimeType type = GrbTimeType::Seconds;
    LongOptionType minValue = -1;
    LongOptionType maxValue = -1;

public:
    ConfigTimeSetup(
        ConfigVal option,
        const char* xpath,
        const char* help,
        GrbTimeType type,
        LongOptionType defaultValue = 0,
        LongOptionType minValue = -1,
        LongOptionType maxValue = -1)
        : ConfigSetup(option, xpath, help, false)
        , type(type)
        , minValue(minValue)
        , maxValue(maxValue)
    {
        this->defaultValue = fmt::to_string(defaultValue);
    }

    std::string getTypeString() const override { return "Time"; }

    void makeOption(
        const pugi::xml_node& root,
        const std::shared_ptr<Config>& config,
        const std::map<std::string, std::string>* arguments = nullptr) override;

    void makeOption(
        std::string optValue,
        const std::shared_ptr<Config>& config,
        const std::map<std::string, std::string>* arguments = nullptr) override;

    std::shared_ptr<ConfigOption> newOption(std::string& optValue);
    LongOptionType getXmlContent(
        const pugi::xml_node& root,
        const std::shared_ptr<Config>& config);
    LongOptionType checkTimeValue(std::string& optValue);
};

#endif // __CONFIG_SETUP_TIME_H__
