/*GRB*

    Gerbera - https://gerbera.io/

    config_setup.h - this file is part of Gerbera.
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

/// \file config_setup.h
///\brief Definitions of the ConfigSetup classes.

#ifndef __CONFIG_SETUP_H__
#define __CONFIG_SETUP_H__

#include "config.h"
#include "config_options.h"
#include "config_val.h"

#include <functional>
#include <map>
#include <memory>
#include <pugixml.hpp>

#define YES "yes"
#define NO "no"

#define ITEM_PATH_ROOT (-1)
#define ITEM_PATH_NEW (-2)
#define ITEM_PATH_PREFIX (-3)

using StringCheckFunction = std::function<bool(std::string& value)>;

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
    StringCheckFunction rawCheck {};
    std::string defaultValue;
    const char* help;

    static std::size_t extractIndex(const std::string& item);

    void setOption(const std::shared_ptr<Config>& config)
    {
        config->addOption(option, optionValue);
    }
    static std::string buildCpath(const char* xpath)
    {
        auto cPath = std::string_view(xpath);
        if (!cPath.empty() && cPath.front() != '/')
            return fmt::format("/{}/{}", ROOT_NAME, cPath);
        return fmt::format("/{}{}", ROOT_NAME, cPath);
    }

public:
    virtual ~ConfigSetup() = default;

    static constexpr auto ROOT_NAME = std::string_view("config");

    pugi::xpath_node_set getXmlTree(const pugi::xml_node& element) const;

    ConfigVal option;
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

    void setDefaultValue(std::string defaultValue)
    {
        this->defaultValue = std::move(defaultValue);
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
        return !rawCheck || rawCheck(optValue);
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

    virtual std::string getItemPath(int index = 0, ConfigVal propOption = ConfigVal::MAX, ConfigVal propOption2 = ConfigVal::MAX, ConfigVal propOption3 = ConfigVal::MAX, ConfigVal propOption4 = ConfigVal::MAX) const { return xpath; }
    virtual std::string getItemPath(int index, std::vector<ConfigVal> propOptions) const { return xpath; }

    virtual std::string getUniquePath() const { return xpath; }
    virtual std::string getCurrentValue() const { return optionValue ? optionValue->getOption() : ""; }
};

#endif // __CONFIG_SETUP_H__
