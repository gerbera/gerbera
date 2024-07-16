/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_tweak.cc - this file is part of Gerbera.
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
#include "util/logger.h"

#include <numeric>

/// \brief Creates an array of DirectoryTweak objects from a XML nodeset.
/// \param element starting element of the nodeset.
bool ConfigDirectorySetup::createOptionFromNode(const pugi::xml_node& element, std::shared_ptr<DirectoryConfigList>& result)
{
    if (!element)
        return true;

    auto&& tcs = ConfigDefinition::findConfigSetup<ConfigSetup>(ConfigVal::A_DIRECTORIES_TWEAK);
    for (auto&& it : tcs->getXmlTree(element)) {
        const pugi::xml_node& child = it.node();
        fs::path location = ConfigDefinition::findConfigSetup<ConfigPathSetup>(ConfigVal::A_DIRECTORIES_TWEAK_LOCATION)->getXmlContent(child);

        auto inherit = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_INHERIT)->getXmlContent(child);

        auto dir = std::make_shared<DirectoryTweak>(location, inherit);

        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE);
            if (cs->hasXmlElement(child)) {
                dir->setRecursive(cs->getXmlContent(child));
            }
            cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN);
            if (cs->hasXmlElement(child)) {
                dir->setHidden(cs->getXmlContent(child));
            }
            cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE);
            if (cs->hasXmlElement(child)) {
                dir->setCaseSensitive(cs->getXmlContent(child));
            }
            cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS);
            if (cs->hasXmlElement(child)) {
                dir->setFollowSymlinks(cs->getXmlContent(child));
            }
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET);
            if (cs->hasXmlElement(child)) {
                dir->setMetaCharset(cs->getXmlContent(child));
            }
            cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setFanArtFile(cs->getXmlContent(child));
            }
            cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setSubTitleFile(cs->getXmlContent(child));
            }
            cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setMetafile(cs->getXmlContent(child));
            }
            cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE);
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

bool ConfigDirectorySetup::updateItem(std::size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<DirectoryTweak>& entry, std::string& optValue, const std::string& status) const
{
    if (optItem == getItemPath(i, {}) && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
        return true;
    }

    auto index = getItemPath(i, { ConfigVal::A_DIRECTORIES_TWEAK_LOCATION });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getLocation());
        auto pathValue = optValue;
        if (ConfigDefinition::findConfigSetup<ConfigPathSetup>(ConfigVal::A_DIRECTORIES_TWEAK_LOCATION)->checkPathValue(optValue, pathValue)) {
            entry->setLocation(pathValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getLocation().string());
            return true;
        }
    }
    // inherit
    index = getItemPath(i, { ConfigVal::A_DIRECTORIES_TWEAK_INHERIT });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getInherit());
        entry->setInherit(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_INHERIT)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getInherit());
        return true;
    }
    // recursive
    index = getItemPath(i, { ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getRecursive());
        entry->setRecursive(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RECURSIVE)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getRecursive());
        return true;
    }
    // hidden
    index = getItemPath(i, { ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getHidden());
        entry->setHidden(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_HIDDEN)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getHidden());
        return true;
    }
    // case sensitive
    index = getItemPath(i, { ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getCaseSensitive());
        entry->setCaseSensitive(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_CASE_SENSITIVE)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getCaseSensitive());
        return true;
    }
    // follow symlinks
    index = getItemPath(i, { ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getFollowSymlinks());
        entry->setFollowSymlinks(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getFollowSymlinks());
        return true;
    }
    // charset
    index = getItemPath(i, { ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasMetaCharset() ? entry->getMetaCharset() : "");
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_META_CHARSET)->checkValue(optValue)) {
            entry->setMetaCharset(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getMetaCharset());
            return true;
        }
    }
    // fanart file
    index = getItemPath(i, { ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasFanArtFile() ? entry->getFanArtFile() : "");
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_FANART_FILE)->checkValue(optValue)) {
            entry->setFanArtFile(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getFanArtFile());
            return true;
        }
    }
    // resource file
    index = getItemPath(i, { ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasResourceFile() ? entry->getResourceFile() : "");
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_RESOURCE_FILE)->checkValue(optValue)) {
            entry->setResourceFile(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getResourceFile());
            return true;
        }
    }
    // subtitle file
    index = getItemPath(i, { ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasSubTitleFile() ? entry->getSubTitleFile() : "");
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_SUBTITLE_FILE)->checkValue(optValue)) {
            entry->setSubTitleFile(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getSubTitleFile());
            return true;
        }
    }
    // metadata file
    index = getItemPath(i, { ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasMetafile() ? entry->getMetafile() : "");
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DIRECTORIES_TWEAK_METAFILE_FILE)->checkValue(optValue)) {
            entry->setMetafile(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getMetafile());
            return true;
        }
    }

    return false;
}

bool ConfigDirectorySetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        log_debug("Updating Client Detail {} {} {}", xpath, optItem, optValue);
        auto value = std::dynamic_pointer_cast<DirectoryTweakOption>(optionValue);
        auto list = value->getDirectoryTweakOption();
        auto index = extractIndex(optItem);

        if (index < std::numeric_limits<std::size_t>::max()) {
            auto entry = list->get(index, true);
            std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";

            if (!entry && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
                entry = std::make_shared<DirectoryTweak>();
                list->add(entry, index);
            }
            if (entry && (status == STATUS_REMOVED || status == STATUS_KILLED)) {
                list->remove(index, true);
                return true;
            }
            if (entry && status == STATUS_RESET) {
                list->add(entry, index);
            }
            if (entry && updateItem(index, optItem, config, entry, optValue, status)) {
                return true;
            }
        }
        for (std::size_t tweak = 0; tweak < list->size(); tweak++) {
            auto entry = value->getDirectoryTweakOption()->get(tweak);
            if (updateItem(tweak, optItem, config, entry, optValue)) {
                return true;
            }
        }
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

std::string ConfigDirectorySetup::getItemPath(int index, const std::vector<ConfigVal>& propOptions) const
{
    if (index < 0) {
        return ConfigDefinition::mapConfigOption(ConfigVal::A_DIRECTORIES_TWEAK);
    }
    if (propOptions.size() > 0) {
        return fmt::format("{}[{}]/{}", ConfigDefinition::mapConfigOption(ConfigVal::A_DIRECTORIES_TWEAK), index, ConfigDefinition::ensureAttribute(propOptions[0]));
    }
    return fmt::format("{}[{}]", ConfigDefinition::mapConfigOption(ConfigVal::A_DIRECTORIES_TWEAK), index);
}
