/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_array.cc - this file is part of Gerbera.
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

/// \file config_setup_array.cc

#include "config/config_setup.h" // API

#include <numeric>

#include "config/client_config.h"
#include "config/config_definition.h"
#include "config/config_options.h"

/// \brief Creates an array of strings from an XML nodeset.
/// \param element starting element of the nodeset.
/// \param nodeOption name of each node in the set
/// \param attrOption name of the attribute, the value of which shouldbe extracted
///
/// Similar to \fn createDictionaryFromNode() this one extracts
/// data from the following XML:
/// <some-section>
///     <tag attr="data"/>
///     <tag attr="otherdata"/>
/// <some-section>
///
/// This function will create an array like that: ["data", "otherdata"]
bool ConfigArraySetup::createOptionFromNode(const pugi::xml_node& element, std::vector<std::string>& result) const
{
    if (element) {
        for (auto&& it : element.select_nodes(ConfigDefinition::mapConfigOption(nodeOption))) {
            const pugi::xml_node& child = it.node();
            std::string attrValue = attrOption != CFG_MAX ? child.attribute(ConfigDefinition::removeAttribute(attrOption).c_str()).as_string() : child.text().as_string();
            if (itemCheck) {
                if (!itemCheck(attrValue))
                    throw_std_runtime_error("Invalid array {} value {} empty '{}'", element.path(), xpath, attrValue);
            } else if (itemNotEmpty && attrValue.empty()) {
                throw_std_runtime_error("Invalid array {} value {} empty '{}'", element.path(), xpath, attrValue);
            }
            if (!attrValue.empty())
                result.push_back(std::move(attrValue));
        }
    }
    return true;
}

void ConfigArraySetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlContent(getXmlElement(root)));
    setOption(config);
}

bool ConfigArraySetup::updateItem(std::size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, const std::shared_ptr<ArrayOption>& value, const std::string& optValue, const std::string& status) const
{
    auto index = getItemPath(i);
    if (optItem == index || !status.empty()) {
        auto realIndex = value->getIndex(i);
        if (realIndex < std::numeric_limits<std::size_t>::max()) {
            auto&& array = value->getArrayOption();
            config->setOrigValue(index, array.size() > realIndex ? array[realIndex] : "");
            if (status == STATUS_REMOVED) {
                config->setOrigValue(optItem, array.size() > realIndex ? array[realIndex] : "");
            }
        }
        if (itemCheck && !itemCheck(optValue))
            return false;

        value->setItem(i, optValue);
        return true;
    }
    return false;
}

bool ConfigArraySetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (startswith(optItem, xpath) && optionValue) {
        auto value = std::dynamic_pointer_cast<ArrayOption>(optionValue);
        log_debug("Updating Array Detail {} {} {}", xpath, optItem, optValue);

        std::size_t i = extractIndex(optItem);
        if (i < std::numeric_limits<std::size_t>::max()) {
            if (updateItem(i, optItem, config, value, optValue)) {
                return true;
            }
            std::string status = arguments && arguments->find("status") != arguments->end() ? arguments->at("status") : "";
            if (status == STATUS_REMOVED && updateItem(i, optItem, config, value, "", status)) {
                return true;
            }
            if (status == STATUS_RESET && updateItem(i, optItem, config, value, config->getOrigValue(optItem), status)) {
                return true;
            }
            // new entry has parent xpath, value is in other entry
            if (status == STATUS_ADDED || status == STATUS_MANUAL) {
                return true;
            }
        }

        auto editSize = value->getEditSize();
        for (i = 0; i < editSize; i++) {
            if (updateItem(i, optItem, config, value, optValue)) {
                return true;
            }
        }
    }
    return false;
}

std::string ConfigArraySetup::getItemPath(int index, config_option_t propOption, config_option_t propOption2, config_option_t propOption3, config_option_t propOption4) const
{
    if (index < 0) {
        return fmt::format("{}/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption));
    }
    return attrOption != CFG_MAX ? fmt::format("{}/{}[{}]/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption), index, ConfigDefinition::ensureAttribute(attrOption)) : fmt::format("{}/{}[{}]", xpath, ConfigDefinition::mapConfigOption(nodeOption), index);
}

std::vector<std::string> ConfigArraySetup::getXmlContent(const pugi::xml_node& optValue)
{
    std::vector<std::string> result;
    if (initArray) {
        if (!initArray(optValue, result, ConfigDefinition::mapConfigOption(nodeOption))) {
            throw_std_runtime_error("Invalid {} array value '{}'", xpath, optValue.name());
        }
    } else {
        if (!createOptionFromNode(optValue, result)) {
            throw_std_runtime_error("Invalid {} array value '{}'", xpath, optValue.name());
        }
    }
    if (result.empty()) {
        log_debug("{} assigning {} default values", xpath, defaultEntries.size());
        useDefault = true;
        result = defaultEntries;
    }
    if (notEmpty && result.empty()) {
        throw_std_runtime_error("Invalid array {} empty '{}'", xpath, optValue.name());
    }
    return result;
}

bool ConfigArraySetup::checkArrayValue(const std::string& value, std::vector<std::string>& result) const
{
    for (auto&& attrValue : splitString(value, ',')) {
        trimStringInPlace(attrValue);
        if (itemCheck) {
            if (!itemCheck(attrValue))
                return false;
        } else if (itemNotEmpty && attrValue.empty()) {
            return false;
        }
        if (!attrValue.empty())
            result.push_back(std::move(attrValue));
    }
    return true;
}

std::shared_ptr<ConfigOption> ConfigArraySetup::newOption(const std::vector<std::string>& optValue)
{
    optionValue = std::make_shared<ArrayOption>(optValue);
    return optionValue;
}

bool ConfigArraySetup::InitPlayedItemsMark(const pugi::xml_node& value, std::vector<std::string>& result, const char* nodeName)
{
    if (value && !value.empty()) {
        for (auto&& it : value.select_nodes(nodeName)) {
            const pugi::xml_node& content = it.node();
            std::string markContent = content.text().as_string();
            if (markContent.empty()) {
                log_error("error in configuration, <{}>, empty <{}> parameter", value.name(), nodeName);
                return false;
            }

            if ((markContent != DEFAULT_MARK_PLAYED_CONTENT_VIDEO) && (markContent != DEFAULT_MARK_PLAYED_CONTENT_AUDIO) && (markContent != DEFAULT_MARK_PLAYED_CONTENT_IMAGE)) {
                log_error("(error in configuration, <{}>, invalid <{}> parameter! Allowed values are '{}', '{}', '{}')",
                    value.name(), nodeName,
                    DEFAULT_MARK_PLAYED_CONTENT_VIDEO, DEFAULT_MARK_PLAYED_CONTENT_AUDIO, DEFAULT_MARK_PLAYED_CONTENT_IMAGE);
                return false;
            }

            result.push_back(std::move(markContent));
        }
    }
    return true;
}

bool ConfigArraySetup::InitItemsPerPage(const pugi::xml_node& value, std::vector<std::string>& result, const char* nodeName)
{
    if (value && !value.empty()) {
        // create the array from user settings
        for (auto&& it : value.select_nodes(nodeName)) {
            const pugi::xml_node& child = it.node();
            int i = child.text().as_int();
            if (i < 1) {
                log_error("Error in config file: incorrect <{}> value for <{}>", nodeName, value.name());
                return false;
            }

            auto str = std::string(child.text().as_string());
            result.push_back(std::move(str));
        }
    }
    return true;
}
