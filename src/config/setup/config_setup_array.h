/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_array.h - this file is part of Gerbera.

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

/// \file config_setup_array.h
///\brief Definitions of the ConfigSetupArray classes.

#ifndef __CONFIG_SETUP_ARRAY_H__
#define __CONFIG_SETUP_ARRAY_H__

#include "config/config_setup.h"
#include "config/config_val.h"

using ArrayInitFunction = std::function<bool(const pugi::xml_node& value, std::vector<std::string>& result, const char* node_name)>;
using ArrayItemCheckFunction = std::function<bool(const std::string& value)>;

class ConfigArraySetup : public ConfigSetup {
protected:
    bool notEmpty = false;
    bool itemNotEmpty = false;
    ArrayInitFunction initArray = nullptr;
    ArrayItemCheckFunction itemCheck = nullptr;
    std::vector<std::string> defaultEntries;
    bool doExtend = false;

    /// \brief Creates an array of strings from an XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param result vector with contents of array
    ///
    /// Similar to \fn ConfigDictionarySetup#createOptionFromNode() this one extracts
    /// data from the following XML:
    /// \<some-section\>
    ///     \<tag attr="data"/\>
    ///     \<tag attr="otherdata"/\>
    /// \</some-section\>
    ///
    /// This function will create an array like that: ["data", "otherdata"]
    bool createOptionFromNode(
        const pugi::xml_node& element,
        std::vector<std::string>& result);

    bool updateItem(
        const std::vector<std::size_t>& indexList,
        const std::string& optItem,
        const std::shared_ptr<Config>& config,
        const std::shared_ptr<ArrayOption>& value,
        const std::string& optValue,
        const std::string& status = "") const;

public:
    ConfigVal nodeOption;
    ConfigVal attrOption = ConfigVal::MAX;

    ConfigArraySetup(ConfigVal option, const char* xpath, const char* help, ConfigVal nodeOption,
        ArrayInitFunction init = nullptr, bool notEmpty = false, std::vector<std::string> defaultEntries = {})
        : ConfigSetup(option, xpath, help)
        , notEmpty(notEmpty)
        , initArray(std::move(init))
        , defaultEntries(std::move(defaultEntries))
        , nodeOption(nodeOption)
    {
    }

    ConfigArraySetup(ConfigVal option, const char* xpath, const char* help, ConfigVal nodeOption, ConfigVal attrOption,
        bool notEmpty, bool itemNotEmpty, std::vector<std::string> defaultEntries = {})
        : ConfigSetup(option, xpath, help)
        , notEmpty(notEmpty)
        , itemNotEmpty(itemNotEmpty)
        , defaultEntries(std::move(defaultEntries))
        , nodeOption(nodeOption)
        , attrOption(attrOption)
    {
    }

    ConfigArraySetup(ConfigVal option, const char* xpath, const char* help, ConfigVal nodeOption, ConfigVal attrOption,
        ArrayItemCheckFunction itemCheck, std::vector<std::string> defaultEntries = {})
        : ConfigSetup(option, xpath, help)
        , itemCheck(std::move(itemCheck))
        , defaultEntries(std::move(defaultEntries))
        , nodeOption(nodeOption)
        , attrOption(attrOption)
    {
    }

    std::string getTypeString() const override { return "List"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool createNodeFromDefaults(const std::shared_ptr<pugi::xml_node>& result) const override;
    bool updateDetail(
        const std::string& optItem,
        std::string& optValue,
        const std::shared_ptr<Config>& config,
        const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions, const std::string& propText = "") const override;
    std::string getItemPathRoot(bool prefix = false) const override;

    std::vector<std::string> getXmlContent(const pugi::xml_node& optValue);

    bool checkArrayValue(const std::string& value, std::vector<std::string>& result) const;

    std::shared_ptr<ConfigOption> newOption(const std::vector<std::string>& optValue);

    std::string getCurrentValue() const override { return {}; }

    static bool InitPlayedItemsMark(const pugi::xml_node& value, std::vector<std::string>& result, const char* nodeName);

    static bool InitItemsPerPage(const pugi::xml_node& value, std::vector<std::string>& result, const char* nodeName);
};

#endif // __CONFIG_SETUP_ARRAY_H__
