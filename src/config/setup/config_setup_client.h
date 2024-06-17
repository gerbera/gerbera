/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_client.h - this file is part of Gerbera.
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

/// \file config_setup_client.h
///\brief Definitions of the ConfigClientSetup classes.

#ifndef __CONFIG_SETUP_CLIENT_H__
#define __CONFIG_SETUP_CLIENT_H__

#include "config/config_setup.h"

class ClientConfig;

class ConfigClientSetup : public ConfigSetup {
    using ConfigSetup::ConfigSetup;

protected:
    bool isEnabled = false;

    /// \brief Creates an array of ClientConfig objects from a XML nodeset.
    /// \param element starting element of the nodeset.
    static bool createOptionFromNode(const pugi::xml_node& element, const std::shared_ptr<ClientConfigList>& result);

    bool updateItem(std::size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<ClientConfig>& entry, std::string& optValue, const std::string& status = "") const;

public:
    std::string getTypeString() const override { return "List"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getItemPath(int index, const std::vector<ConfigVal>& propOptions) const override;

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue);

    std::string getCurrentValue() const override { return {}; }
};

#endif // __CONFIG_SETUP_CLIENT_H__
