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

#include "cds/cds_resource.h"
#include "config/config_definition.h"
#include "config/config_options.h"
#include "config/config_val.h"
#include "config/result/client_config.h"
#include "config_setup_array.h"
#include "config_setup_bool.h"
#include "config_setup_dictionary.h"
#include "config_setup_int.h"
#include "config_setup_path.h"
#include "config_setup_string.h"
#include "config_setup_vector.h"
#include "setup_util.h"
#include "util/logger.h"

#include <iterator>
#include <numeric>

/// \brief Creates an array of ClientConfig objects from a XML nodeset.
/// \param element starting element of the nodeset.
bool ConfigClientSetup::createOptionFromNode(const pugi::xml_node& element, const std::shared_ptr<ClientConfigList>& result)
{
    if (!element)
        return true;

    auto&& gcs = ConfigDefinition::findConfigSetup<ConfigSetup>(ConfigVal::A_CLIENTS_GROUP);
    std::map<std::string, std::shared_ptr<ClientGroupConfig>> groupCache;
    for (auto&& it : gcs->getXmlTree(element)) {
        const pugi::xml_node& child = it.node();
        auto name = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_CLIENTS_GROUP_NAME)->getXmlContent(child);
        auto group = std::make_shared<ClientGroupConfig>(name);
        auto forbiddenDirectories = ConfigDefinition::findConfigSetup<ConfigArraySetup>(ConfigVal::A_CLIENTS_GROUP_HIDDEN_LIST)->getXmlContent(child);
        group->setForbiddenDirectories(forbiddenDirectories);
        EDIT_CAST(EditHelperClientGroupConfig, result)->add(group);
        groupCache[name] = group;
    }

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
        auto fullFilter = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_CLIENTS_UPNP_FILTER_FULL)->getXmlContent(child);
        auto isAllowed = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_CLIENTS_CLIENT_ALLOWED)->getXmlContent(child);
        auto mappings = ConfigDefinition::findConfigSetup<ConfigDictionarySetup>(ConfigVal::A_CLIENTS_UPNP_MAP_MIMETYPE)->getXmlContent(child);
        auto headers = ConfigDefinition::findConfigSetup<ConfigDictionarySetup>(ConfigVal::A_CLIENTS_UPNP_HEADERS)->getXmlContent(child);
        auto profiles = ConfigDefinition::findConfigSetup<ConfigVectorSetup>(ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE)->getXmlContent(child);
        auto matchValues = std::map<ClientMatchType, std::string>();
        for (auto&& attr : child.attributes()) {
            auto matchType = ClientConfig::remapMatchType(attr.name());
            if (matchType != ClientMatchType::None && matchType != ClientMatchType::IP && matchType != ClientMatchType::UserAgent) {
                matchValues[matchType] = attr.value();
            }
        }

        auto client = std::make_shared<ClientConfig>(flags, group, ip, userAgent, matchValues, captionInfoCount, stringLimit, multiValue, isAllowed);
        try {
            if (groupCache.find(group) != groupCache.end())
                client->setGroup(group, groupCache.at(group));
            client->setFullFilter(fullFilter);
            if (!mappings.empty())
                client->setMimeMappings(mappings);
            if (!headers.empty())
                client->setHeaders(headers);
            if (!profiles.empty())
                client->setDlnaMappings(profiles);
            EDIT_CAST(EditHelperClientConfig, result)->add(client);
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

bool ConfigClientSetup::updateItem(const std::vector<std::size_t>& indexList,
    const std::string& optItem,
    const std::shared_ptr<Config>& config,
    std::shared_ptr<ClientConfig>& entry,
    std::string& optValue,
    const std::string& status) const
{
    auto statusList = splitString(status, ',');
    auto i = indexList.at(0);
    if (optItem == getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT }) && (statusList.at(0) == STATUS_ADDED || statusList.at(0) == STATUS_MANUAL)) {
        return true;
    }
    {
        // set client flags
        auto index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_FLAGS });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, ClientConfig::mapFlags(entry->getFlags()));
            entry->setFlags(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_CLIENTS_CLIENT_FLAGS)->checkIntValue(optValue));
            log_debug("New Client Flags {} {}", index, ClientConfig::mapFlags(EDIT_CAST(EditHelperClientConfig, config->getClientConfigListOption(option))->get(i)->getFlags()));
            return true;
        }
        // set client ip
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_IP });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getIp());
            if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_IP)->checkValue(optValue)) {
                entry->setIp(optValue);
                log_debug("New Client IP {} {}", index, EDIT_CAST(EditHelperClientConfig, config->getClientConfigListOption(option))->get(i)->getIp());
                return true;
            }
        }
        // set client group
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_GROUP });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getGroup());
            if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_GROUP)->checkValue(optValue)) {
                entry->setGroup(optValue);
                log_debug("New Client Group {} {}", index, EDIT_CAST(EditHelperClientConfig, config->getClientConfigListOption(option))->get(i)->getGroup());
                return true;
            }
        }
        // set client allowed
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_ALLOWED });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getAllowed());
            entry->setAllowed(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_CLIENTS_CLIENT_ALLOWED)->checkValue(optValue));
            log_debug("New Client Allow {} {}", index, EDIT_CAST(EditHelperClientConfig, config->getClientConfigListOption(option))->get(i)->getAllowed());
            return true;
        }
        // set client userAgent
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_CLIENT_USERAGENT });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getUserAgent());
            if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ConfigVal::A_CLIENTS_CLIENT_USERAGENT)->checkValue(optValue)) {
                entry->setUserAgent(optValue);
                log_debug("New Client UserAgent {} {}", index, EDIT_CAST(EditHelperClientConfig, config->getClientConfigListOption(option))->get(i)->getUserAgent());
                return true;
            }
        }
        // set client upnp caption count
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getCaptionInfoCount());
            entry->setCaptionInfoCount(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_CLIENTS_UPNP_CAPTION_COUNT)->checkIntValue(optValue));
            log_debug("New Client CaptionInfoCount {} {}", index, EDIT_CAST(EditHelperClientConfig, config->getClientConfigListOption(option))->get(i)->getCaptionInfoCount());
            return true;
        }
        // set client string limit
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getStringLimit());
            entry->setStringLimit(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ConfigVal::A_CLIENTS_UPNP_STRING_LIMIT)->checkIntValue(optValue));
            log_debug("New Client StringLimit {} {}", index, EDIT_CAST(EditHelperClientConfig, config->getClientConfigListOption(option))->get(i)->getStringLimit());
            return true;
        }
        // set client upnp multi value
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getMultiValue());
            entry->setMultiValue(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_CLIENTS_UPNP_MULTI_VALUE)->checkValue(optValue));
            log_debug("New Client MultiValue {} {}", index, EDIT_CAST(EditHelperClientConfig, config->getClientConfigListOption(option))->get(i)->getMultiValue());
            return true;
        }
        // set client upnp full filter
        index = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_FILTER_FULL });
        if (optItem == index) {
            if (entry->getOrig())
                config->setOrigValue(index, entry->getFullFilter());
            entry->setFullFilter(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ConfigVal::A_CLIENTS_UPNP_FILTER_FULL)->checkValue(optValue));
            log_debug("New Client FullFilter {} {}", index, EDIT_CAST(EditHelperClientConfig, config->getClientConfigListOption(option))->get(i)->getFullFilter());
            return true;
        }
    }
    // set client dictionaries
    if (statusList.size() > 1) {
        log_debug("New Client Dictionary {} {} ({}, {})", optItem, statusList.at(1), indexList.at(0), indexList.at(1));
        // set client mimetype from
        auto keyIndex = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM });
        auto valIndex = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_MAP, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO });
        auto j = indexList.at(1);
        if (optItem == keyIndex) {
            auto mappings = entry->getMimeMappings(true);
            auto optKey = j < mappings.size() ? (*(std::next(mappings.cbegin(), j))).first : "";
            bool done = true;
            if (statusList.at(1) == STATUS_REMOVED || statusList.at(1) == STATUS_KILLED) {
                config->setOrigValue(optItem, optKey);
                if (!optKey.empty())
                    config->setOrigValue(valIndex, mappings.at(optKey));
                done = false;
            } else if (entry->getOrig())
                config->setOrigValue(keyIndex, optKey);
            entry->setMimeMappingsFrom(j, optValue);
            if (statusList.at(1) == STATUS_RESET && !optValue.empty()) {
                entry->setMimeMappingsFrom(j, config->getOrigValue(valIndex));
                log_debug("Reset Client Mapping From{} {}", valIndex, entry->getMimeMappings().at(optKey));
            }
            if (done)
                log_debug("New Client Mapping From {} {}", keyIndex, (*(std::next(EDIT_CAST(EditHelperClientConfig, config->getClientConfigListOption(option))->get(i)->getMimeMappings().cbegin(), j))).first);
            return true;
        }
        // set client mimetype to
        if (optItem == valIndex) {
            auto mappings = entry->getMimeMappings(true);
            if (entry->getOrig())
                config->setOrigValue(valIndex, (*(std::next(mappings.cbegin(), j))).second);
            entry->setMimeMappingsTo(j, optValue);
            if (statusList.at(1) != STATUS_REMOVED && statusList.at(1) != STATUS_KILLED)
                log_debug("New Client Mapping To {} {}", valIndex, (*(std::next(EDIT_CAST(EditHelperClientConfig, config->getClientConfigListOption(option))->get(i)->getMimeMappings().cbegin(), j))).second);
            return true;
        }

        // set client headers key
        keyIndex = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_KEY });
        valIndex = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_HEADERS_HEADER, ConfigVal::A_CLIENTS_UPNP_HEADERS_VALUE });
        if (optItem == keyIndex) {
            auto headers = entry->getHeaders(true);
            auto optKey = j < headers.size() ? (*(std::next(headers.cbegin(), j))).first : "";
            bool done = true;
            if (statusList.at(1) == STATUS_REMOVED || statusList.at(1) == STATUS_KILLED) {
                config->setOrigValue(optItem, optKey);
                if (!optKey.empty())
                    config->setOrigValue(valIndex, headers.at(optKey));
                done = false;
            } else if (entry->getOrig())
                config->setOrigValue(keyIndex, optKey);
            entry->setHeadersKey(j, optValue);
            if (statusList.at(1) == STATUS_RESET && !optValue.empty()) {
                entry->setHeadersKey(j, config->getOrigValue(valIndex));
                log_debug("Reset Client Mapping From{} {}", valIndex, entry->getHeaders().at(optKey));
            }
            if (done)
                log_debug("New Client Headers Key {} {}", keyIndex, (*(std::next(EDIT_CAST(EditHelperClientConfig, config->getClientConfigListOption(option))->get(i)->getHeaders().cbegin(), j))).first);
            return true;
        }
        // set client headers value
        if (optItem == valIndex) {
            auto headers = entry->getHeaders(true);
            if (entry->getOrig())
                config->setOrigValue(valIndex, (*(std::next(headers.cbegin(), j))).second);
            entry->setHeadersValue(j, optValue);
            if (statusList.at(1) != STATUS_REMOVED && statusList.at(1) != STATUS_KILLED)
                log_debug("New Client Headers Key {} {}", valIndex, (*(std::next(EDIT_CAST(EditHelperClientConfig, config->getClientConfigListOption(option))->get(i)->getHeaders().cbegin(), j))).second);
            return true;
        }

        // set known client dlna values
        keyIndex = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM });
        if (optItem == keyIndex) {
            auto profiles = entry->getDlnaMappings(true).at(j);
            std::size_t k = 0;
            auto tag = ConfigDefinition::removeAttribute(ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_FROM);
            for (auto&& [key, val] : profiles) {
                if (key == tag)
                    break;
                ++k;
            }
            if (status == STATUS_RESET && !optValue.empty()) {
                entry->setDlnaMapping(j, k, config->getOrigValue(keyIndex));
                log_debug("Reset dlna value {} {}", keyIndex, entry->getDlnaMappings(true).at(j).at(k).second);
            } else {
                config->setOrigValue(keyIndex, profiles.at(k).second);
                entry->setDlnaMapping(j, k, optValue);
                log_debug("New dlna value {} {}", keyIndex, optValue);
            }
            return true;
        }
        valIndex = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE, ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO });
        if (optItem == valIndex) {
            auto profiles = entry->getDlnaMappings(true).at(j);
            std::size_t k = 0;
            auto tag = ConfigDefinition::removeAttribute(ConfigVal::A_IMPORT_MAPPINGS_MIMETYPE_TO);
            for (auto&& [key, val] : profiles) {
                if (key == tag)
                    break;
                ++k;
            }
            if (status == STATUS_RESET && !optValue.empty()) {
                entry->setDlnaMapping(j, k, config->getOrigValue(valIndex));
                log_debug("Reset dlna value {} {}", valIndex, entry->getDlnaMappings(true).at(j).at(k).second);
            } else {
                config->setOrigValue(valIndex, profiles.at(k).second);
                entry->setDlnaMapping(j, k, optValue);
                log_debug("New dlna value {} {}", valIndex, optValue);
            }
            return true;
        }
        // set free client dlna values
        {
            for (auto&& attr : ResourceAttributeIterator()) {
                auto tag = EnumMapper::getAttributeDisplay(attr);
                valIndex = getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT, ConfigVal::A_CLIENTS_UPNP_MAP_DLNAPROFILE_PROFILE }, tag);
                if (optItem == valIndex) {
                    auto profiles = entry->getDlnaMappings(true).at(j);
                    std::size_t k = 0;
                    for (auto&& [key, val] : profiles) {
                        if (key == tag)
                            break;
                        ++k;
                    }
                    if (status == STATUS_RESET && !optValue.empty()) {
                        entry->setDlnaMapping(j, k, config->getOrigValue(valIndex));
                        log_debug("Reset dlna value {} {}", valIndex, entry->getDlnaMappings(true).at(j).at(k).second);
                    } else {
                        config->setOrigValue(valIndex, profiles.at(k).second);
                        entry->setDlnaMapping(j, k, optValue);
                        log_debug("New dlna value {} {}", valIndex, optValue);
                    }
                    return true;
                }
            }
        }
    }

    return false;
}

bool ConfigClientSetup::updateItem(const std::vector<std::size_t>& indexList,
    const std::string& optItem,
    const std::shared_ptr<Config>& config,
    std::shared_ptr<ClientGroupConfig>& entry,
    std::string& optValue,
    const std::string& status) const
{
    auto statusList = splitString(status, ',');
    if (optItem == getItemPath(indexList, { ConfigVal::A_CLIENTS_GROUP }) && (statusList.at(0) == STATUS_ADDED || statusList.at(0) == STATUS_MANUAL)) {
        return true;
    }
    {
        // set group name
        auto index = getItemPath(indexList, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_NAME });
        if (optItem == index) {
            log_warning("Group Name cannot be changed {} {}", index, optValue);
            return true;
        }
    }
    {
        // set forbidden directory
        auto keyIndex = getItemPath(indexList, { ConfigVal::A_CLIENTS_GROUP, ConfigVal::A_CLIENTS_GROUP_HIDE, ConfigVal::A_CLIENTS_GROUP_LOCATION });
        if (optItem == keyIndex) {
            auto j = indexList.at(1);
            auto profiles = entry->getForbiddenDirectories(true).at(j);
            auto location = ConfigDefinition::removeAttribute(ConfigVal::A_CLIENTS_GROUP_LOCATION);
            if (status == STATUS_RESET && !optValue.empty()) {
                entry->setForbiddenDirectory(j, config->getOrigValue(keyIndex));
                log_debug("Reset location value {} {}", keyIndex, entry->getForbiddenDirectories(true).at(j));
            } else {
                config->setOrigValue(keyIndex, profiles);
                entry->setForbiddenDirectory(j, optValue);
                log_debug("location dlna value {} {}", keyIndex, optValue);
            }
            return true;
        }
    }
    return false;
}

bool ConfigClientSetup::updateDetail(const std::string& optItem,
    std::string& optValue,
    const std::shared_ptr<Config>& config,
    const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        log_debug("Updating Client Detail {} {} {}", xpath, optItem, optValue);
        auto value = std::dynamic_pointer_cast<ClientConfigListOption>(optionValue);
        auto list = value->getClientConfigListOption();
        auto indexList = extractIndexList(optItem);
        std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";
        auto statusList = splitString(status, ',');

        if (startswith(optItem, getItemPath(indexList, { ConfigVal::A_CLIENTS_CLIENT }))) {
            if (updateConfig<EditHelperClientConfig, ConfigClientSetup, ClientConfigListOption, ClientConfig>(list, config, this, value, optItem, optValue, indexList, status))
                return true;
        }

        if (startswith(optItem, getItemPath(indexList, { ConfigVal::A_CLIENTS_GROUP }))) {
            if (updateConfig<EditHelperClientGroupConfig, ConfigClientSetup, ClientConfigListOption, ClientGroupConfig>(list, config, this, value, optItem, optValue, indexList, status))
                return true;
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

std::string ConfigClientSetup::getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions, const std::string& propText) const
{
    if (indexList.size() == 0) {
        if (!propText.empty() && propOptions.size() > 1) {
            return fmt::format("{}[_]/{}[_]/{}{}",
                ConfigDefinition::mapConfigOption(propOptions.at(0)),
                ConfigDefinition::mapConfigOption(propOptions.at(1)),
                ConfigDefinition::ATTRIBUTE,
                propText);
        }
        if (propOptions.size() > 2) {
            return fmt::format("{}[_]/{}[_]/{}",
                ConfigDefinition::mapConfigOption(propOptions.at(0)),
                ConfigDefinition::mapConfigOption(propOptions.at(1)),
                ConfigDefinition::ensureAttribute(propOptions.at(2)));
        }
        if (propOptions.size() > 1) {
            return fmt::format("{}[_]/{}",
                ConfigDefinition::mapConfigOption(propOptions.at(0)),
                ConfigDefinition::ensureAttribute(propOptions.at(1)));
        }
        return fmt::format("{}[_]", ConfigDefinition::mapConfigOption(propOptions.at(0)));
    }
    if (propOptions.size() > 1) {
        if (!propText.empty() && indexList.size() > 1) {
            return fmt::format("{}[{}]/{}[{}]/{}{}",
                ConfigDefinition::mapConfigOption(propOptions.at(0)),
                indexList.at(0),
                ConfigDefinition::mapConfigOption(propOptions.at(1)),
                indexList.at(1),
                ConfigDefinition::ATTRIBUTE,
                propText);
        }
        if (propOptions.size() > 2 && indexList.size() > 1) {
            return fmt::format("{}[{}]/{}[{}]/{}",
                ConfigDefinition::mapConfigOption(propOptions.at(0)),
                indexList.at(0),
                ConfigDefinition::mapConfigOption(propOptions.at(1)),
                indexList.at(1),
                ConfigDefinition::ensureAttribute(propOptions.at(2)));
        }
        return fmt::format("{}[{}]/{}",
            ConfigDefinition::mapConfigOption(propOptions.at(0)),
            indexList.at(0),
            ConfigDefinition::ensureAttribute(propOptions.at(1)));
    }
    if (propOptions.size() > 0) {
        return fmt::format("{}[{}]",
            ConfigDefinition::mapConfigOption(propOptions.at(0)),
            indexList.at(0));
    }
    return fmt::format("{}[{}]",
        ConfigDefinition::mapConfigOption(ConfigVal::A_CLIENTS_CLIENT),
        indexList.at(0));
}

std::string ConfigClientSetup::getItemPathRoot(bool prefix) const
{
    return ConfigDefinition::mapConfigOption(ConfigVal::A_CLIENTS_CLIENT);
}
