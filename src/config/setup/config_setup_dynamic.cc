/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_dynamic.cc - this file is part of Gerbera.

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

/// \file config_setup_dynamic.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config/setup/config_setup_dynamic.h" // API

#include "config/config_definition.h"
#include "config/config_options.h"
#include "config/config_val.h"
#include "config/result/dynamic_content.h"
#include "config_setup_int.h"
#include "config_setup_path.h"
#include "config_setup_string.h"
#include "setup_util.h"
#include "util/logger.h"

#include <numeric>

/// \brief Creates an array of DynamicContent objects from a XML nodeset.
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
            cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT);
            cont->setUpnpShortcut(cs->getXmlContent(child));
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

bool ConfigDynamicContentSetup::updateItem(const std::vector<std::size_t>& indexList, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<DynamicContent>& entry, std::string& optValue, const std::string& status) const
{
    if (optItem == getItemPath(indexList, {}) && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
        return true;
    }
    auto i = indexList.at(0);

    auto index = getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_LOCATION });
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
    index = getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_IMAGE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getImage());
        if (ConfigDefinition::findConfigSetup<ConfigPathSetup>(ConfigVal::A_DYNAMIC_CONTAINER_IMAGE)->checkValue(optValue)) {
            entry->setImage(optValue);
            log_debug("New DynamicContent Detail {} {}", index, config->getDynamicContentListOption(option)->get(i)->getImage().string());
            return true;
        }
    }
    index = getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_TITLE });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getTitle());
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_TITLE)->checkValue(optValue)) {
            entry->setTitle(optValue);
            log_debug("New DynamicContent Detail {} {}", index, config->getDynamicContentListOption(option)->get(i)->getTitle());
            return true;
        }
    }
    index = getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_FILTER });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getFilter());
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_FILTER)->checkValue(optValue)) {
            entry->setFilter(optValue);
            log_debug("New DynamicContent Detail {} {}", index, config->getDynamicContentListOption(option)->get(i)->getFilter());
            return true;
        }
    }
    index = getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getUpnpShortcut());
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT)->checkValue(optValue)) {
            entry->setUpnpShortcut(optValue);
            log_debug("New DynamicContent Detail {} {}", index, config->getDynamicContentListOption(option)->get(i)->getUpnpShortcut());
            return true;
        }
    }
    index = getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_SORT });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getSort());
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_SORT)->checkValue(optValue)) {
            entry->setSort(optValue);
            log_debug("New DynamicContent Detail {} {}", index, config->getDynamicContentListOption(option)->get(i)->getSort());
            return true;
        }
    }
    index = getItemPath(indexList, { ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT });
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getMaxCount());
        entry->setMaxCount(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT)->checkIntValue(optValue));
        log_debug("New DynamicContent Detail {} {}", index, config->getDynamicContentListOption(option)->get(i)->getMaxCount());
        return true;
    }

    return false;
}

bool ConfigDynamicContentSetup::updateDetail(const std::string& optItem,
    std::string& optValue,
    const std::shared_ptr<Config>& config,
    const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        log_debug("Updating DynamicContent Detail {} {} {}", xpath, optItem, optValue);
        auto value = std::dynamic_pointer_cast<DynamicContentListOption>(optionValue);
        auto list = value->getDynamicContentListOption();
        auto indexList = extractIndexList(optItem);
        std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";
        if (updateConfig<EditHelperDynamicContent, ConfigDynamicContentSetup, DynamicContentListOption, DynamicContent>(list, config, this, value, optItem, optValue, indexList, status))
            return true;
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

std::string ConfigDynamicContentSetup::getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions, const std::string& propText) const
{
    if (indexList.size() == 0) {
        if (propOptions.size() > 0) {
            return fmt::format("{}/{}[_]/{}", xpath, ConfigDefinition::mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER), ConfigDefinition::ensureAttribute(propOptions[0]));
        }
        return fmt::format("{}/{}[_]", xpath, ConfigDefinition::mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER));
    }
    if (propOptions.size() > 0) {
        return fmt::format("{}/{}[{}]/{}", xpath, ConfigDefinition::mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER), indexList[0], ConfigDefinition::ensureAttribute(propOptions[0]));
    }
    return fmt::format("{}/{}[{}]", xpath, ConfigDefinition::mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER), indexList[0]);
}

std::string ConfigDynamicContentSetup::getItemPathRoot(bool prefix) const
{
    return fmt::format("{}/{}", xpath, ConfigDefinition::mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER));
}
