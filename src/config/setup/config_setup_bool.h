/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_bool.h - this file is part of Gerbera.
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

/// \file config_setup_bool.h
///\brief Definitions of the ConfigBoolSetup class.

#ifndef __CONFIG_SETUP_BOOL_H__
#define __CONFIG_SETUP_BOOL_H__

#include "config/config_setup.h"

class ConfigBoolSetup : public ConfigSetup {
    using ConfigSetup::ConfigSetup;

public:
    ConfigBoolSetup(ConfigVal option, const char* xpath, const char* help, const char* defaultValue, StringCheckFunction check = nullptr)
        : ConfigSetup(option, xpath, help, std::move(check), defaultValue)
    {
    }

    ConfigBoolSetup(ConfigVal option, const char* xpath, const char* help, bool defaultValue, StringCheckFunction check = nullptr)
        : ConfigSetup(option, xpath, help, std::move(check))
    {
        this->defaultValue = defaultValue ? YES : NO;
    }

    ConfigBoolSetup(ConfigVal option, const char* xpath, const char* help, bool defaultValue, bool required)
        : ConfigSetup(option, xpath, help, required)
    {
        this->defaultValue = defaultValue ? YES : NO;
    }

    std::string getTypeString() const override { return "Boolean"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    void makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool getXmlContent(const pugi::xml_node& root);

    bool checkValue(std::string& optValue, const std::string& pathName = "") const;

    std::shared_ptr<ConfigOption> newOption(bool optValue);

    std::string getCurrentValue() const override { return optionValue ? fmt::to_string(optionValue->getBoolOption()) : ""; }

    static bool CheckInotifyValue(std::string& value);

    static bool CheckMarkPlayedValue(std::string& value);
};

#endif // __CONFIG_SETUP_BOOL_H__