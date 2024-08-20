/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_vector.h - this file is part of Gerbera.
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

/// \file config_setup_vector.h
/// \brief Definitions of the ConfigSetupVector class.

#ifndef __CONFIG_SETUP_VECTOR_H__
#define __CONFIG_SETUP_VECTOR_H__

#include "config/config_setup.h"

class ConfigVectorSetup : public ConfigSetup {
protected:
    bool notEmpty = false;
    bool itemNotEmpty = false;
    bool tolower = false;
    std::vector<std::vector<std::pair<std::string, std::string>>> defaultEntries;

    /// \brief Creates a vector from an XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param result contents of config.
    ///
    /// The basic idea is the following:
    /// You have a piece of XML that looks like this
    /// \<some-section\>
    ///    \<map from="1" via="3" to="2"/\>
    ///    \<map from="3" to="4"/\>
    /// \</some-section\>
    ///
    /// This function will create a vector with the following
    /// list: { { "1", "3", "2" }, {"3", "", "4"}
    bool createOptionFromNode(const pugi::xml_node& element, std::vector<std::vector<std::pair<std::string, std::string>>>& result) const;

    bool updateItem(std::size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, const std::shared_ptr<VectorOption>& value, const std::string& optValue, const std::string& status = "") const;

public:
    ConfigVal nodeOption {};
    std::vector<ConfigVal> optionList {};

    explicit ConfigVectorSetup(ConfigVal option, const char* xpath, const char* help,
        ConfigVal nodeOption, std::vector<ConfigVal> optionList,
        bool notEmpty = false, bool itemNotEmpty = false, bool required = false, std::vector<std::vector<std::pair<std::string, std::string>>> defaultEntries = {})
        : ConfigSetup(option, xpath, help, required && defaultEntries.empty())
        , notEmpty(notEmpty)
        , itemNotEmpty(itemNotEmpty)
        , defaultEntries(std::move(defaultEntries))
        , nodeOption(nodeOption)
        , optionList(std::move(optionList))
    {
    }

    std::string getTypeString() const override { return "List"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getItemPath(const std::vector<std::size_t>& indexList, const std::string& propOption) const;
    std::string getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions) const override;
    std::string getItemPathRoot(bool prefix = false) const override;

    std::vector<std::vector<std::pair<std::string, std::string>>> getXmlContent(const pugi::xml_node& optValue);

    std::shared_ptr<ConfigOption> newOption(const std::vector<std::vector<std::pair<std::string, std::string>>>& optValue);

    std::string getCurrentValue() const override { return {}; }
};

#endif // __CONFIG_SETUP_VECTOR_H__