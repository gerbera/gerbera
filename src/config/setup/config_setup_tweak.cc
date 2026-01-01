/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_tweak.cc - this file is part of Gerbera.

    Copyright (C) 2020-2026 Gerbera Contributors

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

/// @file config/setup/config_setup_tweak.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_tweak.h" // API

#include "config/config_definition.h"
#include "config/config_options.h"
#include "config/config_val.h"
#include "config/result/directory_tweak.h"
#include "config_setup_bool.h"
#include "config_setup_path.h"
#include "config_setup_string.h"
#include "setup_util.h"
#include "util/logger.h"

#include <numeric>
#include <pugixml.hpp>

/// @brief Creates an array of DirectoryTweak objects from a XML nodeset.
bool ConfigDirectorySetup::createOptionFromNode(
    const std::shared_ptr<Config>& config,
    const pugi::xml_node& element,
    std::shared_ptr<DirectoryConfigList>& result) const
{
    if (!element)
        return true;

    if (config) {
        config->registerNode(element.path());
    }

    auto&& tcs = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_DIRECTORIES_TWEAK);
    for (auto&& it : tcs->getXmlTree(element)) {
        auto child = it.node();
        if (config) {
            config->registerNode(child.path());
        }
        fs::path location = definition->findConfigSetup<ConfigPathSetup>(ConfigVal::A_DIRECTORIES_TWEAK_LOCATION)->getXmlContent(child, config);

        auto inherit = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_INHERIT)->getXmlContent(child, config);

        auto dir = std::make_shared<DirectoryTweak>(location, inherit);

        {
            auto cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE);
            if (cs->hasXmlElement(child)) {
                dir->setRecursive(cs->getXmlContent(child, config));
            }
            cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN);
            if (cs->hasXmlElement(child)) {
                dir->setHidden(cs->getXmlContent(child, config));
            }
            cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE);
            if (cs->hasXmlElement(child)) {
                dir->setCaseSensitive(cs->getXmlContent(child, config));
            }
            cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS);
            if (cs->hasXmlElement(child)) {
                dir->setFollowSymlinks(cs->getXmlContent(child, config));
            }
        }
        {
            auto cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET);
            if (cs->hasXmlElement(child)) {
                dir->setMetaCharset(cs->getXmlContent(child, config));
            }
            cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setFanArtFile(cs->getXmlContent(child, config));
            }
            cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setSubTitleFile(cs->getXmlContent(child, config));
            }
            cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setMetafile(cs->getXmlContent(child, config));
            }
            cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setResourceFile(cs->getXmlContent(child, config));
            }
        }
        try {
            result->add(dir);
        } catch (const std::runtime_error& e) {
            throw_std_runtime_error("Could not add {} directory: {}", location.string(), e.what());
        }
    }

    return true;
}

void ConfigDirectorySetup::makeOption(
    const pugi::xml_node& root,
    const std::shared_ptr<Config>& config,
    const std::map<std::string, std::string>* arguments)
{
    newOption(config, getXmlElement(root));
    setOption(config);
}

bool ConfigDirectorySetup::updateItem(
    const std::vector<std::size_t>& indexList,
    const std::string& optItem,
    const std::shared_ptr<Config>& config,
    std::shared_ptr<DirectoryTweak>& entry,
    std::string& optValue,
    const std::string& status) const
{
    if (optItem == getItemPath(indexList, {}) && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
        return true;
    }

    static auto resultProperties = std::vector<ConfigResultProperty<DirectoryTweak>> {
        // Location
        {
            { ConfigVal::A_DIRECTORIES_TWEAK_LOCATION },
            "Location",
            [&](const std::shared_ptr<DirectoryTweak>& entry) { return entry->getLocation(); },
            [&](const std::shared_ptr<DirectoryTweak>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                auto pathValue = optValue;
                if (definition->findConfigSetup<ConfigPathSetup>(cfg)->checkPathValue(optValue, pathValue)) {
                    entry->setLocation(pathValue);
                    return true;
                }
                return false;
            },
        },
        // Inherit
        {
            { ConfigVal::A_DIRECTORIES_TWEAK_INHERIT },
            "Inherit",
            [&](const std::shared_ptr<DirectoryTweak>& entry) { return fmt::to_string(entry->getInherit()); },
            [&](const std::shared_ptr<DirectoryTweak>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setInherit(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                return true;
            },
        },
        // Recursive
        {
            { ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE },
            "Recursive",
            [&](const std::shared_ptr<DirectoryTweak>& entry) { return fmt::to_string(entry->getRecursive()); },
            [&](const std::shared_ptr<DirectoryTweak>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setRecursive(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                return true;
            },
        },
        // Hidden
        {
            { ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN },
            "Hidden",
            [&](const std::shared_ptr<DirectoryTweak>& entry) { return fmt::to_string(entry->getHidden()); },
            [&](const std::shared_ptr<DirectoryTweak>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setHidden(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                return true;
            },
        },
        // case sensitive
        {
            { ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE },
            "Case Sensitive",
            [&](const std::shared_ptr<DirectoryTweak>& entry) { return fmt::to_string(entry->getCaseSensitive()); },
            [&](const std::shared_ptr<DirectoryTweak>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setCaseSensitive(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                return true;
            },
        },
        // FollowSymlinks
        {
            { ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS },
            "FollowSymlinks",
            [&](const std::shared_ptr<DirectoryTweak>& entry) { return fmt::to_string(entry->getFollowSymlinks()); },
            [&](const std::shared_ptr<DirectoryTweak>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setFollowSymlinks(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                return true;
            },
        },
        // Charset
        {
            { ConfigVal::A_DIRECTORIES_TWEAK_LOCATION },
            "Charset",
            [&](const std::shared_ptr<DirectoryTweak>& entry) { return entry->hasMetaCharset() ? entry->getMetaCharset() : ""; },
            [&](const std::shared_ptr<DirectoryTweak>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setMetaCharset(optValue);
                    return true;
                }
                return false;
            },
        },
        // Fanart File
        {
            { ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE },
            "Fanart File",
            [&](const std::shared_ptr<DirectoryTweak>& entry) { return entry->hasFanArtFile() ? entry->getFanArtFile() : ""; },
            [&](const std::shared_ptr<DirectoryTweak>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setFanArtFile(optValue);
                    return true;
                }
                return false;
            },
        },
        // Resource File
        {
            { ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE },
            "Resource File",
            [&](const std::shared_ptr<DirectoryTweak>& entry) { return entry->hasResourceFile() ? entry->getResourceFile() : ""; },
            [&](const std::shared_ptr<DirectoryTweak>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setResourceFile(optValue);
                    return true;
                }
                return false;
            },
        },
        // Subtitle File
        {
            { ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE },
            "Subtitle File",
            [&](const std::shared_ptr<DirectoryTweak>& entry) { return entry->hasSubTitleFile() ? entry->getSubTitleFile() : ""; },
            [&](const std::shared_ptr<DirectoryTweak>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setSubTitleFile(optValue);
                    return true;
                }
                return false;
            },
        },
        // Metadata File
        {
            { ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE },
            "Metadata File",
            [&](const std::shared_ptr<DirectoryTweak>& entry) { return entry->hasMetafile() ? entry->getMetafile() : ""; },
            [&](const std::shared_ptr<DirectoryTweak>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setMetafile(optValue);
                    return true;
                }
                return false;
            },
        },
    };

    auto i = indexList.at(0);
    for (auto&& [cfg, label, getProperty, setProperty] : resultProperties) {
        auto index = getItemPath(indexList, cfg);
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, getProperty(entry));
            if (setProperty(entry, definition, cfg.at(0), optValue)) {
                auto nEntry = config->getDirectoryTweakOption(option)->get(i);
                log_debug("New value for Directory Tweak {} {} = {}", label.data(), index, getProperty(nEntry));
                return true;
            }
        }
    }

    return false;
}

bool ConfigDirectorySetup::updateDetail(const std::string& optItem,
    std::string& optValue,
    const std::shared_ptr<Config>& config,
    const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        log_debug("Updating Client Detail {} {} {}", xpath, optItem, optValue);
        auto value = std::dynamic_pointer_cast<DirectoryTweakOption>(optionValue);
        auto list = value->getDirectoryTweakOption();
        auto indexList = extractIndexList(optItem);
        std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";
        if (updateConfig<EditHelperDirectoryTweak, ConfigDirectorySetup, DirectoryTweakOption, DirectoryTweak>(list, config, this, value, optItem, optValue, indexList, status))
            return true;
    }
    return false;
}

std::shared_ptr<ConfigOption> ConfigDirectorySetup::newOption(
    const std::shared_ptr<Config>& config,
    const pugi::xml_node& optValue)
{
    auto result = std::make_shared<DirectoryConfigList>();

    if (!createOptionFromNode(config, optValue, result)) {
        throw_std_runtime_error("Init {} DirectoryConfigList failed '{}'", xpath, optValue.name());
    }
    optionValue = std::make_shared<DirectoryTweakOption>(result);
    return optionValue;
}

std::string ConfigDirectorySetup::getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions, const std::string& propText) const
{
    if (indexList.empty()) {
        if (!propOptions.empty()) {
            return fmt::format("{}[_]/{}", definition->mapConfigOption(ConfigVal::A_DIRECTORIES_TWEAK), definition->ensureAttribute(propOptions[0]));
        }
        return fmt::format("{}[_]", definition->mapConfigOption(ConfigVal::A_DIRECTORIES_TWEAK));
    }
    if (!propOptions.empty()) {
        return fmt::format("{}[{}]/{}", definition->mapConfigOption(ConfigVal::A_DIRECTORIES_TWEAK), indexList[0], definition->ensureAttribute(propOptions[0]));
    }
    return fmt::format("{}[{}]", definition->mapConfigOption(ConfigVal::A_DIRECTORIES_TWEAK), indexList[0]);
}

std::string ConfigDirectorySetup::getItemPathRoot(bool prefix) const
{
    return definition->mapConfigOption(ConfigVal::A_DIRECTORIES_TWEAK);
}
