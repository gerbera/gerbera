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
enum class ScanMode;
enum class ClientType;
class ConfigOption;
class Storage;
class TranscodingProfileList;

class ConfigManager : public Config, public std::enable_shared_from_this<Config> {
public:
    ConfigManager(fs::path filename,
        const fs::path& userhome, const fs::path& config_dir,
        fs::path prefix_dir, fs::path magic_file,
        std::string ip, std::string interface, int port,
        bool debug_logging);
    virtual ~ConfigManager();

    /// \brief Returns the name of the config file that was used to launch the server.
    fs::path getConfigFilename() const override { return filename; }

    static const char* mapConfigOption(config_option_t option);

    static std::shared_ptr<ConfigSetup> findConfigSetup(config_option_t option, bool save = false);

    void load(const fs::path& userHome);

    /// \brief add a config option
    /// \param option option type to add.
    /// \param option option to add.
    void addOption(config_option_t option, std::shared_ptr<ConfigOption> optionValue) override;

    /// \brief returns a config option of type std::string
    /// \param option option to retrieve.
    std::string getOption(config_option_t option) override;

    /// \brief returns a config option of type int
    /// \param option option to retrieve.
    int getIntOption(config_option_t option) override;

    /// \brief returns a config option of type bool
    /// \param option option to retrieve.
    bool getBoolOption(config_option_t option) override;

    /// \brief returns a config option of type dictionary
    /// \param option option to retrieve.
    std::map<std::string, std::string> getDictionaryOption(config_option_t option) override;

    /// \brief returns a config option of type array of string
    /// \param option option to retrieve.
    std::vector<std::string> getArrayOption(config_option_t option) override;

    /// \brief returns a config option of type AutoscanList
    /// \param option to retrieve
    std::shared_ptr<AutoscanList> getAutoscanListOption(config_option_t option) override;

    /// \brief returns a config option of type ClientConfigList
    /// \param option to retrieve
    std::shared_ptr<ClientConfigList> getClientConfigListOption(config_option_t option) override;

    /// \brief returns a config option of type TranscodingProfileList
    /// \param option to retrieve
    std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption(config_option_t option) override;

    static bool isDebugLogging() { return debug_logging; }

protected:
    fs::path filename;
    fs::path prefix_dir;
    fs::path magic_file;
    std::string ip;
    std::string interface;
    int port;
    static bool debug_logging;

    std::unique_ptr<pugi::xml_document> xmlDoc;

    std::unique_ptr<std::vector<std::shared_ptr<ConfigOption>>> options;

    std::shared_ptr<ConfigOption> setOption(const pugi::xml_node& root, config_option_t option, const std::map<std::string, std::string>* arguments = nullptr);

    std::shared_ptr<Config> getSelf();

    void dumpOptions();
};

#endif // __CONFIG_MANAGER_H__
