/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_boxlayout.cc - this file is part of Gerbera.
    Copyright (C) 2023-2024 Gerbera Contributors

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
    auto&& ccs = ConfigDefinition::findConfigSetup<ConfigSetup>(option);
    for (auto&& it : ccs->getXmlTree(element)) {
        const pugi::xml_node& child = it.node();

        auto key = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_KEY)->getXmlContent(child);
        auto title = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_TITLE)->getXmlContent(child);
        auto objClass = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_CLASS)->getXmlContent(child);
        auto size = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_BOXLAYOUT_BOX_SIZE)->getXmlContent(child);
        auto enabled = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_BOXLAYOUT_BOX_ENABLED)->getXmlContent(child);

        if (!enabled && ((key == BoxKeys::audioRoot) || (key == BoxKeys::audioAll) || (key == BoxKeys::imageRoot) || (key == BoxKeys::imageAll) || (key == BoxKeys::videoRoot) || (key == BoxKeys::videoAll))) {
            log_warning("Box '{}' cannot be disabled", key);
            enabled = true;
        }

        auto box = std::make_shared<BoxLayout>(key, title, objClass, enabled, size);
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
            auto box = std::make_shared<BoxLayout>(defEntry.getKey(), defEntry.getTitle(), defEntry.getClass(), defEntry.getEnabled(), defEntry.getSize());
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

bool ConfigBoxLayoutSetup::updateItem(std::size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<BoxLayout>& entry, std::string& optValue, const std::string& status) const
{
    if (optItem == getItemPath(i, {}) && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
        return true;
    }
    auto index = getItemPath(i, { ConfigVal::A_BOXLAYOUT_BOX_KEY });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getKey());
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_KEY)->checkValue(optValue)) {
            entry->setKey(optValue);
            log_debug("New BoxLayout Detail {} {}", index, config->getBoxLayoutListOption(option)->get(i)->getKey());
            return true;
        }
    }
    index = getItemPath(i, { ConfigVal::A_BOXLAYOUT_BOX_TITLE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getTitle());
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_TITLE)->checkValue(optValue)) {
            entry->setTitle(optValue);
            log_debug("New BoxLayout Detail {} {}", index, config->getBoxLayoutListOption(option)->get(i)->getTitle());
            return true;
        }
    }
    index = getItemPath(i, { ConfigVal::A_BOXLAYOUT_BOX_CLASS });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getClass());
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_BOXLAYOUT_BOX_CLASS)->checkValue(optValue)) {
            entry->setClass(optValue);
            log_debug("New BoxLayout Detail {} {}", index, config->getBoxLayoutListOption(option)->get(i)->getClass());
            return true;
        }
    }
    index = getItemPath(i, { ConfigVal::A_BOXLAYOUT_BOX_SIZE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getSize());
        entry->setSize(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_BOXLAYOUT_BOX_SIZE)->checkIntValue(optValue));
        log_debug("New BoxLayout Detail {} {}", index, config->getBoxLayoutListOption(option)->get(i)->getSize());
        return true;
    }
    index = getItemPath(i, { ConfigVal::A_BOXLAYOUT_BOX_ENABLED });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getEnabled());
        entry->setEnabled(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_BOXLAYOUT_BOX_ENABLED)->checkValue(optValue));
        log_debug("New BoxLayout Detail {} {}", index, config->getBoxLayoutListOption(option)->get(i)->getEnabled());
        return true;
    }
    return false;
}

bool ConfigBoxLayoutSetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        log_debug("Updating BoxLayout Detail {} {} {}", xpath, optItem, optValue);
        auto value = std::dynamic_pointer_cast<BoxLayoutListOption>(optionValue);
        auto list = value->getBoxLayoutListOption();
        auto index = extractIndex(optItem);

        if (index < std::numeric_limits<std::size_t>::max()) {
            auto entry = list->get(index, true);
            std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";

            if (!entry && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
                entry = std::make_shared<BoxLayout>();
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
        for (std::size_t box = 0; box < list->size(); box++) {
            auto entry = value->getBoxLayoutListOption()->get(box);
            if (updateItem(box, optItem, config, entry, optValue)) {
                return true;
            }
        }
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

std::string ConfigBoxLayoutSetup::getItemPath(int index, const std::vector<ConfigVal>& propOptions) const
{
    if (index == ITEM_PATH_PREFIX) {
        return ConfigDefinition::mapConfigOption(option);
    }
    if (index == ITEM_PATH_ROOT) {
        return ConfigDefinition::mapConfigOption(option);
    }
    if (index == ITEM_PATH_NEW) {
        if (propOptions.size() > 0) {
            return fmt::format("{}[_]/{}", ConfigDefinition::mapConfigOption(option), ConfigDefinition::ensureAttribute(propOptions[0]));
        }
        return fmt::format("{}[_]", ConfigDefinition::mapConfigOption(option));
    }
    if (propOptions.size() > 0) {
        return fmt::format("{}[{}]/{}", ConfigDefinition::mapConfigOption(option), index, ConfigDefinition::ensureAttribute(propOptions[0]));
    }
    return fmt::format("{}[{}]", ConfigDefinition::mapConfigOption(option), index);
}
