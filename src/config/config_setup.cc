/*GRB*

    Gerbera - https://gerbera.io/

    config_setup.cc - this file is part of Gerbera.
    Copyright (C) 2020-2021 Gerbera Contributors

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

/// \file config_setup.cc

#include "config_setup.h" // API

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <numeric>

#include <sys/stat.h>

#include "client_config.h"
#include "config_definition.h"
#include "config_options.h"
#include "config_setup.h"
#include "content/autoscan.h"
#include "directory_tweak.h"
#include "metadata/metadata_handler.h"
#include "transcoding/transcoding.h"

#ifdef HAVE_INOTIFY
#include "util/mt_inotify.h"
#endif

pugi::xml_node ConfigSetup::getXmlElement(const pugi::xml_node& root) const
{
    pugi::xpath_node xpathNode = root.select_node(cpath.c_str());
    if (xpathNode.node() == nullptr) {
        xpathNode = root.select_node(xpath);
    }
    if (required && xpathNode.node() == nullptr) {
        throw_std_runtime_error("Error in config file: {}/{} tag not found", root.path(), xpath);
    }
    return xpathNode.node();
}

bool ConfigSetup::hasXmlElement(const pugi::xml_node& root) const
{
    return root.select_node(cpath.c_str()).node() != nullptr || root.select_node(xpath).node() != nullptr;
}

/// \brief Returns a config option with the given xpath, if option does not exist a default value is returned.
/// \param xpath option xpath
/// \param defaultValue default value if option not found
///
/// The xpath parameter has XPath syntax:
/// "/path/to/option" will return the text value of the given "option" element
/// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
std::string ConfigSetup::getXmlContent(const pugi::xml_node& root, bool trim)
{
    pugi::xpath_node xpathNode = root.select_node(cpath.c_str());

    if (xpathNode.node() != nullptr) {
        std::string optValue = trim ? trimString(xpathNode.node().text().as_string()) : xpathNode.node().text().as_string();
        if (!checkValue(optValue)) {
            throw_std_runtime_error("Invalid {}{} value '{}'", root.path(), xpath, optValue.c_str());
        }
        return optValue;
    }

    if (xpathNode.attribute() != nullptr) {
        std::string optValue = trim ? trimString(xpathNode.attribute().value()) : xpathNode.attribute().value();
        if (!checkValue(optValue)) {
            throw_std_runtime_error("Invalid {}/attribute::{} value '{}'", root.path(), xpath, optValue.c_str());
        }
        return optValue;
    }

    auto xAttr = ConfigDefinition::removeAttribute(option);

    if (root.attribute(xAttr.c_str()) != nullptr) {
        std::string optValue = trim ? trimString(root.attribute(xAttr.c_str()).as_string()) : root.attribute(xAttr.c_str()).as_string();
        if (!checkValue(optValue)) {
            throw_std_runtime_error("Invalid {}/attribute::{} value '{}'", root.path(), xAttr.c_str(), optValue.c_str());
        }
        return optValue;
    }

    if (root.child(xpath) != nullptr) {
        std::string optValue = trim ? trimString(root.child(xpath).text().as_string()) : root.child(xpath).text().as_string();
        if (!checkValue(optValue)) {
            throw_std_runtime_error("Invalid {}/{} value '{}'", root.path(), xpath, optValue.c_str());
        }
        return optValue;
    }

    if (required)
        throw_std_runtime_error("Error in config file: {}/{} not found", root.path(), xpath);

    log_debug("Config: option not found: '{}/{}' using default value: '{}'", root.path(), xpath, defaultValue.c_str());

    useDefault = true;
    return defaultValue;
}

pugi::xpath_node_set ConfigSetup::getXmlTree(const pugi::xml_node& element) const
{
    if (xpath[0] == '/') {
        return element.root().select_nodes(cpath.c_str());
    }
    return element.select_nodes(xpath);
}

void ConfigSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    optionValue = std::make_shared<Option>("");
    setOption(config);
}

void ConfigSetup::makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    optionValue = std::make_shared<Option>(std::move(optValue));
    setOption(config);
}

size_t ConfigSetup::extractIndex(const std::string& item)
{
    size_t i = std::numeric_limits<std::size_t>::max();
    if (item.find_first_of('[') != std::string::npos && item.find_first_of(']', item.find_first_of('[')) != std::string::npos) {
        auto startPos = item.find_first_of('[') + 1;
        auto endPos = item.find_first_of(']', startPos);
        try {
            i = std::stoi(item.substr(startPos, endPos - startPos));
        } catch (const std::invalid_argument& ex) {
            log_error(ex.what());
        }
    }
    return i;
}

void ConfigStringSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    bool trim = true;
    if (arguments != nullptr && arguments->find("trim") != arguments->end()) {
        trim = arguments->at("trim") == "true";
    }
    newOption(getXmlContent(root, trim));
    setOption(config);
}

std::shared_ptr<ConfigOption> ConfigStringSetup::newOption(const std::string& optValue)
{
    if (notEmpty && optValue.empty()) {
        throw_std_runtime_error("Invalid {} empty value '{}'", xpath, optValue.c_str());
    }
    optionValue = std::make_shared<Option>(optValue);
    return optionValue;
}

bool ConfigPathSetup::checkPathValue(std::string& optValue, std::string& pathValue) const
{
    if (rawCheck != nullptr && !rawCheck(optValue)) {
        return false;
    }
    pathValue.assign(resolvePath(optValue).string());
    return !(notEmpty && pathValue.empty());
}

bool ConfigPathSetup::checkAgentPath(std::string& optValue)
{
    fs::path tmp_path;
    if (fs::path(optValue).is_absolute()) {
        std::error_code ec;
        fs::directory_entry dirEnt(optValue, ec);
        if (!isRegularFile(dirEnt, ec) && !dirEnt.is_symlink(ec)) {
            log_error("Error in configuration, transcoding profile: could not find transcoding command \"{}\"", optValue.c_str());
            return false;
        }
        tmp_path = optValue;
    } else {
        tmp_path = findInPath(optValue);
        if (tmp_path.empty()) {
            log_error("Error in configuration, transcoding profile: could not find transcoding command \"{}\" in $PATH", optValue.c_str());
            return false;
        }
    }

    int err = 0;
    if (!isExecutable(tmp_path, &err)) {
        log_error("Error in configuration, transcoding profile: transcoder {} is not executable: {}", optValue.c_str(), std::strerror(err));
        return false;
    }
    return true;
}

/// \brief resolve path against home, an exception is raised if path does not exist on filesystem.
/// \param path path to be resolved
/// \param isFile file or directory
/// \param mustExist file/directory must exist
fs::path ConfigPathSetup::resolvePath(fs::path path) const
{
    if (!resolveEmpty && path.string().empty()) {
        return path;
    }
    if (path.is_absolute() || (Home.is_relative() && path.is_relative()))
        ; // absolute or relative, nothing to resolve
    else if (Home.empty())
        path = "." / path;
    else
        path = Home / path;

    // verify that file/directory is there
    std::error_code ec;
    if (isFile) {
        if (mustExist) {
            fs::directory_entry dirEnt(path, ec);
            if (!isRegularFile(dirEnt, ec) && !dirEnt.is_symlink(ec)) {
                throw_std_runtime_error("File '{}' does not exist", path.string());
            }
        } else {
            fs::directory_entry dirEnt(path.parent_path(), ec);
            if (!dirEnt.is_directory(ec) && !dirEnt.is_symlink(ec)) {
                throw_std_runtime_error("Parent directory '{}' does not exist", path.string());
            }
        }
    } else if (mustExist) {
        fs::directory_entry dirEnt(path, ec);
        if (!dirEnt.is_directory(ec) && !dirEnt.is_symlink(ec)) {
            throw_std_runtime_error("Directory '{}' does not exist", path.string());
        }
    }

    log_debug("resolvePath {} = {}", xpath, path.string());
    return path;
}

void ConfigPathSetup::loadArguments(const std::map<std::string, std::string>* arguments)
{
    if (arguments != nullptr) {
        if (arguments->find("isFile") != arguments->end()) {
            isFile = arguments->at("isFile") == "true";
        }
        if (arguments->find("mustExist") != arguments->end()) {
            mustExist = arguments->at("mustExist") == "true";
        }
        if (arguments->find("notEmpty") != arguments->end()) {
            notEmpty = arguments->at("notEmpty") == "true";
        }
        if (arguments->find("resolveEmpty") != arguments->end()) {
            resolveEmpty = arguments->at("resolveEmpty") == "true";
        }
    }
}

void ConfigPathSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    loadArguments(arguments);
    auto optValue = getXmlContent(root, true);
    newOption(optValue);
    setOption(config);
}

void ConfigPathSetup::makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    loadArguments(arguments);
    newOption(optValue);
    setOption(config);
}

std::shared_ptr<ConfigOption> ConfigPathSetup::newOption(std::string& optValue)
{
    auto pathValue = optValue;
    if (!checkPathValue(optValue, pathValue)) {
        throw_std_runtime_error("Invalid {} resolves to empty value '{}'", xpath, optValue.c_str());
    }
    optionValue = std::make_shared<Option>(pathValue);
    return optionValue;
}

fs::path ConfigPathSetup::Home = "";

void ConfigIntSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlContent(root));
    setOption(config);
}

void ConfigIntSetup::makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (rawCheck != nullptr) {
        if (!rawCheck(optValue)) {
            throw_std_runtime_error("Invalid {} value '{}'", xpath, optValue.c_str());
        }
    } else if (valueCheck != nullptr) {
        if (!valueCheck(std::stoi(optValue))) {
            throw_std_runtime_error("Invalid {} value '{}'", xpath, optValue.c_str());
        }
    } else if (minCheck != nullptr) {
        if (!minCheck(std::stoi(optValue), minValue)) {
            throw_std_runtime_error("Invalid {} value '{}', must be at least {}", xpath, optValue.c_str(), minValue);
        }
    }
    try {
        optionValue = std::make_shared<IntOption>(std::stoi(optValue));
        setOption(config);
    } catch (const std::runtime_error& e) {
        throw_std_runtime_error("Error in config file: {} unsupported int value '{}'", xpath, optValue.c_str());
    }
}

int ConfigIntSetup::checkIntValue(std::string& sVal, const std::string& pathName) const
{
    try {
        if (rawCheck != nullptr) {
            if (!rawCheck(sVal)) {
                throw_std_runtime_error("Invalid {}/{} value '{}'", pathName, xpath, sVal);
            }
        } else if (valueCheck != nullptr) {
            if (!valueCheck(std::stoi(sVal))) {
                throw_std_runtime_error("Invalid {}/{} value {}", pathName, xpath, sVal);
            }
        } else if (minCheck != nullptr) {
            if (!minCheck(std::stoi(sVal), minValue)) {
                throw_std_runtime_error("Invalid {}/{} value '{}', must be at least {}", pathName, xpath, sVal, minValue);
            }
        }
        return std::stoi(sVal);
    } catch (const std::runtime_error& e) {
        throw_std_runtime_error("Error in config file: {}/{} unsupported int value '{}'", pathName, xpath, sVal);
    }
}

int ConfigIntSetup::getXmlContent(const pugi::xml_node& root)
{
    std::string sVal = ConfigSetup::getXmlContent(root, true);
    log_debug("Config: option: '{}/{}' value: '{}'", root.path(), xpath, sVal.c_str());
    return checkIntValue(sVal, root.path());
}

std::shared_ptr<ConfigOption> ConfigIntSetup::newOption(int optValue)
{
    if (valueCheck != nullptr && !valueCheck(optValue)) {
        throw_std_runtime_error("Invalid {} value {}", xpath, optValue);
    }
    if (minCheck != nullptr && !minCheck(optValue, minValue)) {
        throw_std_runtime_error("Invalid {} value {}, must be at least {}", xpath, optValue, minValue);
    }
    optionValue = std::make_shared<IntOption>(optValue);
    return optionValue;
}

bool ConfigIntSetup::CheckSqlLiteSyncValue(std::string& value)
{
    auto temp_int = 0;
    if (value == "off" || value == fmt::to_string(MT_SQLITE_SYNC_OFF))
        temp_int = MT_SQLITE_SYNC_OFF;
    else if (value == "normal" || value == fmt::to_string(MT_SQLITE_SYNC_NORMAL))
        temp_int = MT_SQLITE_SYNC_NORMAL;
    else if (value == "full" || value == fmt::to_string(MT_SQLITE_SYNC_FULL))
        temp_int = MT_SQLITE_SYNC_FULL;
    else
        return false;
    value.assign(fmt::to_string(temp_int));
    return true;
}

bool ConfigIntSetup::CheckProfileNumberValue(std::string& value)
{
    auto temp_int = 0;
    if (value == "source" || value == fmt::to_string(SOURCE))
        temp_int = SOURCE;
    else if (value == "off" || value == fmt::to_string(OFF))
        temp_int = OFF;
    else {
        temp_int = std::stoi(value);
        if (temp_int <= 0)
            return false;
    }
    value.assign(fmt::to_string(temp_int));
    return true;
}

bool ConfigIntSetup::CheckMinValue(int value, int minValue)
{
    return value >= minValue;
}

bool ConfigIntSetup::CheckImageQualityValue(int value)
{
    return !(value < 0 || value > 10);
}

bool ConfigIntSetup::CheckUpnpStringLimitValue(int value)
{
    return !((value != -1) && (value < 4));
}

bool ConfigIntSetup::CheckPortValue(int value)
{
    return !(value < 0 || value > 65535);
}

void ConfigBoolSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlContent(root));
    setOption(config);
}

static bool validateTrueFalse(const std::string& optValue)
{
    return (optValue == "true" || optValue == "false");
}

void ConfigBoolSetup::makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (!validateTrueFalse(optValue) && !validateYesNo(optValue))
        throw_std_runtime_error("Invalid {} value {}", xpath, optValue.c_str());
    optionValue = std::make_shared<BoolOption>(optValue == YES || optValue == "true");
    setOption(config);
}

bool ConfigBoolSetup::getXmlContent(const pugi::xml_node& root)
{
    std::string optValue = ConfigSetup::getXmlContent(root, true);
    return checkValue(optValue, root.path());
}

bool ConfigBoolSetup::checkValue(std::string& optValue, const std::string& pathName) const
{
    if (rawCheck != nullptr) {
        if (!rawCheck(optValue)) {
            throw_std_runtime_error("Invalid {}/{} value '{}'", pathName, xpath, optValue.c_str());
        }
    }

    if (!validateTrueFalse(optValue) && !validateYesNo(optValue))
        throw_std_runtime_error("Invalid {}/{} value {}", pathName, xpath, optValue.c_str());
    return optValue == YES || optValue == "true";
}

std::shared_ptr<ConfigOption> ConfigBoolSetup::newOption(bool optValue)
{
    optionValue = std::make_shared<BoolOption>(optValue);
    return optionValue;
}

bool ConfigBoolSetup::CheckSqlLiteRestoreValue(std::string& value)
{
    bool tmp_bool = true;
    if (value == "restore" || value == YES)
        tmp_bool = true;
    else if (value == "fail" || value == NO)
        tmp_bool = false;
    else
        return false;

    value.assign(tmp_bool ? YES : NO);
    return true;
}

bool ConfigBoolSetup::CheckInotifyValue(std::string& value)
{
    bool temp_bool = false;
    if ((value != "auto") && !validateYesNo(value)) {
        log_error("Error in config file: incorrect parameter for \"<autoscan use-inotify=\" attribute");
        return false;
    }

#ifdef HAVE_INOTIFY
    bool inotify_supported = Inotify::supported();
    temp_bool = (inotify_supported && value == "auto") ? true : temp_bool;
#endif

    if (value == YES) {
#ifdef HAVE_INOTIFY
        if (!inotify_supported) {
            log_error("You specified \"yes\" in \"<autoscan use-inotify=\"\">"
                      " however your system does not have inotify support");
            return false;
        }
        temp_bool = true;
#else
        log_error("You specified \"yes\" in \"<autoscan use-inotify=\"\">"
                  " however this version of Gerbera was compiled without inotify support");
        return false;
#endif
    }

    value.assign(temp_bool ? YES : NO);
    return true;
}

bool ConfigBoolSetup::CheckMarkPlayedValue(std::string& value)
{
    bool tmp_bool = true;
    if (value == "prepend" || value == DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE || value == YES)
        tmp_bool = true;
    else if (value == "append" || value == NO)
        tmp_bool = false;
    else
        return false;
    value.assign(tmp_bool ? YES : NO);
    return true;
}

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
bool ConfigArraySetup::createArrayFromNode(const pugi::xml_node& element, std::vector<std::string>& result) const
{
    if (element != nullptr) {
        for (auto&& it : element.select_nodes(ConfigDefinition::mapConfigOption(nodeOption))) {
            const pugi::xml_node& child = it.node();
            std::string attrValue = attrOption != CFG_MAX ? child.attribute(ConfigDefinition::removeAttribute(attrOption).c_str()).as_string() : child.text().as_string();
            if (itemNotEmpty && attrValue.empty()) {
                throw_std_runtime_error("Invalid array {} value {} empty '{}'", element.path(), xpath, attrValue);
            }
            if (!attrValue.empty())
                result.push_back(attrValue);
        }
    }
    return true;
}

void ConfigArraySetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlContent(getXmlElement(root)));
    setOption(config);
}

bool ConfigArraySetup::updateItem(size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, const std::shared_ptr<ArrayOption>& value, const std::string& optValue, const std::string& status) const
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
        value->setItem(i, optValue);
        return true;
    }
    return false;
}

bool ConfigArraySetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (optItem.substr(0, strlen(xpath)) == xpath && optionValue != nullptr) {
        std::shared_ptr<ArrayOption> value = std::dynamic_pointer_cast<ArrayOption>(optionValue);
        log_debug("Updating Array Detail {} {} {}", xpath, optItem, optValue.c_str());

        size_t i = extractIndex(optItem);
        if (i < std::numeric_limits<std::size_t>::max()) {
            if (updateItem(i, optItem, config, value, optValue)) {
                return true;
            }
            std::string status = arguments != nullptr && arguments->find("status") != arguments->end() ? arguments->at("status") : "";
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
    if (initArray != nullptr) {
        if (!initArray(optValue, result, ConfigDefinition::mapConfigOption(nodeOption))) {
            throw_std_runtime_error("Invalid {} array value '{}'", xpath, optValue);
        }
    } else {
        if (!createArrayFromNode(optValue, result)) {
            throw_std_runtime_error("Invalid {} array value '{}'", xpath, optValue);
        }
    }
    if (result.empty()) {
        log_debug("{} assigning {} default values", xpath, defaultEntries.size());
        useDefault = true;
        result.assign(defaultEntries.begin(), defaultEntries.end());
    }
    if (notEmpty && result.empty()) {
        throw_std_runtime_error("Invalid array {} empty '{}'", xpath, optValue);
    }
    return result;
}

bool ConfigArraySetup::checkArrayValue(const std::string& value, std::vector<std::string>& result) const
{
    for (auto&& attrValue : splitString(value, ',')) {
        attrValue = trimString(attrValue);
        if (itemNotEmpty && attrValue.empty()) {
            return false;
        }
        if (!attrValue.empty())
            result.push_back(attrValue);
    }
    return true;
}

std::shared_ptr<ConfigOption> ConfigArraySetup::newOption(const std::vector<std::string>& optValue)
{
    optionValue = std::make_shared<ArrayOption>(optValue);
    return optionValue;
}

bool ConfigArraySetup::InitPlayedItemsMark(const pugi::xml_node& value, std::vector<std::string>& result, const char* node_name)
{
    if (value != nullptr && !value.empty()) {
        for (auto&& it : value.select_nodes(node_name)) {
            const pugi::xml_node& content = it.node();
            std::string mark_content = content.text().as_string();
            if (mark_content.empty()) {
                log_error("error in configuration, <{}>, empty <{}> parameter", value.name(), node_name);
                return false;
            }

            if ((mark_content != DEFAULT_MARK_PLAYED_CONTENT_VIDEO) && (mark_content != DEFAULT_MARK_PLAYED_CONTENT_AUDIO) && (mark_content != DEFAULT_MARK_PLAYED_CONTENT_IMAGE)) {
                log_error("(error in configuration, <{}>, invalid <{}> parameter! Allowed values are '{}', '{}', '{}')",
                    value.name(), node_name,
                    DEFAULT_MARK_PLAYED_CONTENT_VIDEO, DEFAULT_MARK_PLAYED_CONTENT_AUDIO, DEFAULT_MARK_PLAYED_CONTENT_IMAGE);
                return false;
            }

            result.push_back(mark_content);
        }
    }
    return true;
}

bool ConfigArraySetup::InitItemsPerPage(const pugi::xml_node& value, std::vector<std::string>& result, const char* node_name)
{
    if (value != nullptr && !value.empty()) {
        // create the array from user settings
        for (auto&& it : value.select_nodes(node_name)) {
            const pugi::xml_node& child = it.node();
            int i = child.text().as_int();
            if (i < 1) {
                log_error("Error in config file: incorrect <{}> value for <{}>", node_name, value.name());
                return false;
            }
            result.emplace_back(child.text().as_string());
        }
    }
    return true;
}

/// \brief Creates a dictionary from an XML nodeset.
/// \param element starting element of the nodeset.
/// \param nodeName name of each node in the set
/// \param keyAttr attribute name to be used as a key
/// \param valAttr attribute name to be used as value
///
/// The basic idea is the following:
/// You have a piece of XML that looks like this
/// <some-section>
///    <map from="1" to="2"/>
///    <map from="3" to="4"/>
/// </some-section>
///
/// This function will create a dictionary with the following
/// key:value paris: "1":"2", "3":"4"
bool ConfigDictionarySetup::createDictionaryFromNode(const pugi::xml_node& optValue, std::map<std::string, std::string>& result) const
{
    if (optValue != nullptr) {
        const auto dictNodes = optValue.select_nodes(ConfigDefinition::mapConfigOption(nodeOption));
        auto keyAttr = ConfigDefinition::removeAttribute(keyOption);
        auto valAttr = ConfigDefinition::removeAttribute(valOption);

        for (auto&& it : dictNodes) {
            const pugi::xml_node child = it.node();
            std::string key = child.attribute(keyAttr.c_str()).as_string();
            std::string value = child.attribute(valAttr.c_str()).as_string();
            if (!key.empty() && !value.empty()) {
                if (tolower) {
                    key = toLower(key);
                }
                result[key] = value;
            } else if (itemNotEmpty) {
                return false;
            }
        }
    }
    return true;
}

void ConfigDictionarySetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (arguments != nullptr && arguments->find("tolower") != arguments->end()) {
        tolower = arguments->at("tolower") == "true";
    }
    newOption(getXmlContent(getXmlElement(root)));
    setOption(config);
}

bool ConfigDictionarySetup::updateItem(size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, const std::shared_ptr<DictionaryOption>& value, const std::string& optKey, const std::string& optValue, const std::string& status) const
{
    auto keyIndex = getItemPath(i, keyOption);
    auto valIndex = getItemPath(i, valOption);
    if (optItem == keyIndex || !status.empty()) {
        config->setOrigValue(keyIndex, optKey);
        if (status == STATUS_REMOVED) {
            config->setOrigValue(optItem, optKey);
            config->setOrigValue(valIndex, value->getDictionaryOption()[optKey]);
        }
        value->setKey(i, optValue);
        if (status == STATUS_RESET && !optValue.empty()) {
            value->setValue(i, config->getOrigValue(valIndex));
            log_debug("Reset Dictionary value {} {}", valIndex, config->getDictionaryOption(option)[optKey]);
        }
        log_debug("New Dictionary key {} {}", keyIndex, optValue.c_str());
        return true;
    }
    if (optItem == valIndex) {
        config->setOrigValue(valIndex, value->getDictionaryOption()[optKey]);
        value->setValue(i, optValue);
        log_debug("New Dictionary value {} {}", valIndex, config->getDictionaryOption(option)[optKey]);
        return true;
    }
    return false;
}

bool ConfigDictionarySetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (optItem.substr(0, strlen(xpath)) == xpath && optionValue != nullptr) {
        std::shared_ptr<DictionaryOption> value = std::dynamic_pointer_cast<DictionaryOption>(optionValue);
        log_debug("Updating Dictionary Detail {} {} {}", xpath, optItem, optValue.c_str());

        size_t i = extractIndex(optItem);
        if (i < std::numeric_limits<std::size_t>::max()) {
            if (updateItem(i, optItem, config, value, value->getKey(i), optValue)) {
                return true;
            }
            std::string status = arguments != nullptr && arguments->find("status") != arguments->end() ? arguments->at("status") : "";
            if (status == STATUS_REMOVED) {
                if (updateItem(i, optItem, config, value, value->getKey(i), "", status)) {
                    return true;
                }
            }
            if (status == STATUS_RESET) {
                if (updateItem(i, optItem, config, value, optValue, optValue, status)) {
                    return true;
                }
            }
            // new entry has parent xpath, value is in other entry
            if (status == STATUS_ADDED || status == STATUS_MANUAL) {
                return true;
            }
        }

        i = 0;
        for (auto&& [key, val] : value->getDictionaryOption()) {
            if (updateItem(i, optItem, config, value, key, optValue)) {
                return true;
            }
            i++;
        }
    }
    return false;
}

std::string ConfigDictionarySetup::getItemPath(int index, config_option_t propOption, config_option_t propOption2, config_option_t propOption3, config_option_t propOption4) const
{
    auto opt = ConfigDefinition::ensureAttribute(propOption);

    return index >= 0 //
        ? fmt::format("{}/{}[{}]/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption), index, opt) //
        : fmt::format("{}/{}", xpath, ConfigDefinition::mapConfigOption(nodeOption));
}

std::map<std::string, std::string> ConfigDictionarySetup::getXmlContent(const pugi::xml_node& optValue)
{
    std::map<std::string, std::string> result;
    if (initDict != nullptr) {
        if (!initDict(optValue, result)) {
            throw_std_runtime_error("Init {} dictionary failed '{}'", xpath, optValue);
        }
    } else {
        if (!createDictionaryFromNode(optValue, result) && required) {
            throw_std_runtime_error("Init {} dictionary failed '{}'", xpath, optValue);
        }
    }
    if (result.empty()) {
        log_debug("{} assigning {} default values", xpath, defaultEntries.size());
        useDefault = true;
        for (auto&& entry : defaultEntries) {
            result.insert(entry); // this should be an insert without a loop, but Debian 10 doesn't compile.
        }
    }
    if (notEmpty && result.empty()) {
        throw_std_runtime_error("Invalid dictionary {} empty '{}'", xpath, optValue);
    }
    return result;
}

std::shared_ptr<ConfigOption> ConfigDictionarySetup::newOption(const std::map<std::string, std::string>& optValue)
{
    optionValue = std::make_shared<DictionaryOption>(optValue);
    return optionValue;
}

std::string ConfigAutoscanSetup::getItemPath(int index, config_option_t propOption, config_option_t propOption2, config_option_t propOption3, config_option_t propOption4) const
{
    return index >= 0 //
        ? fmt::format("{}/{}/{}[{}]/{}", xpath, AutoscanDirectory::mapScanmode(scanMode), ConfigDefinition::mapConfigOption(ATTR_AUTOSCAN_DIRECTORY), index, ConfigDefinition::ensureAttribute(propOption)) //
        : index > -2 ? fmt::format("{}/{}/{}", xpath, AutoscanDirectory::mapScanmode(scanMode), ConfigDefinition::mapConfigOption(ATTR_AUTOSCAN_DIRECTORY)) //
                     : fmt::format("{}/{}", xpath, ConfigDefinition::mapConfigOption(ATTR_AUTOSCAN_DIRECTORY));
}

/// \brief Creates an array of AutoscanDirectory objects from a XML nodeset.
/// \param element starting element of the nodeset.
/// \param scanmode add only directories with the specified scanmode to the array
bool ConfigAutoscanSetup::createAutoscanListFromNode(const pugi::xml_node& element, std::shared_ptr<AutoscanList>& result)
{
    if (element == nullptr)
        return true;

    auto&& cs = ConfigDefinition::findConfigSetup<ConfigSetup>(ATTR_AUTOSCAN_DIRECTORY);
    for (auto&& it : cs->getXmlTree(element)) {
        const pugi::xml_node& child = it.node();

        fs::path location;
        try {
            location = ConfigDefinition::findConfigSetup<ConfigPathSetup>(ATTR_AUTOSCAN_DIRECTORY_LOCATION)->getXmlContent(child);
        } catch (const std::runtime_error& e) {
            log_warning("Found an Autoscan directory with invalid location!");
            continue;
        }

        ScanMode mode = ConfigDefinition::findConfigSetup<ConfigEnumSetup<ScanMode>>(ATTR_AUTOSCAN_DIRECTORY_MODE)->getXmlContent(child);

        if (mode != scanMode) {
            continue; // skip scan modes that we are not interested in (content manager needs one mode type per array)
        }

        unsigned int interval = 0;
        if (mode == ScanMode::Timed) {
            interval = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_AUTOSCAN_DIRECTORY_INTERVAL)->getXmlContent(child);
        }

        bool recursive = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_AUTOSCAN_DIRECTORY_RECURSIVE)->getXmlContent(child);
        auto cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES);
        bool hidden = cs->hasXmlElement(child) ? cs->getXmlContent(child) : hiddenFiles;

        auto dir = std::make_shared<AutoscanDirectory>(location, mode, recursive, true, INVALID_SCAN_ID, interval, hidden);
        try {
            result->add(dir);
        } catch (const std::runtime_error& e) {
            log_error("Could not add {}: {}", location.string().c_str(), e.what());
            return false;
        }
    }

    return true;
}

bool ConfigAutoscanSetup::updateItem(size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<AutoscanDirectory>& entry, std::string& optValue, const std::string& status) const
{
    auto index = getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_LOCATION);
    if (optItem == getUniquePath() && status != STATUS_CHANGED) {
        return true;
    }

    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getLocation().string());
        auto pathValue = optValue;
        if (ConfigDefinition::findConfigSetup<ConfigPathSetup>(ATTR_AUTOSCAN_DIRECTORY_LOCATION)->checkPathValue(optValue, pathValue)) {
            entry->setLocation(pathValue);
        }
        log_debug("New Autoscan Detail {} {}", index, config->getAutoscanListOption(option)->get(i)->getLocation().string());
        return true;
    }

    index = getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_MODE);
    if (optItem == index) {
        log_error("Autoscan Mode cannot be changed {} {}", index, AutoscanDirectory::mapScanmode(entry->getScanMode()));
        return true;
    }

    index = getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_INTERVAL);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, fmt::format("{}", entry->getInterval().count()));
        entry->setInterval(std::chrono::seconds(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_AUTOSCAN_DIRECTORY_INTERVAL)->checkIntValue(optValue)));
        log_debug("New Autoscan Detail {} {}", index, config->getAutoscanListOption(option)->get(i)->getInterval().count());
        return true;
    }

    index = getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_RECURSIVE);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getRecursive());
        entry->setRecursive(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_AUTOSCAN_DIRECTORY_RECURSIVE)->checkValue(optValue));
        log_debug("New Autoscan Detail {} {}", index, config->getAutoscanListOption(option)->get(i)->getRecursive());
        return true;
    }

    index = getItemPath(i, ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getHidden());
        entry->setHidden(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES)->checkValue(optValue));
        log_debug("New Autoscan Detail {} {}", index, config->getAutoscanListOption(option)->get(i)->getHidden());
        return true;
    }
    return false;
}

bool ConfigAutoscanSetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    auto uPath = getUniquePath();
    if (optItem.substr(0, uPath.length()) == uPath) {
        log_debug("Updating Autoscan Detail {} {} {}", uPath, optItem, optValue.c_str());
        std::shared_ptr<AutoscanListOption> value = std::dynamic_pointer_cast<AutoscanListOption>(optionValue);

        auto list = value->getAutoscanListOption();
        auto i = extractIndex(optItem);

        if (i < std::numeric_limits<std::size_t>::max()) {
            auto entry = list->get(i, true);
            std::string status = arguments != nullptr && arguments->find("status") != arguments->end() ? arguments->at("status") : "";
            if (entry == nullptr && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
                entry = std::make_shared<AutoscanDirectory>();
                entry->setScanMode(scanMode);
                list->add(entry, i);
            }
            if (entry != nullptr && (status == STATUS_REMOVED || status == STATUS_KILLED)) {
                list->remove(i, true);
                return true;
            }
            if (entry != nullptr && status == STATUS_RESET) {
                list->add(entry, i);
            }
            if (entry != nullptr && updateItem(i, optItem, config, entry, optValue, status)) {
                return true;
            }
        }
        for (i = 0; i < list->getEditSize(); i++) {
            auto entry = list->get(i, true);
            if (updateItem(i, optItem, config, entry, optValue)) {
                return true;
            }
        }
    }
    return false;
}

void ConfigAutoscanSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (arguments != nullptr && arguments->find("hiddenFiles") != arguments->end()) {
        hiddenFiles = arguments->at("hiddenFiles") == "true";
    }
    newOption(getXmlElement(root));
    setOption(config);
}

std::shared_ptr<ConfigOption> ConfigAutoscanSetup::newOption(const pugi::xml_node& optValue)
{
    auto result = std::make_shared<AutoscanList>(nullptr);
    if (!createAutoscanListFromNode(optValue, result)) {
        throw_std_runtime_error("Init {} autoscan failed '{}'", xpath, optValue);
    }
    optionValue = std::make_shared<AutoscanListOption>(result);
    return optionValue;
}

/// \brief Creates an array of TranscodingProfile objects from an XML
/// nodeset.
/// \param element starting element of the nodeset.
bool ConfigTranscodingSetup::createTranscodingProfileListFromNode(const pugi::xml_node& element, std::shared_ptr<TranscodingProfileList>& result)
{
    if (element == nullptr)
        return true;

    const pugi::xml_node& root = element.root();

    // initialize mapping dictionary
    std::map<std::string, std::string> mt_mappings;
    {
        auto cs = ConfigDefinition::findConfigSetup<ConfigDictionarySetup>(ATTR_TRANSCODING_MIMETYPE_PROF_MAP);
        if (cs->hasXmlElement(root)) {
            mt_mappings = cs->getXmlContent(cs->getXmlElement(root));
        }
    }

    auto cs = ConfigDefinition::findConfigSetup<ConfigSetup>(ATTR_TRANSCODING_PROFILES_PROFLE);
    const auto profileNodes = cs->getXmlTree(element);
    if (profileNodes.empty())
        return true;

    bool allow_unused_profiles = !ConfigDefinition::findConfigSetup<ConfigBoolSetup>(CFG_TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED)->getXmlContent(root);
    if (!allow_unused_profiles && mt_mappings.empty()) {
        log_error("error in configuration: transcoding "
                  "profiles exist, but no mimetype to profile mappings specified");
        return false;
    }

    // go through profiles
    for (auto&& it : profileNodes) {
        const pugi::xml_node child = it.node();
        if (!ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED)->getXmlContent(child))
            continue;

        auto prof = std::make_shared<TranscodingProfile>(
            ConfigDefinition::findConfigSetup<ConfigEnumSetup<transcoding_type_t>>(ATTR_TRANSCODING_PROFILES_PROFLE_TYPE)->getXmlContent(child),
            ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_NAME)->getXmlContent(child));
        prof->setTargetMimeType(ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE)->getXmlContent(child));

        pugi::xml_node sub;
        sub = ConfigDefinition::findConfigSetup<ConfigSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_RES)->getXmlElement(child);
        if (sub != nullptr) {
            std::string param = sub.text().as_string();
            if (!param.empty()) {
                if (checkResolution(param))
                    prof->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), param);
            }
        }

        // read 4cc options
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigArraySetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC);
            if (cs->hasXmlElement(child)) {
                sub = cs->getXmlElement(child);
                avi_fourcc_listmode_t fcc_mode = ConfigDefinition::findConfigSetup<ConfigEnumSetup<avi_fourcc_listmode_t>>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE)->getXmlContent(sub);
                if (fcc_mode != FCC_None) {
                    prof->setAVIFourCCList(cs->getXmlContent(sub), fcc_mode);
                }
            }
        }
        // read profile options
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL);
            if (cs->hasXmlElement(child))
                prof->setAcceptURL(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ);
            if (cs->hasXmlElement(child))
                prof->setSampleFreq(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN);
            if (cs->hasXmlElement(child))
                prof->setNumChannels(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG);
            if (cs->hasXmlElement(child))
                prof->setHideOriginalResource(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_THUMB);
            if (cs->hasXmlElement(child))
                prof->setThumbnail(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_FIRST);
            if (cs->hasXmlElement(child))
                prof->setFirstResource(cs->getXmlContent(child));
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG);
            if (cs->hasXmlElement(child))
                prof->setTheora(cs->getXmlContent(child));
        }

        // read agent options
        sub = ConfigDefinition::findConfigSetup<ConfigSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT)->getXmlElement(child);
        prof->setCommand(ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND)->getXmlContent(sub));
        prof->setArguments(ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS)->getXmlContent(sub));

        // set buffer options
        sub = ConfigDefinition::findConfigSetup<ConfigSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER)->getXmlElement(child);
        size_t buffer = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE)->getXmlContent(sub);
        size_t chunk = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK)->getXmlContent(sub);
        size_t fill = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL)->getXmlContent(sub);

        if (chunk > buffer) {
            log_error("Error in configuration: transcoding profile \"{}\" chunk size can not be greater than buffer size", prof->getName().c_str());
            return false;
        }
        if (fill > buffer) {
            log_error("Error in configuration: transcoding profile \"{}\" fill size can not be greater than buffer size", prof->getName().c_str());
            return false;
        }

        prof->setBufferOptions(buffer, chunk, fill);

        bool set = false;
        for (auto&& [key, val] : mt_mappings) {
            if (val == prof->getName()) {
                result->add(key, prof);
                set = true;
            }
        }

        if (!set) {
            log_error("Error in configuration: you specified a mimetype to transcoding profile mapping, but no match for profile \"{}\" exists", prof->getName().c_str());
            if (!allow_unused_profiles) {
                return false;
            }
        }
    }

    // validate profiles
    auto tpl = result->getList();
    for (auto&& [key, val] : mt_mappings) {
        if (tpl.find(key) == tpl.end()) {
            log_error("Error in configuration: you specified a mimetype to transcoding profile mapping, but the profile \"{}\" for mimetype \"{}\" does not exists", val.c_str(), key.c_str());
            if (!ConfigDefinition::findConfigSetup<ConfigBoolSetup>(CFG_TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED)->getXmlContent(root)) {
                return false;
            }
        }
    }
    return true;
}

void ConfigTranscodingSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (arguments != nullptr && arguments->find("isEnabled") != arguments->end()) {
        isEnabled = arguments->at("isEnabled") == "true";
    }
    newOption(getXmlElement(root));
    setOption(config);
}

std::string ConfigTranscodingSetup::getItemPath(int index, config_option_t propOption, config_option_t propOption2, config_option_t propOption3, config_option_t propOption4) const
{
    if (index < 0) {
        return fmt::format("{}", xpath);
    }

    auto opt2 = ConfigDefinition::ensureAttribute(propOption2, propOption3 == CFG_MAX);
    auto opt3 = ConfigDefinition::ensureAttribute(propOption3, propOption4 == CFG_MAX);
    auto opt4 = ConfigDefinition::ensureAttribute(propOption4, propOption4 != CFG_MAX);

    if (propOption == ATTR_TRANSCODING_PROFILES_PROFLE) {
        if (propOption3 == CFG_MAX) {
            return fmt::format("{}[{}]/{}", ConfigDefinition::mapConfigOption(propOption), index, opt2);
        }
        return fmt::format("{}[{}]/{}/{}", ConfigDefinition::mapConfigOption(propOption), index, opt2, opt3);
    }
    if (propOption == ATTR_TRANSCODING_MIMETYPE_PROF_MAP) {
        if (propOption4 == CFG_MAX) {
            return fmt::format("{}/{}[{}]/{}", ConfigDefinition::mapConfigOption(propOption), ConfigDefinition::mapConfigOption(propOption2), index, opt3);
        }
        return fmt::format("{}/{}[{}]/{}/{}", ConfigDefinition::mapConfigOption(propOption), ConfigDefinition::mapConfigOption(propOption2), index, opt3, opt4);
    }
    if (propOption4 == CFG_MAX) {
        return fmt::format("{}/{}/{}[{}]/{}", xpath, ConfigDefinition::mapConfigOption(propOption), ConfigDefinition::mapConfigOption(propOption2), index, opt3);
    }
    return fmt::format("{}/{}/{}[{}]/{}/{}", xpath, ConfigDefinition::mapConfigOption(propOption), ConfigDefinition::mapConfigOption(propOption2), index, opt3, opt4);
}

bool ConfigTranscodingSetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (optItem.substr(0, strlen(xpath)) == xpath) {
        std::shared_ptr<TranscodingProfileListOption> value = std::dynamic_pointer_cast<TranscodingProfileListOption>(optionValue);
        log_debug("Updating Transcoding Detail {} {} {}", xpath, optItem, optValue.c_str());
        std::map<std::string, int> profiles;
        int i = 0;

        // update properties in profile part
        for (auto&& [key, val] : value->getTranscodingProfileListOption()->getList()) {
            for (auto&& [a, name] : *val) {
                profiles[name->getName()] = i;
                auto index = getItemPath(i, ATTR_TRANSCODING_MIMETYPE_PROF_MAP, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE);
                if (optItem == index) {
                    config->setOrigValue(index, key);
                    value->setKey(key, optValue);
                    log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->get(optValue)->begin()->first);
                    return true;
                }
                index = getItemPath(i, ATTR_TRANSCODING_MIMETYPE_PROF_MAP, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE, ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING);
                if (optItem == index) {
                    log_error("Cannot change profile name in Transcoding Detail {} {}", index, val->begin()->first);
                    //value->setKey(key, optValue);
                    //log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->get(optValue)->begin()->first);
                    return false;
                }
                i++;
            }
        }
        i = 0;

        // update properties in transcoding part
        for (auto&& [key, val] : profiles) {
            auto entry = value->getTranscodingProfileListOption()->getByName(key, true);
            auto index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_NAME);
            if (optItem == index) {
                log_error("Cannot change profile name in Transcoding Detail {} {}", index, entry->getName());
                return false;
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED);
            if (optItem == index) {
                config->setOrigValue(index, entry->getEnabled());
                entry->setEnabled(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED)->checkValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getEnabled());
                return true;
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_TYPE);
            if (optItem == index) {
                transcoding_type_t type;
                if (ConfigDefinition::findConfigSetup<ConfigEnumSetup<transcoding_type_t>>(ATTR_TRANSCODING_PROFILES_PROFLE_TYPE)->checkEnumValue(optValue, type)) {
                    config->setOrigValue(index, entry->getType());
                    entry->setType(type);
                    log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getType());
                    return true;
                }
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE);
            if (optItem == index) {
                if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE)->checkValue(optValue)) {
                    config->setOrigValue(index, entry->getTargetMimeType());
                    entry->setTargetMimeType(optValue);
                    log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getTargetMimeType());
                    return true;
                }
            }

            // update profile options
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_RES);
            if (optItem == index) {
                if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_RES)->checkValue(optValue)) {
                    config->setOrigValue(index, entry->getAttributes()[MetadataHandler::getResAttrName(R_RESOLUTION)]);
                    entry->getAttributes()[MetadataHandler::getResAttrName(R_RESOLUTION)] = optValue;
                    log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getAttributes()[MetadataHandler::getResAttrName(R_RESOLUTION)]);
                    return true;
                }
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL);
            if (optItem == index) {
                config->setOrigValue(index, entry->acceptURL());
                entry->setAcceptURL(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL)->checkValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->acceptURL());
                return true;
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ);
            if (optItem == index) {
                config->setOrigValue(index, entry->getSampleFreq());
                entry->setSampleFreq(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ)->checkIntValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getSampleFreq());
                return true;
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN);
            if (optItem == index) {
                config->setOrigValue(index, entry->getNumChannels());
                entry->setNumChannels(ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN)->checkIntValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getNumChannels());
                return true;
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG);
            if (optItem == index) {
                config->setOrigValue(index, entry->hideOriginalResource());
                entry->setHideOriginalResource(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG)->checkValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->hideOriginalResource());
                return true;
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_THUMB);
            if (optItem == index) {
                config->setOrigValue(index, entry->isThumbnail());
                entry->setThumbnail(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_THUMB)->checkValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->isThumbnail());
                return true;
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_FIRST);
            if (optItem == index) {
                config->setOrigValue(index, entry->firstResource());
                entry->setFirstResource(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_FIRST)->checkValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->firstResource());
                return true;
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG);
            if (optItem == index) {
                config->setOrigValue(index, entry->isTheora());
                entry->setTheora(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG)->checkValue(optValue));
                log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->isTheora());
                return true;
            }

            // update buffer options
            size_t buffer = entry->getBufferSize();
            size_t chunk = entry->getBufferChunkSize();
            size_t fill = entry->getBufferInitialFillSize();
            bool setBuffer = false;
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE);
            if (optItem == index) {
                config->setOrigValue(index, int(buffer));
                buffer = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE)->checkIntValue(optValue);
                setBuffer = true;
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK);
            if (optItem == index) {
                config->setOrigValue(index, int(chunk));
                chunk = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK)->checkIntValue(optValue);
                setBuffer = true;
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL);
            if (optItem == index) {
                config->setOrigValue(index, int(fill));
                fill = ConfigDefinition::findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL)->checkIntValue(optValue);
                setBuffer = true;
            }
            if (setBuffer && chunk > buffer) {
                log_error("error in configuration: transcoding profile \""
                    + entry->getName() + "\" chunk size can not be greater than buffer size");
                return false;
            }
            if (setBuffer && fill > buffer) {
                log_error("error in configuration: transcoding profile \""
                    + entry->getName() + "\" fill size can not be greater than buffer size");
                return false;
            }
            if (setBuffer) {
                entry->setBufferOptions(buffer, chunk, fill);
                return true;
            }

            // update agent options
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND);
            if (optItem == index) {
                if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND)->checkValue(optValue)) {
                    config->setOrigValue(index, entry->getCommand().string());
                    entry->setCommand(optValue);
                    log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getCommand().string());
                    return true;
                }
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT, ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS);
            if (optItem == index) {
                if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS)->checkValue(optValue)) {
                    config->setOrigValue(index, entry->getArguments());
                    entry->setArguments(optValue);
                    log_debug("New Transcoding Detail {} {}", index, config->getTranscodingProfileListOption(option)->getByName(entry->getName(), true)->getArguments());
                    return true;
                }
            }

            // update 4cc options
            avi_fourcc_listmode_t fcc_mode = entry->getAVIFourCCListMode();
            auto fcc_list = entry->getAVIFourCCList();
            bool set4cc = false;
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE);
            if (optItem == index) {
                config->setOrigValue(index, TranscodingProfile::mapFourCcMode(fcc_mode));
                if (ConfigDefinition::findConfigSetup<ConfigEnumSetup<avi_fourcc_listmode_t>>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE)->checkEnumValue(optValue, fcc_mode)) {
                    set4cc = true;
                }
            }
            index = getItemPath(i, ATTR_TRANSCODING_PROFILES_PROFLE, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC, ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC);
            if (optItem == index) {
                config->setOrigValue(index, std::accumulate(std::next(fcc_list.begin()), fcc_list.end(), fcc_list[0], [](auto&& a, auto&& b) { return fmt::format("{}, {}", a.c_str(), b.c_str()); }));
                fcc_list.clear();
                if (ConfigDefinition::findConfigSetup<ConfigArraySetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC)->checkArrayValue(optValue, fcc_list)) {
                    set4cc = true;
                }
            }
            if (set4cc) {
                entry->setAVIFourCCList(fcc_list, fcc_mode);
                return true;
            }
            i++;
        }
    }
    return false;
}

std::shared_ptr<ConfigOption> ConfigTranscodingSetup::newOption(const pugi::xml_node& optValue)
{
    auto result = std::make_shared<TranscodingProfileList>();

    if (!createTranscodingProfileListFromNode(isEnabled ? optValue : pugi::xml_node(nullptr), result)) {
        throw_std_runtime_error("Init {} transcoding failed '{}'", xpath, optValue);
    }
    optionValue = std::make_shared<TranscodingProfileListOption>(result);
    return optionValue;
}

/// \brief Creates an array of ClientConfig objects from a XML nodeset.
/// \param element starting element of the nodeset.
bool ConfigClientSetup::createClientConfigListFromNode(const pugi::xml_node& element, std::shared_ptr<ClientConfigList>& result)
{
    if (element == nullptr)
        return true;

    auto&& cs = ConfigDefinition::findConfigSetup<ConfigSetup>(ATTR_CLIENTS_CLIENT);
    for (auto&& it : cs->getXmlTree(element)) {
        const pugi::xml_node& child = it.node();

        auto flags = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_FLAGS)->getXmlContent(child);
        auto ip = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_IP)->getXmlContent(child);
        auto userAgent = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_USERAGENT)->getXmlContent(child);

        std::vector<std::string> flagsVector = splitString(flags, '|', false);
        int flag = std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](auto flg, auto&& i) { return flg | ClientConfig::remapFlag(i); });

        auto client = std::make_shared<ClientConfig>(flag, ip, userAgent);
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
    if (arguments != nullptr && arguments->find("isEnabled") != arguments->end()) {
        isEnabled = arguments->at("isEnabled") == "true";
    }
    newOption(getXmlElement(root));
    setOption(config);
}

bool ConfigClientSetup::updateItem(size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<ClientConfig>& entry, std::string& optValue, const std::string& status) const
{
    if (optItem == getItemPath(i) && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
        return true;
    }
    auto index = getItemPath(i, ATTR_CLIENTS_CLIENT_FLAGS);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, ClientConfig::mapFlags(entry->getFlags()));
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_FLAGS)->checkValue(optValue)) {
            std::vector<std::string> flagsVector = splitString(optValue, '|', false);
            int flag = std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](auto flg, auto&& i) { return flg | ClientConfig::remapFlag(i); });
            entry->setFlags(flag);
            log_debug("New Client Detail {} {}", index, ClientConfig::mapFlags(config->getClientConfigListOption(option)->get(i)->getFlags()));
            return true;
        }
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
    return false;
}

bool ConfigClientSetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (optItem.substr(0, strlen(xpath)) == xpath) {
        log_debug("Updating Client Detail {} {} {}", xpath, optItem, optValue);
        std::shared_ptr<ClientConfigListOption> value = std::dynamic_pointer_cast<ClientConfigListOption>(optionValue);
        auto list = value->getClientConfigListOption();
        auto index = extractIndex(optItem);

        if (index < std::numeric_limits<std::size_t>::max()) {
            auto entry = list->get(index, true);
            std::string status = arguments != nullptr && arguments->find("status") != arguments->end() ? arguments->at("status") : "";

            if (entry == nullptr && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
                entry = std::make_shared<ClientConfig>();
                list->add(entry, index);
            }
            if (entry != nullptr && (status == STATUS_REMOVED || status == STATUS_KILLED)) {
                list->remove(index, true);
                return true;
            }
            if (entry != nullptr && status == STATUS_RESET) {
                list->add(entry, index);
            }
            if (entry != nullptr && updateItem(index, optItem, config, entry, optValue, status)) {
                return true;
            }
        }
        for (size_t client = 0; client < list->size(); client++) {
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

    if (!createClientConfigListFromNode(isEnabled ? optValue : pugi::xml_node(nullptr), result)) {
        throw_std_runtime_error("Init {} client config failed '{}'", xpath, optValue);
    }
    optionValue = std::make_shared<ClientConfigListOption>(result);
    return optionValue;
}

std::string ConfigClientSetup::getItemPath(int index, config_option_t propOption, config_option_t propOption2, config_option_t propOption3, config_option_t propOption4) const
{
    if (index < 0) {
        return fmt::format("{}", ConfigDefinition::mapConfigOption(ATTR_CLIENTS_CLIENT));
    }
    if (propOption != CFG_MAX) {
        return fmt::format("{}[{}]/{}", ConfigDefinition::mapConfigOption(ATTR_CLIENTS_CLIENT), index, ConfigDefinition::ensureAttribute(propOption));
    }
    return fmt::format("{}[{}]", ConfigDefinition::mapConfigOption(ATTR_CLIENTS_CLIENT), index);
}

/// \brief Creates an array of ClientConfig objects from a XML nodeset.
/// \param element starting element of the nodeset.
bool ConfigDirectorySetup::createDirectoryTweakListFromNode(const pugi::xml_node& element, std::shared_ptr<DirectoryConfigList>& result)
{
    if (element == nullptr)
        return true;

    auto&& cs = ConfigDefinition::findConfigSetup<ConfigSetup>(ATTR_DIRECTORIES_TWEAK);
    for (auto&& it : cs->getXmlTree(element)) {
        const pugi::xml_node& child = it.node();
        fs::path location = ConfigDefinition::findConfigSetup<ConfigPathSetup>(ATTR_DIRECTORIES_TWEAK_LOCATION)->getXmlContent(child);

        auto inherit = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_INHERIT)->getXmlContent(child);

        auto dir = std::make_shared<DirectoryTweak>(location, inherit);

        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_RECURSIVE);
            if (cs->hasXmlElement(child)) {
                dir->setRecursive(cs->getXmlContent(child));
            }
            cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_HIDDEN);
            if (cs->hasXmlElement(child)) {
                dir->setHidden(cs->getXmlContent(child));
            }
            cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_CASE_SENSITIVE);
            if (cs->hasXmlElement(child)) {
                dir->setCaseSensitive(cs->getXmlContent(child));
            }
            cs = ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS);
            if (cs->hasXmlElement(child)) {
                dir->setFollowSymlinks(cs->getXmlContent(child));
            }
        }
        {
            auto cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_META_CHARSET);
            if (cs->hasXmlElement(child)) {
                dir->setMetaCharset(cs->getXmlContent(child));
            }
            cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_FANART_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setFanArtFile(cs->getXmlContent(child));
            }
            cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_SUBTILTE_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setSubTitleFile(cs->getXmlContent(child));
            }
            cs = ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_RESOURCE_FILE);
            if (cs->hasXmlElement(child)) {
                dir->setResourceFile(cs->getXmlContent(child));
            }
        }
        try {
            result->add(dir);
        } catch (const std::runtime_error& e) {
            throw_std_runtime_error("Could not add {} directory: {}", location.string(), e.what());
        }
    }

    return true;
}

void ConfigDirectorySetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlElement(root));
    setOption(config);
}

bool ConfigDirectorySetup::updateItem(size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<DirectoryTweak>& entry, std::string& optValue, const std::string& status) const
{
    if (optItem == getItemPath(i) && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
        return true;
    }

    auto index = getItemPath(i, ATTR_DIRECTORIES_TWEAK_LOCATION);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getLocation().string());
        auto pathValue = optValue;
        if (ConfigDefinition::findConfigSetup<ConfigPathSetup>(ATTR_DIRECTORIES_TWEAK_LOCATION)->checkPathValue(optValue, pathValue)) {
            entry->setLocation(pathValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getLocation().string());
            return true;
        }
    }
    index = getItemPath(i, ATTR_DIRECTORIES_TWEAK_INHERIT);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getInherit());
        entry->setInherit(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_INHERIT)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getInherit());
        return true;
    }
    index = getItemPath(i, ATTR_DIRECTORIES_TWEAK_RECURSIVE);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getRecursive());
        entry->setRecursive(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_RECURSIVE)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getRecursive());
        return true;
    }
    index = getItemPath(i, ATTR_DIRECTORIES_TWEAK_HIDDEN);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getHidden());
        entry->setHidden(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_HIDDEN)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getHidden());
        return true;
    }
    index = getItemPath(i, ATTR_DIRECTORIES_TWEAK_CASE_SENSITIVE);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getCaseSensitive());
        entry->setCaseSensitive(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_CASE_SENSITIVE)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getCaseSensitive());
        return true;
    }
    index = getItemPath(i, ATTR_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->getFollowSymlinks());
        entry->setFollowSymlinks(ConfigDefinition::findConfigSetup<ConfigBoolSetup>(ATTR_DIRECTORIES_TWEAK_FOLLOW_SYMLINKS)->checkValue(optValue));
        log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getFollowSymlinks());
        return true;
    }
    index = getItemPath(i, ATTR_DIRECTORIES_TWEAK_META_CHARSET);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasMetaCharset() ? entry->getMetaCharset() : "");
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_META_CHARSET)->checkValue(optValue)) {
            entry->setMetaCharset(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getMetaCharset());
            return true;
        }
    }
    index = getItemPath(i, ATTR_DIRECTORIES_TWEAK_FANART_FILE);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasFanArtFile() ? entry->getFanArtFile() : "");
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_FANART_FILE)->checkValue(optValue)) {
            entry->setFanArtFile(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getFanArtFile());
            return true;
        }
    }
    index = getItemPath(i, ATTR_DIRECTORIES_TWEAK_RESOURCE_FILE);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasResourceFile() ? entry->getResourceFile() : "");
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_RESOURCE_FILE)->checkValue(optValue)) {
            entry->setResourceFile(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getResourceFile());
            return true;
        }
    }
    index = getItemPath(i, ATTR_DIRECTORIES_TWEAK_SUBTILTE_FILE);
    if (optItem == index) {
        if (entry->getOrig())
            config->setOrigValue(index, entry->hasSubTitleFile() ? entry->getSubTitleFile() : "");
        if (ConfigDefinition::findConfigSetup<ConfigStringSetup>(ATTR_DIRECTORIES_TWEAK_SUBTILTE_FILE)->checkValue(optValue)) {
            entry->setSubTitleFile(optValue);
            log_debug("New Tweak Detail {} {}", index, config->getDirectoryTweakOption(option)->get(i)->getSubTitleFile());
            return true;
        }
    }

    return false;
}

bool ConfigDirectorySetup::updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (optItem.substr(0, strlen(xpath)) == xpath) {
        log_debug("Updating Client Detail {} {} {}", xpath, optItem, optValue);
        std::shared_ptr<DirectoryTweakOption> value = std::dynamic_pointer_cast<DirectoryTweakOption>(optionValue);
        auto list = value->getDirectoryTweakOption();
        auto index = extractIndex(optItem);

        if (index < std::numeric_limits<std::size_t>::max()) {
            auto entry = list->get(index, true);
            std::string status = arguments != nullptr && arguments->find("status") != arguments->end() ? arguments->at("status") : "";

            if (entry == nullptr && (status == STATUS_ADDED || status == STATUS_MANUAL)) {
                entry = std::make_shared<DirectoryTweak>();
                list->add(entry, index);
            }
            if (entry != nullptr && (status == STATUS_REMOVED || status == STATUS_KILLED)) {
                list->remove(index, true);
                return true;
            }
            if (entry != nullptr && status == STATUS_RESET) {
                list->add(entry, index);
            }
            if (entry != nullptr && updateItem(index, optItem, config, entry, optValue, status)) {
                return true;
            }
        }
        for (size_t tweak = 0; tweak < list->size(); tweak++) {
            auto entry = value->getDirectoryTweakOption()->get(tweak);
            if (updateItem(tweak, optItem, config, entry, optValue)) {
                return true;
            }
        }
    }
    return false;
}

std::shared_ptr<ConfigOption> ConfigDirectorySetup::newOption(const pugi::xml_node& optValue)
{
    auto result = std::make_shared<DirectoryConfigList>();

    if (!createDirectoryTweakListFromNode(optValue, result)) {
        throw_std_runtime_error("Init {} DirectoryConfigList failed '{}'", xpath, optValue);
    }
    optionValue = std::make_shared<DirectoryTweakOption>(result);
    return optionValue;
}

std::string ConfigDirectorySetup::getItemPath(int index, config_option_t propOption, config_option_t propOption2, config_option_t propOption3, config_option_t propOption4) const
{
    if (index < 0) {
        return fmt::format("{}", ConfigDefinition::mapConfigOption(ATTR_DIRECTORIES_TWEAK));
    }
    if (propOption != CFG_MAX) {
        return fmt::format("{}[{}]/{}", ConfigDefinition::mapConfigOption(ATTR_DIRECTORIES_TWEAK), index, ConfigDefinition::ensureAttribute(propOption));
    }
    return fmt::format("{}[{}]", ConfigDefinition::mapConfigOption(ATTR_DIRECTORIES_TWEAK), index);
}
