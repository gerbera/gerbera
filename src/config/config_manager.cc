/*MT*

    MediaTomb - http://www.mediatomb.cc/

    config_manager.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/

/// \file config_manager.cc

#include "config_manager.h" // API

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(HAVE_NL_LANGINFO) && defined(HAVE_SETLOCALE)
#include <clocale>
#include <langinfo.h>
#include <utility>
#endif

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#include "autoscan.h"
#include "client_config.h"
#include "config_options.h"
#include "metadata/metadata_handler.h"
#include "storage/storage.h"
#include "transcoding/transcoding.h"
#include "util/string_converter.h"
#include "util/tools.h"

#ifdef HAVE_INOTIFY
#include "util/mt_inotify.h"
#endif

bool ConfigManager::debug_logging = false;

ConfigManager::ConfigManager(fs::path filename,
    const fs::path& userhome, const fs::path& config_dir,
    fs::path prefix_dir, fs::path magic_file,
    std::string ip, std::string interface, int port,
    bool debug_logging)
    : filename(filename)
    , prefix_dir(std::move(prefix_dir))
    , magic_file(std::move(magic_file))
    , ip(std::move(ip))
    , interface(std::move(interface))
    , port(port)
    , xmlDoc(std::make_unique<pugi::xml_document>())
    , options(std::make_unique<std::vector<std::shared_ptr<ConfigOption>>>())
{
    ConfigManager::debug_logging = debug_logging;

    options->resize(CFG_MAX);

    if (filename.empty()) {
        // No config file path provided, so lets find one.
        fs::path home = userhome / config_dir;
        filename += home / DEFAULT_CONFIG_NAME;
    }

    std::error_code ec;
    if (!isRegularFile(filename, ec)) {
        std::ostringstream expErrMsg;
        expErrMsg << "\nThe server configuration file could not be found: ";
        expErrMsg << filename << "\n";
        expErrMsg << "Gerbera could not find a default configuration file.\n";
        expErrMsg << "Try specifying an alternative configuration file on the command line.\n";
        expErrMsg << "For a list of options run: gerbera -h\n";

        throw std::runtime_error(expErrMsg.str());
    }

    load(filename, userhome);

#ifdef TOMBDEBUG
    dumpOptions();
#endif

    // now the XML is no longer needed we can destroy it
    xmlDoc = nullptr;
}

ConfigManager::~ConfigManager()
{
    log_debug("ConfigManager destroyed");
}

typedef bool (*StringCheckFunction)(std::string& value);
typedef bool (*ArrayInitFunction)(const pugi::xml_node& value, std::vector<std::string>& result);
typedef bool (*DictionaryInitFunction)(const pugi::xml_node& value, std::map<std::string, std::string>& result);
typedef bool (*IntCheckFunction)(int value);
typedef bool (*IntMinFunction)(int value, int minValue);

class ConfigSetup {
protected:
    std::shared_ptr<ConfigOption> optionValue;
    std::string cpath;
    bool required = false;
    StringCheckFunction rawCheck;
    std::string defaultValue;

public:
    static const char* ROOT_NAME;
    config_option_t option;
    const char* xpath;

    ConfigSetup(config_option_t option, const char* xpath, bool required = false, const char* defaultValue = "")
        : cpath(std::string("/") + ROOT_NAME + xpath)
        , required(required)
        , rawCheck(nullptr)
        , defaultValue(defaultValue)
        , option(option)
        , xpath(xpath)
    {
    }

    ConfigSetup(config_option_t option, const char* xpath, StringCheckFunction check, const char* defaultValue = "")
        : cpath(std::string("/") + ROOT_NAME + xpath)
        , required(false)
        , rawCheck(check)
        , defaultValue(defaultValue)
        , option(option)
        , xpath(xpath)
    {
    }

    void setDefaultValue(std::string defaultValue)
    {
        this->defaultValue.assign(defaultValue);
    }

    void setOption(std::vector<std::shared_ptr<ConfigOption>>& options)
    {
        options.at(option) = optionValue;
    }

    std::shared_ptr<ConfigOption> getValue() { return optionValue; }

    pugi::xml_node getXmlElement(const pugi::xml_node& root) const
    {
        pugi::xpath_node xpathNode = root.select_node(cpath.c_str());
        if (required && xpathNode.node() == nullptr)
            throw std::runtime_error(fmt::format("Error in config file: {} tag not found", xpath));
        return xpathNode.node();
    }

    bool hasXmlElement(const pugi::xml_node& root) const
    {
        pugi::xpath_node xpathNode = root.select_node(cpath.c_str());
        return xpathNode.node() != nullptr;
    }

    /// \brief Returns a config option with the given xpath, if option does not exist a default value is returned.
    /// \param xpath option xpath
    /// \param defaultValue default value if option not found
    ///
    /// The xpath parameter has XPath syntax:
    /// "/path/to/option" will return the text value of the given "option" element
    /// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
    std::string getXmlContent(const pugi::xml_node& root, bool trim = true) const
    {
        pugi::xpath_node xpathNode = root.select_node(cpath.c_str());
        if (required && (xpathNode.node() == nullptr || xpathNode.attribute() == nullptr))
            throw std::runtime_error(fmt::format("Error in config file: {} not found", xpath));

        if (xpathNode.node() != nullptr) {
            std::string optValue = trim ? trim_string(xpathNode.node().text().as_string()) : xpathNode.node().text().as_string();
            if (rawCheck != nullptr && !rawCheck(optValue)) {
                throw std::runtime_error(fmt::format("Invalid {} value '{}'", xpath, optValue));
            }
            return optValue;
        }

        if (xpathNode.attribute() != nullptr) {
            std::string optValue = trim ? trim_string(xpathNode.attribute().value()) : xpathNode.attribute().value();
            if (rawCheck != nullptr && !rawCheck(optValue)) {
                throw std::runtime_error(fmt::format("Invalid {} value '{}'", xpath, optValue));
            }
            return optValue;
        }

        log_info("Config: option not found: '{}' using default value: '{}'",
            xpath, defaultValue.c_str());

        return defaultValue;
    }

    virtual void makeOption(const pugi::xml_node& root, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr)
    {
        optionValue = std::make_shared<Option>("");
        setOption(options);
    }

    virtual void makeOption(std::string optValue, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr)
    {
        optionValue = std::make_shared<Option>(optValue);
        setOption(options);
    };
};

const char* ConfigSetup::ROOT_NAME = "config";

class ConfigStringSetup : public ConfigSetup {
protected:
    bool notEmpty = false;

public:
    ConfigStringSetup(config_option_t option, const char* xpath, bool required = false, const char* defaultValue = "", bool notEmpty = false)
        : ConfigSetup(option, xpath, required, defaultValue)
        , notEmpty(notEmpty)
    {
    }

    ConfigStringSetup(config_option_t option, const char* xpath, const char* defaultValue, StringCheckFunction check = nullptr, bool notEmpty = false)
        : ConfigSetup(option, xpath, check, defaultValue)
        , notEmpty(notEmpty)
    {
    }

    virtual ~ConfigStringSetup() = default;

    virtual void makeOption(const pugi::xml_node& root, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        bool trim = true;
        if (arguments != nullptr && arguments->find("trim") != arguments->end()) {
            trim = arguments->find("trim")->second == "true";
        }
        newOption(getXmlContent(root, trim));
        setOption(options);
    }

    std::shared_ptr<ConfigOption> newOption(const std::string& optValue)
    {
        if (notEmpty && optValue.empty()) {
            throw std::runtime_error(fmt::format("Invalid {} empty value '{}'", xpath, optValue));
        }
        optionValue = std::make_shared<Option>(optValue);
        return optionValue;
    }

    static bool CheckServerAppendToUrlValue(std::string& value)
    {
        return ((value != "none") && (value != "ip") && (value != "port")) ? false : true;
    }

    static bool CheckLayoutTypeValue(std::string& value)
    {
        return ((value != "js") && (value != "builtin") && (value != "disabled")) ? false : true;
    }

    static bool CheckAtrailResValue(std::string& value)
    {
        return (value != "640" && value != "720" && value != "720p") ? false : true;
    }
};

class ConfigPathSetup : public ConfigSetup {
protected:
    bool isFile = false;
    bool mustExist = false;
    bool notEmpty = false;
    bool resolveEmpty = true;

    /// \brief resolve path against home, an exception is raised if path does not exist on filesystem.
    /// \param path path to be resolved
    /// \param isFile file or directory
    /// \param mustExist file/directory must exist
    fs::path resolvePath(fs::path path)
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

    void loadArguments(const std::map<std::string, std::string>* arguments = nullptr)
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

public:
    static fs::path Home;

    ConfigPathSetup(config_option_t option, const char* xpath, const char* defaultValue = "", bool isFile = false, bool mustExist = true, bool notEmpty = false)
        : ConfigSetup(option, xpath, nullptr, defaultValue)
        , isFile(isFile)
        , mustExist(mustExist)
        , notEmpty(notEmpty)
    {
    }

    virtual void makeOption(const pugi::xml_node& root, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        loadArguments(arguments);
        newOption(getXmlContent(root, true));
        setOption(options);
    }

    virtual void makeOption(std::string optValue, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        loadArguments(arguments);
        newOption(optValue);
        setOption(options);
    }

    virtual ~ConfigPathSetup() = default;

    std::shared_ptr<ConfigOption> newOption(const std::string& optValue)
    {
        std::string pathValue = resolvePath(optValue);

        if (notEmpty && pathValue.empty()) {
            throw std::runtime_error(fmt::format("Invalid {} resolves to empty value '{}'", xpath, optValue));
        }
        optionValue = std::make_shared<Option>(pathValue);
        return optionValue;
    }
};
fs::path ConfigPathSetup::Home = "";

class ConfigIntSetup : public ConfigSetup {
protected:
    IntCheckFunction valueCheck = nullptr;
    IntMinFunction minCheck = nullptr;
    int minValue;

public:
    ConfigIntSetup(config_option_t option, const char* xpath)
        : ConfigSetup(option, xpath)
        , valueCheck(nullptr)
        , minCheck(nullptr)
    {
        this->defaultValue = std::to_string(0);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, int defaultValue)
        : ConfigSetup(option, xpath)
        , valueCheck(nullptr)
        , minCheck(nullptr)
    {
        this->defaultValue = std::to_string(defaultValue);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, IntCheckFunction check)
        : ConfigSetup(option, xpath)
        , valueCheck(check)
        , minCheck(nullptr)
    {
        this->defaultValue = std::to_string(0);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, int defaultValue, IntCheckFunction check)
        : ConfigSetup(option, xpath)
        , valueCheck(check)
        , minCheck(nullptr)
    {
        this->defaultValue = std::to_string(defaultValue);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, int defaultValue, int minValue, IntMinFunction check)
        : ConfigSetup(option, xpath)
        , valueCheck(nullptr)
        , minCheck(check)
        , minValue(minValue)
    {
        this->defaultValue = std::to_string(defaultValue);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* defaultValue, IntCheckFunction check = nullptr)
        : ConfigSetup(option, xpath)
        , valueCheck(check)
        , minCheck(nullptr)
    {
        this->defaultValue = defaultValue;
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* defaultValue, StringCheckFunction check)
        : ConfigSetup(option, xpath, check, defaultValue)
        , valueCheck(nullptr)
        , minCheck(nullptr)
    {
    }

    virtual ~ConfigIntSetup() = default;

    virtual void makeOption(const pugi::xml_node& root, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        newOption(getXmlContent(root));
        setOption(options);
    }

    virtual void makeOption(std::string optValue, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        if (rawCheck != nullptr && !rawCheck(optValue)) {
            throw std::runtime_error(fmt::format("Invalid {} value '{}'", xpath, optValue));
        }
        if (valueCheck != nullptr && !valueCheck(std::stoi(optValue))) {
            throw std::runtime_error(fmt::format("Invalid {} value '{}'", xpath, optValue));
        }
        if (minCheck != nullptr && !minCheck(std::stoi(optValue), minValue)) {
            throw std::runtime_error(fmt::format("Invalid {} value '{}', must be at least {}", xpath, optValue, minValue));
        }
        try {
            optionValue = std::make_shared<IntOption>(std::stoi(optValue));
            setOption(options);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(fmt::format("Error in config file: {} unsupported int value '{}'", xpath, optValue).c_str());
        }
    }

    int getXmlContent(const pugi::xml_node& root) const
    {
        std::string sVal = ConfigSetup::getXmlContent(root, true);
        log_info("Config: option: '{}' value: '{}'", xpath, sVal.c_str());
        if (rawCheck != nullptr && !rawCheck(sVal)) {
            throw std::runtime_error(fmt::format("Invalid {} value '{}'", xpath, sVal));
        }
        if (valueCheck != nullptr && !valueCheck(std::stoi(sVal))) {
            throw std::runtime_error(fmt::format("Invalid {} value {}", xpath, sVal));
        }
        if (minCheck != nullptr && !minCheck(std::stoi(sVal), minValue)) {
            throw std::runtime_error(fmt::format("Invalid {} value '{}', must be at least {}", xpath, sVal, minValue));
        }

        try {
            return std::stoi(sVal);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(fmt::format("Error in config file: {} unsupported int value '{}'", xpath, sVal).c_str());
        }
    }

    std::shared_ptr<ConfigOption> newOption(int optValue)
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

    static bool CheckSqlLiteSyncValue(std::string& value)
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

    static bool CheckMinValue(int value, int minValue)
    {
        return (value < minValue) ? false : true;
    }

    static bool CheckImageQualityValue(int value)
    {
        return (value < 0 || value > 10) ? false : true;
    }

    static bool CheckUpnpStringLimitValue(int value)
    {
        return (value != -1) && (value < 4) ? false : true;
    }
};

class ConfigBoolSetup : public ConfigSetup {
public:
    ConfigBoolSetup(config_option_t option, const char* xpath)
        : ConfigSetup(option, xpath)
    {
    }

    ConfigBoolSetup(config_option_t option, const char* xpath, const char* defaultValue, StringCheckFunction check = nullptr)
        : ConfigSetup(option, xpath, check, defaultValue)
    {
    }

    ConfigBoolSetup(config_option_t option, const char* xpath, bool defaultValue, StringCheckFunction check = nullptr)
        : ConfigSetup(option, xpath, check)
    {
        this->defaultValue = defaultValue ? YES : NO;
    }

    virtual ~ConfigBoolSetup() = default;

    virtual void makeOption(const pugi::xml_node& root, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        newOption(getXmlContent(root));
        setOption(options);
    }

    virtual void makeOption(std::string optValue, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        if (!validateYesNo(optValue))
            throw std::runtime_error(fmt::format("Invalid {} value {}", xpath, optValue));
        optionValue = std::make_shared<BoolOption>(optValue == YES);
        setOption(options);
    };

    bool getXmlContent(const pugi::xml_node& root) const
    {
        std::string sVal = ConfigSetup::getXmlContent(root, true);
        if (rawCheck != nullptr && !rawCheck(sVal)) {
            throw std::runtime_error(fmt::format("Invalid {} value '{}'", xpath, sVal));
        }

        if (!validateYesNo(sVal))
            throw std::runtime_error(fmt::format("Invalid {} value {}", xpath, sVal));
        return sVal == YES;
    }

    std::shared_ptr<ConfigOption> newOption(bool optValue)
    {
        optionValue = std::make_shared<BoolOption>(optValue);
        return optionValue;
    }

    static bool CheckSqlLiteRestoreValue(std::string& value)
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

    static bool CheckInotifyValue(std::string& value)
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

    static bool CheckMarkPlayedValue(std::string& value)
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
};

class ConfigArraySetup : public ConfigSetup {
protected:
    bool notEmpty = false;
    ArrayInitFunction initArray = nullptr;
    config_option_t nodeOption;
    config_option_t attrOption;

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
    bool createArrayFromNode(const pugi::xml_node& element, std::vector<std::string>& result)
    {
        if (element != nullptr) {
            for (const pugi::xml_node& child : element.children()) {
                if (std::string(child.name()) == ConfigManager::mapConfigOption(nodeOption)) {
                    std::string attrValue = child.attribute(ConfigManager::mapConfigOption(attrOption)).as_string();
                    if (!attrValue.empty())
                        result.push_back(attrValue);
                }
            }
        }
        return true;
    }

public:
    ConfigArraySetup(config_option_t option, const char* xpath, ArrayInitFunction init = nullptr, bool notEmpty = false)
        : ConfigSetup(option, xpath)
        , notEmpty(notEmpty)
        , initArray(init)
    {
    }

    ConfigArraySetup(config_option_t option, const char* xpath, config_option_t nodeOption, config_option_t attrOption, bool notEmpty = false)
        : ConfigSetup(option, xpath)
        , notEmpty(notEmpty)
        , initArray(nullptr)
        , nodeOption(nodeOption)
        , attrOption(attrOption)
    {
    }

    virtual ~ConfigArraySetup() = default;

    virtual void makeOption(const pugi::xml_node& root, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        newOption(getXmlElement(root));
        setOption(options);
    }

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue)
    {
        std::vector<std::string> result;
        if (initArray != nullptr && !initArray(optValue, result)) {
            throw std::runtime_error(fmt::format("Invalid {} array value '{}'", xpath, optValue));
        } else if (!createArrayFromNode(optValue, result)) {
            throw std::runtime_error(fmt::format("Invalid {} array value '{}'", xpath, optValue));
        }
        if (notEmpty && result.empty()) {
            throw std::runtime_error(fmt::format("Invalid array {} empty '{}'", xpath, optValue));
        }
        optionValue = std::make_shared<ArrayOption>(result);
        return optionValue;
    }

    static bool InitPlayedItemsMark(const pugi::xml_node& value, std::vector<std::string>& result)
    {
        if (value != nullptr) {
            for (const pugi::xml_node& content : value.children()) {
                if (std::string(content.name()) != ConfigManager::mapConfigOption(ATTR_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT))
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

    static bool InitItemsPerPage(const pugi::xml_node& value, std::vector<std::string>& result)
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
                if (std::string(child.name()) == ConfigManager::mapConfigOption(ATTR_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION)) {
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
};

class ConfigDictionarySetup : public ConfigSetup {
protected:
    bool notEmpty = false;
    DictionaryInitFunction initDict = nullptr;
    config_option_t nodeOption;
    config_option_t keyOption;
    config_option_t valOption;
    bool tolower = false;

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
    bool createDictionaryFromNode(const pugi::xml_node& optValue, std::map<std::string, std::string>& result)
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

public:
    ConfigDictionarySetup(config_option_t option, const char* xpath, DictionaryInitFunction init = nullptr, bool notEmpty = false)
        : ConfigSetup(option, xpath)
        , notEmpty(notEmpty)
        , initDict(init)
    {
    }

    ConfigDictionarySetup(config_option_t option, const char* xpath, config_option_t nodeOption, config_option_t valOption, config_option_t keyOption, bool notEmpty = false)
        : ConfigSetup(option, xpath)
        , notEmpty(notEmpty)
        , initDict(nullptr)
        , nodeOption(nodeOption)
        , keyOption(keyOption)
        , valOption(valOption)
    {
    }

    virtual ~ConfigDictionarySetup() = default;

    virtual void makeOption(const pugi::xml_node& root, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        if (arguments != nullptr && arguments->find("tolower") != arguments->end()) {
            tolower = arguments->find("tolower")->second == "true";
        }
        newOption(getXmlElement(root));
        setOption(options);
    }

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue)
    {
        std::map<std::string, std::string> result;
        if (initDict != nullptr && !initDict(optValue, result)) {
            throw std::runtime_error(fmt::format("Init {} dictionary failed '{}'", xpath, optValue));
        } else if (!createDictionaryFromNode(optValue, result)) {
            throw std::runtime_error(fmt::format("Init {} dictionary failed '{}'", xpath, optValue));
        }
        if (notEmpty && result.empty()) {
            throw std::runtime_error(fmt::format("Invalid dictionary {} empty '{}'", xpath, optValue));
        }
        optionValue = std::make_shared<DictionaryOption>(result);
        return optionValue;
    }
};

class ConfigAutoscanSetup : public ConfigSetup {
protected:
    ScanMode scanmode;
    bool hiddenFiles = false;

    /// \brief Creates an array of AutoscanDirectory objects from a XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param scanmode add only directories with the specified scanmode to the array
    bool createAutoscanListFromNode(const pugi::xml_node& element, std::shared_ptr<AutoscanList>& result)
    {
        if (element == nullptr)
            return true;

        for (const pugi::xml_node& child : element.children()) {

            // We only want directories
            if (std::string(child.name()) != ConfigManager::mapConfigOption(ATTR_AUTOSCAN_DIRECTORY))
                continue;

            fs::path location = child.attribute(ConfigManager::mapConfigOption(ATTR_AUTOSCAN_DIRECTORY_LOCATION)).as_string();
            if (location.empty()) {
                log_warning("Found an Autoscan directory with invalid location!");
                continue;
            }

            if (!fs::is_directory(location)) {
                log_warning("Autoscan path is not a directory: {}", location.string());
                continue;
            }

            std::string temp = child.attribute(ConfigManager::mapConfigOption(ATTR_AUTOSCAN_DIRECTORY_MODE)).as_string();
            if (temp.empty() || ((temp != "timed") && (temp != "inotify"))) {
                log_error("autoscan directory " + location.string() + ": mode attribute is missing or invalid");
                return false;
            }

            ScanMode mode = (temp == "timed") ? ScanMode::Timed : ScanMode::INotify;
            if (mode != scanmode) {
                continue; // skip scan modes that we are not interested in (content manager needs one mode type per array)
            }

            unsigned int interval = 0;
            if (mode == ScanMode::Timed) {
                temp = child.attribute(ConfigManager::mapConfigOption(ATTR_AUTOSCAN_DIRECTORY_INTERVAL)).as_string();
                if (temp.empty()) {
                    log_error("autoscan directory " + location.string() + ": interval attribute is required for timed mode");
                    return false;
                }

                interval = std::stoi(temp);
                if (interval == 0) {
                    log_error("autoscan directory " + location.string() + ": invalid interval attribute");
                    return false;
                }
            }

            temp = child.attribute(ConfigManager::mapConfigOption(ATTR_AUTOSCAN_DIRECTORY_RECURSIVE)).as_string();
            if (temp.empty()) {
                log_error("autoscan directory " + location.string() + ": recursive attribute is missing or invalid");
                return false;
            }

            bool recursive;
            if (temp == YES)
                recursive = true;
            else if (temp == YES)
                recursive = false;
            else {
                throw std::runtime_error("autoscan directory " + location.string() + ": recursive attribute " + temp + " is invalid");
            }

            bool hidden;
            temp = child.attribute(ConfigManager::mapConfigOption(ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES)).as_string();

            if (temp.empty())
                hidden = hiddenFiles;
            else if (temp == YES)
                hidden = true;
            else if (temp == NO)
                hidden = false;
            else {
                log_error("autoscan directory " + location.string() + ": hidden attribute " + temp + " is invalid");
                return false;
            }

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

public:
    ConfigAutoscanSetup(config_option_t option, const char* xpath, ScanMode scanmode)
        : ConfigSetup(option, xpath)
        , scanmode(scanmode)
    {
    }

    virtual ~ConfigAutoscanSetup() = default;

    virtual void makeOption(const pugi::xml_node& root, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        if (arguments != nullptr && arguments->find("hiddenFiles") != arguments->end()) {
            hiddenFiles = arguments->find("hiddenFiles")->second == "true";
        }
        newOption(getXmlElement(root));
        setOption(options);
    }

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue)
    {
        std::shared_ptr<AutoscanList> result = std::make_shared<AutoscanList>(nullptr);
        if (!createAutoscanListFromNode(optValue, result)) {
            throw std::runtime_error(fmt::format("Init {} autoscan faild '{}'", xpath, optValue));
        }
        optionValue = std::make_shared<AutoscanListOption>(result);
        return optionValue;
    }
};

class ConfigTranscodingSetup : public ConfigSetup {
protected:
    bool isEnabled = false;

    /// \brief Creates an array of TranscodingProfile objects from an XML
    /// nodeset.
    /// \param element starting element of the nodeset.
    bool createTranscodingProfileListFromNode(const pugi::xml_node& element, std::shared_ptr<TranscodingProfileList>& result)
    {
        std::string param;
        int param_int;

        if (element == nullptr)
            return true;

        std::map<std::string, std::string> mt_mappings;

        auto mtype_profile = element.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_MIMETYPE_PROF_MAP));
        if (mtype_profile != nullptr) {
            for (const pugi::xml_node& child : mtype_profile.children()) {
                if (std::string(ConfigManager::mapConfigOption(ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE)) == child.name()) {
                    std::string mt = child.attribute(ConfigManager::mapConfigOption(ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE)).as_string();
                    std::string pname = child.attribute(ConfigManager::mapConfigOption(ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING)).as_string();

                    if (!mt.empty() && !pname.empty()) {
                        mt_mappings[mt] = pname;
                    } else {
                        throw std::runtime_error("error in configuration: invalid or missing mimetype to profile mapping");
                    }
                }
            }
        }

        auto profiles = element.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES));
        if (profiles == nullptr)
            return true;

        for (const pugi::xml_node& child : profiles.children()) {
            if (std::string(child.name()) != ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE))
                continue;

            param = child.attribute(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED)).as_string();
            if (!validateYesNo(param))
                throw std::runtime_error("Error in config file: incorrect parameter "
                                         "for <profile enabled=\"\" /> attribute");

            if (param == "no")
                continue;

            param = child.attribute(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_TYPE)).as_string();
            if (param.empty())
                throw std::runtime_error("error in configuration: missing transcoding type in profile");

            transcoding_type_t tr_type;
            if (param == "external")
                tr_type = TR_External;
            /* for the future...
            else if (param == "remote")
                tr_type = TR_Remote;
             */
            else
                throw std::runtime_error("error in configuration: invalid transcoding type " + param + " in profile");

            param = child.attribute(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_NAME)).as_string();
            if (param.empty())
                throw std::runtime_error("error in configuration: invalid transcoding profile name");

            auto prof = std::make_shared<TranscodingProfile>(tr_type, param);

            pugi::xml_node sub;
            sub = child.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE));
            param = sub.text().as_string();
            if (param.empty())
                throw std::runtime_error("error in configuration: invalid target mimetype in transcoding profile");
            prof->setTargetMimeType(param);

            sub = child.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_RES));
            if (sub != nullptr) {
                param = sub.text().as_string();
                if (!param.empty()) {
                    if (check_resolution(param))
                        prof->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), param);
                }
            }

            sub = child.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC));
            if (sub != nullptr) {
                std::string mode = sub.attribute(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE)).as_string();
                if (mode.empty())
                    throw std::runtime_error("error in configuration: avi-fourcc-list requires a valid \"mode\" attribute");

                avi_fourcc_listmode_t fcc_mode;
                if (mode == "ignore")
                    fcc_mode = FCC_Ignore;
                else if (mode == "process")
                    fcc_mode = FCC_Process;
                else if (mode == "disabled")
                    fcc_mode = FCC_None;
                else
                    throw std::runtime_error("error in configuration: invalid mode given for avi-fourcc-list: \"" + mode + "\"");

                if (fcc_mode != FCC_None) {
                    std::vector<std::string> fcc_list;
                    for (const pugi::xml_node& fourcc : sub.children()) {
                        if (std::string(fourcc.name()) != ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC))
                            continue;

                        std::string fcc = fourcc.text().as_string();
                        if (fcc.empty())
                            throw std::runtime_error("error in configuration: empty fourcc specified");
                        fcc_list.push_back(fcc);
                    }

                    prof->setAVIFourCCList(fcc_list, fcc_mode);
                }
            }

            sub = child.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL));
            if (sub != nullptr) {
                param = sub.text().as_string();
                if (!validateYesNo(param))
                    throw std::runtime_error("Error in config file: incorrect parameter for <accept-url> tag");
                if (param == "yes")
                    prof->setAcceptURL(true);
                else
                    prof->setAcceptURL(false);
            }

            sub = child.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ));
            if (sub != nullptr) {
                param = sub.text().as_string();
                if (param == "source")
                    prof->setSampleFreq(SOURCE);
                else if (param == "off")
                    prof->setSampleFreq(OFF);
                else {
                    int freq = std::stoi(param);
                    if (freq <= 0)
                        throw std::runtime_error("Error in config file: incorrect parameter for <sample-frequency> tag");

                    prof->setSampleFreq(freq);
                }
            }

            sub = child.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN));
            if (sub != nullptr) {
                param = sub.text().as_string();
                if (param == "source")
                    prof->setNumChannels(SOURCE);
                else if (param == "off")
                    prof->setNumChannels(OFF);
                else {
                    int chan = std::stoi(param);
                    if (chan <= 0)
                        throw std::runtime_error("Error in config file: incorrect parameter for <number-of-channels> tag");
                    prof->setNumChannels(chan);
                }
            }

            sub = child.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG));
            if (sub != nullptr) {
                param = sub.text().as_string();
                if (!validateYesNo(param))
                    throw std::runtime_error("Error in config file: incorrect parameter for <hide-original-resource> tag");
                if (param == "yes")
                    prof->setHideOriginalResource(true);
                else
                    prof->setHideOriginalResource(false);
            }

            sub = child.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_THUMB));
            if (sub != nullptr) {
                param = sub.text().as_string();
                if (!validateYesNo(param))
                    throw std::runtime_error("Error in config file: incorrect parameter for <thumbnail> tag");
                if (param == "yes")
                    prof->setThumbnail(true);
                else
                    prof->setThumbnail(false);
            }

            sub = child.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_FIRST));
            if (sub != nullptr) {
                param = sub.text().as_string();
                if (!validateYesNo(param))
                    throw std::runtime_error("Error in config file: incorrect parameter for <profile first-resource=\"\" /> attribute");

                if (param == "yes")
                    prof->setFirstResource(true);
                else
                    prof->setFirstResource(false);
            }

            sub = child.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_USECHUNKEDENC));
            if (sub != nullptr) {
                param = sub.text().as_string();
                if (!validateYesNo(param))
                    throw std::runtime_error("Error in config file: incorrect parameter for use-chunked-encoding tag");

                if (param == "yes")
                    prof->setChunked(true);
                else
                    prof->setChunked(false);
            }

            sub = child.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG));
            if (sub != nullptr) {
                param = sub.text().as_string();
                if (!validateYesNo(param))
                    throw std::runtime_error("Error in config file: incorrect parameter for accept-ogg-theora tag");

                if (param == "yes")
                    prof->setTheora(true);
                else
                    prof->setTheora(false);
            }

            sub = child.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT));
            if (sub == nullptr)
                throw std::runtime_error("error in configuration: transcoding profile \""
                    + prof->getName() + "\" is missing the <agent> option");

            param = sub.attribute(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND)).as_string();
            if (param.empty())
                throw std::runtime_error("error in configuration: transcoding profile \""
                    + prof->getName() + "\" has an invalid command setting");
            prof->setCommand(param);

            std::string tmp_path;
            if (fs::path(param).is_absolute()) {
                if (!isRegularFile(param) && !fs::is_symlink(param))
                    throw std::runtime_error("error in configuration, transcoding profile \""
                        + prof->getName() + "\" could not find transcoding command " + param);
                tmp_path = param;
            } else {
                tmp_path = find_in_path(param);
                if (tmp_path.empty())
                    throw std::runtime_error("error in configuration, transcoding profile \""
                        + prof->getName() + "\" could not find transcoding command " + param + " in $PATH");
            }

            int err = 0;
            if (!is_executable(tmp_path, &err))
                throw std::runtime_error("error in configuration, transcoding profile "
                    + prof->getName() + ": transcoder " + param + "is not executable - " + strerror(err));

            param = sub.attribute(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS)).as_string();
            if (param.empty())
                throw std::runtime_error("error in configuration: transcoding profile " + prof->getName() + " has an empty argument string");

            prof->setArguments(param);

            sub = child.child(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER));
            if (sub == nullptr)
                throw std::runtime_error("error in configuration: transcoding profile \""
                    + prof->getName() + "\" is missing the <buffer> option");

            param_int = sub.attribute(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE)).as_int();
            if (param_int < 0)
                throw std::runtime_error("error in configuration: transcoding profile \""
                    + prof->getName() + "\" buffer size can not be negative");
            size_t bs = param_int;

            param_int = sub.attribute(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK)).as_int();
            if (param_int < 0)
                throw std::runtime_error("error in configuration: transcoding profile \""
                    + prof->getName() + "\" chunk size can not be negative");
            size_t cs = param_int;

            if (cs > bs)
                throw std::runtime_error("error in configuration: transcoding profile \""
                    + prof->getName() + "\" chunk size can not be greater than buffer size");

            param_int = sub.attribute(ConfigManager::mapConfigOption(ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL)).as_int();
            if (param_int < 0)
                throw std::runtime_error("error in configuration: transcoding profile \""
                    + prof->getName() + "\" fill size can not be negative");
            size_t fs = param_int;

            if (fs > bs)
                throw std::runtime_error("error in configuration: transcoding profile \""
                    + prof->getName() + "\" fill size can not be greater than buffer size");

            prof->setBufferOptions(bs, cs, fs);

            if (mtype_profile == nullptr) {
                throw std::runtime_error("error in configuration: transcoding "
                                         "profiles exist, but no mimetype to profile mappings specified");
            }

            bool set = false;
            for (const auto& mt_mapping : mt_mappings) {
                if (mt_mapping.second == prof->getName()) {
                    result->add(mt_mapping.first, prof);
                    set = true;
                }
            }

            if (!set)
                throw std::runtime_error("error in configuration: you specified a mimetype to transcoding profile mapping, "
                                         "but no match for profile \""
                    + prof->getName() + "\" exists");

            set = false;
        }

        return true;
    }

public:
    ConfigTranscodingSetup(config_option_t option, const char* xpath)
        : ConfigSetup(option, xpath)
    {
    }

    virtual ~ConfigTranscodingSetup() = default;

    virtual void makeOption(const pugi::xml_node& root, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        if (arguments != nullptr && arguments->find("isEnabled") != arguments->end()) {
            isEnabled = arguments->find("isEnabled")->second == "true";
        }
        newOption(getXmlElement(root));
        setOption(options);
    }

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue)
    {
        std::shared_ptr<TranscodingProfileList> result = std::make_shared<TranscodingProfileList>();

        if (!createTranscodingProfileListFromNode(isEnabled ? optValue : pugi::xml_node(nullptr), result)) {
            throw std::runtime_error(fmt::format("Init {} transcoding failed '{}'", xpath, optValue));
        }
        optionValue = std::make_shared<TranscodingProfileListOption>(result);
        return optionValue;
    }
};

class ConfigClientSetup : public ConfigSetup {
protected:
    bool isEnabled = false;

    /// \brief Creates an array of ClientConfig objects from a XML nodeset.
    /// \param element starting element of the nodeset.
    bool createClientConfigListFromNode(const pugi::xml_node& element, std::shared_ptr<ClientConfigList>& result)
    {
        if (element == nullptr)
            return true;

        for (const pugi::xml_node& child : element.children()) {

            // We only want directories
            if (std::string(child.name()) != ConfigManager::mapConfigOption(ATTR_CLIENTS_CLIENT))
                continue;

            std::string flags = child.attribute(ConfigManager::mapConfigOption(ATTR_CLIENTS_CLIENT_FLAGS)).as_string();
            std::string ip = child.attribute(ConfigManager::mapConfigOption(ATTR_CLIENTS_CLIENT_IP)).as_string();
            std::string userAgent = child.attribute(ConfigManager::mapConfigOption(ATTR_CLIENTS_CLIENT_USERAGENT)).as_string();

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

public:
    ConfigClientSetup(config_option_t option, const char* xpath)
        : ConfigSetup(option, xpath)
    {
    }

    virtual ~ConfigClientSetup() = default;

    virtual void makeOption(const pugi::xml_node& root, std::vector<std::shared_ptr<ConfigOption>>& options, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        if (arguments != nullptr && arguments->find("isEnabled") != arguments->end()) {
            isEnabled = arguments->find("isEnabled")->second == "true";
        }
        newOption(getXmlElement(root));
        setOption(options);
    }

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue)
    {
        std::shared_ptr<ClientConfigList> result = std::make_shared<ClientConfigList>();

        if (!createClientConfigListFromNode(isEnabled ? optValue : pugi::xml_node(nullptr), result)) {
            throw std::runtime_error(fmt::format("Init {} client config failed '{}'", xpath, optValue));
        }
        optionValue = std::make_shared<ClientConfigListOption>(result);
        return optionValue;
    }
};

static std::vector<std::shared_ptr<ConfigSetup>> complexOptions = {
    std::make_shared<ConfigIntSetup>(CFG_SERVER_PORT, "/server/port", 0),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_IP, "/server/ip", ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_NETWORK_INTERFACE, "/server/interface", ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_NAME, "/server/name", DESC_FRIENDLY_NAME),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MANUFACTURER, "/server/manufacturer", DESC_MANUFACTURER),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MANUFACTURER_URL, "/server/manufacturerURL", DESC_MANUFACTURER_URL),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MODEL_NAME, "/server/modelName", DESC_MODEL_NAME),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MODEL_DESCRIPTION, "/server/modelDescription", DESC_MODEL_DESCRIPTION),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MODEL_NUMBER, "/server/modelNumber", DESC_MODEL_NUMBER),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_MODEL_URL, "/server/modelURL", ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_SERIAL_NUMBER, "/server/serialNumber", DESC_SERIAL_NUMBER),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_PRESENTATION_URL, "/server/presentationURL", ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_APPEND_PRESENTATION_URL_TO, "/server/presentationURL/attribute::append-to", DEFAULT_PRES_URL_APPENDTO_ATTR, ConfigStringSetup::CheckServerAppendToUrlValue),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_UDN, "/server/udn"),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_HOME, "/server/home"),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_TMPDIR, "/server/tmpdir", DEFAULT_TMPDIR),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_WEBROOT, "/server/webroot"),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_SERVEDIR, "/server/servedir", ""),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_ALIVE_INTERVAL, "/server/alive", DEFAULT_ALIVE_INTERVAL, ALIVE_INTERVAL_MIN, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_HIDE_PC_DIRECTORY, "/server/pc-directory/attribute::upnp-hide", DEFAULT_HIDE_PC_DIRECTORY),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_BOOKMARK_FILE, "/server/bookmark", DEFAULT_BOOKMARK_FILE, true, false),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT, "/server/upnp-string-limit", DEFAULT_UPNP_STRING_LIMIT, ConfigIntSetup::CheckUpnpStringLimitValue),

    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE, "/server/storage", true),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL, "/server/storage/mysql"),
#ifdef HAVE_MYSQL
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_MYSQL_ENABLED, "/server/storage/mysql/attribute::enabled", DEFAULT_MYSQL_ENABLED),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_HOST, "/server/storage/mysql/host", DEFAULT_MYSQL_HOST),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_STORAGE_MYSQL_PORT, "/server/storage/mysql/port", 0),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_USERNAME, "/server/storage/mysql/username", DEFAULT_MYSQL_USER),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_SOCKET, "/server/storage/mysql/socket", ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_PASSWORD, "/server/storage/mysql/password", ""),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_MYSQL_DATABASE, "/server/storage/mysql/database", DEFAULT_MYSQL_DB),
#endif
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_SQLITE, "/server/storage/sqlite3"),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_STORAGE_DRIVER, "/server/storage/driver"),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_SQLITE_ENABLED, "/server/storage/sqlite3/attribute::enabled", DEFAULT_SQLITE_ENABLED),
    std::make_shared<ConfigPathSetup>(CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE, "/server/storage/sqlite3/database-file", "", true, false),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS, "/server/storage/sqlite3/synchronous", DEFAULT_SQLITE_SYNC, ConfigIntSetup::CheckSqlLiteSyncValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_SQLITE_RESTORE, "/server/storage/sqlite3/on-error", DEFAULT_SQLITE_RESTORE, ConfigBoolSetup::CheckSqlLiteRestoreValue),
#ifdef SQLITE_BACKUP_ENABLED
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED, "/server/storage/sqlite3/backup/attribute::enabled", YES),
#else
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED, "/server/storage/sqlite3/backup/attribute::enabled", DEFAULT_SQLITE_BACKUP_ENABLED),
#endif
    std::make_shared<ConfigIntSetup>(CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL, "/server/storage/sqlite3/backup/attribute::interval", DEFAULT_SQLITE_BACKUP_INTERVAL, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_ENABLED, "/server/ui/attribute::enabled", DEFAULT_UI_EN_VALUE),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_UI_POLL_INTERVAL, "/server/ui/attribute::poll-interval", DEFAULT_POLL_INTERVAL, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_POLL_WHEN_IDLE, "/server/ui/attribute::poll-when-idle", DEFAULT_POLL_WHEN_IDLE_VALUE),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_ACCOUNTS_ENABLED, "/server/ui/accounts/attribute::enabled", DEFAULT_ACCOUNTS_EN_VALUE),
    std::make_shared<ConfigDictionarySetup>(CFG_SERVER_UI_ACCOUNT_LIST, "/server/ui/accounts", ATTR_SERVER_UI_ACCOUNT_LIST_ACCOUNT, ATTR_SERVER_UI_ACCOUNT_LIST_USER, ATTR_SERVER_UI_ACCOUNT_LIST_PASSWORD),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_UI_SESSION_TIMEOUT, "/server/ui/accounts/attribute::session-timeout", DEFAULT_SESSION_TIMEOUT, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE, "/server/ui/items-per-page/attribute::default", DEFAULT_ITEMS_PER_PAGE_2, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigArraySetup>(CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN, "/server/ui/items-per-page", ConfigArraySetup::InitItemsPerPage, true),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_UI_SHOW_TOOLTIPS, "/server/ui/attribute::show-tooltips", DEFAULT_UI_SHOW_TOOLTIPS_VALUE),
    std::make_shared<ConfigClientSetup>(CFG_CLIENTS_LIST, "/clients"),
    std::make_shared<ConfigBoolSetup>(CFG_CLIENTS_LIST_ENABLED, "/clients/attribute::enabled", DEFAULT_CLIENTS_EN_VALUE),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_HIDDEN_FILES, "/import/attribute::hidden-files", DEFAULT_HIDDEN_FILES_VALUE),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_FOLLOW_SYMLINKS, "/import/attribute::follow-symlinks", DEFAULT_FOLLOW_SYMLINKS_VALUE),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS, "/import/mappings/extension-mimetype/attribute::ignore-unknown", DEFAULT_IGNORE_UNKNOWN_EXTENSIONS),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE, "/import/mappings/extension-mimetype/attribute::case-sensitive", DEFAULT_CASE_SENSITIVE_EXTENSION_MAPPINGS),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, "/import/mappings/extension-mimetype", ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST, "/import/mappings/mimetype-upnpclass", ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, ATTR_IMPORT_MAPPINGS_MIMETYPE_TO),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST, "/import/mappings/mimetype-contenttype", ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT, ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE, ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_AS),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_LAYOUT_PARENT_PATH, "/import/layout/attribute::parent-path", DEFAULT_IMPORT_LAYOUT_PARENT_PATH),
    std::make_shared<ConfigDictionarySetup>(CFG_IMPORT_LAYOUT_MAPPING, "/import/layout", ATTR_IMPORT_LAYOUT_MAPPING_PATH, ATTR_IMPORT_LAYOUT_MAPPING_FROM, ATTR_IMPORT_LAYOUT_MAPPING_TO),
#ifdef HAVE_JS
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_CHARSET, "/import/scripting/attribute::script-charset", DEFAULT_JS_CHARSET),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_COMMON_SCRIPT, "/import/scripting/common-script", "", true),
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT, "/import/scripting/playlist-script", "", true),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS, "/import/scripting/playlist-script/attribute::create-link", DEFAULT_PLAYLIST_CREATE_LINK),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT, "/import/scripting/virtual-layout/import-script"),
#endif // JS
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_FILESYSTEM_CHARSET, "/import/filesystem-charset", DEFAULT_FILESYSTEM_CHARSET),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_METADATA_CHARSET, "/import/metadata-charset", DEFAULT_FILESYSTEM_CHARSET),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_PLAYLIST_CHARSET, "/import/playlist-charset", DEFAULT_FILESYSTEM_CHARSET),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE, "/import/scripting/virtual-layout/attribute::type", DEFAULT_LAYOUT_TYPE, ConfigStringSetup::CheckLayoutTypeValue),
    std::make_shared<ConfigBoolSetup>(CFG_TRANSCODING_TRANSCODING_ENABLED, "/transcoding/attribute::enabled", DEFAULT_TRANSCODING_ENABLED),
    std::make_shared<ConfigTranscodingSetup>(CFG_TRANSCODING_PROFILE_LIST, "/transcoding"),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_LIBOPTS_ENTRY_SEP, "/import/library-options/attribute::multi-value-separator", DEFAULT_LIBOPTS_ENTRY_SEPARATOR),
    std::make_shared<ConfigStringSetup>(CFG_IMPORT_LIBOPTS_ENTRY_LEGACY_SEP, "/import/library-options/attribute::legacy-value-separator", ""),
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_RESOURCES_CASE_SENSITIVE, "/import/resources/attribute::case-sensitive", DEFAULT_RESOURCES_CASE_SENSITIVE),
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_RESOURCES_FANART_FILE_LIST, "/import/resources/fanart", ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME),
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST, "/import/resources/subtitle", ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME),
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST, "/import/resources/resource", ATTR_IMPORT_RESOURCES_ADD_FILE, ATTR_IMPORT_RESOURCES_NAME),

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED, "/server/extended-runtime-options/ffmpegthumbnailer/attribute::enabled", DEFAULT_FFMPEGTHUMBNAILER_ENABLED),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE, "/server/extended-runtime-options/ffmpegthumbnailer/thumbnail-size", DEFAULT_FFMPEGTHUMBNAILER_THUMBSIZE, 1, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE, "/server/extended-runtime-options/ffmpegthumbnailer/seek-percentage", DEFAULT_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE, 0, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY, "/server/extended-runtime-options/ffmpegthumbnailer/filmstrip-overlay", DEFAULT_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_WORKAROUND_BUGS, "/server/extended-runtime-options/ffmpegthumbnailer/workaround-bugs", DEFAULT_FFMPEGTHUMBNAILER_WORKAROUND_BUGS),
    std::make_shared<ConfigIntSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY, "/server/extended-runtime-options/ffmpegthumbnailer/image-quality", DEFAULT_FFMPEGTHUMBNAILER_IMAGE_QUALITY, ConfigIntSetup::CheckImageQualityValue),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED, "/server/extended-runtime-options/ffmpegthumbnailer/cache-dir/attribute::enabled", DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR, "/server/extended-runtime-options/ffmpegthumbnailer/cache-dir", DEFAULT_FFMPEGTHUMBNAILER_CACHE_DIR), // ConfigPathSetup
#endif
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED, "/server/extended-runtime-options/mark-played-items/attribute::enabled", DEFAULT_MARK_PLAYED_ITEMS_ENABLED),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND, "/server/extended-runtime-options/mark-played-items/string/attribute::mode", DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE, ConfigBoolSetup::CheckMarkPlayedValue),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING, "/server/extended-runtime-options/mark-played-items/string", false, DEFAULT_MARK_PLAYED_ITEMS_STRING, true),
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES, "/server/extended-runtime-options/mark-played-items/attribute::suppress-cds-updates", DEFAULT_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES),
    std::make_shared<ConfigArraySetup>(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST, "/server/extended-runtime-options/mark-played-items/mark", ConfigArraySetup::InitPlayedItemsMark),
#ifdef HAVE_LASTFMLIB
    std::make_shared<ConfigBoolSetup>(CFG_SERVER_EXTOPTS_LASTFM_ENABLED, "/server/extended-runtime-options/lastfm/attribute::enabled", DEFAULT_LASTFM_ENABLED),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_EXTOPTS_LASTFM_USERNAME, "/server/extended-runtime-options/lastfm/username", false, DEFAULT_LASTFM_USERNAME, true),
    std::make_shared<ConfigStringSetup>(CFG_SERVER_EXTOPTS_LASTFM_PASSWORD, "/server/extended-runtime-options/lastfm/password", false, DEFAULT_LASTFM_PASSWORD, true),
#endif
#ifdef SOPCAST
    std::make_shared<ConfigBoolSetup>(CFG_ONLINE_CONTENT_SOPCAST_ENABLED, "/import/online-content/SopCast/attribute::enabled", DEFAULT_SOPCAST_ENABLED),
    std::make_shared<ConfigIntSetup>(CFG_ONLINE_CONTENT_SOPCAST_REFRESH, "/import/online-content/SopCast/attribute::refresh", 0),
    std::make_shared<ConfigBoolSetup>(CFG_ONLINE_CONTENT_SOPCAST_UPDATE_AT_START, "/import/online-content/SopCast/attribute::update-at-start", DEFAULT_SOPCAST_UPDATE_AT_START),
    std::make_shared<ConfigIntSetup>(CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER, "/import/online-content/SopCast/attribute::purge-after", 0),
#endif
#ifdef ATRAILERS
    std::make_shared<ConfigBoolSetup>(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED, "/import/online-content/AppleTrailers/attribute::enabled", DEFAULT_ATRAILERS_ENABLED),
    std::make_shared<ConfigIntSetup>(CFG_ONLINE_CONTENT_ATRAILERS_REFRESH, "/import/online-content/AppleTrailers/attribute::refresh", DEFAULT_ATRAILERS_REFRESH),
    std::make_shared<ConfigBoolSetup>(CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START, "/import/online-content/AppleTrailers/attribute::update-at-start", DEFAULT_ATRAILERS_UPDATE_AT_START),
    std::make_shared<ConfigIntSetup>(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER, "/import/online-content/AppleTrailers/attribute::purge-after", DEFAULT_ATRAILERS_REFRESH),
    std::make_shared<ConfigStringSetup>(CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION, "/import/online-content/AppleTrailers/attribute::resolution", std::to_string(DEFAULT_ATRAILERS_RESOLUTION).c_str(), ConfigStringSetup::CheckAtrailResValue),
#endif
    std::make_shared<ConfigBoolSetup>(CFG_IMPORT_AUTOSCAN_USE_INOTIFY, "/import/autoscan/attribute::use-inotify", "auto", ConfigBoolSetup::CheckInotifyValue),
#ifdef HAVE_INOTIFY
    std::make_shared<ConfigAutoscanSetup>(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST, "/import/autoscan", ScanMode::INotify),
#endif
#ifdef HAVE_CURL
    std::make_shared<ConfigIntSetup>(CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE, "/transcoding/attribute::fetch-buffer-size", DEFAULT_CURL_BUFFER_SIZE, CURL_MAX_WRITE_SIZE, ConfigIntSetup::CheckMinValue),
    std::make_shared<ConfigIntSetup>(CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE, "/transcoding/attribute::fetch-buffer-fill-size", DEFAULT_CURL_INITIAL_FILL_SIZE, 0, ConfigIntSetup::CheckMinValue),
#endif //HAVE_CURL
#ifdef HAVE_LIBEXIF
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST, "/import/library-options/libexif/auxdata", ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG),
#endif
#ifdef HAVE_EXIV2
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST, "/import/library-options/exiv2/auxdata", ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG),
#endif
#ifdef HAVE_TAGLIB
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST, "/import/library-options/id3/auxdata", ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG),
#endif
#ifdef HAVE_FFMPEG
    std::make_shared<ConfigArraySetup>(CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST, "/import/library-options/ffmpeg/auxdata", ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, ATTR_IMPORT_LIBOPTS_AUXDATA_TAG),
#endif
#ifdef HAVE_MAGIC
    std::make_shared<ConfigPathSetup>(CFG_IMPORT_MAGIC_FILE, "/import/magic-file", ""),
#endif
    std::make_shared<ConfigAutoscanSetup>(CFG_IMPORT_AUTOSCAN_TIMED_LIST, "/import/autoscan", ScanMode::Timed),
};

static std::map<config_option_t, const char*> simpleOptions = {
    { CFG_MAX, "max_option" },
    { ATTR_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN_OPTION, "option" },
    { ATTR_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT, "content" },
    { ATTR_SERVER_UI_ACCOUNT_LIST_ACCOUNT, "acount" },
    { ATTR_SERVER_UI_ACCOUNT_LIST_USER, "user" },
    { ATTR_SERVER_UI_ACCOUNT_LIST_PASSWORD, "password" },
    { ATTR_IMPORT_MAPPINGS_MIMETYPE_MAP, "map" },
    { ATTR_IMPORT_MAPPINGS_MIMETYPE_FROM, "from" },
    { ATTR_IMPORT_MAPPINGS_MIMETYPE_TO, "to" },
    { ATTR_IMPORT_RESOURCES_ADD_FILE, "add-file" },
    { ATTR_IMPORT_RESOURCES_NAME, "name" },
    { ATTR_IMPORT_LIBOPTS_AUXDATA_DATA, "add-data" },
    { ATTR_IMPORT_LIBOPTS_AUXDATA_TAG, "tag" },

    { ATTR_TRANSCODING_MIMETYPE_PROF_MAP, "mimetype-profile-mappings" },
    { ATTR_TRANSCODING_MIMETYPE_PROF_MAP_TRANSCODE, "transcode" },
    { ATTR_TRANSCODING_MIMETYPE_PROF_MAP_MIMETYPE, "mimetype" },
    { ATTR_TRANSCODING_MIMETYPE_PROF_MAP_USING, "using" },
    { ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_TREAT, "treat" },
    { ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_MIMETYPE, "mimetype" },
    { ATTR_IMPORT_MAPPINGS_M2CTYPE_LIST_AS, "as" },
    { ATTR_IMPORT_LAYOUT_MAPPING_PATH, "path" },
    { ATTR_IMPORT_LAYOUT_MAPPING_FROM, "from" },
    { ATTR_IMPORT_LAYOUT_MAPPING_TO, "to" },
    { ATTR_TRANSCODING_PROFILES, "profiles" },
    { ATTR_TRANSCODING_PROFILES_PROFLE, "profile" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_ENABLED, "enabled" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_TYPE, "type" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_NAME, "name" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_MIMETYPE, "mimetype" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_RES, "resolution" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC, "avi-fourcc-list" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_MODE, "mode" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_AVI4CC_4CC, "fourcc" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_ACCURL, "accept-url" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_SAMPFREQ, "sample-frequency" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_NRCHAN, "audio-channels" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_HIDEORIG, "hide-original-resource" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_THUMB, "thumbnail" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_FIRST, "first-resource" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_ACCOGG, "accept-ogg-theora" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_USECHUNKEDENC, "use-chunked-encoding" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_AGENT, "agent" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_COMMAND, "command" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_AGENT_ARGS, "arguments" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER, "buffer" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_SIZE, "size" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_CHUNK, "chunk-size" },
    { ATTR_TRANSCODING_PROFILES_PROFLE_BUFFER_FILL, "fill-size" },

    { ATTR_AUTOSCAN_DIRECTORY, "directory" },
    { ATTR_AUTOSCAN_DIRECTORY_LOCATION, "location" },
    { ATTR_AUTOSCAN_DIRECTORY_MODE, "mode" },
    { ATTR_AUTOSCAN_DIRECTORY_INTERVAL, "interval" },
    { ATTR_AUTOSCAN_DIRECTORY_RECURSIVE, "recursive" },
    { ATTR_AUTOSCAN_DIRECTORY_HIDDENFILES, "hidden-files" },

    { ATTR_CLIENTS_CLIENT, "client" },
    { ATTR_CLIENTS_CLIENT_FLAGS, "flags" },
    { ATTR_CLIENTS_CLIENT_IP, "ip" },
    { ATTR_CLIENTS_CLIENT_USERAGENT, "userAgent" },
};

const char* ConfigManager::mapConfigOption(config_option_t option)
{
    auto co = std::find_if(simpleOptions.begin(), simpleOptions.end(), [&](const auto& s) { return s.first == option; });
    if (co != simpleOptions.end()) {
        return (*co).second;
    }

    auto co2 = std::find_if(complexOptions.begin(), complexOptions.end(), [&](const auto& s) { return s->option == option; });
    if (co2 != complexOptions.end()) {
        return (*co2)->xpath;
    }
    return "";
}

std::shared_ptr<ConfigSetup> findConfigSetup(config_option_t option)
{
    auto co = std::find_if(complexOptions.begin(), complexOptions.end(), [&](const auto& s) { return s->option == option; });
    if (co != complexOptions.end()) {
        log_debug("Config: option found: '{}'", (*co)->xpath);
        return *co;
    }

    throw std::runtime_error(fmt::format("Error in config code: {} tag not found", static_cast<int>(option)));
}

std::shared_ptr<ConfigOption> ConfigManager::setOption(const pugi::xml_node& root, config_option_t option, const std::map<std::string, std::string>* arguments)
{
    auto co = findConfigSetup(option);
    co->makeOption(root, *options, arguments);
    log_debug("Config: option set: '{}'", co->xpath);
    return co->getValue();
}

void ConfigManager::load(const fs::path& filename, const fs::path& userHome)
{
    std::string temp;
    pugi::xml_node tmpEl;

    std::map<std::string, std::string> args;
    std::shared_ptr<ConfigSetup> co;

    log_info("Loading configuration from: {}", filename.c_str());
    this->filename = filename;
    pugi::xml_parse_result result = xmlDoc->load_file(filename.c_str());
    if (result.status != pugi::xml_parse_status::status_ok) {
        throw ConfigParseException(result.description());
    }

    log_info("Checking configuration...");

    auto root = xmlDoc->document_element();

    // first check if the config file itself looks ok, it must have a config
    // and a server tag
    if (std::string(root.name()) != ConfigSetup::ROOT_NAME)
        throw std::runtime_error("Error in config file: <config> tag not found");

    if (root.child("server") == nullptr)
        throw std::runtime_error("Error in config file: <server> tag not found");

    std::string version = root.attribute("version").as_string();
    if (std::stoi(version) > CONFIG_XML_VERSION)
        throw std::runtime_error("Config version \"" + version + "\" does not yet exist");

    // now go through the mandatory parameters, if something is missing
    // we will not start the server
    co = findConfigSetup(CFG_SERVER_HOME);
    if (!userHome.empty()) {
        // respect command line; ignore xml value
        temp = userHome;
    } else {
        temp = co->getXmlContent(root);
    }

    if (!fs::is_directory(temp))
        throw std::runtime_error(fmt::format("Directory '{}' does not exist", temp));
    co->makeOption(temp, *options);
    ConfigPathSetup::Home = temp;

    setOption(root, CFG_SERVER_WEBROOT);
    setOption(root, CFG_SERVER_TMPDIR);
    setOption(root, CFG_SERVER_SERVEDIR);

    // udn should be already prepared
    setOption(root, CFG_SERVER_UDN);

    // checking database driver options
    bool mysql_en = false;
    bool sqlite3_en = false;

    co = findConfigSetup(CFG_SERVER_STORAGE);
    co->getXmlElement(root); // fails if missing

    co = findConfigSetup(CFG_SERVER_STORAGE_MYSQL);
    if (co->hasXmlElement(root)) {
        mysql_en = setOption(root, CFG_SERVER_STORAGE_MYSQL_ENABLED)->getBoolOption();
    }

    co = findConfigSetup(CFG_SERVER_STORAGE_SQLITE);
    if (co->hasXmlElement(root)) {
        sqlite3_en = setOption(root, CFG_SERVER_STORAGE_SQLITE_ENABLED)->getBoolOption();
    }

    if (sqlite3_en && mysql_en)
        throw std::runtime_error("You enabled both, sqlite3 and mysql but "
                                 "only one database driver may be active at a time");

    if (!sqlite3_en && !mysql_en)
        throw std::runtime_error("You disabled both, sqlite3 and mysql but "
                                 "one database driver must be active");

#ifdef HAVE_MYSQL
    if (mysql_en) {
        setOption(root, CFG_SERVER_STORAGE_MYSQL_HOST);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_DATABASE);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_USERNAME);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_PORT);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_SOCKET);
        setOption(root, CFG_SERVER_STORAGE_MYSQL_PASSWORD);
    }
#else
    if (mysql_en) {
        throw std::runtime_error("You enabled MySQL storage in configuration, "
                                 "however this version of Gerbera was compiled "
                                 "without MySQL support!");
    }
#endif // HAVE_MYSQL

    if (sqlite3_en) {
        setOption(root, CFG_SERVER_STORAGE_SQLITE_DATABASE_FILE);
        setOption(root, CFG_SERVER_STORAGE_SQLITE_SYNCHRONOUS);
        setOption(root, CFG_SERVER_STORAGE_SQLITE_RESTORE);
        setOption(root, CFG_SERVER_STORAGE_SQLITE_BACKUP_ENABLED);
        setOption(root, CFG_SERVER_STORAGE_SQLITE_BACKUP_INTERVAL);
    }

    std::string dbDriver;
    if (sqlite3_en)
        dbDriver = "sqlite3";
    if (mysql_en)
        dbDriver = "mysql";

    co = findConfigSetup(CFG_SERVER_STORAGE_DRIVER);
    co->makeOption(dbDriver, *options);

    // now go through the optional settings and fix them if anything is missing
    setOption(root, CFG_SERVER_UI_ENABLED);
    setOption(root, CFG_SERVER_UI_SHOW_TOOLTIPS);
    setOption(root, CFG_SERVER_UI_POLL_WHEN_IDLE);
    setOption(root, CFG_SERVER_UI_POLL_INTERVAL);

    auto def_ipp = setOption(root, CFG_SERVER_UI_DEFAULT_ITEMS_PER_PAGE)->getIntOption();

    // now get the option list for the drop down menu
    auto menu_opts = setOption(root, CFG_SERVER_UI_ITEMS_PER_PAGE_DROPDOWN)->getArrayOption();
    if (std::find_if(menu_opts.begin(), menu_opts.end(), [=](const auto& s) { return s == std::to_string(def_ipp); }) == menu_opts.end())
        throw std::runtime_error("Error in config file: at least one <option> "
                                 "under <items-per-page> must match the "
                                 "<items-per-page default=\"\" /> attribute");
    setOption(root, CFG_SERVER_UI_ACCOUNTS_ENABLED);
    setOption(root, CFG_SERVER_UI_ACCOUNT_LIST);
    setOption(root, CFG_SERVER_UI_SESSION_TIMEOUT);

    bool cl_en = setOption(root, CFG_CLIENTS_LIST_ENABLED)->getBoolOption();
    args["isEnabled"] = cl_en ? "true" : "false";
    setOption(root, CFG_CLIENTS_LIST, &args);
    args.clear();

    setOption(root, CFG_IMPORT_HIDDEN_FILES);
    setOption(root, CFG_IMPORT_FOLLOW_SYMLINKS);
    setOption(root, CFG_IMPORT_MAPPINGS_IGNORE_UNKNOWN_EXTENSIONS);
    bool csens = setOption(root, CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_CASE_SENSITIVE)->getBoolOption();
    args["tolower"] = std::to_string(!csens);
    setOption(root, CFG_IMPORT_MAPPINGS_EXTENSION_TO_MIMETYPE_LIST, &args);
    args.clear();
    setOption(root, CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    setOption(root, CFG_IMPORT_LAYOUT_PARENT_PATH);
    setOption(root, CFG_IMPORT_LAYOUT_MAPPING);

#if defined(HAVE_NL_LANGINFO) && defined(HAVE_SETLOCALE)
    if (setlocale(LC_ALL, "") != nullptr) {
        temp = nl_langinfo(CODESET);
        log_debug("received {} from nl_langinfo", temp.c_str());
    }

    if (temp.empty())
        temp = DEFAULT_FILESYSTEM_CHARSET;
#else
    temp = DEFAULT_FILESYSTEM_CHARSET;
#endif
    // check if the one we take as default is actually available
    co = findConfigSetup(CFG_IMPORT_FILESYSTEM_CHARSET);
    try {
        auto conv = std::make_unique<StringConverter>(temp,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        temp = DEFAULT_FALLBACK_CHARSET;
    }
    co->setDefaultValue(temp);
    std::string charset = co->getXmlContent(root);
    try {
        auto conv = std::make_unique<StringConverter>(charset,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported filesystem-charset specified: " + charset);
    }
    log_info("Setting filesystem import charset to {}", charset.c_str());
    co->makeOption(charset, *options);

    co = findConfigSetup(CFG_IMPORT_METADATA_CHARSET);
    co->setDefaultValue(temp);
    charset = co->getXmlContent(root);
    try {
        auto conv = std::make_unique<StringConverter>(charset,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported metadata-charset specified: " + charset);
    }
    log_info("Setting metadata import charset to {}", charset.c_str());
    co->makeOption(charset, *options);

    co = findConfigSetup(CFG_IMPORT_PLAYLIST_CHARSET);
    co->setDefaultValue(temp);
    charset = co->getXmlContent(root);
    try {
        auto conv = std::make_unique<StringConverter>(charset,
            DEFAULT_INTERNAL_CHARSET);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Error in config file: unsupported playlist-charset specified: " + charset);
    }
    log_info("Setting playlist charset to {}", charset.c_str());
    co->makeOption(charset, *options);

    setOption(root, CFG_SERVER_HIDE_PC_DIRECTORY);

    co = findConfigSetup(CFG_SERVER_NETWORK_INTERFACE);
    if (interface.empty()) {
        temp = co->getXmlContent(root);
    } else {
        temp = interface;
    }
    co->makeOption(temp, *options);

    co = findConfigSetup(CFG_SERVER_IP);
    if (ip.empty()) {
        temp = co->getXmlContent(root); // bind to any IP address
    } else {
        temp = ip;
    }
    co->makeOption(temp, *options);

    if (!getOption(CFG_SERVER_NETWORK_INTERFACE).empty() && !getOption(CFG_SERVER_IP).empty())
        throw std::runtime_error("Error in config file: you can not specify interface and ip at the same time");

    setOption(root, CFG_SERVER_BOOKMARK_FILE);
    setOption(root, CFG_SERVER_NAME);
    setOption(root, CFG_SERVER_MODEL_NAME);
    setOption(root, CFG_SERVER_MODEL_DESCRIPTION);
    setOption(root, CFG_SERVER_MODEL_NUMBER);
    setOption(root, CFG_SERVER_MODEL_URL);
    setOption(root, CFG_SERVER_SERIAL_NUMBER);
    setOption(root, CFG_SERVER_MANUFACTURER);
    setOption(root, CFG_SERVER_MANUFACTURER_URL);
    setOption(root, CFG_SERVER_PRESENTATION_URL);
    setOption(root, CFG_SERVER_UPNP_TITLE_AND_DESC_STRING_LIMIT);

    temp = setOption(root, CFG_SERVER_APPEND_PRESENTATION_URL_TO)->getOption();
    if (((temp == "ip") || (temp == "port")) && getOption(CFG_SERVER_PRESENTATION_URL).empty()) {
        throw std::runtime_error("Error in config file: \"append-to\" attribute "
                                 "value in <presentationURL> tag is set to \""
            + temp + "\" but no URL is specified");
    }

#ifdef HAVE_JS
    co = findConfigSetup(CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT);
    co->setDefaultValue(prefix_dir / DEFAULT_JS_DIR / DEFAULT_PLAYLISTS_SCRIPT);
    co->makeOption(root, *options);

    co = findConfigSetup(CFG_IMPORT_SCRIPTING_COMMON_SCRIPT);
    co->setDefaultValue(prefix_dir / DEFAULT_JS_DIR / DEFAULT_COMMON_SCRIPT);
    co->makeOption(root, *options);

    setOption(root, CFG_IMPORT_SCRIPTING_PLAYLIST_SCRIPT_LINK_OBJECTS);
#endif

    auto layoutType = setOption(root, CFG_IMPORT_SCRIPTING_VIRTUAL_LAYOUT_TYPE)->getOption();

#ifndef HAVE_JS
    if (layoutType == "js")
        throw std::runtime_error("Gerbera was compiled without JS support, "
                                 "however you specified \"js\" to be used for the "
                                 "virtual-layout.");
#else
    charset = setOption(root, CFG_IMPORT_SCRIPTING_CHARSET)->getOption();
    if (layoutType == "js") {
        try {
            auto conv = std::make_unique<StringConverter>(charset,
                DEFAULT_INTERNAL_CHARSET);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error("Error in config file: unsupported import script charset specified: " + charset);
        }
    }

    co = findConfigSetup(CFG_IMPORT_SCRIPTING_IMPORT_SCRIPT);
    args["isFile"] = std::to_string(true);
    args["mustExist"] = std::to_string(layoutType == "js");
    args["notEmpty"] = std::to_string(layoutType == "js");
    co->setDefaultValue(prefix_dir / DEFAULT_JS_DIR / DEFAULT_IMPORT_SCRIPT);
    co->makeOption(root, *options, &args);
    args.clear();
    auto script_path = co->getValue()->getOption();

#endif
    co = findConfigSetup(CFG_SERVER_PORT);
    // 0 means, that the SDK will any free port itself
    co->makeOption((port <= 0) ? co->getXmlContent(root) : std::to_string(port), *options);

    setOption(root, CFG_SERVER_ALIVE_INTERVAL);
    setOption(root, CFG_IMPORT_MAPPINGS_MIMETYPE_TO_UPNP_CLASS_LIST);

    auto useInotify = setOption(root, CFG_IMPORT_AUTOSCAN_USE_INOTIFY)->getBoolOption();

    args["hiddenFiles"] = getBoolOption(CFG_IMPORT_HIDDEN_FILES) ? "true" : "false";
    setOption(root, CFG_IMPORT_AUTOSCAN_TIMED_LIST, &args);

#ifdef HAVE_INOTIFY
    if (useInotify) {
        setOption(root, CFG_IMPORT_AUTOSCAN_INOTIFY_LIST, &args);
    }
#endif
    args.clear();

    auto tr_en = setOption(root, CFG_TRANSCODING_TRANSCODING_ENABLED)->getBoolOption();
    args["isEnabled"] = tr_en ? "true" : "false";
    setOption(root, CFG_TRANSCODING_PROFILE_LIST, &args);
    args.clear();

#ifdef HAVE_CURL
    if (tr_en) {
        setOption(root, CFG_EXTERNAL_TRANSCODING_CURL_BUFFER_SIZE);
        setOption(root, CFG_EXTERNAL_TRANSCODING_CURL_FILL_SIZE);
    }
#endif //HAVE_CURL

    args["trim"] = "false";
    setOption(root, CFG_IMPORT_LIBOPTS_ENTRY_SEP, &args);
    setOption(root, CFG_IMPORT_LIBOPTS_ENTRY_LEGACY_SEP, &args);
    args.clear();

#ifdef HAVE_LIBEXIF
    setOption(root, CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST);
#endif // HAVE_LIBEXIF

    setOption(root, CFG_IMPORT_RESOURCES_CASE_SENSITIVE);
    setOption(root, CFG_IMPORT_RESOURCES_FANART_FILE_LIST);
    setOption(root, CFG_IMPORT_RESOURCES_SUBTITLE_FILE_LIST);
    setOption(root, CFG_IMPORT_RESOURCES_RESOURCE_FILE_LIST);

#ifdef HAVE_EXIV2
    setOption(root, CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST);
#endif // HAVE_EXIV2

#ifdef HAVE_TAGLIB
    setOption(root, CFG_IMPORT_LIBOPTS_ID3_AUXDATA_TAGS_LIST);
#endif

#ifdef HAVE_FFMPEG
    setOption(root, CFG_IMPORT_LIBOPTS_FFMPEG_AUXDATA_TAGS_LIST);
#endif

#if defined(HAVE_FFMPEG) && defined(HAVE_FFMPEGTHUMBNAILER)
    auto ffmp_en = setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_ENABLED)->getBoolOption();
    if (ffmp_en) {
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_THUMBSIZE);
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_SEEK_PERCENTAGE);
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_FILMSTRIP_OVERLAY);
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_WORKAROUND_BUGS);
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_IMAGE_QUALITY);
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR_ENABLED);
        setOption(root, CFG_SERVER_EXTOPTS_FFMPEGTHUMBNAILER_CACHE_DIR);
    }
#endif

    bool markingEnabled = setOption(root, CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED)->getBoolOption();
    setOption(root, CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);
    setOption(root, CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING_MODE_PREPEND);
    setOption(root, CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_STRING);
    bool contentArrayEmpty = setOption(root, CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST)->getArrayOption().empty();
    if (markingEnabled && contentArrayEmpty) {
        throw std::runtime_error("Error in config file: <mark-played-items>/<mark> tag must contain at least one <content> tag");
    }

#if defined(HAVE_LASTFMLIB)
    auto lfm_en = setOption(root, CFG_SERVER_EXTOPTS_LASTFM_ENABLED)->getBoolOption();
    if (lfm_en) {
        setOption(root, CFG_SERVER_EXTOPTS_LASTFM_USERNAME);
        setOption(root, CFG_SERVER_EXTOPTS_LASTFM_PASSWORD);
    }
#endif

#ifdef HAVE_MAGIC
    co = findConfigSetup(CFG_IMPORT_MAGIC_FILE);
    args["isFile"] = "true";
    args["resolveEmpty"] = "false";
    co->makeOption(!magic_file.empty() ? magic_file.string() : co->getXmlContent(root), *options, &args);
    args.clear();
#endif

#ifdef HAVE_INOTIFY
    auto config_timed_list = getAutoscanListOption(CFG_IMPORT_AUTOSCAN_TIMED_LIST);
    auto config_inotify_list = getAutoscanListOption(CFG_IMPORT_AUTOSCAN_INOTIFY_LIST);

    for (size_t i = 0; i < config_inotify_list->size(); i++) {
        auto i_dir = config_inotify_list->get(i);
        for (size_t j = 0; j < config_timed_list->size(); j++) {
            auto t_dir = config_timed_list->get(j);
            if (i_dir->getLocation() == t_dir->getLocation())
                throw std::runtime_error("Error in config file: same path used in both inotify and timed scan modes");
        }
    }
#endif

#ifdef SOPCAST
    setOption(root, CFG_ONLINE_CONTENT_SOPCAST_ENABLED);

    int sopcast_refresh = setOption(root, CFG_ONLINE_CONTENT_SOPCAST_REFRESH)->getIntOption();
    int sopcast_purge = setOption(root, CFG_ONLINE_CONTENT_SOPCAST_PURGE_AFTER)->getIntOption();

    if (sopcast_refresh >= sopcast_purge) {
        if (sopcast_purge != 0)
            throw std::runtime_error("Error in config file: SopCast purge-after value must be greater than refresh interval");
    }

    setOption(root, CFG_ONLINE_CONTENT_SOPCAST_UPDATE_AT_START);
#endif

#ifdef ATRAILERS
    setOption(root, CFG_ONLINE_CONTENT_ATRAILERS_ENABLED);
    int atrailers_refresh = setOption(root, CFG_ONLINE_CONTENT_ATRAILERS_REFRESH)->getIntOption();

    co = findConfigSetup(CFG_ONLINE_CONTENT_ATRAILERS_PURGE_AFTER);
    co->makeOption(std::to_string(atrailers_refresh), *options);

    setOption(root, CFG_ONLINE_CONTENT_ATRAILERS_UPDATE_AT_START);
    setOption(root, CFG_ONLINE_CONTENT_ATRAILERS_RESOLUTION);
#endif

    log_info("Configuration check succeeded.");

    std::ostringstream buf;
    xmlDoc->print(buf, "  ");
    log_debug("Config file dump after validation: {}", buf.str().c_str());
}

void ConfigManager::dumpOptions()
{
#ifdef TOMBDEBUG
    log_debug("Dumping options!");
    for (int i = 0; i < static_cast<int>(CFG_MAX); i++) {
        try {
            std::string opt = getOption(static_cast<config_option_t>(i));
            log_debug("    Option {:02d} {}", i, opt.c_str());
        } catch (const std::runtime_error& e) {
        }
        try {
            int opt = getIntOption(static_cast<config_option_t>(i));
            log_debug(" IntOption {:02d} {}", i, opt);
        } catch (const std::runtime_error& e) {
        }
        try {
            bool opt = getBoolOption(static_cast<config_option_t>(i));
            log_debug("BoolOption {:02d} {}", i, opt ? "true" : "false");
        } catch (const std::runtime_error& e) {
        }
    }
#endif
}

// The validate function ensures that the array is completely filled!
std::string ConfigManager::getOption(config_option_t option)
{
    auto o = options->at(option);
    if (o == nullptr) {
        throw std::runtime_error("option not set");
    }
    return o->getOption();
}

int ConfigManager::getIntOption(config_option_t option)
{
    auto o = options->at(option);
    if (o == nullptr) {
        throw std::runtime_error("option not set");
    }
    return o->getIntOption();
}

bool ConfigManager::getBoolOption(config_option_t option)
{
    auto o = options->at(option);
    if (o == nullptr) {
        throw std::runtime_error("option not set");
    }
    return o->getBoolOption();
}

std::map<std::string, std::string> ConfigManager::getDictionaryOption(config_option_t option)
{
    return options->at(option)->getDictionaryOption();
}

std::vector<std::string> ConfigManager::getArrayOption(config_option_t option)
{
    return options->at(option)->getArrayOption();
}

std::shared_ptr<AutoscanList> ConfigManager::getAutoscanListOption(config_option_t option)
{
    return options->at(option)->getAutoscanListOption();
}

std::shared_ptr<ClientConfigList> ConfigManager::getClientConfigListOption(config_option_t option)
{
    return options->at(option)->getClientConfigListOption();
}

std::shared_ptr<TranscodingProfileList> ConfigManager::getTranscodingProfileListOption(config_option_t option)
{
    return options->at(option)->getTranscodingProfileListOption();
}
