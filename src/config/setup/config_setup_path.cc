/*GRB*

    Gerbera - https://gerbera.io/

    config_setup_path.cc - this file is part of Gerbera.
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

/// \file config_setup_path.cc

#include "config/config_setup.h" // API

#include "config/client_config.h"
#include "config/config_definition.h"
#include "config/config_options.h"
#include "content/autoscan.h"

bool ConfigPathSetup::checkPathValue(std::string& optValue, std::string& pathValue) const
{
    if (rawCheck && !rawCheck(optValue)) {
        return false;
    }
    pathValue.assign(resolvePath(optValue));
    return !isSet(ConfigPathArguments::notEmpty) || !pathValue.empty();
}

bool ConfigPathSetup::checkExecutable(std::string& optValue) const
{
    fs::path tmpPath;
    if (fs::path(optValue).is_absolute()) {
        std::error_code ec;
        fs::directory_entry dirEnt(optValue, ec);
        if (!isRegularFile(dirEnt, ec) && !dirEnt.is_symlink(ec)) {
            log_warning("Error in configuration, could not find command \"{}\" for '{}'", optValue, cpath);
            return !isSet(ConfigPathArguments::mustExist);
        }
        tmpPath = optValue;
    } else {
        tmpPath = findInPath(optValue);
        if (tmpPath.empty()) {
            log_warning("Error in configuration, could not find  command \"{}\" in $PATH for '{}'", optValue, cpath);
            return !isSet(ConfigPathArguments::mustExist);
        }
    }

    int err = 0;
    if (!isExecutable(tmpPath, &err)) {
        log_warning("Error in configuration, file {} is not executable: {} for '{}'", optValue, std::strerror(err), cpath);
        return !isSet(ConfigPathArguments::mustExist);
    }
    return true;
}

std::string ConfigPathSetup::getXmlContent(const pugi::xml_node& root)
{
    auto optValue = ConfigSetup::getXmlContent(root, true);
    if (isSet(ConfigPathArguments::isExe)) {
        if (!checkExecutable(optValue)) {
            throw_std_runtime_error("Invalid {} file is not an executable '{}'", xpath, optValue);
        }
    }
    return optValue;
}

/// \brief resolve path against home, an exception is raised if path does not exist on filesystem.
/// \param path path to be resolved
/// \param isFile file or directory
/// \param mustExist file/directory must exist
fs::path ConfigPathSetup::resolvePath(fs::path path) const
{
    if (!isSet(ConfigPathArguments::resolveEmpty) && path.empty()) {
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
    if (isSet(ConfigPathArguments::isFile)) {
        if (isSet(ConfigPathArguments::mustExist)) {
            fs::directory_entry dirEnt(path, ec);
            if (!isRegularFile(dirEnt, ec) && !dirEnt.is_symlink(ec)) {
                throw_std_runtime_error("File '{}' does not exist for '{}'", path.string(), cpath);
            }
        } else {
            fs::directory_entry dirEnt(path.parent_path(), ec);
            if (!dirEnt.is_directory(ec) && !dirEnt.is_symlink(ec)) {
                throw_std_runtime_error("Parent directory '{}' does not exist for '{}'", path.string(), cpath);
            }
        }
    } else if (isSet(ConfigPathArguments::mustExist)) {
        fs::directory_entry dirEnt(path, ec);
        if (!dirEnt.is_directory(ec) && !dirEnt.is_symlink(ec)) {
            throw_std_runtime_error("Directory '{}' does not exist for '{}'", path.string(), cpath);
        }
    }

    log_debug("resolvePath {} = {}", xpath, path.string());
    return path;
}

void ConfigPathSetup::loadArguments(const std::map<std::string, std::string>* arguments)
{
    if (arguments) {
        if (arguments->find("isFile") != arguments->end()) {
            setFlag(arguments->at("isFile") == "true", ConfigPathArguments::isFile);
        }
        if (arguments->find("mustExist") != arguments->end()) {
            setFlag(arguments->at("mustExist") == "true", ConfigPathArguments::mustExist);
        }
        if (arguments->find("notEmpty") != arguments->end()) {
            setFlag(arguments->at("notEmpty") == "true", ConfigPathArguments::notEmpty);
        }
        if (arguments->find("resolveEmpty") != arguments->end()) {
            setFlag(arguments->at("resolveEmpty") == "true", ConfigPathArguments::resolveEmpty);
        }
        if (arguments->find("isExe") != arguments->end()) {
            setFlag(arguments->at("isExe") == "true", ConfigPathArguments::isExe);
        }
    }
}

void ConfigPathSetup::setFlag(bool hasFlag, ConfigPathArguments flag)
{
    this->arguments = hasFlag ? (this->arguments | flag) : (this->arguments & static_cast<ConfigPathArguments>(~static_cast<int>(flag)));
}

void ConfigPathSetup::makeOption(const pugi::xml_node& root, const std::shared_ptr<Config>& config, const std::map<std::string, std::string>* arguments)
{
    loadArguments(arguments);
    auto optValue = ConfigSetup::getXmlContent(root, true);
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
    if (isSet(ConfigPathArguments::isExe)) {
        if (!checkExecutable(optValue)) {
            throw_std_runtime_error("Invalid {} file is not an executable '{}'", xpath, optValue);
        }
    } else if (!checkPathValue(optValue, pathValue)) {
        throw_std_runtime_error("Invalid {} resolves to empty value '{}'", xpath, optValue);
    }
    optionValue = std::make_shared<Option>(pathValue);
    return optionValue;
}

fs::path ConfigPathSetup::Home = "";
