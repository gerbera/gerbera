/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_dictionary.cc - this file is part of Gerbera.

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

/// @file config/setup/config_setup_dictionary.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_dictionary.h" // API

#include "config/config_definition.h"
#include "config/config_options.h"
#include "config/config_val.h"
#include "config_setup_bool.h"
#include "exceptions.h"
#include "util/logger.h"
#include "util/tools.h"

#include <numeric>
#include <pugixml.hpp>

/// @brief Creates a dictionary from an XML nodeset.
bool ConfigDictionarySetup::createOptionFromNode(
    const std::shared_ptr<Config>& config,
    const pugi::xml_node& element,
    std::map<std::string, std::string>& result)
{
    if (element) {
        doExtend = definition->findConfigSetup<ConfigBoolSetup>(ConfigVal::A_LIST_EXTEND)->getXmlContent(element, config);
        const auto dictNodes = element.select_nodes(definition->mapConfigOption(nodeOption));
        auto keyAttr = definition->removeAttribute(keyOption);
        auto valAttr = definition->removeAttribute(valOption);
        if (config) {
            config->registerNode(element.path());
        }

        for (auto&& it : dictNodes) {
            auto child = it.node();
            if (config) {
                config->registerNode(child.path());
                config->registerNode(fmt::format("{}/attribute::{}", child.path(), keyAttr));
                config->registerNode(fmt::format("{}/attribute::{}", child.path(), valAttr));
            }
            std::string key = child.attribute(keyAttr.c_str()).as_string();
            std::string value = child.attribute(valAttr.c_str()).as_string();
            if (key.empty()) {
                log_debug("Empty or missing key '{}'", keyAttr);
            }
            if (value.empty()) {
                log_debug("Empty or missing value '{}'", valAttr);
            }
            if (!key.empty() && !value.empty()) {
                if (tolower) {
                    toLowerInPlace(key);
                }
                result[key] = std::move(value);
            } else if (itemNotEmpty) {
                return false;
            }
        }
    }
    return true;
}

void ConfigDictionarySetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (arguments && arguments->find("tolower") != arguments->end()) {
        tolower = arguments->at("tolower") == "true";
    }
    newOption(getXmlContent(getXmlElement(root), config));
    setOption(config);
}

bool ConfigDictionarySetup::createNodeFromDefaults(const std::shared_ptr<pugi::xml_node>& result) const
{
    if (defaultEntries.empty())
        return false;

    auto section = definition->mapConfigOption(nodeOption);
    auto keyAttr = definition->removeAttribute(keyOption);
    auto valAttr = definition->removeAttribute(valOption);
    for (auto&& [key, val] : defaultEntries) {
        auto entry = result->append_child(section);
        entry.append_attribute(keyAttr.c_str()) = key.c_str();
        entry.append_attribute(valAttr.c_str()) = val.c_str();
    }
    return true;
}

bool ConfigDictionarySetup::updateItem(
    const std::vector<std::size_t>& indexList,
    const std::string& optItem,
    const std::shared_ptr<Config>& config,
    const std::shared_ptr<DictionaryOption>& value,
    const std::string& optKey,
    const std::string& optValue,
    const std::string& status) const
{
    auto i = indexList.at(0);
    auto keyIndex = getItemPath(indexList, { keyOption });
    auto valIndex = getItemPath(indexList, { valOption });
    auto configOption = config->getDictionaryOption(option);
    if (optItem == keyIndex || !status.empty()) {
        config->setOrigValue(keyIndex, optKey);
        if (status == STATUS_REMOVED && configOption.find(optKey) != configOption.end()) {
            config->setOrigValue(optItem, optKey);
            config->setOrigValue(valIndex, value->getDictionaryOption().at(optKey));
        }
        value->setKey(i, optValue);
        if (status == STATUS_RESET && !optValue.empty()) {
            value->setValue(i, config->getOrigValue(valIndex));
            log_debug("Reset Dictionary {} Value '{}'", valIndex, configOption.at(optKey));
        }
        log_debug("New Dictionary {} Key '{}'", keyIndex, optValue);
        return true;
    }
    if (optItem == valIndex) {
        if (status != STATUS_REMOVED && status != STATUS_KILLED && configOption.find(optKey) != configOption.end()) {
            config->setOrigValue(valIndex, value->getDictionaryOption().at(optKey));
            value->setValue(i, optValue);
            log_debug("New Dictionary {} Value '{}'", valIndex, config->getDictionaryOption(option).at(optKey));
        } else {
            value->setKey(i, ""); // key should already be removed
        }
        return true;
    }
    return false;
}

bool ConfigDictionarySetup::updateDetail(
    const std::string& optItem,
    std::string& optValue,
    const std::shared_ptr<Config>& config,
    const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        auto value = std::dynamic_pointer_cast<DictionaryOption>(optionValue);
        log_debug("Updating Dictionary Detail {} {} {}", xpath, optItem, optValue);

        auto indexList = extractIndexList(optItem);
        if (!indexList.empty()) {
            auto i = indexList.at(0);
            if (updateItem(indexList, optItem, config, value, value->getKey(i), optValue)) {
                return true;
            }
            std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";
            if (status == STATUS_REMOVED) {
                if (updateItem(indexList, optItem, config, value, value->getKey(i), "", status)) {
                    return true;
                }
            }
            if (status == STATUS_RESET) {
                if (updateItem(indexList, optItem, config, value, optValue, optValue, status)) {
                    return true;
                }
            }
            // new entry has parent xpath, value is in other entry
            if (status == STATUS_ADDED || status == STATUS_MANUAL) {
                return true;
            }
        } else {
            indexList.push_back(0);
        }

        std::size_t i = 0;
        for (auto&& [key, val] : value->getDictionaryOption()) {
            indexList[0] = i;
            if (updateItem(indexList, optItem, config, value, key, optValue)) {
                return true;
            }
            i++;
        }
    }
    return false;
}

std::string ConfigDictionarySetup::getItemPath(
    const std::vector<std::size_t>& indexList,
    const std::vector<ConfigVal>& propOptions,
    const std::string& propText) const
{
    auto opt = !propOptions.empty() ? definition->ensureAttribute(propOptions[0]) : "";
    if (indexList.empty()) {
        return fmt::format("{}/{}[_]/{}", xpath, definition->mapConfigOption(nodeOption), opt);
    }

    return fmt::format("{}/{}[{}]/{}", xpath, definition->mapConfigOption(nodeOption), indexList[0], opt);
}

std::string ConfigDictionarySetup::getItemPathRoot(bool prefix) const
{
    if (prefix)
        return xpath;
    return fmt::format("{}/{}", xpath, definition->mapConfigOption(nodeOption));
}

std::string ConfigDictionarySetup::getUniquePath() const
{
    if (!xpath)
        return fmt::format("{}", definition->mapConfigOption(nodeOption));
    return fmt::format("{}/{}", xpath, definition->mapConfigOption(nodeOption));
}

std::map<std::string, std::string> ConfigDictionarySetup::getXmlContent(
    const pugi::xml_node& optValue,
    const std::shared_ptr<Config>& config)
{
    std::map<std::string, std::string> result;
    if (initDict) {
        if (!initDict(optValue, result)) {
            throw_std_runtime_error("Init {} dictionary failed '{}'", xpath, optValue.name());
        }
    } else {
        if (!createOptionFromNode(config, optValue, result) && required) {
            throw_std_runtime_error("Init {} dictionary failed '{}'", xpath, optValue.name());
        }
    }
    if (result.empty()) {
        log_debug("{} assigning {} default values", xpath, defaultEntries.size());
        useDefault = true;
        result = defaultEntries;
    } else if (doExtend) {
        log_debug("{} extending by {} default values", xpath, defaultEntries.size());
        result.merge(defaultEntries);
    }
    if (notEmpty && result.empty()) {
        throw_std_runtime_error("Invalid dictionary {} empty '{}'", xpath, optValue.name());
    }
    return result;
}

std::string ConfigDictionarySetup::getCurrentValue() const
{
    auto dict = optionValue->getDictionaryOption();
    auto list = std::vector<std::string>();
    list.reserve(dict.size());
    for (auto&& [k, v] : dict)
        list.push_back(fmt::format("{}={}", k, v));
    return fmt::format("[{}]", fmt::join(list, ", "));
}

std::shared_ptr<ConfigOption> ConfigDictionarySetup::newOption(const std::map<std::string, std::string>& optValue)
{
    optionValue = std::make_shared<DictionaryOption>(optValue);
    return optionValue;
}
