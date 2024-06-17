/*GRB*

    Gerbera - https://gerbera.io/

    config_definition.h - this file is part of Gerbera.

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

/// \file config_definition.h
///\brief Definitions of class ConfigDefinition for default values and setup for configuration.

#ifndef __CONFIG_DEFINITION_H__
#define __CONFIG_DEFINITION_H__

#include "config.h"
#include "exceptions.h"
#include "util/tools.h"

#include <fmt/format.h>

#define CONFIG_XML_VERSION 2

// default values
#define DEFAULT_FILESYSTEM_CHARSET "UTF-8"
#define DEFAULT_FALLBACK_CHARSET "US-ASCII"

#define DEFAULT_CONFIG_NAME "config.xml"
#define DEFAULT_ACCOUNT_USER "gerbera"
#define DEFAULT_ACCOUNT_PASSWORD "gerbera"
#define ALIVE_INTERVAL_MIN 62 // seconds
#define DEFAULT_WEB_DIR "web"
#define DEFAULT_JS_DIR "js"

#define DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE "prepend"

#define DEFAULT_AUDIO_BUFFER_SIZE 1048576
#define DEFAULT_AUDIO_CHUNK_SIZE 131072
#define DEFAULT_AUDIO_FILL_SIZE 262144

#define DEFAULT_VIDEO_BUFFER_SIZE 14400000
#define DEFAULT_VIDEO_CHUNK_SIZE 512000
#define DEFAULT_VIDEO_FILL_SIZE 120000

// device description defaults
#define DESC_MANUFACTURER_URL "https://gerbera.io/"

class ConfigSetup;

class ConfigDefinition {
public:
    static constexpr auto ATTRIBUTE = std::string_view("attribute::");

    static std::vector<std::shared_ptr<ConfigSetup>> getOptionList()
    {
        return complexOptions;
    }

    static const char* mapConfigOption(ConfigVal option);
    /// \brief check whether option is only available when other option is set
    static bool isDependent(ConfigVal option);

    static std::shared_ptr<ConfigSetup> findConfigSetup(ConfigVal option, bool save = false);
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
    static std::shared_ptr<CS> findConfigSetup(ConfigVal option, bool save = false)
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

    static std::string ensureAttribute(ConfigVal option, bool check = true)
    {
        auto attr = std::string(mapConfigOption(option));
        if (!startswith(attr, ATTRIBUTE) && check)
            return fmt::format("{}{}", ATTRIBUTE, attr);
        return attr;
    }
    static std::string removeAttribute(ConfigVal option)
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
    static const std::map<ConfigVal, std::vector<ConfigVal>> parentOptions;
    /// \brief option dependencies for automatic loading
    static const std::map<ConfigVal, ConfigVal> dependencyMap;
};

#endif // __CONFIG_DEFINITION_H__
