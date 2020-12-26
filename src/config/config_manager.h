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

#include <filesystem>
#include <map>
#include <memory>
#include <netinet/in.h>
#include <pugixml.hpp>
namespace fs = std::filesystem;

#include "common.h"
#include "config.h"

// forward declaration
class AutoscanList;
class AutoscanDirectory;
class ClientConfigList;
class ClientConfig;
class ConfigSetup;
class DirectoryConfigList;
enum class ScanMode;
enum class ClientType;
class ConfigOption;
class Database;
class TranscodingProfileList;

class ConfigManager : public Config, public std::enable_shared_from_this<Config> {
public:
    ConfigManager(fs::path filename,
        const fs::path& userhome, const fs::path& config_dir,
        fs::path prefix_dir, fs::path magic_file,
        std::string ip, std::string interface, in_port_t port,
        bool debug_logging);
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

    static bool isDebugLogging() { return debug_logging; }

protected:
    static const std::vector<std::shared_ptr<ConfigSetup>> complexOptions;
    static const std::array<std::pair<config_option_t, const char*>, 16> simpleOptions;
    static const std::map<config_option_t, std::vector<config_option_t>> parentOptions;

    fs::path filename;
    fs::path prefix_dir;
    fs::path magic_file;
    std::string ip;
    std::string interface;
    in_port_t port;
    static bool debug_logging;
    std::map<std::string, std::string> origValues;

    std::unique_ptr<pugi::xml_document> xmlDoc;

    std::unique_ptr<std::vector<std::shared_ptr<ConfigOption>>> options;

    std::shared_ptr<ConfigOption> setOption(const pugi::xml_node& root, config_option_t option, const std::map<std::string, std::string>* arguments = nullptr);

    std::shared_ptr<Config> getSelf();

    void dumpOptions() const;
};

#endif // __CONFIG_MANAGER_H__
