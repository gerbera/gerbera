/*GRB*

    Gerbera - https://gerbera.io/

    config.h - this file is part of Gerbera.

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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "config_int_types.h"
#include "util/grb_fs.h"

#include <map>
#include <vector>

// forward declarations
class AutoscanDirectory;
class BoxLayoutList;
class ClientConfigList;
class ConfigOption;
class Database;
class DirectoryConfigList;
class DynamicContentList;
class TranscodingProfileList;

enum class ConfigVal;
enum class UrlAppendMode;
enum class LayoutType;

class Config {
public:
    virtual ~Config() = default;
    virtual void updateConfigFromDatabase(const std::shared_ptr<Database>& database) = 0;
    virtual std::string getOrigValue(const std::string& item) const = 0;
    virtual void setOrigValue(const std::string& item, const std::string& value) = 0;
    virtual void setOrigValue(const std::string& item, bool value) = 0;
    virtual void setOrigValue(const std::string& item, IntOptionType value) = 0;
    virtual void setOrigValue(const std::string& item, UIntOptionType value) = 0;
    virtual void setOrigValue(const std::string& item, LongOptionType value) = 0;
    virtual bool hasOrigValue(const std::string& item) const = 0;

    /// \brief Returns the path of the config file that was used to launch the server.
    virtual fs::path getConfigFilename() const = 0;

    /// \brief add a config option
    /// \param option option type to add.
    /// \param optionValue option to add.
    virtual void addOption(ConfigVal option, const std::shared_ptr<ConfigOption>& optionValue) = 0;

    virtual std::shared_ptr<ConfigOption> getConfigOption(ConfigVal option) const = 0;

    /// \brief returns a config option of type std::string
    /// \param option option to retrieve.
    virtual std::string getOption(ConfigVal option) const = 0;

    /// \brief returns a config option of type int
    /// \param option option to retrieve.
    virtual IntOptionType getIntOption(ConfigVal option) const = 0;
    virtual UIntOptionType getUIntOption(ConfigVal option) const = 0;
    virtual LongOptionType getLongOption(ConfigVal option) const = 0;

    /// \brief returns a config option of type bool
    /// \param option option to retrieve.
    virtual bool getBoolOption(ConfigVal option) const = 0;

    /// \brief returns a config option of type dictionary
    /// \param option option to retrieve.
    virtual std::map<std::string, std::string> getDictionaryOption(ConfigVal option) const = 0;

    /// \brief returns a config option of type vector
    /// \param option option to retrieve.
    virtual std::vector<std::vector<std::pair<std::string, std::string>>> getVectorOption(ConfigVal option) const = 0;

    /// \brief returns a config option of type array of string
    /// \param option option to retrieve.
    virtual std::vector<std::string> getArrayOption(ConfigVal option) const = 0;

    /// \brief returns a config option of type AutoscanList
    /// \param option to retrieve
    virtual std::vector<std::shared_ptr<AutoscanDirectory>> getAutoscanListOption(ConfigVal option) const = 0;

    /// \brief returns a config option of type ClientConfigList
    /// \param option to retrieve
    virtual std::shared_ptr<ClientConfigList> getClientConfigListOption(ConfigVal option) const = 0;

    /// \brief returns a config option of type BoxLayoutList
    /// \param option to retrieve
    virtual std::shared_ptr<BoxLayoutList> getBoxLayoutListOption(ConfigVal option) const = 0;

    /// \brief returns a config option of type DirectoryConfigList
    /// \param option to retrieve
    virtual std::shared_ptr<DirectoryConfigList> getDirectoryTweakOption(ConfigVal option) const = 0;

    /// \brief returns a config option of type TranscodingProfileList
    /// \param option to retrieve
    virtual std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption(ConfigVal option) const = 0;

    /// \brief returns a config option of type DynamicContentList
    /// \param option to retrieve
    virtual std::shared_ptr<DynamicContentList> getDynamicContentListOption(ConfigVal option) const = 0;
};

#endif // __CONFIG_H__
