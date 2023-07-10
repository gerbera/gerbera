/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_transcoding.h - this file is part of Gerbera.
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

/// \file config_setup_transcoding.h
///\brief Definitions of the ConfigTranscodingSetup classes.

#ifndef __CONFIG_SETUP_TRANSCODING_H__
#define __CONFIG_SETUP_TRANSCODING_H__

#include "config/config_setup.h"

class ConfigTranscodingSetup : public ConfigSetup {
    using ConfigSetup::ConfigSetup;

protected:
    bool isEnabled = false;

    /// \brief Creates an array of TranscodingProfile objects from an XML
    /// nodeset.
    /// \param element starting element of the nodeset.
    static bool createOptionFromNode(const pugi::xml_node& element, std::shared_ptr<TranscodingProfileList>& result);

public:
    std::string getTypeString() const override { return "List"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getItemPath(int index = 0, config_option_t propOption = CFG_MAX, config_option_t propOption2 = CFG_MAX, config_option_t propOption3 = CFG_MAX, config_option_t propOption4 = CFG_MAX) const override;

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue);

    std::string getCurrentValue() const override { return {}; }
};

#endif // __CONFIG_SETUP_TRANSCODING_H__
