/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_bool.cc - this file is part of Gerbera.
    Copyright (C) 2020-2023 Gerbera Contributors

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

#include "config/config_setup.h" // API

#include "config/client_config.h"
#include "config/config_definition.h"
#include "config/config_options.h"

#ifdef HAVE_INOTIFY
#include "util/mt_inotify.h"
#endif

void ConfigBoolSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    newOption(getXmlContent(root));
    setOption(config);
}

static bool validateTrueFalse(const std::string& optValue)
{
    return (optValue == "true" || optValue == "false");
}

static bool validateYesNo(std::string_view value)
{
    return value == "yes" || value == "no";
}

void ConfigBoolSetup::makeOption(std::string optValue, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    if (!validateTrueFalse(optValue) && !validateYesNo(optValue))
        throw_std_runtime_error("Invalid {} value {}", xpath, optValue);
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
    if (rawCheck && !rawCheck(optValue)) {
        throw_std_runtime_error("Invalid {}/{} value '{}'", pathName, xpath, optValue);
    }

    if (!validateTrueFalse(optValue) && !validateYesNo(optValue))
        throw_std_runtime_error("Invalid {}/{} value {}", pathName, xpath, optValue);
    return optValue == YES || optValue == "true";
}

std::shared_ptr<ConfigOption> ConfigBoolSetup::newOption(bool optValue)
{
    optionValue = std::make_shared<BoolOption>(optValue);
    return optionValue;
}

bool ConfigBoolSetup::CheckSqlLiteRestoreValue(std::string& value)
{
    bool tmpBool = true;
    if (value == "restore" || value == YES)
        tmpBool = true;
    else if (value == "fail" || value == NO)
        tmpBool = false;
    else
        return false;

    value.assign(tmpBool ? YES : NO);
    return true;
}

bool ConfigBoolSetup::CheckInotifyValue(std::string& value)
{
    bool tempBool = false;
    if ((value != "auto") && !validateYesNo(value)) {
        log_error("Error in config file: incorrect parameter for \"<autoscan use-inotify=\" attribute");
        return false;
    }

#ifdef HAVE_INOTIFY
    bool inotifySupported = Inotify::supported();
    tempBool = (inotifySupported && value == "auto");
#endif

    if (value == YES) {
#ifdef HAVE_INOTIFY
        if (!inotifySupported) {
            log_error("You specified \"yes\" in \"<autoscan use-inotify=\"\">"
                      " however your system does not have inotify support");
            return false;
        }
        tempBool = true;
#else
        log_error("You specified \"yes\" in \"<autoscan use-inotify=\"\">"
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
