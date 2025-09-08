/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_int.h - this file is part of Gerbera.

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

/// @file config/setup/config_setup_int.h
/// @brief Definitions of the ConfigIntSetup classes.

#ifndef __CONFIG_SETUP_INT_H__
#define __CONFIG_SETUP_INT_H__

#include "config/config_setup.h" // API

template <typename T, class OptionClass>
class ConfigIntegerSetup : public ConfigSetup {
    static_assert(std::is_integral_v<T>, "Integral required.");

public:
    using IntCheckFunction = std::function<bool(T value)>;
    using IntParseFunction = std::function<T(const std::string& value)>;
    using IntPrintFunction = std::function<std::string(T value)>;
    using IntMinFunction = std::function<bool(T value, T minValue)>;
    using ConvertFunction = std::function<T(const std::string& str, T def, T base)>;

    static ConvertFunction converter;

protected:
    IntCheckFunction valueCheck = nullptr;
    IntParseFunction parseValue = nullptr;
    IntPrintFunction printValue = nullptr;
    IntMinFunction minCheck = nullptr;
    T minValue {};

public:
    ConfigIntegerSetup(ConfigVal option, const char* xpath, const char* help)
        : ConfigSetup(option, xpath, help)
    {
        this->defaultValue = fmt::to_string(0);
    }

    ConfigIntegerSetup(ConfigVal option, const char* xpath, const char* help, T defaultValue)
        : ConfigSetup(option, xpath, help)
    {
        this->defaultValue = fmt::to_string(defaultValue);
    }

    ConfigIntegerSetup(ConfigVal option, const char* xpath, const char* help, IntCheckFunction check)
        : ConfigSetup(option, xpath, help)
        , valueCheck(std::move(check))
    {
        this->defaultValue = fmt::to_string(0);
    }

    ConfigIntegerSetup(ConfigVal option, const char* xpath, const char* help, T defaultValue, IntParseFunction parseValue, IntPrintFunction printValue = nullptr)
        : ConfigSetup(option, xpath, help)
        , parseValue(std::move(parseValue))
        , printValue(std::move(printValue))
    {
        this->defaultValue = fmt::to_string(defaultValue);
    }

    ConfigIntegerSetup(ConfigVal option, const char* xpath, const char* help, T defaultValue, IntCheckFunction check, IntPrintFunction printValue = nullptr)
        : ConfigSetup(option, xpath, help)
        , valueCheck(std::move(check))
        , printValue(std::move(printValue))
    {
        this->defaultValue = fmt::to_string(defaultValue);
    }

    ConfigIntegerSetup(ConfigVal option, const char* xpath, const char* help, T defaultValue, T minValue, IntMinFunction check)
        : ConfigSetup(option, xpath, help)
        , minCheck(std::move(check))
        , minValue(minValue)
    {
        this->defaultValue = fmt::to_string(defaultValue);
    }

    ConfigIntegerSetup(ConfigVal option, const char* xpath, const char* help, const char* defaultValue, IntCheckFunction check = nullptr, IntPrintFunction printValue = nullptr)
        : ConfigSetup(option, xpath, help)
        , valueCheck(std::move(check))
        , printValue(std::move(printValue))
    {
        this->defaultValue = defaultValue;
    }

    ConfigIntegerSetup(ConfigVal option, const char* xpath, const char* help, const char* defaultValue, StringCheckFunction check, IntPrintFunction printValue = nullptr)
        : ConfigSetup(option, xpath, help, std::move(check), defaultValue)
        , printValue(std::move(printValue))
    {
    }

    std::string getTypeString() const override { return parseValue ? "String" : "Number"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    void makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    T getXmlContent(const pugi::xml_node& root);

    std::shared_ptr<ConfigOption> newOption(T optValue);

    T checkIntValue(std::string& sVal, const std::string& pathName = "") const;

    std::string getCurrentValue() const override { return optionValue ? optionValue->getOption() : ""; }

    static bool CheckMinValue(T value, T minValue);
};

bool CheckUpnpStringLimitValue(IntOptionType value);
bool CheckProfileNumberValue(std::string& value);
bool CheckImageQualityValue(IntOptionType value);
bool CheckPortValue(UIntOptionType value);

using ConfigIntSetup = ConfigIntegerSetup<IntOptionType, IntOption>;
using ConfigUIntSetup = ConfigIntegerSetup<UIntOptionType, UIntOption>;
using ConfigLongSetup = ConfigIntegerSetup<LongOptionType, LongOption>;
using ConfigULongSetup = ConfigIntegerSetup<ULongOptionType, ULongOption>;

#endif // __CONFIG_SETUP_INT_H__
