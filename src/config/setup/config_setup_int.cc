/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_int.cc - this file is part of Gerbera.

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

/// @file config/setup/config_setup_int.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_int.h" // API

#include "common.h"
#include "config/config_definition.h"
#include "config/config_options.h"
#include "util/logger.h"

template <typename T, class OptionClass>
void ConfigIntegerSetup<T, OptionClass>::makeOption(
    const pugi::xml_node& root,
    const std::shared_ptr<Config>& config,
    const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlContent(root, config));
    setOption(config);
}

template <typename T, class OptionClass>
void ConfigIntegerSetup<T, OptionClass>::makeOption(
    std::string optValue,
    const std::shared_ptr<Config>& config,
    const std::map<std::string, std::string>* arguments)
{
    try {
        if (rawCheck) {
            if (!rawCheck(optValue)) {
                throw_std_runtime_error("Invalid {} value '{}'", xpath, optValue);
            }
        } else if (valueCheck) {
            if (!valueCheck(converter(optValue, 0, 10))) {
                throw_std_runtime_error("Invalid {} value '{}'", xpath, optValue);
            }
        } else if (minCheck) {
            if (!minCheck(converter(optValue, 0, 10), minValue)) {
                throw_std_runtime_error("Invalid {} value '{}', must be at least {}", xpath, optValue, minValue);
            }
        }
    } catch (const std::logic_error& e) {
        throw_std_runtime_error("Invalid {} value '{}' unreadable", xpath, optValue);
    }
    try {
        auto intValue = (parseValue) ? parseValue(optValue) : std::stoi(optValue);
        optionValue = std::make_shared<OptionClass>(intValue);
        setOption(config);
    } catch (const std::runtime_error& e) {
        throw_std_runtime_error("Error in config file: {} unsupported int value '{}'", xpath, optValue);
    } catch (const std::logic_error& e) {
        throw_std_runtime_error("Error in config file: {} unreadable int value '{}'", xpath, optValue);
    }
}

template <typename T, class OptionClass>
T ConfigIntegerSetup<T, OptionClass>::checkIntValue(std::string& sVal, const std::string& pathName) const
{
    try {
        if (rawCheck) {
            if (!rawCheck(sVal)) {
                throw_std_runtime_error("Invalid {}/{} value '{}'", pathName, xpath, sVal);
            }
        } else if (valueCheck) {
            if (!valueCheck(converter(sVal, 0, 10))) {
                throw_std_runtime_error("Invalid {}/{} value {}", pathName, xpath, sVal);
            }
        } else if (minCheck) {
            if (!minCheck(converter(sVal, 0, 10), minValue)) {
                throw_std_runtime_error("Invalid {}/{} value '{}', must be at least {}", pathName, xpath, sVal, minValue);
            }
        }
        if (parseValue)
            return parseValue(sVal);

        return std::stol(sVal);
    } catch (const std::runtime_error& e) {
        throw_std_runtime_error("Error in config file: {}/{} unsupported int value '{}'", pathName, xpath, sVal);
    } catch (const std::logic_error& e) {
        throw_std_runtime_error("Error in config file: {}/{} unreadable int value '{}'", pathName, xpath, sVal);
    }
}

template <typename T, class OptionClass>
T ConfigIntegerSetup<T, OptionClass>::getXmlContent(
    const pugi::xml_node& root,
    const std::shared_ptr<Config>& config)
{
    std::string sVal = ConfigSetup::getXmlContent(root, config, true);
    log_debug("Config: option: '{}/{}' value: '{}'", root.path(), xpath, sVal);
    return checkIntValue(sVal, root.path());
}

template <typename T, class OptionClass>
std::shared_ptr<ConfigOption> ConfigIntegerSetup<T, OptionClass>::newOption(T optValue)
{
    if (valueCheck && !valueCheck(optValue)) {
        throw_std_runtime_error("Invalid {} value {}", xpath, optValue);
    }
    if (minCheck && !minCheck(optValue, minValue)) {
        throw_std_runtime_error("Invalid {} value {}, must be at least {}", xpath, optValue, minValue);
    }
    if (printValue)
        optionValue = std::make_shared<OptionClass>(optValue, printValue(optValue));
    else
        optionValue = std::make_shared<OptionClass>(optValue);
    return optionValue;
}

bool CheckProfileNumberValue(std::string& value)
{
    auto tempInt = 0;
    if (value == "source" || value == fmt::to_string(SOURCE))
        tempInt = SOURCE;
    else if (value == "off" || value == fmt::to_string(OFF))
        tempInt = OFF;
    else {
        try {
            tempInt = std::stoi(value);
            if (tempInt <= 0)
                return false;
        } catch (const std::logic_error& ex) {
            log_error("CheckProfileNumberValue failed: {}", ex.what());
            return false;
        }
    }
    value.assign(fmt::to_string(tempInt));
    return true;
}

template <>
ConfigIntSetup::ConvertFunction ConfigIntSetup::converter = stoiString;

template <>
ConfigUIntSetup::ConvertFunction ConfigUIntSetup::converter = stoulString;

template <>
ConfigULongSetup::ConvertFunction ConfigULongSetup::converter = stoulString;

template <>
ConfigLongSetup::ConvertFunction ConfigLongSetup::converter = stolString;

template <typename T, class OptionClass>
bool ConfigIntegerSetup<T, OptionClass>::CheckMinValue(T value, T minValue)
{
    return value >= minValue;
}

bool CheckImageQualityValue(IntOptionType value)
{
    return value >= 0 && value <= 10;
}

bool CheckUpnpStringLimitValue(IntOptionType value)
{
    return value == -1 || value >= 4;
}

bool CheckPortValue(UIntOptionType value)
{
    return value >= 0 && value <= 65535;
}

template class ConfigIntegerSetup<IntOptionType, IntOption>;
template class ConfigIntegerSetup<UIntOptionType, UIntOption>;
template class ConfigIntegerSetup<LongOptionType, LongOption>;
template class ConfigIntegerSetup<ULongOptionType, ULongOption>;
