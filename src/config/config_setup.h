/*GRB*

    Gerbera - https://gerbera.io/

    config_setup.h - this file is part of Gerbera.
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

/// @file config/config_setup.h
/// @brief Definitions of the ConfigSetup classes.

#ifndef __CONFIG_SETUP_H__
#define __CONFIG_SETUP_H__

#include "config.h"
#include "config_options.h"

#include <functional>
#include <map>
#include <memory>
#include <pugixml.hpp>
#include <vector>

#define YES "yes"
#define NO "no"

using StringCheckFunction = std::function<bool(std::string& value)>;

class ConfigDefinition;
enum class ConfigVal;

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

/// @brief Base class for config parsers
class ConfigSetup {
protected:
    /// @brief Reference to value object
    std::shared_ptr<ConfigOption> optionValue;
    /// @brief full path of the config item
    std::string cpath;
    /// @brief true if option must be in config file
    bool required = false;
    /// @brief true if used default value
    bool useDefault = false;
    /// @brief true if option will be removed soon
    bool isDeprecated = false;
    /// @brief Function to check value format and content
    StringCheckFunction rawCheck;
    /// @brief defaultValue default value if option not found
    std::string defaultValue;
    /// @brief name of the help page
    const char* help;
    /// @brief reference to definition management
    std::shared_ptr<ConfigDefinition> definition;

    /// @brief extract array indicies from xpath statement
    static std::vector<std::size_t> extractIndexList(const std::string& item);

    /// @brief Get descriptive text to documentation for home page
    std::string getDocs();

    /// @brief add config option to configuration manager
    void setOption(const std::shared_ptr<Config>& config)
    {
        config->addOption(option, optionValue);
    }

    /// @brief construct config item path from settings
    static std::string buildCpath(const char* xpath)
    {
        auto cPath = std::string_view(xpath);
        if (!cPath.empty() && cPath.front() != '/')
            return fmt::format("/{}/{}", ROOT_NAME, cPath);
        return fmt::format("/{}{}", ROOT_NAME, cPath);
    }

public:
    static constexpr auto ROOT_NAME = std::string_view("config");

    /// @brief Id of the config item
    ConfigVal option;

    /// @brief xpath option xpath
    /// The xpath parameter has XPath syntax:
    /// "/path/to/option" will return the text value of the given "option" element
    /// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
    const char* xpath;

    ConfigSetup(ConfigVal option, const char* xpath, const char* help, bool required = false, const char* defaultValue = "")
        : cpath(buildCpath(xpath))
        , required(required)
        , rawCheck(nullptr)
        , defaultValue(defaultValue)
        , help(help)
        , option(option)
        , xpath(xpath)
    {
    }

    ConfigSetup(ConfigVal option, const char* xpath, const char* help, StringCheckFunction check, const char* defaultValue = "", bool required = false)
        : cpath(buildCpath(xpath))
        , required(required)
        , rawCheck(std::move(check))
        , defaultValue(defaultValue)
        , help(help)
        , option(option)
        , xpath(xpath)
    {
    }

    virtual ~ConfigSetup() = default;

    ConfigSetup(const ConfigSetup&) = delete;
    ConfigSetup& operator=(const ConfigSetup&) = delete;

    /// @brief get xml tree based on config item
    pugi::xpath_node_set getXmlTree(const pugi::xml_node& element) const;

    /// @brief set default value
    void setDefaultValue(std::string defaultValue)
    {
        this->defaultValue = std::move(defaultValue);
    }

    /// @brief get default value
    std::string getDefaultValue() const
    {
        return this->defaultValue;
    }

    /// @brief inject definition object
    void setDefinition(const std::shared_ptr<ConfigDefinition>& definition)
    {
        this->definition = definition;
    }

    /// @brief Create the xml representation of the defaultEntries
    virtual bool createNodeFromDefaults(const std::shared_ptr<pugi::xml_node>& result) const
    {
        return false;
    }

    /// @brief Report if option is deprecated
    bool isOptionDeprecated() const
    {
        return this->isDeprecated;
    }

    /// @brief Set option to deprecated
    void setDeprecated()
    {
        this->isDeprecated = true;
    }

    /// @brief Report if default value was used
    bool isDefaultValueUsed() const
    {
        return this->useDefault;
    }

    /// @brief Check config value complies with rules
    bool checkValue(std::string& optValue) const
    {
        return !rawCheck || rawCheck(optValue);
    }

    /// @brief Get link to help page for web ui
    const char* getHelp() const { return help; }

    /// @brief Get current value of config option
    std::shared_ptr<ConfigOption> getValue() const { return optionValue; }

    /// @brief Get Type of config item for web ui
    virtual std::string getTypeString() const { return "Unset"; }

    /// @brief Extract node based on config item
    pugi::xml_node getXmlElement(const pugi::xml_node& root) const;

    /// @brief Check if node for config item exists
    bool hasXmlElement(const pugi::xml_node& root) const;

    /// @brief Returns a config option with the given xpath, if option does not exist a default value is returned.
    std::string getXmlContent(
        const pugi::xml_node& root,
        const std::shared_ptr<Config>& config,
        bool trim = true);

    /// @brief Gererate Option from config file entry
    virtual void makeOption(
        const pugi::xml_node& root,
        const std::shared_ptr<Config>& config,
        const std::map<std::string, std::string>* arguments = nullptr);

    /// @brief Gererate Option from config value
    virtual void makeOption(
        std::string optValue,
        const std::shared_ptr<Config>& config,
        const std::map<std::string, std::string>* arguments = nullptr);

    /// @brief Update Option from Web UI or Database
    virtual bool updateDetail(
        const std::string& optItem,
        std::string& optValue,
        const std::shared_ptr<Config>& config,
        const std::map<std::string, std::string>* arguments = nullptr)
    {
        return false;
    }

    /// @brief calculate xpath for entry
    /// @param indexList indexed for arrays
    /// @param propOptions options to get section or attribute from
    /// @param propText text option (mostly for dynamic vectors)
    /// @return xpath of entry
    virtual std::string getItemPath(
        const std::vector<std::size_t>& indexList,
        const std::vector<ConfigVal>& propOptions,
        const std::string& propText = "") const
    {
        return xpath;
    }

    /// @brief Get root path to config item in xml
    virtual std::string getItemPathRoot(bool prefix = false) const { return xpath; }

    /// @brief Get full path to config item in xml
    virtual std::string getUniquePath() const { return xpath; }

    /// @brief Get current value of config item
    virtual std::string getCurrentValue() const { return optionValue ? optionValue->getOption() : ""; }
};

#endif // __CONFIG_SETUP_H__
