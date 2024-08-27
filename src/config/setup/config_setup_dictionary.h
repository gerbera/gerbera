/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_dictionary.h - this file is part of Gerbera.
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

/// \file config_setup_dictionary.h
///\brief Definitions of the ConfigSetup classes.

#ifndef __CONFIG_SETUP_DICTIONARY_H__
#define __CONFIG_SETUP_DICTIONARY_H__

#include "config/config_setup.h"

using DictionaryInitFunction = std::function<bool(const pugi::xml_node& value, std::map<std::string, std::string>& result)>;

class ConfigDictionarySetup : public ConfigSetup {
protected:
    bool notEmpty = false;
    bool itemNotEmpty = false;
    DictionaryInitFunction initDict = nullptr;
    bool tolower = false;
    std::map<std::string, std::string> defaultEntries;

    /// \brief Creates a dictionary from an XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param result contents of config.
    ///
    /// The basic idea is the following:
    /// You have a piece of XML that looks like this
    /// \<some-section\>
    ///    \<map from="1" to="2"/\>
    ///    \<map from="3" to="4"/\>
    /// \</some-section\>
    ///
    /// This function will create a dictionary with the following
    /// key:value pairs: "1":"2", "3":"4"
    bool createOptionFromNode(const pugi::xml_node& element, std::map<std::string, std::string>& result) const;

    bool updateItem(const std::vector<std::size_t>& indexList, const std::string& optItem, const std::shared_ptr<Config>& config, const std::shared_ptr<DictionaryOption>& value, const std::string& optKey, const std::string& optValue, const std::string& status = "") const;

public:
    /// \brief name of each node in the set
    ConfigVal nodeOption {};
    /// \brief attribute name to be used as a key
    ConfigVal keyOption {};
    /// \brief attribute name to be used as value
    ConfigVal valOption {};

    explicit ConfigDictionarySetup(ConfigVal option, const char* xpath, const char* help, DictionaryInitFunction init = nullptr,
        bool notEmpty = false, bool itemNotEmpty = false, bool required = false, std::map<std::string, std::string> defaultEntries = {})
        : ConfigSetup(option, xpath, help, required && defaultEntries.empty())
        , notEmpty(notEmpty)
        , itemNotEmpty(itemNotEmpty)
        , initDict(std::move(init))
        , defaultEntries(std::move(defaultEntries))
    {
    }

    explicit ConfigDictionarySetup(ConfigVal option, const char* xpath, const char* help,
        ConfigVal nodeOption, ConfigVal keyOption, ConfigVal valOption,
        bool notEmpty = false, bool itemNotEmpty = false, bool required = false, std::map<std::string, std::string> defaultEntries = {})
        : ConfigSetup(option, xpath, help, required && defaultEntries.empty())
        , notEmpty(notEmpty)
        , itemNotEmpty(itemNotEmpty)
        , defaultEntries(std::move(defaultEntries))
        , nodeOption(nodeOption)
        , keyOption(keyOption)
        , valOption(valOption)
    {
    }

    std::string getTypeString() const override { return "List"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions) const override;
    std::string getItemPathRoot(bool prefix = false) const override;
    std::string getUniquePath() const override;

    std::map<std::string, std::string> getXmlContent(const pugi::xml_node& optValue);

    std::shared_ptr<ConfigOption> newOption(const std::map<std::string, std::string>& optValue);

    std::string getCurrentValue() const override { return {}; }
};

#endif // __CONFIG_SETUP_DICTIONARY_H__
