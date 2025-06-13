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

/// \file config_setup_boxlayout.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_boxlayout.h" // API

#include "config/config_definition.h"
#include "config/config_option_enum.h"
#include "config/config_options.h"
#include "config/config_val.h"
#include "config/result/box_layout.h"
#include "config_setup_bool.h"
#include "config_setup_int.h"
#include "config_setup_string.h"
#include "setup_util.h"
#include "util/logger.h"

#include <algorithm>
#include <numeric>

/// \brief Creates an array of BoxLayout objects from a XML nodeset.
/// \param element starting element of the nodeset.
bool ConfigBoxLayoutSetup::createOptionFromNode(const pugi::xml_node& element, const std::shared_ptr<BoxLayoutList>& result)
{
    if (!element)
        return true;

    std::vector<std::string> allKeys;
    auto&& ccs = definition->findConfigSetup<ConfigSetup>(option);
    auto doExtend = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_LIST_EXTEND)->getXmlContent(element.parent());
    log_debug("is {} extensible {}", element.parent().path(), doExtend);
    for (auto&& it : ccs->getXmlTree(element)) {
        const pugi::xml_node& child = it.node();

        auto key = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_KEY)->getXmlContent(child);
        auto title = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_TITLE)->getXmlContent(child);
        auto objClass = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_CLASS)->getXmlContent(child);
        auto upnpShortcut = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT)->getXmlContent(child);
        auto size = definition->findConfigSetup<ConfigIntSetup>(ConfigVal::A_BOXLAYOUT_BOX_SIZE)->getXmlContent(child);
        auto enabled = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_BOXLAYOUT_BOX_ENABLED)->getXmlContent(child);
        auto sortKey = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_SORT_KEY)->getXmlContent(child);

        if (!enabled && ((key == BoxKeys::audioRoot) || (key == BoxKeys::audioAll) || (key == BoxKeys::imageRoot) || (key == BoxKeys::imageAll) || (key == BoxKeys::videoRoot) || (key == BoxKeys::videoAll))) {
            log_warning("Box '{}' cannot be disabled", key);
            enabled = true;
        }

        auto box = std::make_shared<BoxLayout>(key, title, objClass, upnpShortcut, sortKey, enabled, size);
        try {
            result->add(box);
            allKeys.push_back(key);
            log_debug("Created BoxLayout key={}, title={}, objClass={}, enabled={}, size={}", key, title, objClass, enabled, size);
        } catch (const std::runtime_error& e) {
            throw_std_runtime_error("Could not add {} boxlayout: {}", key, e.what());
        }
    }
    for (auto&& defEntry : defaultEntries) {
        if (std::find(allKeys.begin(), allKeys.end(), defEntry.getKey()) == allKeys.end()) {
            auto box = std::make_shared<BoxLayout>(defEntry.getKey(), defEntry.getTitle(), defEntry.getClass(), defEntry.getUpnpShortcut(), defEntry.getSortKey(), defEntry.getEnabled(), defEntry.getSize());
            if (doExtend)
                log_debug("Created default BoxLayout key={}, title={}, objClass={}, enabled={}, size={}", defEntry.getKey(), defEntry.getTitle(), defEntry.getClass(), defEntry.getEnabled(), defEntry.getSize());
            else
                log_info("Created default BoxLayout key={}, title={}, objClass={}, enabled={}, size={}", defEntry.getKey(), defEntry.getTitle(), defEntry.getClass(), defEntry.getEnabled(), defEntry.getSize());
            result->add(box);
            allKeys.push_back(defEntry.getKey());
        }
    }

    return true;
}

bool ConfigBoxLayoutSetup::validate(const std::shared_ptr<Config>& config, const std::shared_ptr<BoxLayoutList>& values)
{
    auto layoutType = EnumOption<LayoutType>::getEnumOption(config, ConfigVal::IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE);
    if (layoutType == LayoutType::Js)
        return true;

    for (auto&& theBox : values->getArrayCopy()) {
        if (std::find_if(defaultEntries.cbegin(), defaultEntries.cend(), [&theBox](auto& box) { return theBox->getKey() == box.getKey(); }) == defaultEntries.cend()) {
            // Warn the user that his option will be ignored in built-in layout. But allow to use "unknown" options for js
            log_warning("Box key={}, title={}, objClass={}, enabled={}, size={} will be ignored in built-in layout. Unknown Key '{}'",
                theBox->getKey(), theBox->getTitle(), theBox->getClass(),
                theBox->getEnabled(), theBox->getSize(), theBox->getKey());
        }
    }
    return true;
}

void ConfigBoxLayoutSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlElement(root));
    setOption(config);
}

bool ConfigBoxLayoutSetup::updateItem(const std::vector<std::size_t>& indexList,
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
            { ConfigVal::A_BOXLAYOUT_BOX_KEY },
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
            { ConfigVal::A_BOXLAYOUT_BOX_TITLE },
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
            { ConfigVal::A_BOXLAYOUT_BOX_CLASS },
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
            { ConfigVal::A_BOXLAYOUT_BOX_UPNP_SHORTCUT },
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
            { ConfigVal::A_BOXLAYOUT_BOX_SIZE },
            "BoxSize",
            [&](const std::shared_ptr<BoxLayout>& entry) { return fmt::to_string(entry->getSize()); },
            [&](const std::shared_ptr<BoxLayout>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setSize(definition->findConfigSetup<ConfigIntSetup>(cfg)->checkIntValue(optValue));
                return true;
            },
        },
        // Enabled
        {
            { ConfigVal::A_BOXLAYOUT_BOX_ENABLED },
            "Enabled",
            [&](const std::shared_ptr<BoxLayout>& entry) { return fmt::to_string(entry->getEnabled()); },
            [&](const std::shared_ptr<BoxLayout>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setEnabled(definition->findConfigSetup<ConfigBoolSetup>(cfg)->checkValue(optValue));
                return true;
            },
        },
        // SortKey
        {
            { ConfigVal::A_BOXLAYOUT_BOX_SORT_KEY },
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
                auto nEntry = config->getBoxLayoutListOption(option)->get(i);
                log_debug("New value for BoxLayout {} {} = {}", label.data(), index, getProperty(nEntry));
                return true;
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

std::shared_ptr<ConfigOption> ConfigBoxLayoutSetup::newOption(const pugi::xml_node& optValue)
{
    auto result = std::make_shared<BoxLayoutList>();

    if (!createOptionFromNode(optValue, result)) {
        throw_std_runtime_error("Init {} BoxLayout config failed '{}'", xpath, optValue.name());
    }
    if (result->size() == 0) {
        log_debug("{} assigning {} default values", xpath, defaultEntries.size());
        useDefault = true;
        for (auto&& bl : defaultEntries) {
            result->add(std::make_shared<BoxLayout>(bl));
        }
    }
    optionValue = std::make_shared<BoxLayoutListOption>(result);
    return optionValue;
}

std::string ConfigBoxLayoutSetup::getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions, const std::string& propText) const
{
    if (indexList.size() == 0) {
        if (propOptions.size() > 0) {
            return fmt::format("{}[_]/{}", definition->mapConfigOption(option), definition->ensureAttribute(propOptions[0]));
        }
        return fmt::format("{}[_]", definition->mapConfigOption(option));
    }
    if (propOptions.size() > 0) {
        return fmt::format("{}[{}]/{}", definition->mapConfigOption(option), indexList[0], definition->ensureAttribute(propOptions[0]));
    }
    return fmt::format("{}[{}]", definition->mapConfigOption(option), indexList[0]);
}

std::string ConfigBoxLayoutSetup::getItemPathRoot(bool prefix) const
{
    if (prefix) {
        return definition->mapConfigOption(option);
    }
    return definition->mapConfigOption(option);
}
