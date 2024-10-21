/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_dictionary.cc - this file is part of Gerbera.
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

/// \file config_setup_dictionary.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_dictionary.h" // API

#include "config/config_definition.h"
#include "config/config_options.h"
#include "config/config_val.h"
#include "util/logger.h"

#include <numeric>

/// \brief Creates a dictionary from an XML nodeset.
bool ConfigDictionarySetup::createOptionFromNode(const pugi::xml_node& element, std::map<std::string, std::string>& result) const
{
    if (element) {
        const auto dictNodes = element.select_nodes(ConfigDefinition::mapConfigOption(nodeOption));
        auto keyAttr = ConfigDefinition::removeAttribute(keyOption);
        auto valAttr = ConfigDefinition::removeAttribute(valOption);

        for (auto&& it : dictNodes) {
            const pugi::xml_node child = it.node();
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
    newOption(getXmlContent(getXmlElement(root)));
    setOption(config);
}

bool ConfigDictionarySetup::updateItem(const std::vector<std::size_t>& indexList,
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

bool ConfigDictionarySetup::updateDetail(const std::string& optItem,
    std::string& optValue,
    const std::shared_ptr<Config>& config,
    const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        auto value = std::dynamic_pointer_cast<DictionaryOption>(optionValue);
        log_debug("Updating Dictionary Detail {} {} {}", xpath, optItem, optValue);

        auto indexList = extractIndexList(optItem);
        if (indexList.size() > 0) {
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

std::string ConfigDictionarySetup::getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions, const std::string& propText) const
{
    auto opt = propOptions.size() > 0 ? ConfigDefinition::ensureAttribute(propOptions[0]) : "";
    if (indexList.size() == 0) {
        return fmt::format("{}/{}[_]/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption), opt);
    }

    return fmt::format("{}/{}[{}]/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption), indexList[0], opt);
}

std::string ConfigDictionarySetup::getItemPathRoot(bool prefix) const
{
    if (prefix)
        return xpath;
    return fmt::format("{}/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption));
}

std::string ConfigDictionarySetup::getUniquePath() const
{
    if (!xpath)
        return fmt::format("{}", ConfigDefinition::mapConfigOption(nodeOption));
    return fmt::format("{}/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption));
}

std::map<std::string, std::string> ConfigDictionarySetup::getXmlContent(const pugi::xml_node& optValue)
{
    std::map<std::string, std::string> result;
    if (initDict) {
        if (!initDict(optValue, result)) {
            throw_std_runtime_error("Init {} dictionary failed '{}'", xpath, optValue.name());
        }
    } else {
        if (!createOptionFromNode(optValue, result) && required) {
            throw_std_runtime_error("Init {} dictionary failed '{}'", xpath, optValue.name());
        }
    }
    if (result.empty()) {
        log_debug("{} assigning {} default values", xpath, defaultEntries.size());
        useDefault = true;
        result = defaultEntries;
    }
    if (notEmpty && result.empty()) {
        throw_std_runtime_error("Invalid dictionary {} empty '{}'", xpath, optValue.name());
    }
    return result;
}

std::shared_ptr<ConfigOption> ConfigDictionarySetup::newOption(const std::map<std::string, std::string>& optValue)
{
    optionValue = std::make_shared<DictionaryOption>(optValue);
    return optionValue;
}
