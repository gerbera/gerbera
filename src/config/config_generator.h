/*GRB*

Gerbera - https://gerbera.io/

    config_generator.h - this file is part of Gerbera.

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

/// \file config_generator.h

#ifndef GERBERA_CONFIG_GENERATOR_H
#define GERBERA_CONFIG_GENERATOR_H

#include <mxml/element.h>

class ConfigGenerator {
 public:
  ConfigGenerator();
  ~ConfigGenerator();

  std::string generate(std::string userHome, std::string configDir, std::string prefixDir, std::string magicFile);
  zmm::Ref<mxml::Element> generateServer(std::string userHome, std::string configDir, std::string prefixDir);
  zmm::Ref<mxml::Element> generateUi();
  zmm::Ref<mxml::Element> generateExtendedRuntime();
  zmm::Ref<mxml::Element> generateStorage();
  zmm::Ref<mxml::Element> generateImport(std::string prefixDir, std::string magicFile);
  zmm::Ref<mxml::Element> generateMappings();
  zmm::Ref<mxml::Element> generateOnlineContent();
  zmm::Ref<mxml::Element> generateTranscoding();
  zmm::Ref<mxml::Element> generateUdn();
 protected:
  zmm::Ref<mxml::Element> map_from_to(std::string from, std::string to);
  zmm::Ref<mxml::Element> treat_as(std::string mimetype, std::string as);
};

#endif //GERBERA_CONFIG_GENERATOR_H
