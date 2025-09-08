/*GRB*

    Gerbera - https://gerbera.io/

    config_definition.h - this file is part of Gerbera.

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

/// @file config/config_definition.h
/// @brief Definitions of class ConfigDefinition for default values and setup for configuration.

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

#define GRB_UDN_AUTO "grb_udn_auto"

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
    /// @brief get list of all definied config options
    std::vector<std::shared_ptr<ConfigSetup>> getOptionList() const
    {
        return complexOptions;
    }

    /// @brief get path name of config option
    const char* mapConfigOption(ConfigVal option) const;

    /// @brief check whether option is only available when other option is set
    bool isDependent(ConfigVal option) const;
    /// @brief get all options that depend on argument
    std::vector<ConfigVal> getDependencies(ConfigVal option) const;

    /// @brief find setup for config option
    std::shared_ptr<ConfigSetup> findConfigSetup(ConfigVal option, bool save = false) const;
    /// @brief find setup by xml path
    std::shared_ptr<ConfigSetup> findConfigSetupByPath(
        const std::string& key,
        bool save = false,
        const std::shared_ptr<ConfigSetup>& parent = nullptr) const;

    /// @brief get list of all config setup with respective type
    template <class CS>
    std::vector<std::shared_ptr<CS>> getConfigSetupList() const
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

    /// @brief find setup for config option and convert to proper type
    template <class CS>
    std::shared_ptr<CS> findConfigSetup(ConfigVal option, bool save = false) const
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

    /// @brief Make sure that attribute string is prepended with "attribute::"
    std::string ensureAttribute(ConfigVal option, bool check = true) const
    {
        auto attr = std::string(mapConfigOption(option));
        if (!startswith(attr, ATTRIBUTE) && check)
            return fmt::format("{}{}", ATTRIBUTE, attr);
        return attr;
    }

    /// @brief Make sure that attribute string is not prepended with "attribute::"
    std::string removeAttribute(ConfigVal option) const
    {
        auto attr = std::string(mapConfigOption(option));
        if (attr.size() > ATTRIBUTE.size() && startswith(attr, ATTRIBUTE))
            attr = attr.substr(ATTRIBUTE.size());
        return attr;
    }

    /// @brief initialise definition object with all ConfigSetup instances
    void init(const std::shared_ptr<ConfigDefinition>& self)
    {
        initOptions(self);
        initHierarchy();
        initDependencies();
        markDeprecated();
    }

    static constexpr auto ATTRIBUTE = std::string_view("attribute::");

private:
    /// @brief configure all known options
    void initOptions(const std::shared_ptr<ConfigDefinition>& self);
    static std::vector<std::shared_ptr<ConfigSetup>> getServerOptions();
    static std::vector<std::shared_ptr<ConfigSetup>> getClientOptions();
    static std::vector<std::shared_ptr<ConfigSetup>> getImportOptions();
    static std::vector<std::shared_ptr<ConfigSetup>> getLibraryOptions();
    static std::vector<std::shared_ptr<ConfigSetup>> getTranscodingOptions();
    /// @brief simpleOptions for group names with content
    static std::vector<std::shared_ptr<ConfigSetup>> getSimpleOptions();

    /// @brief define option dependencies for automatic loading
    void initDependencies();
    /// @brief define parent options for path search
    void initHierarchy();
    /// @brief mark option to be removed soon
    void markDeprecated() const;

    /// @brief all known options
    std::vector<std::shared_ptr<ConfigSetup>> complexOptions;
    /// @brief parent options for path search
    std::map<ConfigVal, std::vector<ConfigVal>> parentOptions;
    /// @brief option dependencies for automatic loading
    std::map<ConfigVal, ConfigVal> dependencyMap;
};

#endif // __CONFIG_DEFINITION_H__
