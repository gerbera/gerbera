/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_tweak.cc - this file is part of Gerbera.

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

/// \file config_setup_tweak.cc
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

/// \brief Creates an array of DirectoryTweak objects from a XML nodeset.
bool ConfigDirectorySetup::createOptionFromNode(const pugi::xml_node& element, std::shared_ptr<DirectoryConfigList>& result) const
{
    if (!element)
        return true;

    auto&& tcs = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_DIRECTORIES_TWEAK);
    for (auto&& it : tcs->getXmlTree(element)) {
        const pugi::xml_node& child = it.node();
        fs::path location = definition->findConfigSetup<ConfigPathSetup>(ConfigVal::A_DIRECTORIES_TWEAK_LOCATION)->getXmlContent(child);

        auto inherit = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_INHERIT)->getXmlContent(child);

        auto dir = std::make_shared<DirectoryTweak>(location, inherit);

        {
            auto cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE);
            if (cs->hasXmlElement(child)) {
                dir->setRecursive(cs->getXmlContent(child));
            }
            cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN);
            if (cs->hasXmlElement(child)) {
                dir->setHidden(cs->getXmlContent(child));
            }
            cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE);
            if (cs->hasXmlElement(child)) {
                dir->setCaseSensitive(cs->getXmlContent(child));
            }
            cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS);
            if (cs->hasXmlElement(child)) {
                dir->setFollowSymlinks(cs->getXmlContent(child));
            }
        }
        {
            auto cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET);
            if (cs->hasXmlElement(child)) {
                dir->setMetaCharset(cs->getXmlContent(child));
            }
            cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setFanArtFile(cs->getXmlContent(child));
            }
            cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setSubTitleFile(cs->getXmlContent(child));
            }
            cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setMetafile(cs->getXmlContent(child));
            }
            cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setResourceFile(cs->getXmlContent(child));
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

void ConfigDirectorySetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlElement(root));
    setOption(config);
}

bool ConfigDirectorySetup::updateItem(const std::vector<std::size_t>& indexList, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<DirectoryTweak>& entry, std::string& optValue, const std::string& status) const
{
    if (optItem == getItemPath(indexList, {}) && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
        return true;
    }

    auto index = getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_LOCATION });
    auto i = indexList.at(0);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getLocation());
        auto pathValue = optValue;
        if (definition->findConfigSetup<ConfigPathSetup>(ConfigVal::A_DIRECTORIES_TWEAK_LOCATION)->checkPathValue(optValue, pathValue)) {
            entry->setLocation(pathValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getLocation().string());
            return true;
        }
    }
    // inherit
    index = getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_INHERIT });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getInherit());
        entry->setInherit(definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_INHERIT)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getInherit());
        return true;
    }
    // recursive
    index = getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getRecursive());
        entry->setRecursive(definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getRecursive());
        return true;
    }
    // hidden
    index = getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getHidden());
        entry->setHidden(definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getHidden());
        return true;
    }
    // case sensitive
    index = getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getCaseSensitive());
        entry->setCaseSensitive(definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getCaseSensitive());
        return true;
    }
    // follow symlinks
    index = getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getFollowSymlinks());
        entry->setFollowSymlinks(definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getFollowSymlinks());
        return true;
    }
    // charset
    index = getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasMetaCharset() ? entry->getMetaCharset() : "");
        if (definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET)->checkValue(optValue)) {
            entry->setMetaCharset(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getMetaCharset());
            return true;
        }
    }
    // fanart file
    index = getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasFanArtFile() ? entry->getFanArtFile() : "");
        if (definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE)->checkValue(optValue)) {
            entry->setFanArtFile(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getFanArtFile());
            return true;
        }
    }
    // resource file
    index = getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasResourceFile() ? entry->getResourceFile() : "");
        if (definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE)->checkValue(optValue)) {
            entry->setResourceFile(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getResourceFile());
            return true;
        }
    }
    // subtitle file
    index = getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasSubTitleFile() ? entry->getSubTitleFile() : "");
        if (definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE)->checkValue(optValue)) {
            entry->setSubTitleFile(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getSubTitleFile());
            return true;
        }
    }
    // metadata file
    index = getItemPath(indexList, { ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasMetafile() ? entry->getMetafile() : "");
        if (definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE)->checkValue(optValue)) {
            entry->setMetafile(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getMetafile());
            return true;
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

std::shared_ptr<ConfigOption> ConfigDirectorySetup::newOption(const pugi::xml_node& optValue)
{
    auto result = std::make_shared<DirectoryConfigList>();

    if (!createOptionFromNode(optValue, result)) {
        throw_std_runtime_error("Init {} DirectoryConfigList failed '{}'", xpath, optValue.name());
    }
    optionValue = std::make_shared<DirectoryTweakOption>(result);
    return optionValue;
}

std::string ConfigDirectorySetup::getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions, const std::string& propText) const
{
    if (indexList.size() == 0) {
        if (propOptions.size() > 0) {
            return fmt::format("{}[_]/{}", definition->mapConfigOption(ConfigVal::A_DIRECTORIES_TWEAK), definition->ensureAttribute(propOptions[0]));
        }
        return fmt::format("{}[_]", definition->mapConfigOption(ConfigVal::A_DIRECTORIES_TWEAK));
    }
    if (propOptions.size() > 0) {
        return fmt::format("{}[{}]/{}", definition->mapConfigOption(ConfigVal::A_DIRECTORIES_TWEAK), indexList[0], definition->ensureAttribute(propOptions[0]));
    }
    return fmt::format("{}[{}]", definition->mapConfigOption(ConfigVal::A_DIRECTORIES_TWEAK), indexList[0]);
}

std::string ConfigDirectorySetup::getItemPathRoot(bool prefix) const
{
    return definition->mapConfigOption(ConfigVal::A_DIRECTORIES_TWEAK);
}
