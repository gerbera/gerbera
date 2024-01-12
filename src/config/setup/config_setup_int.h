/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_int.h - this file is part of Gerbera.
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

/// \file config_setup_int.h
///\brief Definitions of the ConfigIntSetup classes.

#ifndef __CONFIG_SETUP_INT_H__
#define __CONFIG_SETUP_INT_H__

#include "config/config_setup.h" // API

using IntCheckFunction = std::function<bool(int value)>;
using IntParseFunction = std::function<int(const std::string& value)>;
using IntPrintFunction = std::function<std::string(int value)>;
using IntMinFunction = std::function<bool(int value, int minValue)>;

class ConfigIntSetup : public ConfigSetup {
protected:
    IntCheckFunction valueCheck = nullptr;
    IntParseFunction parseValue = nullptr;
    IntPrintFunction printValue = nullptr;
    IntMinFunction minCheck = nullptr;
    int minValue {};

public:
    ConfigIntSetup(config_option_t option, const char* xpath, const char* help)
        : ConfigSetup(option, xpath, help)
    {
        this->defaultValue = fmt::to_string(0);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* help, int defaultValue)
        : ConfigSetup(option, xpath, help)
    {
        this->defaultValue = fmt::to_string(defaultValue);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* help, IntCheckFunction check)
        : ConfigSetup(option, xpath, help)
        , valueCheck(std::move(check))
    {
        this->defaultValue = fmt::to_string(0);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* help, int defaultValue, IntParseFunction parseValue, IntPrintFunction printValue = nullptr)
        : ConfigSetup(option, xpath, help)
        , parseValue(std::move(parseValue))
        , printValue(std::move(printValue))
    {
        this->defaultValue = fmt::to_string(defaultValue);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* help, int defaultValue, IntCheckFunction check, IntPrintFunction printValue = nullptr)
        : ConfigSetup(option, xpath, help)
        , valueCheck(std::move(check))
        , printValue(std::move(printValue))
    {
        this->defaultValue = fmt::to_string(defaultValue);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* help, int defaultValue, int minValue, IntMinFunction check)
        : ConfigSetup(option, xpath, help)
        , minCheck(std::move(check))
        , minValue(minValue)
    {
        this->defaultValue = fmt::to_string(defaultValue);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* help, const char* defaultValue, IntCheckFunction check = nullptr, IntPrintFunction printValue = nullptr)
        : ConfigSetup(option, xpath, help)
        , valueCheck(std::move(check))
        , printValue(std::move(printValue))
    {
        this->defaultValue = defaultValue;
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* help, const char* defaultValue, StringCheckFunction check, IntPrintFunction printValue = nullptr)
        : ConfigSetup(option, xpath, help, std::move(check), defaultValue)
        , printValue(std::move(printValue))
    {
    }

    std::string getTypeString() const override { return parseValue ? "String" : "Number"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    void makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    int getXmlContent(const pugi::xml_node& root);

    std::shared_ptr<ConfigOption> newOption(int optValue);

    int checkIntValue(std::string& sVal, const std::string& pathName = "") const;

    std::string getCurrentValue() const override { return optionValue ? optionValue->getOption() : ""; }

    static bool CheckProfileNumberValue(std::string& value);

    static bool CheckMinValue(int value, int minValue);

    static bool CheckImageQualityValue(int value);

    static bool CheckPortValue(int value);

    static bool CheckUpnpStringLimitValue(int value);
};

#endif // __CONFIG_SETUP_INT_H__