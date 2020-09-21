/*GRB*

    Gerbera - https://gerbera.io/

    config_setup.h - this file is part of Gerbera.
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
#include "config_options.h"

class ConfigOption;
enum class ScanMode;

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

    void setOption(Config* config)
    {
        config->addOption(option, optionValue);
    }

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

    virtual ~ConfigSetup() = default;

    void setDefaultValue(std::string defaultValue)
    {
        this->defaultValue.assign(defaultValue);
    }

    std::shared_ptr<ConfigOption> getValue() { return optionValue; }

    pugi::xml_node getXmlElement(const pugi::xml_node& root) const;

    bool hasXmlElement(const pugi::xml_node& root) const;

    /// \brief Returns a config option with the given xpath, if option does not exist a default value is returned.
    /// \param xpath option xpath
    /// \param defaultValue default value if option not found
    ///
    /// The xpath parameter has XPath syntax:
    /// "/path/to/option" will return the text value of the given "option" element
    /// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
    std::string getXmlContent(const pugi::xml_node& root, bool trim = true) const;

    virtual void makeOption(const pugi::xml_node& root, Config* config, const std::map<std::string, std::string>* arguments = nullptr);

    virtual void makeOption(std::string optValue, Config* config, const std::map<std::string, std::string>* arguments = nullptr);
};

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

    virtual void makeOption(const pugi::xml_node& root, Config* config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::shared_ptr<ConfigOption> newOption(const std::string& optValue);
};

template <class En>
class ConfigEnumSetup : public ConfigSetup {
protected:
    bool notEmpty = true;
    std::map<std::string, En> valueMap;

    bool CheckEnumValue(const std::string& value, En& result) const
    {
        if (valueMap.find(value) != valueMap.end()) {
            result = valueMap.at(value);
            return true;
        }
        return false;
    }

public:
    ConfigEnumSetup(config_option_t option, const char* xpath, const std::map<std::string, En>& valueMap, bool notEmpty = false)
        : ConfigSetup(option, xpath, false, "")
        , valueMap(valueMap)
    {
        this->notEmpty = notEmpty;
    }

    ConfigEnumSetup(config_option_t option, const char* xpath, const char* defaultValue, const std::map<std::string, En>& valueMap, bool notEmpty = false)
        : ConfigSetup(option, xpath, false, defaultValue)
        , valueMap(valueMap)
    {
        this->notEmpty = notEmpty;
    }

    virtual ~ConfigEnumSetup() = default;

    virtual void makeOption(const pugi::xml_node& root, Config* config, const std::map<std::string, std::string>* arguments = nullptr) override
    {
        if (arguments != nullptr && arguments->find("notEmpty") != arguments->end()) {
            notEmpty = arguments->find("notEmpty")->second == "true";
        }
        newOption(ConfigSetup::getXmlContent(root, true));
        setOption(config);
    }

    En getXmlContent(const pugi::xml_node& root) const
    {
        std::string optValue = ConfigSetup::getXmlContent(root, true);
        log_debug("Config: option: '{}' value: '{}'", xpath, optValue.c_str());
        if (notEmpty && optValue.empty()) {
            throw std::runtime_error(fmt::format("Error in config file: Invalid {}/{} empty value '{}'", root.path(), xpath, optValue));
        }
        En result;
        if (!CheckEnumValue(optValue, result)) {
            throw std::runtime_error(fmt::format("Error in config file: {}/{} unsupported Enum value '{}'", root.path(), xpath, optValue).c_str());
        }
        return result;
    }

    std::shared_ptr<ConfigOption> newOption(const std::string& optValue)
    {
        if (notEmpty && optValue.empty()) {
            throw std::runtime_error(fmt::format("Invalid {} empty value '{}'", xpath, optValue));
        }
        En result;
        if (!CheckEnumValue(optValue, result)) {
            throw std::runtime_error(fmt::format("Error in config file: {} unsupported Enum value '{}'", xpath, optValue).c_str());
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
    fs::path resolvePath(fs::path path);

    void loadArguments(const std::map<std::string, std::string>* arguments = nullptr);

public:
    static fs::path Home;

    ConfigPathSetup(config_option_t option, const char* xpath, const char* defaultValue = "", bool isFile = false, bool mustExist = true, bool notEmpty = false)
        : ConfigSetup(option, xpath, nullptr, defaultValue)
        , isFile(isFile)
        , mustExist(mustExist)
        , notEmpty(notEmpty)
    {
    }

    virtual void makeOption(const pugi::xml_node& root, Config* config, const std::map<std::string, std::string>* arguments = nullptr) override;

    virtual void makeOption(std::string optValue, Config* config, const std::map<std::string, std::string>* arguments = nullptr) override;

    virtual ~ConfigPathSetup() = default;

    std::shared_ptr<ConfigOption> newOption(const std::string& optValue);
};

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

    virtual void makeOption(const pugi::xml_node& root, Config* config, const std::map<std::string, std::string>* arguments = nullptr) override;

    virtual void makeOption(std::string optValue, Config* config, const std::map<std::string, std::string>* arguments = nullptr) override;

    int getXmlContent(const pugi::xml_node& root) const;

    std::shared_ptr<ConfigOption> newOption(int optValue);

    static bool CheckSqlLiteSyncValue(std::string& value);

    static bool CheckProfleNumberValue(std::string& value);

    static bool CheckMinValue(int value, int minValue);

    static bool CheckImageQualityValue(int value);

    static bool CheckUpnpStringLimitValue(int value);
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

    ConfigBoolSetup(config_option_t option, const char* xpath, bool defaultValue, bool required)
        : ConfigSetup(option, xpath, required)
    {
        this->defaultValue = defaultValue ? YES : NO;
    }

    virtual ~ConfigBoolSetup() = default;

    virtual void makeOption(const pugi::xml_node& root, Config* config, const std::map<std::string, std::string>* arguments = nullptr) override;

    virtual void makeOption(std::string optValue, Config* config, const std::map<std::string, std::string>* arguments = nullptr) override;

    bool getXmlContent(const pugi::xml_node& root) const;

    std::shared_ptr<ConfigOption> newOption(bool optValue);

    static bool CheckSqlLiteRestoreValue(std::string& value);

    static bool CheckInotifyValue(std::string& value);

    static bool CheckMarkPlayedValue(std::string& value);
};

class ConfigArraySetup : public ConfigSetup {
protected:
    bool notEmpty = false;
    bool itemNotEmpty = false;
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
    bool createArrayFromNode(const pugi::xml_node& element, std::vector<std::string>& result) const;

public:
    ConfigArraySetup(config_option_t option, const char* xpath, ArrayInitFunction init = nullptr, bool notEmpty = false)
        : ConfigSetup(option, xpath)
        , notEmpty(notEmpty)
        , initArray(init)
    {
    }

    ConfigArraySetup(config_option_t option, const char* xpath, config_option_t nodeOption, config_option_t attrOption, bool notEmpty = false, bool itemNotEmpty = false)
        : ConfigSetup(option, xpath)
        , notEmpty(notEmpty)
        , itemNotEmpty(itemNotEmpty)
        , initArray(nullptr)
        , nodeOption(nodeOption)
        , attrOption(attrOption)
    {
    }

    virtual ~ConfigArraySetup() = default;

    virtual void makeOption(const pugi::xml_node& root, Config* config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::vector<std::string> getXmlContent(const pugi::xml_node& optValue) const;

    std::shared_ptr<ConfigOption> newOption(const std::vector<std::string>& optValue);

    static bool InitPlayedItemsMark(const pugi::xml_node& value, std::vector<std::string>& result);

    static bool InitItemsPerPage(const pugi::xml_node& value, std::vector<std::string>& result);
};

class ConfigDictionarySetup : public ConfigSetup {
protected:
    bool notEmpty = false;
    bool itemNotEmpty = false;
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
    bool createDictionaryFromNode(const pugi::xml_node& optValue, std::map<std::string, std::string>& result) const;

public:
    ConfigDictionarySetup(config_option_t option, const char* xpath, DictionaryInitFunction init = nullptr, bool notEmpty = false, bool itemNotEmpty = false)
        : ConfigSetup(option, xpath)
        , notEmpty(notEmpty)
        , itemNotEmpty(itemNotEmpty)
        , initDict(init)
    {
    }

    ConfigDictionarySetup(config_option_t option, const char* xpath, config_option_t nodeOption, config_option_t keyOption, config_option_t valOption, bool notEmpty = false, bool itemNotEmpty = false)
        : ConfigSetup(option, xpath)
        , notEmpty(notEmpty)
        , itemNotEmpty(itemNotEmpty)
        , initDict(nullptr)
        , nodeOption(nodeOption)
        , keyOption(keyOption)
        , valOption(valOption)
    {
    }

    virtual ~ConfigDictionarySetup() = default;

    virtual void makeOption(const pugi::xml_node& root, Config* config, const std::map<std::string, std::string>* arguments = nullptr) override;
    std::map<std::string, std::string> getXmlContent(const pugi::xml_node& optValue) const;

    std::shared_ptr<ConfigOption> newOption(const std::map<std::string, std::string>& optValue);
};

class ConfigAutoscanSetup : public ConfigSetup {
protected:
    ScanMode scanmode;
    bool hiddenFiles = false;

    /// \brief Creates an array of AutoscanDirectory objects from a XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param scanmode add only directories with the specified scanmode to the array
    bool createAutoscanListFromNode(const pugi::xml_node& element, std::shared_ptr<AutoscanList>& result);

public:
    ConfigAutoscanSetup(config_option_t option, const char* xpath, ScanMode scanmode)
        : ConfigSetup(option, xpath)
        , scanmode(scanmode)
    {
    }

    virtual ~ConfigAutoscanSetup() = default;

    virtual void makeOption(const pugi::xml_node& root, Config* config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue);
};

class ConfigTranscodingSetup : public ConfigSetup {
protected:
    bool isEnabled = false;

    /// \brief Creates an array of TranscodingProfile objects from an XML
    /// nodeset.
    /// \param element starting element of the nodeset.
    bool createTranscodingProfileListFromNode(const pugi::xml_node& element, std::shared_ptr<TranscodingProfileList>& result);

public:
    ConfigTranscodingSetup(config_option_t option, const char* xpath)
        : ConfigSetup(option, xpath)
    {
    }

    virtual ~ConfigTranscodingSetup() = default;

    virtual void makeOption(const pugi::xml_node& root, Config* config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue);
};

class ConfigClientSetup : public ConfigSetup {
protected:
    bool isEnabled = false;

    /// \brief Creates an array of ClientConfig objects from a XML nodeset.
    /// \param element starting element of the nodeset.
    bool createClientConfigListFromNode(const pugi::xml_node& element, std::shared_ptr<ClientConfigList>& result);

public:
    ConfigClientSetup(config_option_t option, const char* xpath)
        : ConfigSetup(option, xpath)
    {
    }

    virtual ~ConfigClientSetup() = default;

    virtual void makeOption(const pugi::xml_node& root, Config* config, const std::map<std::string, std::string>* arguments = nullptr) override;

    std::shared_ptr<ConfigOption> newOption(const pugi::xml_node& optValue);
};

#endif // __CONFIG_SETUP_H__
