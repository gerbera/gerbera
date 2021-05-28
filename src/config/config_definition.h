/*GRB*

    Gerbera - https://gerbera.io/

    config_definition.h - this file is part of Gerbera.

    Copyright (C) 2020-2021 Gerbera Contributors

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

/// \file config_definition.h
///\brief Definitions of class ConfigDefinition for default values and setup for configuration.

#ifndef __CONFIG_DEFINITION_H__
#define __CONFIG_DEFINITION_H__

#include "common.h"
#include "config.h"

class ConfigSetup;

class ConfigDefinition {
public:
    static constexpr auto ATTRIBUTE = std::string_view("attribute::");

    static const std::vector<std::shared_ptr<ConfigSetup>>& getOptionList()
    {
        return complexOptions;
    }

    static const char* mapConfigOption(config_option_t option);

    static std::shared_ptr<ConfigSetup> findConfigSetup(config_option_t option, bool save = false);
    static std::shared_ptr<ConfigSetup> findConfigSetupByPath(const std::string& key, bool save = false, const std::shared_ptr<ConfigSetup>& parent = nullptr);
    template <class CS>
    static std::vector<std::shared_ptr<CS>> getConfigSetupList()
    {
        std::vector<std::shared_ptr<CS>> result;
        for (auto&& co : complexOptions) {
            std::shared_ptr<CS> tco = std::dynamic_pointer_cast<CS>(co);
            if (tco != nullptr && tco->getValue() != nullptr) {
                result.emplace_back(tco);
            }
        }
        return result;
    }

    template <class CS>
    static std::shared_ptr<CS> findConfigSetup(config_option_t option, bool save = false)
    {
        std::shared_ptr<ConfigSetup> base = findConfigSetup(option, save);
        if (base == nullptr && save)
            return nullptr;

        std::shared_ptr<CS> result = std::dynamic_pointer_cast<CS>(base);
        if (result == nullptr) {
            throw_std_runtime_error("Error in config code: {} has wrong class", option);
        }
        return result;
    }

    static std::string ensureAttribute(config_option_t option, bool check = true)
    {
        auto attr = std::string(mapConfigOption(option));
        if (attr.substr(0, ATTRIBUTE.size()) != ATTRIBUTE && check)
            attr.insert(0, ATTRIBUTE);
        return attr;
    }
    static std::string removeAttribute(config_option_t option)
    {
        auto attr = std::string(mapConfigOption(option));
        if (attr.size() > ATTRIBUTE.size() && attr.substr(0, ATTRIBUTE.size()) == ATTRIBUTE)
            attr = attr.substr(ATTRIBUTE.size());
        return attr;
    }

private:
    static const std::vector<std::shared_ptr<ConfigSetup>> complexOptions;
    static const std::map<config_option_t, std::vector<config_option_t>> parentOptions;
};

#endif // __CONFIG_DEFINITION_H__
