/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_client.cc - this file is part of Gerbera.
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

/// \file config_setup_client.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_client.h" // API

#include "config/config_definition.h"
#include "config/config_options.h"
#include "config/config_val.h"
#include "config/result/client_config.h"
#include "config_setup_bool.h"
#include "config_setup_dictionary.h"
#include "config_setup_int.h"
#include "config_setup_string.h"
#include "util/logger.h"

#include <iterator>
#include <numeric>

/// \brief Creates an array of ClientConfig objects from a XML nodeset.
/// \param element starting element of the nodeset.
bool ConfigClientSetup::createOptionFromNode(const pugi::xml_node& element, const std::shared_ptr<ClientConfigList>& result)
{
    if (!element)
        return true;

    auto&& ccs = ConfigDefinition::findConfigSetup<ConfigSetup>(ConfigVal::A_CLIENTS_CLIENT);
    for (auto&& it : ccs->getXmlTree(element)) {
        const pugi::xml_node& child = it.node();

        auto flags = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_CLIENTS_CLIENT_FLAGS)->getXmlContent(child);
        auto group = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_GROUP)->getXmlContent(child);
        auto ip = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_IP)->getXmlContent(child);
        auto userAgent = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_USERAGENT)->getXmlContent(child);
        auto captionInfoCount = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT)->getXmlContent(child);
        auto stringLimit = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT)->getXmlContent(child);
        auto multiValue = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE)->getXmlContent(child);
        auto isAllowed = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_CLIENTS_CLIENT_ALLOWED)->getXmlContent(child);
        auto mappings = ConfigDefinition::findConfigSetup<ConfigDictionarySetup>(ConfigVal::A_CLIENTS_UPNP_MAP_MIMETYPE)->getXmlContent(child);
        auto headers = ConfigDefinition::findConfigSetup<ConfigDictionarySetup>(ConfigVal::A_CLIENTS_UPNP_HEADERS)->getXmlContent(child);
        auto matchValues = std::map<ClientMatchType, std::string>();
        for (auto&& attr : child.attributes()) {
            auto matchType = ClientConfig::remapMatchType(attr.name());
            if (matchType != ClientMatchType::None && matchType != ClientMatchType::IP && matchType != ClientMatchType::UserAgent) {
                matchValues[matchType] = attr.value();
            }
        }

        auto client = std::make_shared<ClientConfig>(flags, group, ip, userAgent, mappings, headers, matchValues, captionInfoCount, stringLimit, multiValue, isAllowed);
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

bool ConfigClientSetup::updateItem(const std::vector<std::size_t>& indexList, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<ClientConfig>& entry, std::string& optValue, const std::string& status) const
{
    auto statusList = splitString(status, ',');
    auto i = indexList.at(0);
    if (optItem == getItemPath(indexList, {}) && (statusList[0] == STATUS_ADDED || statusList[0] == STATUS_MANUAL)) {
        return true;
    }
    {
        // set client flags
        auto index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT_FLAGS });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, ClientConfig::mapFlags(entry->getFlags()));
            entry->setFlags(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_CLIENTS_CLIENT_FLAGS)->checkIntValue(optValue));
            log_debug("New Client Flags {} {}", index, ClientConfig::mapFlags(config->getClientConfigListOption(option)->get(i)->getFlags()));
            return true;
        }
        // set client ip
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT_IP });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getIp());
            if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_IP)->checkValue(optValue)) {
                entry->setIp(optValue);
                log_debug("New Client IP {} {}", index, config->getClientConfigListOption(option)->get(i)->getIp());
                return true;
            }
        }
        // set client group
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT_GROUP });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getGroup());
            if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_GROUP)->checkValue(optValue)) {
                entry->setGroup(optValue);
                log_debug("New Client Group {} {}", index, config->getClientConfigListOption(option)->get(i)->getGroup());
                return true;
            }
        }
        // set client allowed
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT_ALLOWED });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getAllowed());
            entry->setAllowed(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_CLIENTS_CLIENT_ALLOWED)->checkValue(optValue));
            log_debug("New Client Allow {} {}", index, config->getClientConfigListOption(option)->get(i)->getAllowed());
            return true;
        }
        // set client userAgent
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT_USERAGENT });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getUserAgent());
            if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_USERAGENT)->checkValue(optValue)) {
                entry->setUserAgent(optValue);
                log_debug("New Client UserAgent {} {}", index, config->getClientConfigListOption(option)->get(i)->getUserAgent());
                return true;
            }
        }
        // set client upnp caption count
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getCaptionInfoCount());
            entry->setCaptionInfoCount(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT)->checkIntValue(optValue));
            log_debug("New Client CaptionInfoCount {} {}", index, config->getClientConfigListOption(option)->get(i)->getCaptionInfoCount());
            return true;
        }
        // set client string limit
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getStringLimit());
            entry->setStringLimit(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT)->checkIntValue(optValue));
            log_debug("New Client StringLimit {} {}", index, config->getClientConfigListOption(option)->get(i)->getStringLimit());
            return true;
        }
        // set client upnp multi value
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getMultiValue());
            entry->setMultiValue(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE)->checkValue(optValue));
            log_debug("New Client MultiValue {} {}", index, config->getClientConfigListOption(option)->get(i)->getMultiValue());
            return true;
        }
    }
    // set client dictionaries
    if (statusList.size() > 1) {
        log_debug("New Client Dictionary {} {} ({}, {})", optItem, statusList[1], indexList[0], indexList[1]);
        // set client mimetype from
        auto keyIndex = getItemPath(indexList, { ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM });
        auto valIndex = getItemPath(indexList, { ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO });
        auto j = indexList.at(1);
        if (optItem == keyIndex) {
            auto mappings = entry->getMimeMappings();
            auto optKey = j < mappings.size() ? (*(std::next(mappings.cbegin(), j))).first : "";
            bool done = true;
            if (statusList[1] == STATUS_REMOVED || statusList[1] == STATUS_KILLED) {
                config->setOrigValue(optItem, optKey);
                if (!optKey.empty())
                    config->setOrigValue(valIndex, mappings.at(optKey));
                done = false;
            } else if (entry->getOrig())
                config->setOrigValue(keyIndex, optKey);
            entry->setMimeMappingsFrom(j, optValue);
            if (statusList[1] == STATUS_RESET && !optValue.empty()) {
                entry->setMimeMappingsFrom(j, config->getOrigValue(valIndex));
                log_debug("Reset Client Mapping From{} {}", valIndex, entry->getMimeMappings().at(optKey));
            }
            if (done)
                log_debug("New Client Mapping From {} {}", keyIndex, (*(std::next(config->getClientConfigListOption(option)->get(i)->getMimeMappings().cbegin(), j))).first);
            return true;
        }
        // set client mimetype to
        if (optItem == valIndex) {
            auto mappings = entry->getMimeMappings();
            if (entry->getOrig())
                config->setOrigValue(valIndex, (*(std::next(mappings.cbegin(), j))).second);
            entry->setMimeMappingsTo(j, optValue);
            if (statusList[1] != STATUS_REMOVED && statusList[1] != STATUS_KILLED)
                log_debug("New Client Mapping To {} {}", valIndex, (*(std::next(config->getClientConfigListOption(option)->get(i)->getMimeMappings().cbegin(), j))).second);
            return true;
        }

        // set client headers key
        keyIndex = getItemPath(indexList, { ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY });
        valIndex = getItemPath(indexList, { ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE });
        if (optItem == keyIndex) {
            auto headers = entry->getHeaders();
            auto optKey = j < headers.size() ? (*(std::next(headers.cbegin(), j))).first : "";
            bool done = true;
            if (statusList[1] == STATUS_REMOVED || statusList[1] == STATUS_KILLED) {
                config->setOrigValue(optItem, optKey);
                if (!optKey.empty())
                    config->setOrigValue(valIndex, headers.at(optKey));
                done = false;
            } else if (entry->getOrig())
                config->setOrigValue(keyIndex, optKey);
            entry->setHeadersKey(j, optValue);
            if (statusList[1] == STATUS_RESET && !optValue.empty()) {
                entry->setHeadersKey(j, config->getOrigValue(valIndex));
                log_debug("Reset Client Mapping From{} {}", valIndex, entry->getHeaders().at(optKey));
            }
            if (done)
                log_debug("New Client Headers Key {} {}", keyIndex, (*(std::next(config->getClientConfigListOption(option)->get(i)->getHeaders().cbegin(), j))).first);
            return true;
        }
        // set client headers value
        if (optItem == valIndex) {
            auto headers = entry->getHeaders();
            if (entry->getOrig())
                config->setOrigValue(valIndex, (*(std::next(headers.cbegin(), j))).second);
            entry->setHeadersValue(j, optValue);
            if (statusList[1] != STATUS_REMOVED && statusList[1] != STATUS_KILLED)
                log_debug("New Client Headers Key {} {}", valIndex, (*(std::next(config->getClientConfigListOption(option)->get(i)->getHeaders().cbegin(), j))).second);
            return true;
        }
    }

    return false;
}

bool ConfigClientSetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        log_debug("Updating Client Detail {} {} {}", xpath, optItem, optValue);
        auto value = std::dynamic_pointer_cast<ClientConfigListOption>(optionValue);
        auto list = value->getClientConfigListOption();
        auto indexList = extractIndexList(optItem);
        std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";
        auto statusList = splitString(status, ',');

        if (indexList.size() > 0) {
            auto entry = list->get(indexList.at(0), true);

            if (!entry && (statusList.front() == STATUS_ADDED || statusList.front() == STATUS_MANUAL)) {
                entry = std::make_shared<ClientConfig>();
                list->add(entry, indexList.at(0));
            }
            if (entry && (statusList.front() == STATUS_REMOVED || statusList.front() == STATUS_KILLED)) {
                list->remove(indexList.at(0), true);
                return true;
            }
            if (entry && statusList.front() == STATUS_RESET) {
                list->add(entry, indexList.at(0));
            }
            if (entry && updateItem(indexList, optItem, config, entry, optValue, status)) {
                return true;
            }
        } else {
            indexList.push_back(0);
        }
        for (std::size_t client = 0; client < list->size(); client++) {
            auto entry = value->getClientConfigListOption()->get(client);
            indexList[0] = client;
            if (updateItem(indexList, optItem, config, entry, optValue, status)) {
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
        throw_std_runtime_error("Init {} client config failed '{}'", xpath, optValue.name());
    }
    optionValue = std::make_shared<ClientConfigListOption>(result);
    return optionValue;
}

std::string ConfigClientSetup::getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions) const
{
    if (indexList.size() == 0) {
        if (propOptions.size() > 1) {
            return fmt::format("{}[_]/{}[_]/{}", ConfigDefinition::mapConfigOption(ConfigVal::A_CLIENTS_CLIENT), ConfigDefinition::mapConfigOption(propOptions[0]), ConfigDefinition::ensureAttribute(propOptions[1]));
        }
        if (propOptions.size() > 0) {
            return fmt::format("{}[_]/{}", ConfigDefinition::mapConfigOption(ConfigVal::A_CLIENTS_CLIENT), ConfigDefinition::ensureAttribute(propOptions[0]));
        }
        return fmt::format("{}[_]", ConfigDefinition::mapConfigOption(ConfigVal::A_CLIENTS_CLIENT));
    }
    if (propOptions.size() > 0) {
        if (indexList.size() > 1) {
            return fmt::format("{}[{}]/{}[{}]/{}",
                ConfigDefinition::mapConfigOption(ConfigVal::A_CLIENTS_CLIENT),
                indexList[0],
                ConfigDefinition::mapConfigOption(propOptions[0]),
                indexList[1],
                ConfigDefinition::ensureAttribute(propOptions[1]));
        }
        return fmt::format("{}[{}]/{}", ConfigDefinition::mapConfigOption(ConfigVal::A_CLIENTS_CLIENT), indexList[0], ConfigDefinition::ensureAttribute(propOptions[0]));
    }
    return fmt::format("{}[{}]", ConfigDefinition::mapConfigOption(ConfigVal::A_CLIENTS_CLIENT), indexList[0]);
}

std::string ConfigClientSetup::getItemPathRoot(bool prefix) const
{
    return ConfigDefinition::mapConfigOption(ConfigVal::A_CLIENTS_CLIENT);
}
