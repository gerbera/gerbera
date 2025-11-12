/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_boxlayout.h - this file is part of Gerbera.

    Copyright (C) 2023-2025 Gerbera Contributors

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

/// @file config/setup/config_setup_boxlayout.h
/// @brief Definitions of the ConfigBoxLayoutSetup classes.

#ifndef __CONFIG_SETUP_BOXLAYOUT_H__
#define __CONFIG_SETUP_BOXLAYOUT_H__

#include "config/config_setup.h"

class BoxChain;
class BoxLayout;
class BoxLayoutList;

/// @brief Class for BoxLayout config parser
class ConfigBoxLayoutSetup : public ConfigSetup {
protected:
    std::vector<BoxLayout> defaultEntries;

    /// @brief Creates an array of BoxLayout objects from a XML nodeset.
    /// @param config manager for registration
    /// @param element starting element of the nodeset.
    /// @param result contents of config.
    bool createOptionFromNode(
        const std::shared_ptr<Config>& config,
        const pugi::xml_node& element,
        const std::shared_ptr<BoxLayoutList>& result);

public:
    ConfigBoxLayoutSetup(
        ConfigVal option,
        const char* xpath,
        const char* help,
        std::vector<BoxLayout> defaultEntries);
    ~ConfigBoxLayoutSetup() override;

    /// @brief Update Option from Web UI or Database
    bool updateItem(
        const std::vector<std::size_t>& indexList,
        const std::string& optItem,
        const std::shared_ptr<Config>& config,
        std::shared_ptr<BoxLayout>& entry,
        std::string& optValue,
        const std::string& status = "") const;

    /// @brief Update Option from Web UI or Database
    bool updateItem(
        const std::vector<std::size_t>& indexList,
        const std::string& optItem,
        const std::shared_ptr<Config>& config,
        std::shared_ptr<BoxChain>& entry,
        std::string& optValue,
        const std::string& status = "") const;

    /// @brief make config option from xml content
    std::shared_ptr<ConfigOption> newOption(
        const std::shared_ptr<Config>& config,
        const pugi::xml_node& optValue);

    /// @brief get default value
    const std::vector<BoxLayout>& getDefault() const { return defaultEntries; }

    /// @brief Ensure that config value complies with rules
    bool validate(
        const std::shared_ptr<Config>& config,
        const std::shared_ptr<BoxLayoutList>& values);

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

    std::string getCurrentValue() const override { return {}; }

    constexpr static std::string_view linkKey = "key";
    constexpr static std::string_view linkValue = "value";
};

#endif // __CONFIG_SETUP_BOXLAYOUT_H__
