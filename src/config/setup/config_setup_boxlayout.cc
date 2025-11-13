/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_boxlayout.cc - this file is part of Gerbera.

    Copyright (C) 2023-2025 Gerbera Contributors

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

/// @file config/setup/config_setup_boxlayout.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_boxlayout.h" // API

#include "config/config_definition.h"
#include "config/config_option_enum.h"
#include "config/config_options.h"
#include "config/config_val.h"
#include "config/result/autoscan.h"
#include "config/result/box_layout.h"
#include "config_setup_bool.h"
#include "config_setup_enum.h"
#include "config_setup_int.h"
#include "config_setup_string.h"
#include "config_setup_vector.h"
#include "setup_util.h"
#include "util/logger.h"

#include <algorithm>
#include <array>
#include <numeric>

ConfigBoxLayoutSetup::ConfigBoxLayoutSetup(
    ConfigVal option,
    const char* xpath,
    const char* help,
    std::vector<BoxLayout> defaultEntries)
    : ConfigSetup(option, xpath, help)
    , defaultEntries(std::move(defaultEntries))
{
}

ConfigBoxLayoutSetup::~ConfigBoxLayoutSetup() = default;

/// @brief Creates an array of BoxLayout objects from a XML nodeset.
/// @param element starting element of the nodeset.
bool ConfigBoxLayoutSetup::createOptionFromNode(
    const std::shared_ptr<Config>& config,
    const pugi::xml_node& element,
    const std::shared_ptr<BoxLayoutList>& result)
{
    if (!element)
        return true;

    if (config) {
        config->registerNode(element.path());
    }
    std::vector<std::string> allKeys;
    auto&& listcs = definition->findConfigSetup<ConfigSetup>(option);
    auto doExtend = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_LIST_EXTEND)->getXmlContent(element, config);
    log_debug("is {} extensible {}", element.path(), doExtend);
    static constexpr auto rootKeys = std::array {
        BoxKeys::root,
        BoxKeys::pcDirectory,
        BoxKeys::audioRoot,
        BoxKeys::audioAll,
        BoxKeys::imageRoot,
        BoxKeys::imageAll,
        BoxKeys::videoRoot,
        BoxKeys::videoAll,
    };

    auto&& boxcs = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_BOXLAYOUT_BOX);
    auto&& chaincs = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_BOXLAYOUT_CHAIN);
    for (auto&& itBox : listcs->getXmlTree(element)) {
        auto childBox = itBox.node();
        if (config) {
            config->registerNode(childBox.path());
        }
        for (auto&& it : boxcs->getXmlTree(childBox)) {
            auto child = it.node();
            if (config) {
                config->registerNode(child.path());
            }

            auto key = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_KEY)->getXmlContent(child, config);
            auto title = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_TITLE)->getXmlContent(child, config);
            auto objClass = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_CLASS)->getXmlContent(child, config);
            auto upnpShortcut = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT)->getXmlContent(child, config);
            auto size = definition->findConfigSetup<ConfigIntSetup>(ConfigVal::A_BOXLAYOUT_BOX_SIZE)->getXmlContent(child, config);
            auto enabled = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_BOXLAYOUT_BOX_ENABLED)->getXmlContent(child, config);
            auto searchable = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_BOXLAYOUT_BOX_SEARCHABLE)->getXmlContent(child, config);
            if (!enabled) // disabled boxes connot be searched
                searchable = false;
            auto sortKey = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_SORT_KEY)->getXmlContent(child, config);

            if (!enabled && std::find(rootKeys.begin(), rootKeys.end(), BoxLayout::getBoxKey(key)) != rootKeys.end()) {
                log_warning("Box '{}' cannot be disabled", key);
                enabled = true;
            }

            auto box = std::make_shared<BoxLayout>(key, title, objClass, upnpShortcut, sortKey, enabled, searchable, size);
            try {
                EDIT_CAST(EditHelperBoxLayout, result)->add(box);
                allKeys.push_back(key);
                log_debug("Created BoxLayout key={}, title={}, objClass={}, enabled={}, size={}", key, title, objClass, enabled, size);
            } catch (const std::runtime_error& e) {
                throw_std_runtime_error("Could not add {} boxlayout: {}", key, e.what());
            }
        }
        std::size_t index = 0;
        for (auto&& it : chaincs->getXmlTree(childBox)) {
            auto child = it.node();
            if (config) {
                config->registerNode(child.path());
            }
            AutoscanMediaMode type = definition->findConfigSetup<ConfigEnumSetup<AutoscanMediaMode>>(ConfigVal::A_BOXLAYOUT_CHAIN_TYPE)->getXmlContent(child, config);
            auto links = definition->findConfigSetup<ConfigVectorSetup>(ConfigVal::A_BOXLAYOUT_CHAIN_LINKS)->getXmlContent(child, config);
            auto chain = std::make_shared<BoxChain>(index, type, links);
            EDIT_CAST(EditHelperBoxChain, result)->add(chain);
            ++index;
        }
    }
    for (auto&& defEntry : defaultEntries) {
        if (std::find(allKeys.begin(), allKeys.end(), defEntry.getKey()) == allKeys.end()) {
            auto box = std::make_shared<BoxLayout>(defEntry.getKey(), defEntry.getTitle(), defEntry.getClass(), defEntry.getUpnpShortcut(), defEntry.getSortKey(), defEntry.getEnabled(), defEntry.getSearchable(), defEntry.getSize());
            if (doExtend)
                log_debug("Automatically added default BoxLayout key={}, title={}, objClass={}, enabled={}, size={}", defEntry.getKey(), defEntry.getTitle(), defEntry.getClass(), defEntry.getEnabled(), defEntry.getSize());
            else
                log_info("Automatically added default BoxLayout key={}, title={}, objClass={}, enabled={}, size={}", defEntry.getKey(), defEntry.getTitle(), defEntry.getClass(), defEntry.getEnabled(), defEntry.getSize());
            EDIT_CAST(EditHelperBoxLayout, result)->add(box);
            allKeys.push_back(defEntry.getKey());
        }
    }

    return true;
}

bool ConfigBoxLayoutSetup::validate(
    const std::shared_ptr<Config>& config,
    const std::shared_ptr<BoxLayoutList>& values)
{
    auto layoutType = EnumOption<LayoutType>::getEnumOption(config, ConfigVal::IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);
    if (layoutType == LayoutType::Js)
        return true;

    for (auto&& theBox : EDIT_CAST(EditHelperBoxLayout, values)->getArrayCopy()) {
        if (std::find_if(defaultEntries.cbegin(), defaultEntries.cend(), [&theBox](auto& box) { return theBox->getKey() == box.getKey(); }) == defaultEntries.cend()) {
            // Warn the user that his option will be ignored in built-in layout. But allow to use "unknown" options for js
            log_warning("Box key={}, title={}, objClass={}, enabled={}, size={} will be ignored in built-in layout. Unknown Key '{}'",
                theBox->getKey(), theBox->getTitle(), theBox->getClass(),
                theBox->getEnabled(), theBox->getSize(), theBox->getKey());
        }
    }
    return true;
}

void ConfigBoxLayoutSetup::makeOption(
    const pugi::xml_node& root,
    const std::shared_ptr<Config>& config,
    const std::map<std::string, std::string>* arguments)
{
    newOption(config, getXmlElement(root));
    setOption(config);
}

bool ConfigBoxLayoutSetup::updateItem(
    const std::vector<std::size_t>& indexList,
    const std::string& optItem,
    const std::shared_ptr<Config>& config,
    std::shared_ptr<BoxLayout>& entry,
    std::string& optValue,
    const std::string& status) const
{
    if (optItem == getItemPath(indexList, {}) && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
        return true;
    }

    static auto resultProperties = std::vector<ConfigResultProperty<BoxLayout>> {
        // Key
        {
            { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_KEY },
            "Key",
            [&](const std::shared_ptr<BoxLayout>& entry) { return entry->getKey(); },
            [&](const std::shared_ptr<BoxLayout>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setKey(optValue);
                    return true;
                }
                return false;
            },
        },
        // Title
        {
            { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_TITLE },
            "Title",
            [&](const std::shared_ptr<BoxLayout>& entry) { return entry->getTitle(); },
            [&](const std::shared_ptr<BoxLayout>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setTitle(optValue);
                    return true;
                }
                return false;
            },
        },
        // Class
        {
            { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_CLASS },
            "Class",
            [&](const std::shared_ptr<BoxLayout>& entry) { return entry->getClass(); },
            [&](const std::shared_ptr<BoxLayout>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setClass(optValue);
                    return true;
                }
                return false;
            },
        },
        // UpnpShortcut
        {
            { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT },
            "UpnpShortcut",
            [&](const std::shared_ptr<BoxLayout>& entry) { return entry->getUpnpShortcut(); },
            [&](const std::shared_ptr<BoxLayout>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setUpnpShortcut(optValue);
                    return true;
                }
                return false;
            },
        },
        // BoxSize
        {
            { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_SIZE },
            "BoxSize",
            [&](const std::shared_ptr<BoxLayout>& entry) { return fmt::to_string(entry->getSize()); },
            [&](const std::shared_ptr<BoxLayout>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setSize(definition->findConfigSetup<ConfigIntSetup>(cfg)->checkIntValue(optValue));
                return true;
            },
        },
        // Enabled
        {
            { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_ENABLED },
            "Enabled",
            [&](const std::shared_ptr<BoxLayout>& entry) { return fmt::to_string(entry->getEnabled()); },
            [&](const std::shared_ptr<BoxLayout>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setEnabled(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                return true;
            },
        },
        // searchable
        {
            { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_SEARCHABLE },
            "Searchable",
            [&](const std::shared_ptr<BoxLayout>& entry) { return fmt::to_string(entry->getSearchable()); },
            [&](const std::shared_ptr<BoxLayout>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setSearchable(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                return true;
            },
        },
        // SortKey
        {
            { ConfigVal::A_BOXLAYOUT_BOX, ConfigVal::A_BOXLAYOUT_BOX_SORT_KEY },
            "SortKey",
            [&](const std::shared_ptr<BoxLayout>& entry) { return entry->getSortKey(); },
            [&](const std::shared_ptr<BoxLayout>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setSortKey(optValue);
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
                auto nEntry = EDIT_CAST(EditHelperBoxLayout, config->getBoxLayoutListOption(option))->get(i);
                log_debug("New value for BoxLayout {} {} = {}", label.data(), index, getProperty(nEntry));
                return true;
            }
        }
    }

    return false;
}

bool ConfigBoxLayoutSetup::updateItem(
    const std::vector<std::size_t>& indexList,
    const std::string& optItem,
    const std::shared_ptr<Config>& config,
    std::shared_ptr<BoxChain>& entry,
    std::string& optValue,
    const std::string& status) const
{
    if (optItem == getItemPath(indexList, {}) && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
        return true;
    }

    static auto chainProperties = std::vector<ConfigResultProperty<BoxChain>> {
        // Type
        {
            { ConfigVal::A_BOXLAYOUT_CHAIN, ConfigVal::A_BOXLAYOUT_CHAIN_TYPE },
            "Type",
            [&](const std::shared_ptr<BoxChain>& entry) { return fmt::to_string(entry->getType()); },
            [&](const std::shared_ptr<BoxChain>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                auto setup = definition->findConfigSetup<ConfigEnumSetup<AutoscanMediaMode>>(cfg);
                AutoscanMediaMode type;
                if (setup->checkEnumValue(optValue, type)) {
                    entry->setType(type);
                    return true;
                }
                return false;
            },
        },
    };

    auto i = indexList.at(0);
    for (auto&& [cfg, label, getProperty, setProperty] : chainProperties) {
        auto index = getItemPath(indexList, cfg);
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, getProperty(entry));
            if (setProperty(entry, definition, cfg.at(0), optValue)) {
                auto nEntry = EDIT_CAST(EditHelperBoxChain, config->getBoxLayoutListOption(option))->get(i);
                log_debug("New value for BoxLayout {} {} = {}", label.data(), index, getProperty(nEntry));
                return true;
            }
        }
    }

    // Links vector key
    {
        auto keyIndex = getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_CHAIN, ConfigVal::A_BOXLAYOUT_CHAIN_LINKS, ConfigVal::A_BOXLAYOUT_CHAIN_LINK }, linkKey.data());
        if (optItem == keyIndex) {
            std::size_t j = indexList.at(1);
            std::size_t k = indexList.at(2);
            auto link = entry->getLinks(true).at(j);
            if (status == STATUS_RESET && !optValue.empty()) {
                entry->setLinkKey(j, k, config->getOrigValue(keyIndex));
                log_debug("Reset link key {} {}", keyIndex, entry->getLinks(true).at(j).at(k).first);
            } else {
                config->setOrigValue(keyIndex, link.at(k).second);
                entry->setLinkKey(j, k, optValue);
                log_debug("New link key {} {}", keyIndex, optValue);
            }
        }
    }
    // Links vector value
    {
        auto valIndex = getItemPath(indexList, { ConfigVal::A_BOXLAYOUT_CHAIN, ConfigVal::A_BOXLAYOUT_CHAIN_LINKS, ConfigVal::A_BOXLAYOUT_CHAIN_LINK }, linkValue.data());
        if (optItem == valIndex) {
            std::size_t j = indexList.at(1);
            std::size_t k = indexList.at(2);
            auto link = entry->getLinks(true).at(j);
            if (status == STATUS_RESET && !optValue.empty()) {
                entry->setLinkValue(j, k, config->getOrigValue(valIndex));
                log_debug("Reset link value {} {}", valIndex, entry->getLinks(true).at(j).at(k).first);
            } else {
                config->setOrigValue(valIndex, link.at(k).second);
                entry->setLinkValue(j, k, optValue);
                log_debug("New link value {} {}", valIndex, optValue);
            }
        }
    }

    return false;
}

bool ConfigBoxLayoutSetup::updateDetail(const std::string& optItem,
    std::string& optValue,
    const std::shared_ptr<Config>& config,
    const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        log_debug("Updating BoxLayout Detail {} {} {}", xpath, optItem, optValue);
        auto value = std::dynamic_pointer_cast<BoxLayoutListOption>(optionValue);
        auto list = value->getBoxLayoutListOption();
        auto indexList = extractIndexList(optItem);
        std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";
        if (updateConfig<EditHelperBoxLayout, ConfigBoxLayoutSetup, BoxLayoutListOption, BoxLayout>(list, config, this, value, optItem, optValue, indexList, status))
            return true;
    }
    return false;
}

std::shared_ptr<ConfigOption> ConfigBoxLayoutSetup::newOption(
    const std::shared_ptr<Config>& config,
    const pugi::xml_node& optValue)
{
    auto result = std::make_shared<BoxLayoutList>();

    if (!createOptionFromNode(config, optValue, result)) {
        throw_std_runtime_error("Init {} BoxLayout config failed '{}'", xpath, optValue.name());
    }
    if (EDIT_CAST(EditHelperBoxLayout, result)->size() == 0) {
        log_debug("{} assigning {} default values", xpath, defaultEntries.size());
        useDefault = true;
        for (auto&& bl : defaultEntries) {
            EDIT_CAST(EditHelperBoxLayout, result)->add(std::make_shared<BoxLayout>(bl));
        }
    }
    optionValue = std::make_shared<BoxLayoutListOption>(result);
    return optionValue;
}

std::string ConfigBoxLayoutSetup::getItemPath(
    const std::vector<std::size_t>& indexList,
    const std::vector<ConfigVal>& propOptions,
    const std::string& propText) const
{
    if (indexList.empty()) {
        if (!propText.empty()) {
            return fmt::format("{}/{}[_]/{}[_]/{}[_]/{}", definition->mapConfigOption(option), definition->mapConfigOption(propOptions[0]), definition->mapConfigOption(propOptions[1]), definition->mapConfigOption(propOptions[2]), propText);
        }
        if (propOptions.size() > 2) {
            return fmt::format("{}/{}[_]//{}[_]/{}", definition->mapConfigOption(option), definition->mapConfigOption(propOptions[0]), definition->ensureAttribute(propOptions[1]), definition->mapConfigOption(propOptions[2]));
        }
        if (propOptions.size() > 1) {
            return fmt::format("{}/{}[_]/{}", definition->mapConfigOption(option), definition->mapConfigOption(propOptions[0]), definition->ensureAttribute(propOptions[1]));
        }
        if (!propOptions.empty()) {
            return fmt::format("{}/{}[_]", definition->mapConfigOption(option), definition->mapConfigOption(propOptions[0]));
        }
        return fmt::format("{}", definition->mapConfigOption(option));
    }
    if (indexList.size() == 1) {
        if (propOptions.size() > 1) {
            return fmt::format("{}/{}[{}]/{}", definition->mapConfigOption(option), definition->mapConfigOption(propOptions[0]), indexList[0], definition->ensureAttribute(propOptions[1]));
        }
        if (!propOptions.empty()) {
            return fmt::format("{}/{}[{}]", definition->mapConfigOption(option), definition->mapConfigOption(propOptions[0]), indexList[0]);
        }
    }
    if (indexList.size() == 2) {
        if (propOptions.size() > 1) {
            return fmt::format("{}/{}[{}]/{}[{}]/{}", definition->mapConfigOption(option), definition->mapConfigOption(propOptions[0]), indexList[0], definition->mapConfigOption(propOptions[1]), indexList[1], propText);
        }
        if (!propOptions.empty()) {
            return fmt::format("{}/{}[{}]/{}[{}]", definition->mapConfigOption(option), definition->mapConfigOption(propOptions[0]), indexList[0], definition->mapConfigOption(propOptions[1]), indexList[1]);
        }
    }
    if (propOptions.size() > 2) {
        return fmt::format("{}/{}[{}]/{}[{}]/{}[{}]/{}", definition->mapConfigOption(option), definition->mapConfigOption(propOptions[0]), indexList[0], definition->mapConfigOption(propOptions[1]), indexList[1], definition->mapConfigOption(propOptions[2]), indexList[2], propText);
    }
    if (propOptions.size() > 1) {
        return fmt::format("{}/{}[{}]/{}[{}]/{}", definition->mapConfigOption(option), definition->mapConfigOption(propOptions[0]), indexList[0], definition->mapConfigOption(propOptions[1]), indexList[1], propText);
    }
    if (!propOptions.empty()) {
        return fmt::format("{}/{}[{}]/{}", definition->mapConfigOption(option), definition->mapConfigOption(propOptions[0]), indexList[0], propText);
    }
    return fmt::format("{}", definition->mapConfigOption(option));
}

std::string ConfigBoxLayoutSetup::getItemPathRoot(bool prefix) const
{
    if (prefix) {
        return definition->mapConfigOption(option);
    }
    return definition->mapConfigOption(option);
}
