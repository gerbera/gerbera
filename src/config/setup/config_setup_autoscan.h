/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_autoscan.h - this file is part of Gerbera.
    Copyright (C) 2020-2023 Gerbera Contributors

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

/// \file config_setup_autoscan.h
///\brief Definitions of the ConfigSetup classes.

#ifndef __CONFIG_SETUP_AUTOSCAN_H__
#define __CONFIG_SETUP_AUTOSCAN_H__

#include "config/config_setup.h"

class AutoscanDirectory;
class AutoscanList;
enum class AutoscanScanMode;

class ConfigAutoscanSetup : public ConfigSetup {
protected:
    AutoscanScanMode scanMode;
    bool hiddenFiles = false;
    bool followSymlinks = false;

    /// \brief Creates an array of AutoscanDirectory objects from a XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param scanmode add only directories with the specified scanmode to the array
    bool createOptionFromNode(const pugi::xml_node& element, std::vector<std::shared_ptr<AutoscanDirectory>>& result);

    bool updateItem(std::size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, const std::shared_ptr<AutoscanDirectory>& entry, std::string& optValue, const std::string& status = "") const;

public:
    ConfigAutoscanSetup(config_option_t option, const char* xpath, const char* help, AutoscanScanMode scanmode)
        : ConfigSetup(option, xpath, help)
        , scanMode(scanmode)
    {
    }

    std::string getTypeString() const override { return "List"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getUniquePath() const override;

    std::string getItemPath(int index = 0, config_option_t propOption = CFG_MAX, config_option_t propOption2 = CFG_MAX, config_option_t propOption3 = CFG_MAX, config_option_t propOption4 = CFG_MAX) const override;

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue);

    std::string getCurrentValue() const override { return {}; }
};

#endif
