/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_bool.cc - this file is part of Gerbera.
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

/// \file config_setup_bool.cc
#define GRB_LOG_FAC GrbLogFacility::config

#include "config_setup_bool.h" // API

#include "config/config_definition.h"
#include "config/config_options.h"

#ifdef HAVE_INOTIFY
#include "content/inotify/mt_inotify.h"
#endif

#define B_TRUE "true"
#define B_FALSE "false"

void ConfigBoolSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlContent(root));
    setOption(config);
}

static bool validateTrueFalse(const std::string& optValue)
{
    return (optValue == B_TRUE || optValue == B_FALSE);
}

static bool validateYesNo(std::string_view value)
{
    return value == YES || value == NO;
}

void ConfigBoolSetup::makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (!validateTrueFalse(optValue) && !validateYesNo(optValue))
        throw_std_runtime_error("Invalid {} value {}", xpath, optValue);
    optionValue = std::make_shared<BoolOption>(optValue == YES || optValue == B_TRUE);
    setOption(config);
}

bool ConfigBoolSetup::getXmlContent(const pugi::xml_node& root)
{
    std::string optValue = ConfigSetup::getXmlContent(root, true);
    return checkValue(optValue, root.path());
}

bool ConfigBoolSetup::checkValue(std::string& optValue, const std::string& pathName) const
{
    if (rawCheck && !rawCheck(optValue)) {
        throw_std_runtime_error("Invalid {}/{} value '{}'", pathName, xpath, optValue);
    }

    if (!validateTrueFalse(optValue) && !validateYesNo(optValue))
        throw_std_runtime_error("Invalid {}/{} value {}", pathName, xpath, optValue);
    return optValue == YES || optValue == B_TRUE;
}

std::shared_ptr<ConfigOption> ConfigBoolSetup::newOption(bool optValue)
{
    optionValue = std::make_shared<BoolOption>(optValue);
    return optionValue;
}

#define IN_AUTO "auto"

bool ConfigBoolSetup::CheckInotifyValue(std::string& value)
{
    bool tempBool = false;
    if ((value != IN_AUTO) && !validateYesNo(value) && !validateTrueFalse(value)) {
        log_error("Error in config file: incorrect parameter for \"<autoscan use-inotify=\" attribute");
        return false;
    }

#ifdef HAVE_INOTIFY
    bool inotifySupported = Inotify::supported();
    tempBool = (inotifySupported && value == IN_AUTO);
#endif

    if (value == YES || value == B_TRUE) {
#ifdef HAVE_INOTIFY
        if (!inotifySupported) {
            log_error("You specified \"" YES "\" in \"<autoscan use-inotify=\"\">"
                      " however your system does not have inotify support");
            return false;
        }
        tempBool = true;
#else
        log_error("You specified \"" YES "\" in \"<autoscan use-inotify=\"\">"
                  " however this version of Gerbera was compiled without inotify support");
        return false;
#endif
    }

    value.assign(tempBool ? YES : NO);
    return true;
}

bool ConfigBoolSetup::CheckMarkPlayedValue(std::string& value)
{
    bool tmpBool = true;
    if (value == "prepend" || value == DEFAULT_MARK_PLAYED_ITEMS_STRING_MODE || value == YES)
        tmpBool = true;
    else if (value == "append" || value == NO)
        tmpBool = false;
    else
        return false;
    value.assign(tmpBool ? YES : NO);
    return true;
}
