/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_enum.h - this file is part of Gerbera.
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

/// \file config_setup_enum.h

#ifndef __CONFIG_SETUP_ENUM_H__
#define __CONFIG_SETUP_ENUM_H__

#include "config/config_option_enum.h"
#include "config/config_setup.h"
#include "util/logger.h"

template <class En>
class ConfigEnumSetup : public ConfigSetup {
protected:
    bool notEmpty = true;
    std::map<std::string, En> valueMap;

public:
    ConfigEnumSetup(ConfigVal option, const char* xpath, const char* help, std::map<std::string, En> valueMap, bool notEmpty = false)
        : ConfigSetup(option, xpath, help, false, "")
        , notEmpty(notEmpty)
        , valueMap(std::move(valueMap))
    {
    }

    ConfigEnumSetup(ConfigVal option, const char* xpath, const char* help, En defaultValue, std::map<std::string, En> valueMap, bool notEmpty = false)
        : ConfigSetup(option, xpath, help, false, "")
        , notEmpty(notEmpty)
        , valueMap(std::move(valueMap))
    {
        this->defaultValue = mapEnumValue(defaultValue);
    }

    std::string getTypeString() const override { return "Enum"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        if (arguments && arguments->find("notEmpty") != arguments->end()) {
            notEmpty = arguments->find("notEmpty")->second == "true";
        }
        newOption(ConfigSetup::getXmlContent(root, true));
        setOption(config);
    }

    void makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        if (arguments && arguments->find("notEmpty") != arguments->end()) {
            notEmpty = arguments->find("notEmpty")->second == "true";
        }
        newOption(optValue);
        setOption(config);
    }

    bool checkEnumValue(const std::string& value, En& result) const
    {
        if (valueMap.find(value) != valueMap.end()) {
            result = valueMap.at(value);
            return true;
        }
        return false;
    }

    std::string mapEnumValue(En value) const
    {
        for (auto&& [key, val] : valueMap) {
            if (val == value)
                return key;
        }
        return "";
    }

    En getXmlContent(const pugi::xml_node& root)
    {
        std::string optValue = ConfigSetup::getXmlContent(root, true);
        log_debug("Config: option: '{}' value: '{}'", xpath, optValue);
        if (notEmpty && optValue.empty()) {
            throw_std_runtime_error("Error in config file: Invalid {}/{} empty value '{}'", root.path(), xpath, optValue);
        }
        En result;
        if (!checkEnumValue(optValue, result)) {
            throw_std_runtime_error("Error in config file: {}/{} unsupported Enum value '{}'", root.path(), xpath, optValue);
        }
        return result;
    }

    std::shared_ptr<ConfigOption> newOption(const std::string& optValue)
    {
        if (notEmpty && optValue.empty()) {
            throw_std_runtime_error("Invalid {} empty value '{}'", xpath, optValue);
        }
        En result;
        if (!checkEnumValue(optValue, result)) {
            throw_std_runtime_error("Error in config file: {} unsupported Enum value '{}'", xpath, optValue);
        }
        optionValue = std::make_shared<EnumOption<En>>(result, mapEnumValue(result));
        return optionValue;
    }
};

#endif // __CONFIG_SETUP_ENUM_H__
