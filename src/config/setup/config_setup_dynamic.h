/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_dynamic.h - this file is part of Gerbera.

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

/// @file config/setup/config_setup_dynamic.h
/// @brief Definitions of the ConfigDynamicContentSetup classes.

#ifndef __CONFIG_SETUP_DYNAMIC_H__
#define __CONFIG_SETUP_DYNAMIC_H__

#include "config/config_setup.h"

class DynamicContent;

/// @brief Setup of dynamic content reader
class ConfigDynamicContentSetup : public ConfigSetup {
    using ConfigSetup::ConfigSetup;

protected:
    /// @brief Creates an array of DynamicContent objects from a XML nodeset.
    /// @param config manager for registration
    /// @param element starting element of the nodeset.
    /// @param result contents of config.
    bool createOptionFromNode(
        const std::shared_ptr<Config>& config,
        const pugi::xml_node& element,
        std::shared_ptr<DynamicContentList>& result) const;

public:
    bool updateItem(
        const std::vector<std::size_t>& indexList,
        const std::string& optItem,
        const std::shared_ptr<Config>& config,
        std::shared_ptr<DynamicContent>& entry,
        std::string& optValue,
        const std::string& status = "") const;

    std::string getTypeString() const override { return "List"; }

    void makeOption(
        const pugi::xml_node& root,
        const std::shared_ptr<Config>& config,
        const std::map<std::string, std::string>* arguments = nullptr) override;

    bool updateDetail(
        const std::string& optItem,
        std::string& optValue,
        const std::shared_ptr<Config>& config,
        const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getItemPath(
        const std::vector<std::size_t>& indexList,
        const std::vector<ConfigVal>& propOptions,
        const std::string& propText = "") const override;
    std::string getItemPathRoot(bool prefix = false) const override;

    std::shared_ptr<ConfigOption> newOption(
        const std::shared_ptr<Config>& config,
        const pugi::xml_node& optValue);

    std::string getCurrentValue() const override { return {}; }
};

#endif //__CONFIG_SETUP_DYNAMIC_H__
