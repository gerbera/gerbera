/*GRB*

    Gerbera - https://gerbera.io/

    config_generator.h - this file is part of Gerbera.

    Copyright (C) 2016-2021 Gerbera Contributors

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

/// \file config_generator.h

#ifndef GERBERA_CONFIG_GENERATOR_H
#define GERBERA_CONFIG_GENERATOR_H

#include <filesystem>
#include <pugixml.hpp>
namespace fs = std::filesystem;

class ConfigGenerator {
public:
    ConfigGenerator();
    ~ConfigGenerator() = default;

    static std::string generate(const fs::path& userHome, const fs::path& configDir, const fs::path& prefixDir, const fs::path& magicFile);

    static void generateServer(const fs::path& userHome, const fs::path& configDir, const fs::path& prefixDir, pugi::xml_node* config);
    static void generateUi(pugi::xml_node* server);
    static void generateExtendedRuntime(pugi::xml_node* server);
    static void generateDatabase(pugi::xml_node* server);
    static void generateImport(const fs::path& prefixDir, const fs::path& magicFile, pugi::xml_node* config);
    static void generateMappings(pugi::xml_node* import);
    static void generateOnlineContent(pugi::xml_node* import);
    static void generateTranscoding(pugi::xml_node* config);
    static void generateUdn(pugi::xml_node* server);

protected:
    static void map_from_to(const std::string& from, const std::string& to, pugi::xml_node* parent);
    static void treat_as(const std::string& mimetype, const std::string& as, pugi::xml_node* parent);
};

#endif //GERBERA_CONFIG_GENERATOR_H
