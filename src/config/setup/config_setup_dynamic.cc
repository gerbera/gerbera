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
bool ConfigDynamicContentSetup::createOptionFromNode(const pugi::xml_node& element, std::shared_ptr<DynamicContentList>& result) const
{
    if (!element)
        return true;

    auto&& ccs = definition->findConfigSetup<ConfigSetup>(ConfigVal::A_DYNAMIC_CONTAINER);
    for (auto&& it : ccs->getXmlTree(element)) {
        const pugi::xml_node& child = it.node();
        fs::path location = definition->findConfigSetup<ConfigPathSetup>(ConfigVal::A_DYNAMIC_CONTAINER_LOCATION)->getXmlContent(child);

        auto cont = std::make_shared<DynamicContent>(location);

        {
            auto cs = definition->findConfigSetup<ConfigPathSetup>(ConfigVal::A_DYNAMIC_CONTAINER_IMAGE);
            cont->setImage(cs->getXmlContent(child));
        }
        {
            auto cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_TITLE);
            cont->setTitle(cs->getXmlContent(child));
            if (cont->getTitle().empty()) {
                cont->setTitle(location.filename());
            }
            cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_SORT);
            cont->setSort(cs->getXmlContent(child));
            cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_FILTER);
            cont->setFilter(cs->getXmlContent(child));
            cs = definition->findConfigSetup<ConfigStringSetup>(ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT);
            cont->setUpnpShortcut(cs->getXmlContent(child));
        }
        {
            auto cs = definition->findConfigSetup<ConfigIntSetup>(ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT);
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

bool ConfigDynamicContentSetup::updateItem(
    const std::vector<std::size_t>& indexList,
    const std::string& optItem,
    const std::shared_ptr<Config>& config,
    std::shared_ptr<DynamicContent>& entry,
    std::string& optValue,
    const std::string& status) const
{
    if (optItem == getItemPath(indexList, {}) && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
        return true;
    }

    static auto resultProperties = std::vector<ConfigResultProperty<DynamicContent>> {
        // Location
        {
            { ConfigVal::A_DYNAMIC_CONTAINER_LOCATION },
            "Location",
            [&](const std::shared_ptr<DynamicContent>& entry) { return entry->getLocation(); },
            [&](const std::shared_ptr<DynamicContent>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                auto pathValue = optValue;
                if (definition->findConfigSetup<ConfigPathSetup>(cfg)->checkPathValue(optValue, pathValue)) {
                    entry->setLocation(pathValue);
                    return true;
                }
                return false;
            },
        },
        // Image
        {
            { ConfigVal::A_DYNAMIC_CONTAINER_IMAGE },
            "Image",
            [&](const std::shared_ptr<DynamicContent>& entry) { return entry->getImage(); },
            [&](const std::shared_ptr<DynamicContent>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigPathSetup>(cfg)->checkValue(optValue)) {
                    entry->setImage(optValue);
                    return true;
                }
                return false;
            },
        },
        // Title
        {
            { ConfigVal::A_DYNAMIC_CONTAINER_TITLE },
            "Title",
            [&](const std::shared_ptr<DynamicContent>& entry) { return entry->getTitle(); },
            [&](const std::shared_ptr<DynamicContent>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setTitle(optValue);
                    return true;
                }
                return false;
            },
        },
        // Filter
        {
            { ConfigVal::A_DYNAMIC_CONTAINER_FILTER },
            "Filter",
            [&](const std::shared_ptr<DynamicContent>& entry) { return entry->getFilter(); },
            [&](const std::shared_ptr<DynamicContent>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setFilter(optValue);
                    return true;
                }
                return false;
            },
        },
        // shortcut
        {
            { ConfigVal::A_DYNAMIC_CONTAINER_UPNP_SHORTCUT },
            "shortcut",
            [&](const std::shared_ptr<DynamicContent>& entry) { return entry->getUpnpShortcut(); },
            [&](const std::shared_ptr<DynamicContent>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setUpnpShortcut(optValue);
                    return true;
                }
                return false;
            },
        },
        // sort
        {
            { ConfigVal::A_DYNAMIC_CONTAINER_SORT },
            "sort",
            [&](const std::shared_ptr<DynamicContent>& entry) { return entry->getSort(); },
            [&](const std::shared_ptr<DynamicContent>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                if (definition->findConfigSetup<ConfigStringSetup>(cfg)->checkValue(optValue)) {
                    entry->setSort(optValue);
                    return true;
                }
                return false;
            },
        },
        // max count
        {
            { ConfigVal::A_DYNAMIC_CONTAINER_MAXCOUNT },
            "max count",
            [&](const std::shared_ptr<DynamicContent>& entry) { return fmt::to_string(entry->getMaxCount()); },
            [&](const std::shared_ptr<DynamicContent>& entry, const std::shared_ptr<ConfigDefinition>& definition, ConfigVal cfg, std::string& optValue) {
                entry->setMaxCount(definition->findConfigSetup<ConfigIntSetup>(cfg)->checkIntValue(optValue));
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
                auto nEntry = config->getDynamicContentListOption(option)->get(i);
                log_debug("New value for DynamicContent {} {} = {}", label.data(), index, getProperty(nEntry));
                return true;
            }
        }
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
            return fmt::format("{}/{}[_]/{}", xpath, definition->mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER), definition->ensureAttribute(propOptions[0]));
        }
        return fmt::format("{}/{}[_]", xpath, definition->mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER));
    }
    if (propOptions.size() > 0) {
        return fmt::format("{}/{}[{}]/{}", xpath, definition->mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER), indexList[0], definition->ensureAttribute(propOptions[0]));
    }
    return fmt::format("{}/{}[{}]", xpath, definition->mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER), indexList[0]);
}

std::string ConfigDynamicContentSetup::getItemPathRoot(bool prefix) const
{
    return fmt::format("{}/{}", xpath, definition->mapConfigOption(ConfigVal::A_DYNAMIC_CONTAINER));
}
