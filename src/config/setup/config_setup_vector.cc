/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_vector.cc - this file is part of Gerbera.
    Copyright (C) 2022-2024 Gerbera Contributors

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

/// \file config_setup_vector.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_vector.h" // API

#include "config/config_definition.h"
#include "config/config_options.h"
#include "config/config_val.h"
#include "util/logger.h"

#include <numeric>

/// \brief Creates a vector from an XML nodeset.
bool ConfigVectorSetup::createOptionFromNode(const pugi::xml_node& element, std::vector<std::vector<std::pair<std::string, std::string>>>& result) const
{
    if (element) {
        const auto dictNodes = element.select_nodes(ConfigDefinition::mapConfigOption(nodeOption));
        std::vector<std::string> attrList;
        attrList.reserve(optionList.size());
        for (auto& opt : optionList) {
            attrList.push_back(ConfigDefinition::removeAttribute(opt));
        }

        for (auto&& it : dictNodes) {
            const pugi::xml_node child = it.node();
            std::vector<std::pair<std::string, std::string>> valueList;
            for (auto& attr : child.attributes()) {
                std::string name = attr.name();
                std::string value = attr.value();
                if (!value.empty()) {
                    if (tolower) {
                        toLowerInPlace(value);
                    }
                    valueList.emplace_back(std::move(name), std::move(value));
                } else if (itemNotEmpty) {
                    return false;
                }
            }
            if (itemNotEmpty) {
                for (const auto& attr : attrList) {
                    std::string value = child.attribute(attr.c_str()).as_string();
                    if (value.empty()) {
                        return false;
                    }
                }
            }
            result.push_back(std::move(valueList));
        }
    }
    return true;
}

void ConfigVectorSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (arguments && arguments->find("tolower") != arguments->end()) {
        tolower = arguments->at("tolower") == "true";
    }
    newOption(getXmlContent(getXmlElement(root)));
    setOption(config);
}

bool ConfigVectorSetup::updateItem(const std::vector<std::size_t>& indexList,
    const std::string& optItem,
    const std::shared_ptr<Config>& config,
    const std::shared_ptr<VectorOption>& value,
    const std::string& optValue,
    const std::string& status) const
{
    auto current = config->getVectorOption(option);
    auto i = indexList.at(0);
    if (current.size() > i) {
        std::size_t optIndex = 0;
        for (auto&& [key, val] : current.at(i)) {
            auto optionIndex = getItemPath(indexList, {}, key);
            if (optItem == optionIndex || !status.empty()) {
                if (status == STATUS_RESET && !optValue.empty()) {
                    value->setValue(i, optIndex, config->getOrigValue(optionIndex));
                    log_debug("Reset Vector value {} {}", optionIndex, config->getVectorOption(option).at(i).at(optIndex).second);
                } else {
                    config->setOrigValue(optionIndex, value->getVectorOption().at(i).at(optIndex).second);
                    value->setValue(i, optIndex, optValue);
                    log_debug("New Vector value {} {}", optionIndex, optValue);
                }
                return true;
            }
            ++optIndex;
        }
    }

    return false;
}

bool ConfigVectorSetup::updateDetail(const std::string& optItem,
    std::string& optValue,
    const std::shared_ptr<Config>& config,
    const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        auto value = std::dynamic_pointer_cast<VectorOption>(optionValue);
        log_debug("Updating Dictionary Detail {} {} {}", xpath, optItem, optValue);

        auto indexList = extractIndexList(optItem);
        if (indexList.size() > 0) {
            if (updateItem(indexList, optItem, config, value, optValue)) {
                return true;
            }
            std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";
            if (status == STATUS_REMOVED) {
                if (updateItem(indexList, optItem, config, value, "", status)) {
                    return true;
                }
            }
            if (status == STATUS_RESET) {
                if (updateItem(indexList, optItem, config, value, optValue, status)) {
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
        auto editSize = value->getEditSize();
        for (std::size_t i = 0; i < editSize; ++i) {
            indexList[0] = i;
            if (updateItem(indexList, optItem, config, value, optValue)) {
                return true;
            }
        }
    }
    return false;
}

std::string ConfigVectorSetup::getItemPath(const std::vector<std::size_t>& indexList, const std::vector<ConfigVal>& propOptions, const std::string& propText) const
{
    if (!propText.empty()) {
        if (indexList.size() == 0)
            return fmt::format("{}/{}[_]/{}{}", xpath, ConfigDefinition::mapConfigOption(nodeOption), ConfigDefinition::ATTRIBUTE, propText);
        return fmt::format("{}/{}[{}]/{}{}", xpath, ConfigDefinition::mapConfigOption(nodeOption), indexList.at(0), ConfigDefinition::ATTRIBUTE, propText);
    }
    auto opt = ConfigDefinition::ensureAttribute(propOptions.size() > 0 ? propOptions.at(0) : ConfigVal::MAX);
    if (indexList.size() == 0)
        return fmt::format("{}/{}[_]/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption), opt);

    return fmt::format("{}/{}[{}]/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption), indexList.at(0), opt);
}

std::string ConfigVectorSetup::getItemPathRoot(bool prefix) const
{
    if (prefix)
        return fmt::format("{}", xpath);
    return fmt::format("{}/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption));
}

std::vector<std::vector<std::pair<std::string, std::string>>> ConfigVectorSetup::getXmlContent(const pugi::xml_node& optValue)
{
    std::vector<std::vector<std::pair<std::string, std::string>>> result;
    if (!createOptionFromNode(optValue, result) && required) {
        throw_std_runtime_error("Init {} vector failed '{}'", xpath, optValue.name());
    }
    if (result.empty()) {
        log_debug("{} assigning {} default values", xpath, defaultEntries.size());
        useDefault = true;
        result = defaultEntries;
    }
    if (notEmpty && result.empty()) {
        throw_std_runtime_error("Invalid vector {} empty '{}'", xpath, optValue.name());
    }
    return result;
}

std::shared_ptr<ConfigOption> ConfigVectorSetup::newOption(const std::vector<std::vector<std::pair<std::string, std::string>>>& optValue)
{
    optionValue = std::make_shared<VectorOption>(optValue);
    return optionValue;
}
