/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_autoscan.cc - this file is part of Gerbera.

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

/// \file config_setup_autoscan.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_autoscan.h" // API

#include "config/config_definition.h"
#include "config/config_option_enum.h"
#include "config/config_options.h"
#include "config/config_val.h"
#include "config/result/autoscan.h"
#include "config_setup_bool.h"
#include "config_setup_enum.h"
#include "config_setup_int.h"
#include "config_setup_path.h"
#include "config_setup_string.h"
#include "config_setup_time.h"
#include "content/autoscan_list.h"
#include "setup_util.h"

#include <numeric>

std::string ConfigAutoscanSetup::getUniquePath() const
{
    return fmt::format("{}/{}", xpath, AutoscanDirectory::mapScanmode(scanMode));
}

std::string ConfigAutoscanSetup::getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions, const std::string& propText) const
{
    if (indexList.empty()) {
        if (!propOptions.empty())
            return fmt::format("{}/{}/{}[_]/{}", xpath, AutoscanDirectory::mapScanmode(scanMode), definition->mapConfigOption(ConfigVal::A_AUTOSCAN_DIRECTORY), definition->ensureAttribute(propOptions[0]));
        else
            return fmt::format("{}/{}/{}", xpath, AutoscanDirectory::mapScanmode(scanMode), definition->mapConfigOption(ConfigVal::A_AUTOSCAN_DIRECTORY));
    }
    if (!propOptions.empty())
        return fmt::format("{}/{}/{}[{}]/{}", xpath, AutoscanDirectory::mapScanmode(scanMode), definition->mapConfigOption(ConfigVal::A_AUTOSCAN_DIRECTORY), indexList[0], definition->ensureAttribute(propOptions[0]));

    return fmt::format("{}/{}", xpath, definition->mapConfigOption(ConfigVal::A_AUTOSCAN_DIRECTORY));
}

std::string ConfigAutoscanSetup::getItemPathRoot(bool prefix) const
{
    return fmt::format("{}/{}/{}", xpath, AutoscanDirectory::mapScanmode(scanMode), definition->mapConfigOption(ConfigVal::A_AUTOSCAN_DIRECTORY));
}

/// \brief Creates an array of AutoscanDirectory objects from a XML nodeset.
bool ConfigAutoscanSetup::createOptionFromNode(const pugi::xml_node& element, std::vector<std::shared_ptr<AutoscanDirectory>>& result)
{
    if (!element)
        return true;

    auto&& cs = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY);
    for (auto&& it : cs->getXmlTree(element)) {
        const pugi::xml_node& child = it.node();

        fs::path location;
        try {
            location = definition->findConfigSetup<ConfigPathSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION)->getXmlContent(child, true);
        } catch (const std::runtime_error&) {
            log_warning("Found an Autoscan directory with invalid location!");
            continue;
        }

        AutoscanScanMode mode = definition->findConfigSetup<ConfigEnumSetup<AutoscanScanMode>>(ConfigVal::A_AUTOSCAN_DIRECTORY_MODE)->getXmlContent(child);

        if (mode != scanMode) {
            continue; // skip scan modes that we are not interested in (content manager needs one mode type per array)
        }

        long long interval = 0;
        if (mode == AutoscanScanMode::Timed) {
            interval = definition->findConfigSetup<ConfigTimeSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL)->getXmlContent(child);
        }

        bool recursive = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE)->getXmlContent(child);
        bool forceRescan = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_FORCE_REREAD_UNKNOWN)->getXmlContent(child);
        bool dirtypes = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES)->getXmlContent(child);
        int mt = definition->findConfigSetup<ConfigIntSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_MEDIATYPE)->getXmlContent(child);
        log_debug("mt = {} -> {}", mt, AutoscanDirectory::mapMediaType(mt));

        unsigned int retryCount = definition->findConfigSetup<ConfigUIntSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_RETRYCOUNT)->getXmlContent(child);
        auto cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES);
        bool hidden = cs->hasXmlElement(child) ? cs->getXmlContent(child) : hiddenFiles;

        cs = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS);
        bool follow = cs->hasXmlElement(child) ? cs->getXmlContent(child) : followSymlinks;

        auto ctAudio = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO)->getXmlContent(child);
        auto ctImage = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE)->getXmlContent(child);
        auto ctVideo = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO)->getXmlContent(child);
        try {
            auto containerMap = AutoscanDirectory::ContainerTypesDefaults;
            containerMap[AutoscanMediaMode::Audio] = ctAudio;
            containerMap[AutoscanMediaMode::Image] = ctImage;
            containerMap[AutoscanMediaMode::Video] = ctVideo;
            auto adir = std::make_shared<AutoscanDirectory>(location, mode, recursive, true, interval, hidden, follow, mt, containerMap);
            adir->setRetryCount(retryCount);
            adir->setDirTypes(dirtypes);
            adir->setForceRescan(forceRescan);
            result.push_back(adir);
        } catch (const std::runtime_error& e) {
            log_error("Could not add {}: {}", location.string(), e.what());
            return false;
        }
    }

    return true;
}

bool ConfigAutoscanSetup::updateItem(
    const std::vector<std::size_t>& indexList,
    const std::string& optItem,
    const std::shared_ptr<Config>& config,
    const std::shared_ptr<AutoscanDirectory>& entry,
    std::string& optValue,
    const std::string& status) const
{
    if (optItem == getUniquePath() && status != STATUS_CHANGED) {
        return true;
    }

    static auto resultProperties = std::vector<ConfigResultProperty<AutoscanDirectory>> {
        // Location
        {
            { ConfigVal::A_AUTOSCAN_DIRECTORY_LOCATION },
            "Location",
            [&](const std::shared_ptr<AutoscanDirectory>& entry) { return entry->getLocation(); },
            [&](const std::shared_ptr<AutoscanDirectory>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                auto pathValue = optValue;
                if (definition->findConfigSetup<ConfigPathSetup>(cfg)->checkPathValue(optValue, pathValue)) {
                    entry->setLocation(pathValue);
                }
                return true;
            },
        },
        // Interval
        {
            { ConfigVal::A_AUTOSCAN_DIRECTORY_INTERVAL },
            "Interval",
            [&](const std::shared_ptr<AutoscanDirectory>& entry) { return fmt::to_string(entry->getInterval().count()); },
            [&](const std::shared_ptr<AutoscanDirectory>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setInterval(std::chrono::seconds(definition->findConfigSetup<ConfigTimeSetup>(cfg)->checkTimeValue(optValue)));
                return true;
            },
        },
        // Recursive
        {
            { ConfigVal::A_AUTOSCAN_DIRECTORY_RECURSIVE },
            "Recursive",
            [&](const std::shared_ptr<AutoscanDirectory>& entry) { return fmt::to_string(entry->getRecursive()); },
            [&](const std::shared_ptr<AutoscanDirectory>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setRecursive(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                return true;
            },
        },
        // DirTypes
        {
            { ConfigVal::A_AUTOSCAN_DIRECTORY_DIRTYPES },
            "DirTypes",
            [&](const std::shared_ptr<AutoscanDirectory>& entry) { return fmt::to_string(entry->hasDirTypes()); },
            [&](const std::shared_ptr<AutoscanDirectory>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setDirTypes(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                return true;
            },
        },
        // Hidden
        {
            { ConfigVal::A_AUTOSCAN_DIRECTORY_HIDDENFILES },
            "Hidden",
            [&](const std::shared_ptr<AutoscanDirectory>& entry) { return fmt::to_string(entry->getHidden()); },
            [&](const std::shared_ptr<AutoscanDirectory>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setHidden(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                return true;
            },
        },
        // FollowSymlinks
        {
            { ConfigVal::A_AUTOSCAN_DIRECTORY_FOLLOWSYMLINKS },
            "FollowSymlinks",
            [&](const std::shared_ptr<AutoscanDirectory>& entry) { return fmt::to_string(entry->getFollowSymlinks()); },
            [&](const std::shared_ptr<AutoscanDirectory>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setFollowSymlinks(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                return true;
            },
        },
        // ContainerType Audio
        {
            { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_AUDIO },
            "ContainerType Audio",
            [&](const std::shared_ptr<AutoscanDirectory>& entry) { return entry->getContainerTypes().at(AutoscanMediaMode::Audio); },
            [&](const std::shared_ptr<AutoscanDirectory>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setContainerType(AutoscanMediaMode::Audio, optValue);
                return true;
            },
        },
        // ContainerType Image
        {
            { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_IMAGE },
            "ContainerType Image",
            [&](const std::shared_ptr<AutoscanDirectory>& entry) { return entry->getContainerTypes().at(AutoscanMediaMode::Image); },
            [&](const std::shared_ptr<AutoscanDirectory>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setContainerType(AutoscanMediaMode::Image, optValue);
                return true;
            },
        },
        // ContainerType Video
        {
            { ConfigVal::A_AUTOSCAN_CONTAINER_TYPE_VIDEO },
            "ContainerType Video",
            [&](const std::shared_ptr<AutoscanDirectory>& entry) { return entry->getContainerTypes().at(AutoscanMediaMode::Video); },
            [&](const std::shared_ptr<AutoscanDirectory>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setContainerType(AutoscanMediaMode::Video, optValue);
                return true;
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
                auto nEntry = config->getAutoscanListOption(option).at(i);
                log_debug("New value for Autoscan {} {} = {}", label.data(), index, getProperty(nEntry));
                return true;
            }
        }
    }

    {
        auto index = getItemPath(indexList, { ConfigVal::A_AUTOSCAN_DIRECTORY_MODE });
        if (optItem == index) {
            log_error("Autoscan Mode cannot be changed {} {}", index, AutoscanDirectory::mapScanmode(entry->getScanMode()));
            return true;
        }
    }

    return false;
}

bool ConfigAutoscanSetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    auto uPath = getUniquePath();

    if (!startswith(optItem, uPath))
        return false;

    log_debug("Updating Autoscan Detail {} {} {}", uPath, optItem, optValue);
    auto value = std::dynamic_pointer_cast<AutoscanListOption>(optionValue);

    auto list = value->getAutoscanListOption();
    auto indexList = extractIndexList(optItem);

    if (!indexList.empty()) {
        auto i = indexList.at(0);
        std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";

        try {
            auto autoscan = list.at(i);
            if (status == STATUS_REMOVED || status == STATUS_KILLED) {
                list.erase(list.begin() + i);
                return true;
            }
            if (status == STATUS_RESET) {
                list[i] = autoscan;
            }
            return updateItem(indexList, optItem, config, autoscan, optValue, status);
        } catch (const std::out_of_range& oor) {
            if (status == STATUS_ADDED || status == STATUS_MANUAL) {
                auto scan = list.emplace(list.cbegin());
                (*scan)->setScanMode(scanMode);
            }
        }
    }
    return false;
}

void ConfigAutoscanSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (arguments && arguments->find("hiddenFiles") != arguments->end()) {
        hiddenFiles = arguments->at("hiddenFiles") == "true";
    }
    if (arguments && arguments->find("followSymlinks") != arguments->end()) {
        followSymlinks = arguments->at("followSymlinks") == "true";
    }
    newOption(getXmlElement(root));
    setOption(config);
}

std::shared_ptr<ConfigOption> ConfigAutoscanSetup::newOption(const pugi::xml_node& optValue)
{
    auto result = std::vector<std::shared_ptr<AutoscanDirectory>>();
    if (!createOptionFromNode(optValue, result)) {
        throw_std_runtime_error("Init {} autoscan failed '{}'", xpath, optValue.name());
    }
    optionValue = std::make_shared<AutoscanListOption>(result);
    return optionValue;
}
