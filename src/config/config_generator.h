/*GRB*

Gerbera - https://gerbera.io/

    config_generator.h - this file is part of Gerbera.

    Copyright (C) 2016-2020 Gerbera Contributors

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

#include <pugixml.hpp>
#include <filesystem>
namespace fs = std::filesystem;

class ConfigGenerator {
 public:
  ConfigGenerator();
  ~ConfigGenerator();

  std::string generate(fs::path userHome, fs::path configDir, fs::path prefixDir, fs::path magicFile);
  
  void generateServer(fs::path userHome, fs::path configDir, fs::path prefixDir, pugi::xml_node* config);
  void generateUi(pugi::xml_node* server);
  void generateExtendedRuntime(pugi::xml_node* server);
  void generateStorage(pugi::xml_node* server);
  void generateImport(fs::path prefixDir, fs::path magicFile, pugi::xml_node* config);
  void generateMappings(pugi::xml_node* import);
  void generateOnlineContent(pugi::xml_node* import);
  void generateTranscoding(pugi::xml_node* config);
  void generateUdn(pugi::xml_node* server);

 protected:
  void map_from_to(std::string from, std::string to, pugi::xml_node* parent);
  void treat_as(std::string mimetype, std::string as, pugi::xml_node* parent);
};

#endif //GERBERA_CONFIG_GENERATOR_H
