/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_path.h - this file is part of Gerbera.

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

/// @file config/setup/config_setup_path.h
/// @brief Definitions of the ConfigPathSetup class.

#ifndef __CONFIG_SETUP_PATH_H__
#define __CONFIG_SETUP_PATH_H__

#include "config/config_setup.h"

class Config;
class ConfigOption;

enum class ConfigPathArguments {
    none = 0,
    /// @brief isFile file or directory
    isFile = (1 << 0),
    /// @brief file/directory must exist
    mustExist = (1 << 1),
    /// @brief option must not be empty
    notEmpty = (1 << 2),
    /// @brief file has to be executable
    isExe = (1 << 3),
    /// @brief path can be empty
    resolveEmpty = (1 << 4),
};

inline ConfigPathArguments operator|(ConfigPathArguments a, ConfigPathArguments b)
{
    return static_cast<ConfigPathArguments>(static_cast<int>(a) | static_cast<int>(b));
}

inline ConfigPathArguments operator&(ConfigPathArguments a, ConfigPathArguments b)
{
    return static_cast<ConfigPathArguments>(static_cast<int>(a) & static_cast<int>(b));
}

class ConfigPathSetup : public ConfigSetup {
protected:
    ConfigPathArguments arguments;
    /// @brief resolve path against home, an exception is raised if path does not exist on filesystem.
    /// @param path path to be resolved
    fs::path resolvePath(fs::path path) const;

    void loadArguments(const std::map<std::string, std::string>* arguments = nullptr);
    bool checkExecutable(std::string& optValue) const;

    bool isSet(ConfigPathArguments a) const { return (arguments & a) == a; }

public:
    static fs::path Home;

    ConfigPathSetup(
        ConfigVal option,
        const char* xpath,
        const char* help,
        const char* defaultValue = "",
        ConfigPathArguments arguments = ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty)
        : ConfigSetup(option, xpath, help, false, defaultValue)
        , arguments(arguments)
    {
    }

    std::string getTypeString() const override { return "Path"; }

    void makeOption(
        const pugi::xml_node& root,
        const std::shared_ptr<Config>& config,
        const std::map<std::string, std::string>* arguments = nullptr) override;

    void makeOption(
        std::string optValue,
        const std::shared_ptr<Config>& config,
        const std::map<std::string, std::string>* arguments = nullptr) override;

    std::shared_ptr<ConfigOption> newOption(std::string& optValue);
    fs::path getXmlContent(
        const pugi::xml_node& root,
        const std::shared_ptr<Config>& config,
        bool doResolve = false);

    bool checkPathValue(std::string& optValue, std::string& pathValue) const;

    void setFlag(bool hasFlag, ConfigPathArguments flag);
};

#endif
