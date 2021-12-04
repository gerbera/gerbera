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

#pragma once

#include "common.h"
#include "config.h"
#include "util/tools.h"

class ConfigSetup;

class ConfigDefinition {
public:
    static constexpr auto ATTRIBUTE = std::string_view("attribute::");

    static std::vector<std::shared_ptr<ConfigSetup>> getOptionList()
    {
        return complexOptions;
    }

    static const char* mapConfigOption(config_option_t option);
    /// \brief check whether option is only available when other option is set
    static bool isDependent(config_option_t option);

    static std::shared_ptr<ConfigSetup> findConfigSetup(config_option_t option, bool save = false);
    static std::shared_ptr<ConfigSetup> findConfigSetupByPath(const std::string& key, bool save = false, const std::shared_ptr<ConfigSetup>& parent = nullptr);
    template <class CS>
    static std::vector<std::shared_ptr<CS>> getConfigSetupList()
    {
        std::vector<std::shared_ptr<CS>> result;
        for (auto&& co : complexOptions) {
            auto tco = std::dynamic_pointer_cast<CS>(co);
            if (tco && tco->getValue()) {
                result.push_back(std::move(tco));
            }
        }
        return result;
    }

    template <class CS>
    static std::shared_ptr<CS> findConfigSetup(config_option_t option, bool save = false)
    {
        auto base = findConfigSetup(option, save);
        if (!base && save)
            return nullptr;

        auto result = std::dynamic_pointer_cast<CS>(base);
        if (!result) {
            throw_std_runtime_error("Error in config code: {} has wrong class", option);
        }
        return result;
    }

    static std::string ensureAttribute(config_option_t option, bool check = true)
    {
        auto attr = std::string(mapConfigOption(option));
        if (!startswith(attr, ATTRIBUTE) && check)
            return fmt::format("{}{}", ATTRIBUTE, attr);
        return attr;
    }
    static std::string removeAttribute(config_option_t option)
    {
        auto attr = std::string(mapConfigOption(option));
        if (attr.size() > ATTRIBUTE.size() && startswith(attr, ATTRIBUTE))
            attr = attr.substr(ATTRIBUTE.size());
        return attr;
    }

private:
    /// \brief all known options
    static const std::vector<std::shared_ptr<ConfigSetup>> complexOptions;
    /// \brief parent options for path search
    static const std::map<config_option_t, std::vector<config_option_t>> parentOptions;
    /// \brief option dependencies for automatic loading
    static const std::map<config_option_t, config_option_t> dependencyMap;
};
