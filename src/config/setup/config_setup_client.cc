/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_client.cc - this file is part of Gerbera.
    Copyright (C) 2020-2022 Gerbera Contributors

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

/// \file config_setup_client.cc

#include "config/config_setup.h" // API

#include <numeric>

#include "config/client_config.h"
#include "config/config_definition.h"
#include "config/config_options.h"

/// \brief Creates an array of ClientConfig objects from a XML nodeset.
/// \param element starting element of the nodeset.
bool ConfigClientSetup::createOptionFromNode(const pugi::xml_node& element, std::shared_ptr<ClientConfigList>& result)
{
    if (!element)
        return true;

    auto&& ccs = ConfigDefinition::findConfigSetup<ConfigSetup>(ATTR_CLIENTS_CLIENT);
    for (auto&& it : ccs->getXmlTree(element)) {
        const pugi::xml_node& child = it.node();

        auto flags = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_CLIENTS_CLIENT_FLAGS)->getXmlContent(child);
        auto group = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_GROUP)->getXmlContent(child);
        auto ip = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_IP)->getXmlContent(child);
        auto userAgent = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_USERAGENT)->getXmlContent(child);
        auto captionInfoCount = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_CLIENTS_UPNP_CAPTION_COUNT)->getXmlContent(child);
        auto stringLimit = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_CLIENTS_UPNP_STRING_LIMIT)->getXmlContent(child);
        auto multiValue = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_CLIENTS_UPNP_MULTI_VALUE)->getXmlContent(child);
        auto mappings = ConfigDefinition::findConfigSetup<ConfigDictionarySetup>(ATTR_CLIENTS_UPNP_MAP_MIMETYPE)->getXmlContent(child);

        auto client = std::make_shared<ClientConfig>(flags, group, ip, userAgent, mappings, captionInfoCount, stringLimit, multiValue);
        try {
            result->add(client);
        } catch (const std::runtime_error& e) {
            throw_std_runtime_error("Could not add {} client: {}", ip, e.what());
        }
    }

    return true;
}

void ConfigClientSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (arguments && arguments->find("isEnabled") != arguments->end()) {
        isEnabled = arguments->at("isEnabled") == "true";
    }
    newOption(getXmlElement(root));
    setOption(config);
}

bool ConfigClientSetup::updateItem(std::size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<ClientConfig>& entry, std::string& optValue, const std::string& status) const
{
    if (optItem == getItemPath(i) && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
        return true;
    }
    auto index = getItemPath(i, ATTR_CLIENTS_CLIENT_FLAGS);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, ClientConfig::mapFlags(entry->getFlags()));
        entry->setFlags(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_CLIENTS_CLIENT_FLAGS)->checkIntValue(optValue));
        log_debug("New Client Detail {} {}", index, ClientConfig::mapFlags(config->getClientConfigListOption(option)->get(i)->getFlags()));
        return true;
    }
    index = getItemPath(i, ATTR_CLIENTS_CLIENT_IP);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getIp());
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_IP)->checkValue(optValue)) {
            entry->setIp(optValue);
            log_debug("New Client Detail {} {}", index, config->getClientConfigListOption(option)->get(i)->getIp());
            return true;
        }
    }
    index = getItemPath(i, ATTR_CLIENTS_CLIENT_GROUP);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getGroup());
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_GROUP)->checkValue(optValue)) {
            entry->setGroup(optValue);
            log_debug("New Client Detail {} {}", index, config->getClientConfigListOption(option)->get(i)->getGroup());
            return true;
        }
    }
    index = getItemPath(i, ATTR_CLIENTS_CLIENT_USERAGENT);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getUserAgent());
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_USERAGENT)->checkValue(optValue)) {
            entry->setUserAgent(optValue);
            log_debug("New Client Detail {} {}", index, config->getClientConfigListOption(option)->get(i)->getUserAgent());
            return true;
        }
    }
    index = getItemPath(i, ATTR_CLIENTS_UPNP_CAPTION_COUNT);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getCaptionInfoCount());
        entry->setCaptionInfoCount(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_CLIENTS_UPNP_CAPTION_COUNT)->checkIntValue(optValue));
        log_debug("New Client Detail {} {}", index, config->getClientConfigListOption(option)->get(i)->getCaptionInfoCount());
        return true;
    }
    index = getItemPath(i, ATTR_CLIENTS_UPNP_STRING_LIMIT);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getStringLimit());
        entry->setStringLimit(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_CLIENTS_UPNP_STRING_LIMIT)->checkIntValue(optValue));
        log_debug("New Client Detail {} {}", index, config->getClientConfigListOption(option)->get(i)->getStringLimit());
        return true;
    }
    index = getItemPath(i, ATTR_CLIENTS_UPNP_MULTI_VALUE);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getMultiValue());
        entry->setStringLimit(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_CLIENTS_UPNP_MULTI_VALUE)->checkValue(optValue));
        log_debug("New Client Detail {} {}", index, config->getClientConfigListOption(option)->get(i)->getMultiValue());
        return true;
    }
    return false;
}

bool ConfigClientSetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        log_debug("Updating Client Detail {} {} {}", xpath, optItem, optValue);
        auto value = std::dynamic_pointer_cast<ClientConfigListOption>(optionValue);
        auto list = value->getClientConfigListOption();
        auto index = extractIndex(optItem);

        if (index < std::numeric_limits<std::size_t>::max()) {
            auto entry = list->get(index, true);
            std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";

            if (!entry && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
                entry = std::make_shared<ClientConfig>();
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
        for (std::size_t client = 0; client < list->size(); client++) {
            auto entry = value->getClientConfigListOption()->get(client);
            if (updateItem(client, optItem, config, entry, optValue)) {
                return true;
            }
        }
    }
    return false;
}

std::shared_ptr<ConfigOption> ConfigClientSetup::newOption(const pugi::xml_node& optValue)
{
    auto result = std::make_shared<ClientConfigList>();

    if (!createOptionFromNode(isEnabled ? optValue : pugi::xml_node(nullptr), result)) {
        throw_std_runtime_error("Init {} client config failed '{}'", xpath, optValue);
    }
    optionValue = std::make_shared<ClientConfigListOption>(result);
    return optionValue;
}

std::string ConfigClientSetup::getItemPath(int index, config_option_t propOption, config_option_t propOption2, config_option_t propOption3, config_option_t propOption4) const
{
    if (index == ITEM_PATH_ROOT) {
        return ConfigDefinition::mapConfigOption(ATTR_CLIENTS_CLIENT);
    }
    if (index == ITEM_PATH_NEW) {
        if (propOption != CFG_MAX) {
            return fmt::format("{}[_]/{}", ConfigDefinition::mapConfigOption(ATTR_CLIENTS_CLIENT), ConfigDefinition::ensureAttribute(propOption));
        }
        return fmt::format("{}[_]", ConfigDefinition::mapConfigOption(ATTR_CLIENTS_CLIENT));
    }
    if (propOption != CFG_MAX) {
        return fmt::format("{}[{}]/{}", ConfigDefinition::mapConfigOption(ATTR_CLIENTS_CLIENT), index, ConfigDefinition::ensureAttribute(propOption));
    }
    return fmt::format("{}[{}]", ConfigDefinition::mapConfigOption(ATTR_CLIENTS_CLIENT), index);
}
