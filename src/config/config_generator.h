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

#include <chrono> // for filesystem
#include <filesystem> // for path
#include <map> // for map
#include <memory> // for allocator, shared_ptr
#include <string> // for string

#include "config/config.h" // for config_option_t

namespace pugi {
class xml_document;
class xml_node;
} // namespace pugi

namespace fs = std::filesystem;

class ConfigGenerator {
public:
    ConfigGenerator();
    ~ConfigGenerator() = default;

    static std::string generate(const fs::path& userHome, const fs::path& configDir, const fs::path& dataDir, const fs::path& magicFile);

    static void generateServer(const fs::path& userHome, const fs::path& configDir, const fs::path& dataDir);
    static void generateUi();
    static void generateExtendedRuntime();
    static void generateDatabase();
    static void generateImport(const fs::path& prefixDir, const fs::path& magicFile);
    static void generateMappings();
    static void generateOnlineContent();
    static void generateTranscoding();
    static void generateUdn();

    static std::shared_ptr<pugi::xml_node> init();

    static std::shared_ptr<pugi::xml_node> getNode(const std::string& tag);

protected:
    static std::map<std::string, std::shared_ptr<pugi::xml_node>> generated;
    static pugi::xml_document doc;
    static std::shared_ptr<pugi::xml_node> setValue(const std::string& tag, const std::string& value = "", bool makeLastChild = false);

    static std::shared_ptr<pugi::xml_node> setValue(config_option_t option, const std::string& value = "");
    static std::shared_ptr<pugi::xml_node> setValue(config_option_t option, config_option_t attr, const std::string& value);
    static std::shared_ptr<pugi::xml_node> setValue(config_option_t option, const std::string& key, const std::string& value);
    static std::shared_ptr<pugi::xml_node> setValue(config_option_t option, config_option_t dict, config_option_t attr, const std::string& value);

    static std::shared_ptr<pugi::xml_node> setValue(std::shared_ptr<pugi::xml_node>& parent, config_option_t option, const std::string& value);
};

#endif //GERBERA_CONFIG_GENERATOR_H
