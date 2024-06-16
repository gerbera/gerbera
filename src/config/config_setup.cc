/*GRB*

    Gerbera - https://gerbera.io/

    config_setup.cc - this file is part of Gerbera.
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

/// \brief Returns a config option with the given xpath, if option does not exist a default value is returned.
/// \param xpath option xpath
/// \param defaultValue default value if option not found
///
/// The xpath parameter has XPath syntax:
/// "/path/to/option" will return the text value of the given "option" element
/// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
std::string ConfigSetup::getXmlContent(const pugi::xml_node& root, bool trim)
{
    pugi::xpath_node xpathNode = root.select_node(cpath.c_str());

    if (xpathNode.node()) {
        std::string optValue = trim ? trimString(xpathNode.node().text().as_string()) : xpathNode.node().text().as_string();
        if (!checkValue(optValue)) {
            throw_std_runtime_error("Invalid {}/{} value '{}'", root.path(), xpath, optValue.c_str());
        }
        return optValue;
    }

    if (xpathNode.attribute()) {
        std::string optValue = trim ? trimString(xpathNode.attribute().value()) : xpathNode.attribute().value();
        if (!checkValue(optValue)) {
            throw_std_runtime_error("Invalid {}/attribute::{} value '{}'", root.path(), xpath, optValue.c_str());
        }
        return optValue;
    }

    auto xAttr = ConfigDefinition::removeAttribute(option);

    if (root.attribute(xAttr.c_str())) {
        std::string optValue = trim ? trimString(root.attribute(xAttr.c_str()).as_string()) : root.attribute(xAttr.c_str()).as_string();
        if (!checkValue(optValue)) {
            throw_std_runtime_error("Invalid {}/attribute::{} value '{}'", root.path(), xAttr, optValue);
        }
        return optValue;
    }

    if (root.child(xpath)) {
        std::string optValue = trim ? trimString(root.child(xpath).text().as_string()) : root.child(xpath).text().as_string();
        if (!checkValue(optValue)) {
            throw_std_runtime_error("Invalid {}/{} value '{}'", root.path(), xpath, optValue);
        }
        return optValue;
    }

    if (required)
        throw_std_runtime_error("Error in config file: {}/{} not found", root.path(), xpath);

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

std::size_t ConfigSetup::extractIndex(const std::string& item)
{
    auto i = std::numeric_limits<std::size_t>::max();
    if (item.find_first_of('[') != std::string::npos && item.find_first_of(']', item.find_first_of('[')) != std::string::npos) {
        auto startPos = item.find_first_of('[') + 1;
        auto endPos = item.find_first_of(']', startPos);
        try {
            i = std::stoi(item.substr(startPos, endPos - startPos));
        } catch (const std::invalid_argument& ex) {
            log_error(ex.what());
        }
    }
    return i;
}
