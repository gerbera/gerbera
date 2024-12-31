/*MT*

    MediaTomb - http://www.mediatomb.cc/

    config_manager.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

#include "config.h"

#include <map>
#include <memory>
#include <netinet/in.h>
#include <pugixml.hpp>

// forward declaration
class AutoscanDirectory;
class BoxLayoutList;
class ClientConfig;
class ClientConfigList;
class ConfigOption;
class ConfigSetup;
class Database;
class DirectoryConfigList;
class DynamicContentList;
class TranscodingProfileList;
enum class ClientType;

class ConfigManager : public Config, public std::enable_shared_from_this<Config> {
public:
    ConfigManager(fs::path filename,
        const fs::path& userHome, const fs::path& configDir,
        fs::path dataDir, bool debug);

    /// \brief Returns the name of the config file that was used to launch the server.
    fs::path getConfigFilename() const override { return filename; }

    void load(const fs::path& userHome);
    bool validate();
    void updateConfigFromDatabase(const std::shared_ptr<Database>& database) override;

    /// \brief add a config option
    /// \param option option type to add.
    /// \param optionValue option to add.
    void addOption(ConfigVal option, const std::shared_ptr<ConfigOption>& optionValue) override;

    /// \brief returns a config option of type std::string
    /// \param option option to retrieve.
    std::string getOption(ConfigVal option) const override;

    /// \brief returns a config option of type int
    /// \param option option to retrieve.
    IntOptionType getIntOption(ConfigVal option) const override;
    UIntOptionType getUIntOption(ConfigVal option) const override;
    LongOptionType getLongOption(ConfigVal option) const override;

    /// \brief returns a config option of any type
    /// \param option option to retrieve.
    std::shared_ptr<ConfigOption> getConfigOption(ConfigVal option) const override;

    /// \brief returns a config option of type bool
    /// \param option option to retrieve.
    bool getBoolOption(ConfigVal option) const override;

    /// \brief returns a config option of type dictionary
    /// \param option option to retrieve.
    std::map<std::string, std::string> getDictionaryOption(ConfigVal option) const override;

    /// \brief returns a config option of type vector
    /// \param option option to retrieve.
    std::vector<std::vector<std::pair<std::string, std::string>>> getVectorOption(ConfigVal option) const override;

    /// \brief returns a config option of type array of string
    /// \param option option to retrieve.
    std::vector<std::string> getArrayOption(ConfigVal option) const override;

    /// \brief returns a config option of type AutoscanList
    /// \param option to retrieve
    std::vector<std::shared_ptr<AutoscanDirectory>> getAutoscanListOption(ConfigVal option) const override;

    /// \brief returns a config option of type ClientConfigList
    /// \param option to retrieve
    std::shared_ptr<ClientConfigList> getClientConfigListOption(ConfigVal option) const override;

    /// \brief returns a config option of type BoxLayoutList
    /// \param option to retrieve
    std::shared_ptr<BoxLayoutList> getBoxLayoutListOption(ConfigVal option) const override;

    /// \brief returns a config option of type DirectoryConfigList
    /// \param option to retrieve
    std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption(ConfigVal option) const override;

    /// \brief returns a config option of type DynamicContentList
    /// \param option to retrieve
    std::shared_ptr<DynamicContentList> getDynamicContentListOption(ConfigVal option) const override;

    /// \brief returns a config option of type TranscodingProfileList
    /// \param option to retrieve
    std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption(ConfigVal option) const override;

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
    void setOrigValue(const std::string& item, IntOptionType value) override;
    void setOrigValue(const std::string& item, UIntOptionType value) override;
    void setOrigValue(const std::string& item, LongOptionType value) override;

protected:
    fs::path filename;
    fs::path dataDir;
    fs::path magicFile;
    std::map<std::string, std::string> origValues;

    std::unique_ptr<pugi::xml_document> xmlDoc { std::make_unique<pugi::xml_document>() };

    std::vector<std::shared_ptr<ConfigOption>> options;

    std::shared_ptr<ConfigOption> setOption(const pugi::xml_node& root, ConfigVal option, const std::map<std::string, std::string>* arguments = nullptr);

    std::shared_ptr<Config> getSelf();
};

#endif // __CONFIG_MANAGER_H__
