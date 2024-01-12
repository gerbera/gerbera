/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_path.h - this file is part of Gerbera.
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

/// \file config_setup_path.h
///\brief Definitions of the ConfigPathSetup class.

#ifndef __CONFIG_SETUP_PATH_H__
#define __CONFIG_SETUP_PATH_H__

#include "config/config_setup.h"

enum class ConfigPathArguments {
    none = 0,
    isFile = (1 << 0),
    mustExist = (1 << 1),
    notEmpty = (1 << 2),
    isExe = (1 << 3),
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
    /// \brief resolve path against home, an exception is raised if path does not exist on filesystem.
    /// \param path path to be resolved
    /// \param isFile file or directory
    /// \param mustExist file/directory must exist
    fs::path resolvePath(fs::path path) const;

    void loadArguments(const std::map<std::string, std::string>* arguments = nullptr);
    bool checkExecutable(std::string& optValue) const;

    bool isSet(ConfigPathArguments a) const { return (arguments & a) == a; }

public:
    static fs::path Home;

    ConfigPathSetup(config_option_t option, const char* xpath, const char* help, const char* defaultValue = "", ConfigPathArguments arguments = ConfigPathArguments::mustExist | ConfigPathArguments::resolveEmpty)
        : ConfigSetup(option, xpath, help, false, defaultValue)
        , arguments(ConfigPathArguments(arguments))
    {
    }

    std::string getTypeString() const override { return "Path"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    void makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::shared_ptr<ConfigOption> newOption(std::string& optValue);
    std::string getXmlContent(const pugi::xml_node& root);

    bool checkPathValue(std::string& optValue, std::string& pathValue) const;

    void setFlag(bool hasFlag, ConfigPathArguments flag);
};

#endif