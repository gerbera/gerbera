/*GRB*

Gerbera - https://gerbera.io/

    config_generator.cc - this file is part of Gerbera.

    Copyright (C) 2016-2018 Gerbera Contributors

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

/// \file config_generator.cc


#include <string>
#include <mxml/mxml.h>
#include <common.h>
#include "config_generator.h"

using namespace zmm;
using namespace mxml;

ConfigGenerator::ConfigGenerator() {}

ConfigGenerator::~ConfigGenerator() {}

std::string ConfigGenerator::generate() {
    Ref<Element> config(new Element(_("config")));
    config->setAttribute(_("version"), String::from(CONFIG_XML_VERSION));
    config->setAttribute(_("xmlns"), _(XML_XMLNS) + CONFIG_XML_VERSION);
    config->setAttribute(_("xmlns:xsi"), _(XML_XMLNS_XSI));
    config->setAttribute(_("xsi:schemaLocation"), _(XML_XMLNS) + CONFIG_XML_VERSION + " " + XML_XMLNS + CONFIG_XML_VERSION + ".xsd");

    Ref<Comment> docinfo(new Comment(_("\n\
     See http://gerbera.io or read the docs for more\n\
     information on creating and using config.xml configration files.\n\
    "),
                                     true));
    config->appendChild(RefCast(docinfo, Node));

    config->indent();
    std::string configAsString (config->print().c_str());
    return configAsString;
}
