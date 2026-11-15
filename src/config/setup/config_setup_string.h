/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_string.h - this file is part of Gerbera.

    Copyright (C) 2020-2026 Gerbera Contributors

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

/// @file config/setup/config_setup_string.h
/// @brief Definitions of the ConfigStringSetup classes.

#ifndef __CONFIG_SETUP_STRING_H__
#define __CONFIG_SETUP_STRING_H__

#include "config/config_setup.h"

/// @brief set behaviour of string option
enum class ConfigStringArguments {
    none = 0,
    /// @brief option must not be empty
    notEmpty = (1 << 0),
    /// @brief don't trim the string
    dontTrim = (1 << 1),
};

inline ConfigStringArguments operator|(ConfigStringArguments a, ConfigStringArguments b)
{
    return static_cast<ConfigStringArguments>(static_cast<int>(a) | static_cast<int>(b));
}

inline ConfigStringArguments operator&(ConfigStringArguments a, ConfigStringArguments b)
{
    return static_cast<ConfigStringArguments>(static_cast<int>(a) & static_cast<int>(b));
}

class ConfigStringSetup : public ConfigSetup {
protected:
    ConfigStringArguments arguments;

    /// @brief check for flag
    bool isSet(ConfigStringArguments arg) const { return (arguments & arg) == arg; }

public:
    ConfigStringSetup(
        ConfigVal option,
        const char* xpath,
        const char* help,
        bool required = false,
        const char* defaultValue = "",
        ConfigStringArguments arguments = ConfigStringArguments::none)
        : ConfigSetup(option, xpath, help, required, defaultValue)
        , arguments(arguments)
    {
    }

    ConfigStringSetup(
        ConfigVal option,
        const char* xpath,
        const char* help,
        const char* defaultValue,
        StringCheckFunction check = nullptr,
        bool required = false,
        ConfigStringArguments arguments = ConfigStringArguments::none)
        : ConfigSetup(option, xpath, help, std::move(check), defaultValue, required)
        , arguments(arguments)
    {
    }

    std::string getTypeString() const override { return "String"; }

    void makeOption(
        const pugi::xml_node& root,
        const std::shared_ptr<Config>& config,
        const std::map<std::string, std::string>* arguments = nullptr) override;

    std::shared_ptr<ConfigOption> newOption(const std::string& optValue);

    static bool CheckCharset(std::string& value);
    static bool MergeContentSecurityPolicy(std::string& value);
};

#endif // __CONFIG_SETUP_STRING_H__
