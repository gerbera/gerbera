/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_dynamic.cc - this file is part of Gerbera.
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

/// \file config_setup_dynamic.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config/setup/config_setup_dynamic.h" // API

#include "config/config_definition.h"
#include "config/config_options.h"
#include "config/result/dynamic_content.h"
#include "config_setup_int.h"
#include "config_setup_path.h"
#include "config_setup_string.h"
#include "util/logger.h"

#include <numeric>

/// \brief Creates an array of DynamicContent objects from a XML nodeset.
/// \param element starting element of the nodeset.
bool ConfigDynamicContentSetup::createOptionFromNode(const pugi::xml_node& element, std::shared_ptr<DynamicContentList>& result)
{
    if (!element)
        return true;

    auto&& ccs = ConfigDefinition::findConfigSetup<ConfigSetup>(ConfigVal::A_DYNAMIC_CONTAINER);
    for (auto&& it : ccs->getXmlTree(element)) {
        const pugi::xml_node& child = it.node();
        fs::path location = ConfigDefinition::findConfigSetup<ConfigPathSetup>(ConfigVal::A_DYNAMIC_CONTAINER_LOCATION)->getXmlContent(child);

        auto cont = std::make_shared<DynamicContent>(location);

        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigPathSetup>(ConfigVal::A_DYNAMIC_CONTAINER_IMAGE);
            cont->setImage(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_TITLE);
            cont->setTitle(cs->getXmlContent(child));
            if (cont->getTitle().empty()) {
                cont->setTitle(location.filename());
            }
            cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_SORT);
            cont->setSort(cs->getXmlContent(child));
            cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_FILTER);
            cont->setFilter(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT);
            cont->setMaxCount(cs->getXmlContent(child));
        }
        try {
            result->add(cont);
        } catch (const std::runtime_error& e) {
            throw_std_runtime_error("Could not add {} DynamicContent: {}", location.string(), e.what());
        }
    }

    return true;
}

void ConfigDynamicContentSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlElement(root));
    setOption(config);
}

bool ConfigDynamicContentSetup::updateItem(std::size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<DynamicContent>& entry, std::string& optValue, const std::string& status) const
{
    if (optItem == getItemPath(i) && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
        return true;
    }

    auto index = getItemPath(i, ConfigVal::A_DYNAMIC_CONTAINER_LOCATION);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getLocation());
        auto pathValue = optValue;
        if (ConfigDefinition::findConfigSetup<ConfigPathSetup>(ConfigVal::A_DYNAMIC_CONTAINER_LOCATION)->checkPathValue(optValue, pathValue)) {
            entry->setLocation(pathValue);
            log_debug("New DynamicContent Detail {} {}", index, config->getDynamicContentListOption(option)->get(i)->getLocation().string());
            return true;
        }
    }
    index = getItemPath(i, ConfigVal::A_DYNAMIC_CONTAINER_IMAGE);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getImage());
        if (ConfigDefinition::findConfigSetup<ConfigPathSetup>(ConfigVal::A_DYNAMIC_CONTAINER_IMAGE)->checkValue(optValue)) {
            entry->setImage(optValue);
            log_debug("New DynamicContent Detail {} {}", index, config->getDynamicContentListOption(option)->get(i)->getImage().string());
            return true;
        }
    }
    index = getItemPath(i, ConfigVal::A_DYNAMIC_CONTAINER_TITLE);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getTitle());
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_TITLE)->checkValue(optValue)) {
            entry->setTitle(optValue);
            log_debug("New DynamicContent Detail {} {}", index, config->getDynamicContentListOption(option)->get(i)->getTitle());
            return true;
        }
    }
    index = getItemPath(i, ConfigVal::A_DYNAMIC_CONTAINER_FILTER);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getFilter());
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_FILTER)->checkValue(optValue)) {
            entry->setFilter(optValue);
            log_debug("New DynamicContent Detail {} {}", index, config->getDynamicContentListOption(option)->get(i)->getFilter());
            return true;
        }
    }
    index = getItemPath(i, ConfigVal::A_DYNAMIC_CONTAINER_SORT);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getSort());
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_SORT)->checkValue(optValue)) {
            entry->setSort(optValue);
            log_debug("New DynamicContent Detail {} {}", index, config->getDynamicContentListOption(option)->get(i)->getSort());
            return true;
        }
    }
    index = getItemPath(i, ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getMaxCount());
        entry->setMaxCount(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT)->checkIntValue(optValue));
        log_debug("New DynamicContent Detail {} {}", index, config->getDynamicContentListOption(option)->get(i)->getMaxCount());
        return true;
    }

    return false;
}

bool ConfigDynamicContentSetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        log_debug("Updating DynamicContent Detail {} {} {}", xpath, optItem, optValue);
        auto value = std::dynamic_pointer_cast<DynamicContentListOption>(optionValue);
        auto list = value->getDynamicContentListOption();
        auto index = extractIndex(optItem);

        if (index < std::numeric_limits<std::size_t>::max()) {
            auto entry = list->get(index, true);
            std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";

            if (!entry && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
                entry = std::make_shared<DynamicContent>();
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
            auto entry = value->getDynamicContentListOption()->get(tweak);
            if (updateItem(tweak, optItem, config, entry, optValue)) {
                return true;
            }
        }
    }
    return false;
}

std::shared_ptr<ConfigOption> ConfigDynamicContentSetup::newOption(const pugi::xml_node& optValue)
{
    auto result = std::make_shared<DynamicContentList>();

    if (!createOptionFromNode(optValue, result)) {
        throw_std_runtime_error("Init {} DynamicContentList failed '{}'", xpath, optValue.name());
    }
    optionValue = std::make_shared<DynamicContentListOption>(result);
    return optionValue;
}

std::string ConfigDynamicContentSetup::getItemPath(int index, ConfigVal propOption, ConfigVal propOption2, ConfigVal propOption3, ConfigVal propOption4) const
{
    if (index == ITEM_PATH_ROOT) {
        return fmt::format("{}/{}", xpath, ConfigDefinition::mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER));
    }
    if (index == ITEM_PATH_NEW) {
        if (propOption != ConfigVal::MAX) {
            return fmt::format("{}/{}[_]/{}", xpath, ConfigDefinition::mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER), ConfigDefinition::ensureAttribute(propOption));
        }
        return fmt::format("{}/{}[_]", xpath, ConfigDefinition::mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER));
    }
    if (propOption != ConfigVal::MAX) {
        return fmt::format("{}/{}[{}]/{}", xpath, ConfigDefinition::mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER), index, ConfigDefinition::ensureAttribute(propOption));
    }
    return fmt::format("{}/{}[{}]", xpath, ConfigDefinition::mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER), index);
}
