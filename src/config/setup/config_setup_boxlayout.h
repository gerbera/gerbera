/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_boxlayout.h - this file is part of Gerbera.
    Copyright (C) 2023-2024 Gerbera Contributors

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

/// \file config_setup_boxlayout.h
/// \brief Definitions of the ConfigBoxLayoutSetup classes.

#ifndef __CONFIG_SETUP_BOXLAYOUT_H__
#define __CONFIG_SETUP_BOXLAYOUT_H__

#include "config/config_setup.h"

class BoxLayout;
class BoxLayoutList;

class ConfigBoxLayoutSetup : public ConfigSetup {
protected:
    std::vector<BoxLayout> defaultEntries;

    /// \brief Creates an array of BoxLayout objects from a XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param result contents of config.
    bool createOptionFromNode(const pugi::xml_node& element, const std::shared_ptr<BoxLayoutList>& result);

    bool updateItem(std::size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<BoxLayout>& entry, std::string& optValue, const std::string& status = "") const;

public:
    ConfigBoxLayoutSetup(ConfigVal option, const char* xpath, const char* help, std::vector<BoxLayout> defaultEntries)
        : ConfigSetup(option, xpath, help)
        , defaultEntries(std::move(defaultEntries))
    {
    }

    std::string getTypeString() const override { return "List"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions) const override;
    std::string getItemPathRoot(bool prefix = false) const override;

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue);

    std::string getCurrentValue() const override { return {}; }

    const std::vector<BoxLayout>& getDefault() const { return defaultEntries; }

    bool validate(const std::shared_ptr<Config>& config, const std::shared_ptr<BoxLayoutList>& values);
};

#endif // __CONFIG_SETUP_BOXLAYOUT_H__
