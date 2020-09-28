/*GRB*

    Gerbera - https://gerbera.io/

    config_setup.cc - this file is part of Gerbera.
    Copyright (C) 2020 Gerbera Contributors

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
#include <sys/types.h>

#include "autoscan.h"
#include "client_config.h"
#include "config_manager.h"
#include "config_options.h"
#include "config_setup.h"
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
        throw std::runtime_error(fmt::format("Error in config file: {}/{} tag not found", root.path(), xpath));
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
std::string ConfigSetup::getXmlContent(const pugi::xml_node& root, bool trim) const
{
    pugi::xpath_node xpathNode = root.select_node(cpath.c_str());

    if (xpathNode.node() != nullptr) {
        std::string optValue = trim ? trim_string(xpathNode.node().text().as_string()) : xpathNode.node().text().as_string();
        if (rawCheck != nullptr && !rawCheck(optValue)) {
            throw std::runtime_error(fmt::format("Invalid {}{} value '{}'", root.path(), xpath, optValue));
        }
        return optValue;
    }

    if (xpathNode.attribute() != nullptr) {
        std::string optValue = trim ? trim_string(xpathNode.attribute().value()) : xpathNode.attribute().value();
        if (rawCheck != nullptr && !rawCheck(optValue)) {
            throw std::runtime_error(fmt::format("Invalid {}/attribute::{} value '{}'", root.path(), xpath, optValue));
        }
        return optValue;
    }

    if (root.attribute(xpath) != nullptr) {
        std::string optValue = trim ? trim_string(root.attribute(xpath).as_string()) : root.attribute(xpath).as_string();
        if (rawCheck != nullptr && !rawCheck(optValue)) {
            throw std::runtime_error(fmt::format("Invalid {}/attribute::{} value '{}'", root.path(), xpath, optValue));
        }
        return optValue;
    }

    if (root.child(xpath) != nullptr) {
        std::string optValue = trim ? trim_string(root.child(xpath).text().as_string()) : root.child(xpath).text().as_string();
        if (rawCheck != nullptr && !rawCheck(optValue)) {
            throw std::runtime_error(fmt::format("Invalid {}/{} value '{}'", root.path(), xpath, optValue));
        }
        return optValue;
    }

    if (required)
        throw std::runtime_error(fmt::format("Error in config file: {}/{} not found", root.path(), xpath));

    log_debug("Config: option not found: '{}/{}' using default value: '{}'",
        root.path(), xpath, defaultValue.c_str());

    return defaultValue;
}

void ConfigSetup::makeOption(const pugi::xml_node& root, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    optionValue = std::make_shared<Option>("");
    setOption(config);
}

void ConfigSetup::makeOption(std::string optValue, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    optionValue = std::make_shared<Option>(optValue);
    setOption(config);
};

const char* ConfigSetup::ROOT_NAME = "config";

void ConfigStringSetup::makeOption(const pugi::xml_node& root, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    bool trim = true;
    if (arguments != nullptr && arguments->find("trim") != arguments->end()) {
        trim = arguments->find("trim")->second == "true";
    }
    newOption(getXmlContent(root, trim));
    setOption(config);
}

std::shared_ptr<ConfigOption> ConfigStringSetup::newOption(const std::string& optValue)
{
    if (notEmpty && optValue.empty()) {
        throw std::runtime_error(fmt::format("Invalid {} empty value '{}'", xpath, optValue));
    }
    optionValue = std::make_shared<Option>(optValue);
    return optionValue;
}

/// \brief resolve path against home, an exception is raised if path does not exist on filesystem.
/// \param path path to be resolved
/// \param isFile file or directory
/// \param mustExist file/directory must exist
fs::path ConfigPathSetup::resolvePath(fs::path path)
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
            if (!isRegularFile(path, ec) && !fs::is_symlink(path, ec))
                throw std::runtime_error("File '" + path.string() + "' does not exist");
        } else {
            std::string parent_path = path.parent_path();
            if (!fs::is_directory(parent_path, ec) && !fs::is_symlink(path, ec))
                throw std::runtime_error("Parent directory '" + path.string() + "' does not exist");
        }
    } else if (mustExist) {
        if (!fs::is_directory(path, ec) && !fs::is_symlink(path, ec))
            throw std::runtime_error("Directory '" + path.string() + "' does not exist");
    }

    log_debug("resolvePath {} = {}", xpath, path.string());
    return path;
}

void ConfigPathSetup::loadArguments(const std::map<std::string, std::string>* arguments)
{
    if (arguments != nullptr) {
        if (arguments->find("isFile") != arguments->end()) {
            isFile = arguments->find("isFile")->second == "true";
        }
        if (arguments->find("mustExist") != arguments->end()) {
            mustExist = arguments->find("mustExist")->second == "true";
        }
        if (arguments->find("notEmpty") != arguments->end()) {
            notEmpty = arguments->find("notEmpty")->second == "true";
        }
        if (arguments->find("resolveEmpty") != arguments->end()) {
            resolveEmpty = arguments->find("resolveEmpty")->second == "true";
        }
    }
}

void ConfigPathSetup::makeOption(const pugi::xml_node& root, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    loadArguments(arguments);
    newOption(getXmlContent(root, true));
    setOption(config);
}

void ConfigPathSetup::makeOption(std::string optValue, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    loadArguments(arguments);
    newOption(optValue);
    setOption(config);
}

std::shared_ptr<ConfigOption> ConfigPathSetup::newOption(const std::string& optValue)
{
    std::string pathValue = resolvePath(optValue);

    if (notEmpty && pathValue.empty()) {
        throw std::runtime_error(fmt::format("Invalid {} resolves to empty value '{}'", xpath, optValue));
    }
    optionValue = std::make_shared<Option>(pathValue);
    return optionValue;
}

fs::path ConfigPathSetup::Home = "";

void ConfigIntSetup::makeOption(const pugi::xml_node& root, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlContent(root));
    setOption(config);
}

void ConfigIntSetup::makeOption(std::string optValue, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    if (rawCheck != nullptr) {
        if (!rawCheck(optValue)) {
            throw std::runtime_error(fmt::format("Invalid {} value '{}'", xpath, optValue));
        }
    } else if (valueCheck != nullptr) {
        if (!valueCheck(std::stoi(optValue))) {
            throw std::runtime_error(fmt::format("Invalid {} value '{}'", xpath, optValue));
        }
    } else if (minCheck != nullptr) {
        if (!minCheck(std::stoi(optValue), minValue)) {
            throw std::runtime_error(fmt::format("Invalid {} value '{}', must be at least {}", xpath, optValue, minValue));
        }
    }
    try {
        optionValue = std::make_shared<IntOption>(std::stoi(optValue));
        setOption(config);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(fmt::format("Error in config file: {} unsupported int value '{}'", xpath, optValue).c_str());
    }
}

int ConfigIntSetup::getXmlContent(const pugi::xml_node& root) const
{
    std::string sVal = ConfigSetup::getXmlContent(root, true);
    log_debug("Config: option: '{}/{}' value: '{}'", root.path(), xpath, sVal.c_str());
    if (rawCheck != nullptr) {
        if (!rawCheck(sVal)) {
            throw std::runtime_error(fmt::format("Invalid {}/{} value '{}'", root.path(), xpath, sVal));
        }
    } else if (valueCheck != nullptr) {
        if (!valueCheck(std::stoi(sVal))) {
            throw std::runtime_error(fmt::format("Invalid {}/{} value {}", root.path(), xpath, sVal));
        }
    } else if (minCheck != nullptr) {
        if (!minCheck(std::stoi(sVal), minValue)) {
            throw std::runtime_error(fmt::format("Invalid {}/{} value '{}', must be at least {}", root.path(), xpath, sVal, minValue));
        }
    }

    try {
        return std::stoi(sVal);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(fmt::format("Error in config file: {}/{} unsupported int value '{}'", root.path(), xpath, sVal).c_str());
    }
}

std::shared_ptr<ConfigOption> ConfigIntSetup::newOption(int optValue)
{
    if (valueCheck != nullptr && !valueCheck(optValue)) {
        throw std::runtime_error(fmt::format("Invalid {} value {}", xpath, optValue));
    }
    if (minCheck != nullptr && !minCheck(optValue, minValue)) {
        throw std::runtime_error(fmt::format("Invalid {} value {}, must be at least {}", xpath, optValue, minValue));
    }
    optionValue = std::make_shared<IntOption>(optValue);
    return optionValue;
}

bool ConfigIntSetup::CheckSqlLiteSyncValue(std::string& value)
{
    auto temp_int = 0;
    if (value == "off" || value == std::to_string(MT_SQLITE_SYNC_OFF))
        temp_int = MT_SQLITE_SYNC_OFF;
    else if (value == "normal" || value == std::to_string(MT_SQLITE_SYNC_NORMAL))
        temp_int = MT_SQLITE_SYNC_NORMAL;
    else if (value == "full" || value == std::to_string(MT_SQLITE_SYNC_FULL))
        temp_int = MT_SQLITE_SYNC_FULL;
    else
        return false;
    value.assign(std::to_string(temp_int));
    return true;
}

bool ConfigIntSetup::CheckProfleNumberValue(std::string& value)
{
    auto temp_int = 0;
    if (value == "source" || value == std::to_string(SOURCE))
        temp_int = SOURCE;
    else if (value == "off" || value == std::to_string(OFF))
        temp_int = OFF;
    else {
        temp_int = std::stoi(value);
        if (temp_int <= 0)
            return false;
    }
    value.assign(std::to_string(temp_int));
    return true;
}

bool ConfigIntSetup::CheckMinValue(int value, int minValue)
{
    return (value < minValue) ? false : true;
}

bool ConfigIntSetup::CheckImageQualityValue(int value)
{
    return (value < 0 || value > 10) ? false : true;
}

bool ConfigIntSetup::CheckUpnpStringLimitValue(int value)
{
    return (value != -1) && (value < 4) ? false : true;
}

void ConfigBoolSetup::makeOption(const pugi::xml_node& root, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlContent(root));
    setOption(config);
}

bool validateTrueFalse(const std::string& optValue)
{
    return (optValue == "true" || optValue == "false");
}

void ConfigBoolSetup::makeOption(std::string optValue, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    if (!validateTrueFalse(optValue) && !validateYesNo(optValue))
        throw std::runtime_error(fmt::format("Invalid {} value {}", xpath, optValue));
    optionValue = std::make_shared<BoolOption>(optValue == YES || optValue == "true");
    setOption(config);
};

bool ConfigBoolSetup::getXmlContent(const pugi::xml_node& root) const
{
    std::string optValue = ConfigSetup::getXmlContent(root, true);
    if (rawCheck != nullptr) {
        if (!rawCheck(optValue)) {
            throw std::runtime_error(fmt::format("Invalid {}/{} value '{}'", root.path(), xpath, optValue));
        }
    }

    if (!validateTrueFalse(optValue) && !validateYesNo(optValue))
        throw std::runtime_error(fmt::format("Invalid {}/{} value {}", root.path(), xpath, optValue));
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
        } else {
            temp_bool = true;
        }
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
        for (const pugi::xml_node& child : element.children()) {
            if (std::string(child.name()) == ConfigManager::mapConfigOption(nodeOption)) {
                std::string attrValue = attrOption != CFG_MAX ? child.attribute(ConfigManager::mapConfigOption(attrOption)).as_string() : child.text().as_string();
                if (itemNotEmpty && attrValue.empty()) {
                    throw std::runtime_error(fmt::format("Invalid array {} value {} empty '{}'", element.path(), xpath, attrValue));
                }
                if (!attrValue.empty())
                    result.push_back(attrValue);
            }
        }
    }
    return true;
}

void ConfigArraySetup::makeOption(const pugi::xml_node& root, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlContent(getXmlElement(root)));
    setOption(config);
}

bool ConfigArraySetup::updateDetail(const std::string& optItem, const std::string& optValue, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    if (optItem.substr(0, strlen(xpath)) == xpath && optionValue != nullptr) {
        std::shared_ptr<ArrayOption> value = std::dynamic_pointer_cast<ArrayOption>(optionValue);
        log_info("Updating Array Detail {} {} {}", xpath, optItem, optValue);
        for (size_t i = 0; i < value->getArrayOption().size(); i++) {
            auto index = getItemPath(i);
            if (optItem == index) {
                log_info("Old Array Detail {} {}", index, value->getArrayOption()[i]);
                value->setItem(i, optValue);
                log_info("New Array Detail {} {}", index, config->getArrayOption(option)[i]);
                return true;
            }
        }
    }
    return false;
}

std::vector<std::string> ConfigArraySetup::getXmlContent(const pugi::xml_node& optValue) const
{
    std::vector<std::string> result;
    if (initArray != nullptr) {
        if (!initArray(optValue, result, ConfigManager::mapConfigOption(nodeOption))) {
            throw std::runtime_error(fmt::format("Invalid {} array value '{}'", xpath, optValue));
        }
    } else {
        if (!createArrayFromNode(optValue, result)) {
            throw std::runtime_error(fmt::format("Invalid {} array value '{}'", xpath, optValue));
        }
    }
    if (notEmpty && result.empty()) {
        throw std::runtime_error(fmt::format("Invalid array {} empty '{}'", xpath, optValue));
    }
    return result;
}

std::shared_ptr<ConfigOption> ConfigArraySetup::newOption(const std::vector<std::string>& optValue)
{
    optionValue = std::make_shared<ArrayOption>(optValue);
    return optionValue;
}

bool ConfigArraySetup::InitPlayedItemsMark(const pugi::xml_node& value, std::vector<std::string>& result, const char* node_name)
{
    if (value != nullptr) {
        for (const pugi::xml_node& content : value.children()) {
            if (std::string(content.name()) != node_name)
                continue;

            std::string mark_content = content.text().as_string();
            if (mark_content.empty()) {
                log_error("error in configuration, <mark-played-items>, empty <content> parameter");
                return false;
            }

            if ((mark_content != DEFAULT_MARK_PLAYED_CONTENT_VIDEO) && (mark_content != DEFAULT_MARK_PLAYED_CONTENT_AUDIO) && (mark_content != DEFAULT_MARK_PLAYED_CONTENT_IMAGE)) {
                log_error(R"(error in configuration, <mark-played-items>, invalid <content> parameter! Allowed values are "video", "audio", "image")");
                return false;
            }

            result.push_back(mark_content);
        }
    }
    return true;
}

bool ConfigArraySetup::InitItemsPerPage(const pugi::xml_node& value, std::vector<std::string>& result, const char* node_name)
{
    // create default structure
    if (std::distance(value.begin(), value.end()) == 0) {
        result.emplace_back(std::to_string(DEFAULT_ITEMS_PER_PAGE_1));
        result.emplace_back(std::to_string(DEFAULT_ITEMS_PER_PAGE_2));
        result.emplace_back(std::to_string(DEFAULT_ITEMS_PER_PAGE_3));
        result.emplace_back(std::to_string(DEFAULT_ITEMS_PER_PAGE_4));
    } else {
        // create the array from either user settings
        for (const pugi::xml_node& child : value.children()) {
            if (std::string(child.name()) == node_name) {
                int i = child.text().as_int();
                if (i < 1) {
                    log_error("Error in config file: incorrect <option> value for <items-per-page>");
                    return false;
                }
                result.emplace_back(child.text().as_string());
            }
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
        for (const pugi::xml_node& child : optValue.children()) {
            if (std::string(child.name()) == ConfigManager::mapConfigOption(nodeOption)) {
                std::string key = child.attribute(ConfigManager::mapConfigOption(keyOption)).as_string();
                std::string value = child.attribute(ConfigManager::mapConfigOption(valOption)).as_string();
                if (!key.empty() && !value.empty()) {
                    if (tolower) {
                        key = tolower_string(key);
                    }
                    result[key] = value;
                } else if (itemNotEmpty) {
                    return false;
                }
            }
        }
    } else if (option == CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST) {
        result["audio/mpeg"] = CONTENT_TYPE_MP3;
        result["audio/mp4"] = CONTENT_TYPE_MP4;
        result["video/mp4"] = CONTENT_TYPE_MP4;
        result["application/ogg"] = CONTENT_TYPE_OGG;
        result["audio/x-flac"] = CONTENT_TYPE_FLAC;
        result["audio/flac"] = CONTENT_TYPE_FLAC;
        result["image/jpeg"] = CONTENT_TYPE_JPG;
        result["audio/x-mpegurl"] = CONTENT_TYPE_PLAYLIST;
        result["audio/x-scpls"] = CONTENT_TYPE_PLAYLIST;
        result["audio/x-wav"] = CONTENT_TYPE_PCM;
        result["audio/wave"] = CONTENT_TYPE_PCM;
        result["audio/wav"] = CONTENT_TYPE_PCM;
        result["audio/vnd.wave"] = CONTENT_TYPE_PCM;
        result["audio/L16"] = CONTENT_TYPE_PCM;
        result["audio/x-aiff"] = CONTENT_TYPE_AIFF;
        result["audio/aiff"] = CONTENT_TYPE_AIFF;
        result["video/x-msvideo"] = CONTENT_TYPE_AVI;
        result["video/mpeg"] = CONTENT_TYPE_MPEG;
    }
    return true;
}

void ConfigDictionarySetup::makeOption(const pugi::xml_node& root, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    if (arguments != nullptr && arguments->find("tolower") != arguments->end()) {
        tolower = arguments->find("tolower")->second == "true";
    }
    newOption(getXmlContent(getXmlElement(root)));
    setOption(config);
}

bool ConfigDictionarySetup::updateDetail(const std::string& optItem, const std::string& optValue, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    if (optItem.substr(0, strlen(xpath)) == xpath && optionValue != nullptr) {
        std::shared_ptr<DictionaryOption> value = std::dynamic_pointer_cast<DictionaryOption>(optionValue);
        log_info("Updating Dictionary Detail {} {} {}", xpath, optItem, optValue);
        int i = 0;
        for (const auto& entry : value->getDictionaryOption()) {
            auto index = getItemPath(i, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM);
            if (optItem == index) {
                log_info("Old Dictionary Detail {} {}", index, entry.first);
                value->setKey(entry.first, optValue);
                log_info("New Dictionary Detail {} {}", index, config->getDictionaryOption(option)[optValue]);
                return true;
            }
            index = getItemPath(i, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO);
            if (optItem == index) {
                log_info("Old Dictionary Detail {} {}", index, value->getDictionaryOption()[entry.first]);
                value->setValue(entry.first, optValue);
                log_info("New Dictionary Detail {} {}", index, config->getDictionaryOption(option)[entry.first]);
                return true;
            }
            i++;
        }
    }
    return false;
}

std::map<std::string, std::string> ConfigDictionarySetup::getXmlContent(const pugi::xml_node& optValue) const
{
    std::map<std::string, std::string> result;
    if (initDict != nullptr) {
        if (!initDict(optValue, result)) {
            throw std::runtime_error(fmt::format("Init {} dictionary failed '{}'", xpath, optValue));
        }
    } else {
        if (!createDictionaryFromNode(optValue, result)) {
            throw std::runtime_error(fmt::format("Init {} dictionary failed '{}'", xpath, optValue));
        }
    }
    if (notEmpty && result.empty()) {
        throw std::runtime_error(fmt::format("Invalid dictionary {} empty '{}'", xpath, optValue));
    }
    return result;
}

std::shared_ptr<ConfigOption> ConfigDictionarySetup::newOption(const std::map<std::string, std::string>& optValue)
{
    optionValue = std::make_shared<DictionaryOption>(optValue);
    return optionValue;
}

/// \brief Creates an array of AutoscanDirectory objects from a XML nodeset.
/// \param element starting element of the nodeset.
/// \param scanmode add only directories with the specified scanmode to the array
bool ConfigAutoscanSetup::createAutoscanListFromNode(const pugi::xml_node& element, std::shared_ptr<AutoscanList>& result)
{
    if (element == nullptr)
        return true;

    for (const pugi::xml_node& child : element.children()) {

        // We only want directories
        if (std::string(child.name()) != ConfigManager::mapConfigOption(ATTR_AUTOSCAN_DIRECTORY))
            continue;

        fs::path location;
        try {
            location = findConfigSetup<ConfigPathSetup>(ATTR_AUTOSCAN_DIRECTORY_LOCATION)->getXmlContent(child);
        } catch (const std::runtime_error& e) {
            log_warning("Found an Autoscan directory with invalid location!");
            continue;
        }

        ScanMode mode = findConfigSetup<ConfigEnumSetup<ScanMode>>(ATTR_AUTOSCAN_DIRECTORY_MODE)->getXmlContent(child);

        if (mode != scanmode) {
            continue; // skip scan modes that we are not interested in (content manager needs one mode type per array)
        }

        unsigned int interval = 0;
        if (mode == ScanMode::Timed) {
            interval = findConfigSetup<ConfigIntSetup>(ATTR_AUTOSCAN_DIRECTORY_INTERVAL)->getXmlContent(child);
        }

        bool recursive = findConfigSetup<ConfigBoolSetup>(ATTR_AUTOSCAN_DIRECTORY_RECURSIVE)->getXmlContent(child);
        auto cs = findConfigSetup<ConfigBoolSetup>(ATTR_AUTOSCAN_DIRECTORY_RECURSIVE);
        bool hidden = cs->hasXmlElement(child) ? cs->getXmlContent(child) : hiddenFiles;

        auto dir = std::make_shared<AutoscanDirectory>(location, mode, recursive, true, INVALID_SCAN_ID, interval, hidden);
        try {
            result->add(dir);
        } catch (const std::runtime_error& e) {
            log_error("Could not add " + location.string() + ": " + e.what());
            return false;
        }
    }

    return true;
}

bool ConfigAutoscanSetup::updateDetail(const std::string& optItem, const std::string& optValue, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    if (optItem.substr(0, strlen(xpath)) == xpath) {
        log_info("Updating Autoscan Detail {} {} {}", xpath, optItem, optValue);
        return true;
    }
    return false;
}

void ConfigAutoscanSetup::makeOption(const pugi::xml_node& root, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    if (arguments != nullptr && arguments->find("hiddenFiles") != arguments->end()) {
        hiddenFiles = arguments->find("hiddenFiles")->second == "true";
    }
    newOption(getXmlElement(root));
    setOption(config);
}

std::shared_ptr<ConfigOption> ConfigAutoscanSetup::newOption(const pugi::xml_node& optValue)
{
    std::shared_ptr<AutoscanList> result = std::make_shared<AutoscanList>(nullptr);
    if (!createAutoscanListFromNode(optValue, result)) {
        throw std::runtime_error(fmt::format("Init {} autoscan faild '{}'", xpath, optValue));
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

    std::map<std::string, std::string> mt_mappings;
    {
        auto cs = findConfigSetup<ConfigDictionarySetup>(ATTR_TRANSCODING_MIMETYPE_PROF_MAP);
        if (cs->hasXmlElement(element)) {
            mt_mappings = cs->getXmlContent(cs->getXmlElement(element));
        }
    }

    auto profiles = element.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES));
    if (profiles == nullptr)
        return true;

    bool allow_unused_profiles = !findConfigSetup<ConfigBoolSetup>(CFG_TRANSCODING_PROFILES_PROFILE_ALLOW_UNUSED)->getXmlContent(element.root());
    if (!allow_unused_profiles && mt_mappings.empty() && !profiles.empty()) {
        log_error("error in configuration: transcoding "
                  "profiles exist, but no mimetype to profile mappings specified");
        return false;
    }

    for (const pugi::xml_node& child : profiles.children()) {
        if (std::string(child.name()) != ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE))
            continue;

        if (!findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED)->getXmlContent(child))
            continue;

        auto prof = std::make_shared<TranscodingProfile>(
            findConfigSetup<ConfigEnumSetup<transcoding_type_t>>(ATTR_TRANSCODING_PROFILES_PROFLE_TYPE)->getXmlContent(child),
            findConfigSetup<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_NAME)->getXmlContent(child));
        prof->setTargetMimeType(findConfigSetup<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE)->getXmlContent(child));

        pugi::xml_node sub;
        sub = findConfigSetup<ConfigSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_RES)->getXmlElement(child);
        if (sub != nullptr) {
            std::string param = sub.text().as_string();
            if (!param.empty()) {
                if (check_resolution(param))
                    prof->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), param);
            }
        }

        {
            auto cs = findConfigSetup<ConfigArraySetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC);
            if (cs->hasXmlElement(child)) {
                sub = cs->getXmlElement(child);
                avi_fourcc_listmode_t fcc_mode = findConfigSetup<ConfigEnumSetup<avi_fourcc_listmode_t>>(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE)->getXmlContent(sub);
                if (fcc_mode != FCC_None) {
                    prof->setAVIFourCCList(cs->getXmlContent(sub), fcc_mode);
                }
            }
        }
        {
            auto cs = findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL);
            if (cs->hasXmlElement(child))
                prof->setAcceptURL(cs->getXmlContent(child));
        }
        {
            auto cs = findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ);
            if (cs->hasXmlElement(child))
                prof->setSampleFreq(cs->getXmlContent(child));
        }
        {
            auto cs = findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN);
            if (cs->hasXmlElement(child))
                prof->setNumChannels(cs->getXmlContent(child));
        }
        {
            auto cs = findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG);
            if (cs->hasXmlElement(child))
                prof->setHideOriginalResource(cs->getXmlContent(child));
        }
        {
            auto cs = findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_THUMB);
            if (cs->hasXmlElement(child))
                prof->setThumbnail(cs->getXmlContent(child));
        }
        {
            auto cs = findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_FIRST);
            if (cs->hasXmlElement(child))
                prof->setFirstResource(cs->getXmlContent(child));
        }
        {
            auto cs = findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_USECHUNKEDENC);
            if (cs->hasXmlElement(child))
                prof->setChunked(cs->getXmlContent(child));
        }
        {
            auto cs = findConfigSetup<ConfigBoolSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG);
            if (cs->hasXmlElement(child))
                prof->setTheora(cs->getXmlContent(child));
        }

        sub = findConfigSetup<ConfigSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT)->getXmlElement(child);
        auto command = findConfigSetup<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND)->getXmlContent(sub);
        prof->setCommand(command);

        std::string tmp_path;
        if (fs::path(command).is_absolute()) {
            if (!isRegularFile(command) && !fs::is_symlink(command)) {
                log_error("error in configuration, transcoding profile \""
                    + prof->getName() + "\" could not find transcoding command " + command);
                return false;
            }
            tmp_path = command;
        } else {
            tmp_path = find_in_path(command);
            if (tmp_path.empty()) {
                log_error("error in configuration, transcoding profile \""
                    + prof->getName() + "\" could not find transcoding command " + command + " in $PATH");
                return false;
            }
        }

        int err = 0;
        if (!is_executable(tmp_path, &err)) {
            log_error("error in configuration, transcoding profile "
                + prof->getName() + ": transcoder " + command + "is not executable - " + strerror(err));
            return false;
        }

        prof->setArguments(findConfigSetup<ConfigStringSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS)->getXmlContent(sub));

        sub = findConfigSetup<ConfigSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER)->getXmlElement(child);
        size_t buffer = findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE)->getXmlContent(sub);
        size_t chunk = findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK)->getXmlContent(sub);
        size_t fill = findConfigSetup<ConfigIntSetup>(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL)->getXmlContent(sub);

        if (chunk > buffer) {
            log_error("error in configuration: transcoding profile \""
                + prof->getName() + "\" chunk size can not be greater than buffer size");
            return false;
        }
        if (fill > buffer) {
            log_error("error in configuration: transcoding profile \""
                + prof->getName() + "\" fill size can not be greater than buffer size");
            return false;
        }

        prof->setBufferOptions(buffer, chunk, fill);

        bool set = false;
        for (const auto& mt_mapping : mt_mappings) {
            if (mt_mapping.second == prof->getName()) {
                result->add(mt_mapping.first, prof);
                set = true;
            }
        }

        if (!set) {
            log_error("error in configuration: you specified a mimetype to transcoding profile mapping, "
                      "but no match for profile \""
                + prof->getName() + "\" exists");
            if (!allow_unused_profiles) {
                return false;
            }
        }
    }

    auto tpl = result->getList();
    for (const auto& mt_mapping : mt_mappings) {
        if (tpl.find(mt_mapping.first) == tpl.end()) {
            log_error("error in configuration: you specified a mimetype to transcoding profile mapping, "
                      "but the profile \""
                + mt_mapping.second + "\" for mimetype \"" + mt_mapping.first + "\" does not exists");
            if (!findConfigSetup<ConfigBoolSetup>(CFG_TRANSCODING_MIMETYPE_PROF_MAP_ALLOW_UNUSED)->getXmlContent(element.root())) {
                return false;
            }
        }
    }
    return true;
}

void ConfigTranscodingSetup::makeOption(const pugi::xml_node& root, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    if (arguments != nullptr && arguments->find("isEnabled") != arguments->end()) {
        isEnabled = arguments->find("isEnabled")->second == "true";
    }
    newOption(getXmlElement(root));
    setOption(config);
}

bool ConfigTranscodingSetup::updateDetail(const std::string& optItem, const std::string& optValue, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    if (optItem.substr(0, strlen(xpath)) == xpath) {
        log_info("Updating Transcoding Detail {} {} {}", xpath, optItem, optValue);
        return true;
    }
    return false;
}

std::shared_ptr<ConfigOption> ConfigTranscodingSetup::newOption(const pugi::xml_node& optValue)
{
    std::shared_ptr<TranscodingProfileList> result = std::make_shared<TranscodingProfileList>();

    if (!createTranscodingProfileListFromNode(isEnabled ? optValue : pugi::xml_node(nullptr), result)) {
        throw std::runtime_error(fmt::format("Init {} transcoding failed '{}'", xpath, optValue));
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

    for (const pugi::xml_node& child : element.children()) {

        // We only want directories
        if (std::string(child.name()) != ConfigManager::mapConfigOption(ATTR_CLIENTS_CLIENT))
            continue;

        auto flags = findConfigSetup<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_FLAGS)->getXmlContent(child);
        auto ip = findConfigSetup<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_IP)->getXmlContent(child);
        auto userAgent = findConfigSetup<ConfigStringSetup>(ATTR_CLIENTS_CLIENT_USERAGENT)->getXmlContent(child);

        std::vector<std::string> flagsVector = split_string(flags, '|', false);
        int flag = std::accumulate(flagsVector.begin(), flagsVector.end(), 0, [](int flg, const auto& i) //
            { return flg | ClientConfig::remapFlag(i); });

        auto client = std::make_shared<ClientConfig>(flag, ip, userAgent);
        auto clientInfo = client->getClientInfo();
        Clients::addClientInfo(clientInfo);
        try {
            result->add(client);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error("Could not add " + ip + " client: " + e.what());
        }
    }

    return true;
}

void ConfigClientSetup::makeOption(const pugi::xml_node& root, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    if (arguments != nullptr && arguments->find("isEnabled") != arguments->end()) {
        isEnabled = arguments->find("isEnabled")->second == "true";
    }
    newOption(getXmlElement(root));
    setOption(config);
}

bool ConfigClientSetup::updateDetail(const std::string& optItem, const std::string& optValue, std::shared_ptr<Config> config, const std::map<std::string, std::string>* arguments)
{
    if (optItem.substr(0, strlen(xpath)) == xpath) {
        log_info("Updating Client Detail {} {} {}", xpath, optItem, optValue);
        return true;
    }
    return false;
}

std::shared_ptr<ConfigOption> ConfigClientSetup::newOption(const pugi::xml_node& optValue)
{
    std::shared_ptr<ClientConfigList> result = std::make_shared<ClientConfigList>();

    if (!createClientConfigListFromNode(isEnabled ? optValue : pugi::xml_node(nullptr), result)) {
        throw std::runtime_error(fmt::format("Init {} client config failed '{}'", xpath, optValue));
    }
    optionValue = std::make_shared<ClientConfigListOption>(result);
    return optionValue;
}
