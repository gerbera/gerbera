/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    config_manager.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/// \file config_manager.h
///\brief Definitions of the ConfigManager class.

#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

#include <chrono> // for filesystem
#include <filesystem> // for path
#include <map> // for map, operator!=, operator==, map<>::const...
#include <memory> // for shared_ptr, allocator, unique_ptr, enable...
#include <netinet/in.h> // for in_port_t
#include <string> // for string, basic_string, operator<
#include <vector> // for vector

namespace fs = std::filesystem;

#include "config/config.h" // for config_option_t, Config

class AutoscanDirectory;
// forward declaration
class AutoscanList;
class ClientConfig;
class ClientConfigList;
class ConfigSetup;
class DirectoryConfigList;

namespace pugi {
class xml_document;
class xml_node;
} // namespace pugi

enum class ScanMode;
enum class ClientType;
class ConfigOption;
class Database;
class TranscodingProfileList;

class ConfigManager : public Config, public std::enable_shared_from_this<Config> {
public:
    ConfigManager(fs::path filename,
        const fs::path& userHome, const fs::path& configDir,
        fs::path dataDir, fs::path magicFile,
        std::string ip, std::string interface, in_port_t port,
        bool debug);
    ~ConfigManager() override;

    /// \brief Returns the name of the config file that was used to launch the server.
    fs::path getConfigFilename() const override { return filename; }

    static const std::vector<std::shared_ptr<ConfigSetup>>& getOptionList()
    {
        return complexOptions;
    }

    static const char* mapConfigOption(config_option_t option);

    static std::shared_ptr<ConfigSetup> findConfigSetup(config_option_t option, bool save = false);
    static std::shared_ptr<ConfigSetup> findConfigSetupByPath(const std::string& key, bool save = false, const std::shared_ptr<ConfigSetup>& parent = nullptr);

    void load(const fs::path& userHome);
    void updateConfigFromDatabase(std::shared_ptr<Database> database) override;

    /// \brief add a config option
    /// \param option option type to add.
    /// \param option option to add.
    void addOption(config_option_t option, std::shared_ptr<ConfigOption> optionValue) override;

    /// \brief returns a config option of type std::string
    /// \param option option to retrieve.
    std::string getOption(config_option_t option) const override;

    /// \brief returns a config option of type int
    /// \param option option to retrieve.
    int getIntOption(config_option_t option) const override;

    /// \brief returns a config option of type bool
    /// \param option option to retrieve.
    bool getBoolOption(config_option_t option) const override;

    /// \brief returns a config option of type dictionary
    /// \param option option to retrieve.
    std::map<std::string, std::string> getDictionaryOption(config_option_t option) const override;

    /// \brief returns a config option of type array of string
    /// \param option option to retrieve.
    std::vector<std::string> getArrayOption(config_option_t option) const override;

    /// \brief returns a config option of type AutoscanList
    /// \param option to retrieve
    std::shared_ptr<AutoscanList> getAutoscanListOption(config_option_t option) const override;

    /// \brief returns a config option of type ClientConfigList
    /// \param option to retrieve
    std::shared_ptr<ClientConfigList> getClientConfigListOption(config_option_t option) const override;

    /// \brief returns a config option of type DirectoryConfigList
    /// \param option to retrieve
    std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption(config_option_t option) const override;

    /// \brief returns a config option of type TranscodingProfileList
    /// \param option to retrieve
    std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption(config_option_t option) const override;

    bool hasOrigValue(const std::string& item) const override
    {
        return origValues.find(item) != origValues.end();
    }

    std::string getOrigValue(const std::string& item) const override
    {
        return (origValues.find(item) == origValues.end()) ? "" : origValues.at(item);
    }

    void setOrigValue(const std::string& item, const std::string& value) override;
    void setOrigValue(const std::string& item, bool value) override;
    void setOrigValue(const std::string& item, int value) override;

    static bool isDebugLogging() { return debug; }
    fs::path getDataDir() const override { return dataDir; }

protected:
    static const std::vector<std::shared_ptr<ConfigSetup>> complexOptions;
    static const std::map<config_option_t, std::vector<config_option_t>> parentOptions;

    fs::path filename;
    fs::path dataDir;
    fs::path magicFile;
    std::string ip;
    std::string interface;
    in_port_t port;
    static bool debug;
    std::map<std::string, std::string> origValues;

    std::unique_ptr<pugi::xml_document> xmlDoc;

    std::unique_ptr<std::vector<std::shared_ptr<ConfigOption>>> options;

    std::shared_ptr<ConfigOption> setOption(const pugi::xml_node& root, config_option_t option, const std::map<std::string, std::string>* arguments = nullptr);

    std::shared_ptr<Config> getSelf();
};

#endif // __CONFIG_MANAGER_H__
