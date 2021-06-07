/*GRB*

    Gerbera - https://gerbera.io/

    config_setup.h - this file is part of Gerbera.
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

/// \file config_setup.h
///\brief Definitions of the ConfigSetup classes.

#ifndef __CONFIG_SETUP_H__
#define __CONFIG_SETUP_H__

#include <filesystem>
#include <map>
#include <memory>
#include <pugixml.hpp>

#include "common.h"
#include "config.h"
#include "config_manager.h"
#include "config_options.h"
#include "content/autoscan.h"
#include "content/autoscan_list.h"

class ConfigOption;
class DirectoryTweak;
enum class ScanMode;

using StringCheckFunction = bool (*)(std::string& value);
using ArrayInitFunction = bool (*)(const pugi::xml_node& value, std::vector<std::string>& result, const char* node_name);
using DictionaryInitFunction = bool (*)(const pugi::xml_node& value, std::map<std::string, std::string>& result);
using IntCheckFunction = bool (*)(int value);
using IntMinFunction = bool (*)(int value, int minValue);

class ConfigValue {
public:
    std::string key;
    std::string item;
    std::string value;
    std::string status;
};

static constexpr auto STATUS_REMOVED = std::string_view("removed");
static constexpr auto STATUS_RESET = std::string_view("reset");
static constexpr auto STATUS_ADDED = std::string_view("added");
static constexpr auto STATUS_CHANGED = std::string_view("changed");
static constexpr auto STATUS_UNCHANGED = std::string_view("unchanged");
static constexpr auto STATUS_KILLED = std::string_view("killed");
static constexpr auto STATUS_MANUAL = std::string_view("manual");

class ConfigSetup {
protected:
    std::shared_ptr<ConfigOption> optionValue;
    std::string cpath;
    bool required = false;
    bool useDefault = false;
    StringCheckFunction rawCheck;
    std::string defaultValue;
    const char* help;

    static size_t extractIndex(const std::string& item);

    void setOption(const std::shared_ptr<Config>& config)
    {
        config->addOption(option, optionValue);
    }
    static std::string buildCpath(const char* xpath)
    {
        std::string cPath(xpath);
        if (cPath[0] != '/') {
            cPath = cPath.insert(0, "/");
        }
        cPath = cPath.insert(0, ROOT_NAME);
        cPath = cPath.insert(0, "/");
        return cPath;
    }

public:
    static constexpr auto ROOT_NAME = std::string_view("config");

    pugi::xpath_node_set getXmlTree(const pugi::xml_node& element) const;

    config_option_t option;
    const char* xpath;

    ConfigSetup(config_option_t option, const char* xpath, const char* help, bool required = false, const char* defaultValue = "")
        : cpath(buildCpath(xpath))
        , required(required)
        , rawCheck(nullptr)
        , defaultValue(defaultValue)
        , help(help)
        , option(option)
        , xpath(xpath)
    {
    }

    ConfigSetup(config_option_t option, const char* xpath, const char* help, StringCheckFunction check, const char* defaultValue = "", bool required = false)
        : cpath(buildCpath(xpath))
        , required(required)
        , rawCheck(check)
        , defaultValue(defaultValue)
        , help(help)
        , option(option)
        , xpath(xpath)
    {
    }

    virtual ~ConfigSetup() = default;

    void setDefaultValue(const std::string& defaultValue)
    {
        this->defaultValue.assign(defaultValue);
    }
    std::string getDefaultValue() const
    {
        return this->defaultValue;
    }
    bool isDefaultValueUsed() const
    {
        return this->useDefault;
    }

    bool checkValue(std::string& optValue) const
    {
        return (rawCheck != nullptr && !rawCheck(optValue)) ? false : true;
    }
    const char* getHelp() const { return help; }
    std::shared_ptr<ConfigOption> getValue() const { return optionValue; }
    virtual std::string getTypeString() const { return "Unset"; }

    pugi::xml_node getXmlElement(const pugi::xml_node& root) const;

    bool hasXmlElement(const pugi::xml_node& root) const;

    /// \brief Returns a config option with the given xpath, if option does not exist a default value is returned.
    /// \param xpath option xpath
    /// \param defaultValue default value if option not found
    ///
    /// The xpath parameter has XPath syntax:
    /// "/path/to/option" will return the text value of the given "option" element
    /// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
    std::string getXmlContent(const pugi::xml_node& root, bool trim = true);

    virtual void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr);

    virtual void makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr);

    virtual bool updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) { return false; }

    virtual std::string getItemPath(int index = 0, config_option_t propOption = CFG_MAX, config_option_t propOption2 = CFG_MAX, config_option_t propOption3 = CFG_MAX, config_option_t propOption4 = CFG_MAX) const { return xpath; }

    virtual std::string getUniquePath() const { return xpath; }
    virtual std::string getCurrentValue() const { return optionValue == nullptr ? "" : optionValue->getOption(); }
};

class ConfigStringSetup : public ConfigSetup {
protected:
    bool notEmpty = false;

public:
    ConfigStringSetup(config_option_t option, const char* xpath, const char* help, bool required = false, const char* defaultValue = "", bool notEmpty = false)
        : ConfigSetup(option, xpath, help, required, defaultValue)
        , notEmpty(notEmpty)
    {
    }

    ConfigStringSetup(config_option_t option, const char* xpath, const char* help, const char* defaultValue, StringCheckFunction check = nullptr, bool notEmpty = false, bool required = false)
        : ConfigSetup(option, xpath, help, check, defaultValue, required)
        , notEmpty(notEmpty)
    {
    }

    std::string getTypeString() const override { return "String"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::shared_ptr<ConfigOption> newOption(const std::string& optValue);
};

template <class En>
class ConfigEnumSetup : public ConfigSetup {
protected:
    bool notEmpty = true;
    std::map<std::string, En> valueMap;

public:
    ConfigEnumSetup(config_option_t option, const char* xpath, const char* help, std::map<std::string, En> valueMap, bool notEmpty = false)
        : ConfigSetup(option, xpath, help, false, "")
        , notEmpty(notEmpty)
        , valueMap(std::move(valueMap))
    {
    }

    ConfigEnumSetup(config_option_t option, const char* xpath, const char* help, const char* defaultValue, std::map<std::string, En> valueMap, bool notEmpty = false)
        : ConfigSetup(option, xpath, help, false, defaultValue)
        , notEmpty(notEmpty)
        , valueMap(std::move(valueMap))
    {
    }

    std::string getTypeString() const override { return "Enum"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        if (arguments != nullptr && arguments->find("notEmpty") != arguments->end()) {
            notEmpty = arguments->find("notEmpty")->second == "true";
        }
        newOption(ConfigSetup::getXmlContent(root, true));
        setOption(config);
    }

    bool checkEnumValue(const std::string& value, En& result) const
    {
        if (valueMap.find(value) != valueMap.end()) {
            result = valueMap.at(value);
            return true;
        }
        return false;
    }

    En getXmlContent(const pugi::xml_node& root)
    {
        std::string optValue = ConfigSetup::getXmlContent(root, true);
        log_debug("Config: option: '{}' value: '{}'", xpath, optValue.c_str());
        if (notEmpty && optValue.empty()) {
            throw_std_runtime_error("Error in config file: Invalid {}/{} empty value '{}'", root.path(), xpath, optValue);
        }
        En result;
        if (!checkEnumValue(optValue, result)) {
            throw_std_runtime_error("Error in config file: {}/{} unsupported Enum value '{}'", root.path(), xpath, optValue);
        }
        return result;
    }

    std::shared_ptr<ConfigOption> newOption(const std::string& optValue)
    {
        if (notEmpty && optValue.empty()) {
            throw_std_runtime_error("Invalid {} empty value '{}'", xpath, optValue);
        }
        En result;
        if (!checkEnumValue(optValue, result)) {
            throw_std_runtime_error("Error in config file: {} unsupported Enum value '{}'", xpath, optValue);
        }
        optionValue = std::make_shared<Option>(optValue);
        return optionValue;
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
    fs::path resolvePath(fs::path path) const;

    void loadArguments(const std::map<std::string, std::string>* arguments = nullptr);

public:
    static fs::path Home;

    ConfigPathSetup(config_option_t option, const char* xpath, const char* help, const char* defaultValue = "", bool isFile = false, bool mustExist = true, bool notEmpty = false)
        : ConfigSetup(option, xpath, help, nullptr, defaultValue)
        , isFile(isFile)
        , mustExist(mustExist)
        , notEmpty(notEmpty)
    {
    }

    std::string getTypeString() const override { return "Path"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    void makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::shared_ptr<ConfigOption> newOption(std::string& optValue);

    bool checkPathValue(std::string& optValue, std::string& pathValue) const;

    static bool checkAgentPath(std::string& optValue);
};

class ConfigIntSetup : public ConfigSetup {
protected:
    IntCheckFunction valueCheck = nullptr;
    IntMinFunction minCheck = nullptr;
    int minValue {};

public:
    ConfigIntSetup(config_option_t option, const char* xpath, const char* help)
        : ConfigSetup(option, xpath, help)

    {
        this->defaultValue = fmt::to_string(0);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* help, int defaultValue)
        : ConfigSetup(option, xpath, help)

    {
        this->defaultValue = fmt::to_string(defaultValue);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* help, IntCheckFunction check)
        : ConfigSetup(option, xpath, help)
        , valueCheck(check)

    {
        this->defaultValue = fmt::to_string(0);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* help, int defaultValue, IntCheckFunction check)
        : ConfigSetup(option, xpath, help)
        , valueCheck(check)

    {
        this->defaultValue = fmt::to_string(defaultValue);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* help, int defaultValue, int minValue, IntMinFunction check)
        : ConfigSetup(option, xpath, help)
        , minCheck(check)
        , minValue(minValue)
    {
        this->defaultValue = fmt::to_string(defaultValue);
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* help, const char* defaultValue, IntCheckFunction check = nullptr)
        : ConfigSetup(option, xpath, help)
        , valueCheck(check)

    {
        this->defaultValue = defaultValue;
    }

    ConfigIntSetup(config_option_t option, const char* xpath, const char* help, const char* defaultValue, StringCheckFunction check)
        : ConfigSetup(option, xpath, help, check, defaultValue)

    {
    }

    std::string getTypeString() const override { return "Number"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    void makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    int getXmlContent(const pugi::xml_node& root);

    std::shared_ptr<ConfigOption> newOption(int optValue);

    int checkIntValue(std::string& sVal, const std::string& pathName = "") const;

    std::string getCurrentValue() const override { return optionValue == nullptr ? "" : fmt::format("{}", optionValue->getIntOption()); }

    static bool CheckSqlLiteSyncValue(std::string& value);

    static bool CheckProfileNumberValue(std::string& value);

    static bool CheckMinValue(int value, int minValue);

    static bool CheckImageQualityValue(int value);

    static bool CheckPortValue(int value);

    static bool CheckUpnpStringLimitValue(int value);
};

class ConfigBoolSetup : public ConfigSetup {
public:
    ConfigBoolSetup(config_option_t option, const char* xpath, const char* help)
        : ConfigSetup(option, xpath, help)
    {
    }

    ConfigBoolSetup(config_option_t option, const char* xpath, const char* help, const char* defaultValue, StringCheckFunction check = nullptr)
        : ConfigSetup(option, xpath, help, check, defaultValue)
    {
    }

    ConfigBoolSetup(config_option_t option, const char* xpath, const char* help, bool defaultValue, StringCheckFunction check = nullptr)
        : ConfigSetup(option, xpath, help, check)
    {
        this->defaultValue = defaultValue ? YES : NO;
    }

    ConfigBoolSetup(config_option_t option, const char* xpath, const char* help, bool defaultValue, bool required)
        : ConfigSetup(option, xpath, help, required)
    {
        this->defaultValue = defaultValue ? YES : NO;
    }

    std::string getTypeString() const override { return "Boolean"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    void makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool getXmlContent(const pugi::xml_node& root);

    bool checkValue(std::string& optValue, const std::string& pathName = "") const;

    std::shared_ptr<ConfigOption> newOption(bool optValue);

    std::string getCurrentValue() const override { return optionValue == nullptr ? "" : (optionValue->getBoolOption() ? "true" : "false"); }

    static bool CheckSqlLiteRestoreValue(std::string& value);

    static bool CheckInotifyValue(std::string& value);

    static bool CheckMarkPlayedValue(std::string& value);
};

class ConfigArraySetup : public ConfigSetup {
protected:
    bool notEmpty = false;
    bool itemNotEmpty = false;
    ArrayInitFunction initArray = nullptr;
    std::vector<std::string> defaultEntries = {};

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
    bool createArrayFromNode(const pugi::xml_node& element, std::vector<std::string>& result) const;

    bool updateItem(size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, const std::shared_ptr<ArrayOption>& value, const std::string& optValue, const std::string& status = "") const;

public:
    config_option_t nodeOption;
    config_option_t attrOption = CFG_MAX;

    ConfigArraySetup(config_option_t option, const char* xpath, const char* help, config_option_t nodeOption,
        ArrayInitFunction init = nullptr, bool notEmpty = false, std::vector<std::string> defaultEntries = {})
        : ConfigSetup(option, xpath, help)
        , notEmpty(notEmpty)
        , initArray(init)
        , defaultEntries(std::move(defaultEntries))
        , nodeOption(nodeOption)
    {
    }

    ConfigArraySetup(config_option_t option, const char* xpath, const char* help, config_option_t nodeOption, config_option_t attrOption,
        bool notEmpty = false, bool itemNotEmpty = false, std::vector<std::string> defaultEntries = {})
        : ConfigSetup(option, xpath, help)
        , notEmpty(notEmpty)
        , itemNotEmpty(itemNotEmpty)
        , defaultEntries(std::move(defaultEntries))
        , nodeOption(nodeOption)
        , attrOption(attrOption)
    {
    }

    std::string getTypeString() const override { return "List"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getItemPath(int index = 0, config_option_t propOption = CFG_MAX, config_option_t propOption2 = CFG_MAX, config_option_t propOption3 = CFG_MAX, config_option_t propOption4 = CFG_MAX) const override;

    std::vector<std::string> getXmlContent(const pugi::xml_node& optValue);

    bool checkArrayValue(const std::string& value, std::vector<std::string>& result) const;

    std::shared_ptr<ConfigOption> newOption(const std::vector<std::string>& optValue);

    std::string getCurrentValue() const override { return ""; }

    static bool InitPlayedItemsMark(const pugi::xml_node& value, std::vector<std::string>& result, const char* node_name);

    static bool InitItemsPerPage(const pugi::xml_node& value, std::vector<std::string>& result, const char* node_name);
};

class ConfigDictionarySetup : public ConfigSetup {
protected:
    bool notEmpty = false;
    bool itemNotEmpty = false;
    DictionaryInitFunction initDict = nullptr;
    bool tolower = false;
    std::map<std::string, std::string> defaultEntries = {};

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
    bool createDictionaryFromNode(const pugi::xml_node& optValue, std::map<std::string, std::string>& result) const;

    bool updateItem(size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, const std::shared_ptr<DictionaryOption>& value, const std::string& optKey, const std::string& optValue, const std::string& status = "") const;

public:
    config_option_t nodeOption {};
    config_option_t keyOption {};
    config_option_t valOption {};

    explicit ConfigDictionarySetup(config_option_t option, const char* xpath, const char* help, DictionaryInitFunction init = nullptr,
        bool notEmpty = false, bool itemNotEmpty = false, bool required = false, const std::map<std::string, std::string>& defaultEntries = {})
        : ConfigSetup(option, xpath, help, required && defaultEntries.empty())
        , notEmpty(notEmpty)
        , itemNotEmpty(itemNotEmpty)
        , initDict(init)
        , defaultEntries(defaultEntries)
    {
    }

    explicit ConfigDictionarySetup(config_option_t option, const char* xpath, const char* help,
        config_option_t nodeOption, config_option_t keyOption, config_option_t valOption,
        bool notEmpty = false, bool itemNotEmpty = false, bool required = false, const std::map<std::string, std::string>& defaultEntries = {})
        : ConfigSetup(option, xpath, help, required && defaultEntries.empty())
        , notEmpty(notEmpty)
        , itemNotEmpty(itemNotEmpty)
        , defaultEntries(defaultEntries)
        , nodeOption(nodeOption)
        , keyOption(keyOption)
        , valOption(valOption)
    {
    }

    std::string getTypeString() const override { return "List"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getItemPath(int index = 0, config_option_t propOption = CFG_MAX, config_option_t propOption2 = CFG_MAX, config_option_t propOption3 = CFG_MAX, config_option_t propOption4 = CFG_MAX) const override;

    std::map<std::string, std::string> getXmlContent(const pugi::xml_node& optValue);

    std::shared_ptr<ConfigOption> newOption(const std::map<std::string, std::string>& optValue);

    std::string getCurrentValue() const override { return ""; }
};

class ConfigAutoscanSetup : public ConfigSetup {
protected:
    ScanMode scanMode;
    bool hiddenFiles = false;

    /// \brief Creates an array of AutoscanDirectory objects from a XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param scanmode add only directories with the specified scanmode to the array
    bool createAutoscanListFromNode(const pugi::xml_node& element, std::shared_ptr<AutoscanList>& result);

    bool updateItem(size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<AutoscanDirectory>& entry, std::string& optValue, const std::string& status = "") const;

public:
    ConfigAutoscanSetup(config_option_t option, const char* xpath, const char* help, ScanMode scanmode)
        : ConfigSetup(option, xpath, help)
        , scanMode(scanmode)
    {
    }

    std::string getTypeString() const override { return "List"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getUniquePath() const override { return fmt::format("{}/{}", xpath, AutoscanDirectory::mapScanmode(scanMode)); }

    std::string getItemPath(int index = 0, config_option_t propOption = CFG_MAX, config_option_t propOption2 = CFG_MAX, config_option_t propOption3 = CFG_MAX, config_option_t propOption4 = CFG_MAX) const override;

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue);

    std::string getCurrentValue() const override { return ""; }
};

class ConfigTranscodingSetup : public ConfigSetup {
protected:
    bool isEnabled = false;

    /// \brief Creates an array of TranscodingProfile objects from an XML
    /// nodeset.
    /// \param element starting element of the nodeset.
    static bool createTranscodingProfileListFromNode(const pugi::xml_node& element, std::shared_ptr<TranscodingProfileList>& result);

public:
    ConfigTranscodingSetup(config_option_t option, const char* xpath, const char* help)
        : ConfigSetup(option, xpath, help)
    {
    }

    std::string getTypeString() const override { return "List"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getItemPath(int index = 0, config_option_t propOption = CFG_MAX, config_option_t propOption2 = CFG_MAX, config_option_t propOption3 = CFG_MAX, config_option_t propOption4 = CFG_MAX) const override;

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue);

    std::string getCurrentValue() const override { return ""; }
};

class ConfigClientSetup : public ConfigSetup {
protected:
    bool isEnabled = false;

    /// \brief Creates an array of ClientConfig objects from a XML nodeset.
    /// \param element starting element of the nodeset.
    static bool createClientConfigListFromNode(const pugi::xml_node& element, std::shared_ptr<ClientConfigList>& result);

    bool updateItem(size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<ClientConfig>& entry, std::string& optValue, const std::string& status = "") const;

public:
    ConfigClientSetup(config_option_t option, const char* xpath, const char* help)
        : ConfigSetup(option, xpath, help)
    {
    }

    std::string getTypeString() const override { return "List"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getItemPath(int index = 0, config_option_t propOption = CFG_MAX, config_option_t propOption2 = CFG_MAX, config_option_t propOption3 = CFG_MAX, config_option_t propOption4 = CFG_MAX) const override;

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue);

    std::string getCurrentValue() const override { return ""; }
};

class ConfigDirectorySetup : public ConfigSetup {
protected:
    /// \brief Creates an array of ClientConfig objects from a XML nodeset.
    /// \param element starting element of the nodeset.
    static bool createDirectoryTweakListFromNode(const pugi::xml_node& element, std::shared_ptr<DirectoryConfigList>& result);

    bool updateItem(size_t i, const std::string& optItem, const std::shared_ptr<Config>& config, std::shared_ptr<DirectoryTweak>& entry, std::string& optValue, const std::string& status = "") const;

public:
    ConfigDirectorySetup(config_option_t option, const char* xpath, const char* help)
        : ConfigSetup(option, xpath, help)
    {
    }

    std::string getTypeString() const override { return "List"; }

    void makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool updateDetail(const std::string& optItem, std::string& optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::string getItemPath(int index = 0, config_option_t propOption = CFG_MAX, config_option_t propOption2 = CFG_MAX, config_option_t propOption3 = CFG_MAX, config_option_t propOption4 = CFG_MAX) const override;

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue);

    std::string getCurrentValue() const override { return ""; }
};

#endif // __CONFIG_SETUP_H__
