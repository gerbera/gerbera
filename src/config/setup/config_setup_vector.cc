/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_vector.cc - this file is part of Gerbera.
    Copyright (C) 2022-2023 Gerbera Contributors

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

#include "config/config_setup.h" // API

#include <numeric>

#include "config/client_config.h"
#include "config/config_definition.h"
#include "config/config_options.h"

/// \brief Creates a vector from an XML nodeset.
/// \param optValue starting element of the nodeset.
///
/// The basic idea is the following:
/// You have a piece of XML that looks like this
/// <some-section>
///    <map from="1" via="3" to="2"/>
///    <map from="3" to="4"/>
/// </some-section>
///
/// This function will create a vector with the following
/// list: { { "1", "3", "2" }, {"3", "", "4"}
bool ConfigVectorSetup::createOptionFromNode(const pugi::xml_node& optValue, std::vector<std::vector<std::pair<std::string, std::string>>>& result) const
{
    if (optValue) {
        const auto dictNodes = optValue.select_nodes(ConfigDefinition::mapConfigOption(nodeOption));
        std::vector<std::string> attrList;
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
                for (auto& attr : attrList) {
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

bool ConfigVectorSetup::updateItem(std::size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, const std::shared_ptr<VectorOption>& value, const std::string& optValue, const std::string& status) const
{
    auto current = config->getVectorOption(option);
    if (current.size() > i) {
        std::size_t optIndex = 0;
        for (auto&& [key, val] : current.at(i)) {
            auto optionIndex = getItemPath(i, key);
            if (optItem == optionIndex || !status.empty()) {
                if (status == STATUS_RESET && !optValue.empty()) {
                    value->setValue(i, optIndex, config->getOrigValue(optionIndex));
                    log_debug("Reset Vector value {} {}", optionIndex, config->getVectorOption(option)[i][optIndex].second);
                } else {
                    config->setOrigValue(optionIndex, value->getVectorOption()[i][optIndex].second);
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

bool ConfigVectorSetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        auto value = std::dynamic_pointer_cast<VectorOption>(optionValue);
        log_debug("Updating Dictionary Detail {} {} {}", xpath, optItem, optValue);

        std::size_t i = extractIndex(optItem);
        if (i < std::numeric_limits<std::size_t>::max()) {
            if (updateItem(i, optItem, config, value, optValue)) {
                return true;
            }
            std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";
            if (status == STATUS_REMOVED) {
                if (updateItem(i, optItem, config, value, "", status)) {
                    return true;
                }
            }
            if (status == STATUS_RESET) {
                if (updateItem(i, optItem, config, value, optValue, status)) {
                    return true;
                }
            }
            // new entry has parent xpath, value is in other entry
            if (status == STATUS_ADDED || status == STATUS_MANUAL) {
                return true;
            }
        }

        auto editSize = value->getEditSize();
        for (i = 0; i < editSize; ++i) {
            if (updateItem(i, optItem, config, value, optValue)) {
                return true;
            }
        }
    }
    return false;
}

std::string ConfigVectorSetup::getItemPath(int index, config_option_t propOption, config_option_t propOption2, config_option_t propOption3, config_option_t propOption4) const
{
    auto opt = ConfigDefinition::ensureAttribute(propOption);

    if (index > ITEM_PATH_ROOT)
        return fmt::format("{}/{}[{}]/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption), index, opt);
    if (index == ITEM_PATH_ROOT)
        return fmt::format("{}/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption));
    if (index == ITEM_PATH_NEW)
        return fmt::format("{}/{}[_]/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption), opt);
    return fmt::format("{}", xpath);
}

std::string ConfigVectorSetup::getItemPath(int index, const std::string& propOption) const
{
    if (index > ITEM_PATH_ROOT)
        return fmt::format("{}/{}[{}]/{}{}", xpath, ConfigDefinition::mapConfigOption(nodeOption), index, ConfigDefinition::ATTRIBUTE, propOption);
    if (index == ITEM_PATH_ROOT)
        return fmt::format("{}/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption));
    if (index == ITEM_PATH_NEW)
        return fmt::format("{}/{}[_]/{}{}", xpath, ConfigDefinition::mapConfigOption(nodeOption), ConfigDefinition::ATTRIBUTE, propOption);
    return fmt::format("{}", xpath);
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
