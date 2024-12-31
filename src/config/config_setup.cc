/*GRB*

    Gerbera - https://gerbera.io/

    config_setup.cc - this file is part of Gerbera.
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

/// \file config_setup.cc

#define GRB_LOG_FAC GrbLogFacility::config
#include "config_setup.h" // API

#include "config_definition.h"
#include "config_options.h"
#include "config_val.h"
#include "exceptions.h"
#include "util/logger.h"

#include <numeric>

pugi::xml_node ConfigSetup::getXmlElement(const pugi::xml_node& root) const
{
    pugi::xpath_node xpathNode = root.select_node(cpath.c_str());
    if (!xpathNode.node()) {
        xpathNode = root.select_node(xpath);
    }
    if (required && !xpathNode.node()) {
        throw_std_runtime_error("Error in config file: {}/{} tag not found", root.path(), xpath);
    }
    return xpathNode.node();
}

bool ConfigSetup::hasXmlElement(const pugi::xml_node& root) const
{
    return root.select_node(cpath.c_str()).node()
        || root.select_node(xpath).node()
        || root.select_node(cpath.c_str()).attribute()
        || root.select_node(xpath).attribute();
}

std::string ConfigSetup::getDocs()
{
    if (help)
        return fmt::format(", see help https://docs.gerbera.io/en/stable/{}", help);
    return ", see documentation https://docs.gerbera.io/en/stable/config-overview.html";
}

/// \brief Returns a config option with the given xpath, if option does not exist a default value is returned.
std::string ConfigSetup::getXmlContent(const pugi::xml_node& root, bool trim)
{
    pugi::xpath_node xpathNode = root.select_node(cpath.c_str());

    if (xpathNode.node()) {
        std::string optValue = trim ? trimString(xpathNode.node().text().as_string()) : xpathNode.node().text().as_string();
        if (!checkValue(optValue)) {
            throw_std_runtime_error("\nInvalid {}/{} value '{}'{}", root.path(), xpath, optValue.c_str(), getDocs());
        }
        return optValue;
    }

    if (xpathNode.attribute()) {
        std::string optValue = trim ? trimString(xpathNode.attribute().value()) : xpathNode.attribute().value();
        if (!checkValue(optValue)) {
            throw_std_runtime_error("\nInvalid {}/attribute::{} value '{}'{}", root.path(), xpath, optValue.c_str(), getDocs());
        }
        return optValue;
    }

    auto xAttr = ConfigDefinition::removeAttribute(option);

    if (root.attribute(xAttr.c_str())) {
        std::string optValue = trim ? trimString(root.attribute(xAttr.c_str()).as_string()) : root.attribute(xAttr.c_str()).as_string();
        if (!checkValue(optValue)) {
            throw_std_runtime_error("\nInvalid {}/attribute::{} value '{}'{}", root.path(), xAttr, optValue, getDocs());
        }
        return optValue;
    }

    if (root.child(xpath)) {
        std::string optValue = trim ? trimString(root.child(xpath).text().as_string()) : root.child(xpath).text().as_string();
        if (!checkValue(optValue)) {
            throw_std_runtime_error("\nInvalid {}/{} value '{}'{}", root.path(), xpath, optValue, getDocs());
        }
        return optValue;
    }

    if (required)
        throw_std_runtime_error("\nRequired option {}/{} not found{}", root.path(), xpath, getDocs());

    log_debug("Config: option not found: '{}/{}' using default value: '{}'", root.path(), xpath, defaultValue);

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

std::vector<std::size_t> ConfigSetup::extractIndexList(const std::string& item)
{
    auto result = std::vector<std::size_t>();
    std::size_t startPos = 0;
    do {
        startPos = item.find_first_of('[', startPos);
        if (startPos != std::string::npos) {
            startPos++;
            auto endPos = item.find_first_of(']', startPos);
            if (endPos != std::string::npos) {
                try {
                    std::size_t i = std::stoul(item.substr(startPos, endPos - startPos));
                    result.push_back(i);
                } catch (const std::invalid_argument& ex) {
                    log_error(ex.what());
                    result.push_back(0);
                }
            }
        }
    } while (startPos != std::string::npos);

    return result;
}
