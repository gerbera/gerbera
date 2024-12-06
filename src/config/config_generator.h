/*GRB*

    Gerbera - https://gerbera.io/

    config_generator.h - this file is part of Gerbera.

    Copyright (C) 2016-2024 Gerbera Contributors

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

#include "config/config.h"

// forward declarations
class ConfigSetup;

enum class GeneratorSections {
    Server,
    Ui,
    ExtendedRuntime,
    DynamicContainer,
    Database,
    Import,
    Mappings,
    Boxlayout,
    Transcoding,
    OnlineContent,
    All,
};

class ConfigGenerator {
public:
    ConfigGenerator(bool example, int sections = 0)
        : example(example)
        , generateSections(sections)
    {
    }
    std::string generate(const fs::path& userHome, const fs::path& configDir, const fs::path& dataDir, const fs::path& magicFile);

    void generateServer(const fs::path& userHome, const fs::path& configDir, const fs::path& dataDir);
    void generateUi();
    void generateExtendedRuntime();
    void generateDynamics();
    void generateDatabase(const fs::path& prefixDir);
    void generateImport(const fs::path& prefixDir, const fs::path& configDir, const fs::path& magicFile);
    void generateMappings();
    void generateBoxlayout(ConfigVal option);
    void generateOnlineContent();
    void generateTranscoding();
    void generateUdn(bool doExport = true);

    std::shared_ptr<pugi::xml_node> init();

    std::shared_ptr<pugi::xml_node> getNode(const std::string& tag) const;

    static int remapGeneratorSections(const std::string& arg);
    static std::string printSections(int section);
    static int makeSections(const std::string& optValue);
    bool isGenerated(GeneratorSections section);

protected:
    bool example { false };
    int generateSections { 0 };
    std::map<std::string, std::shared_ptr<pugi::xml_node>> generated;
    pugi::xml_document doc;
    void generateOptions(const std::vector<std::pair<ConfigVal, bool>>& options);
    void generateServerOptions(std::shared_ptr<pugi::xml_node>& server, const fs::path& userHome, const fs::path& configDir, const fs::path& dataDir);
    void generateImportOptions(const fs::path& prefixDir, const fs::path& configDir, const fs::path& magicFile);

    std::shared_ptr<pugi::xml_node> setValue(const std::string& tag, const std::shared_ptr<ConfigSetup>& cs = {}, const std::string& value = "", bool makeLastChild = false);
    std::shared_ptr<pugi::xml_node> setDictionary(ConfigVal option);
    std::shared_ptr<pugi::xml_node> setVector(ConfigVal option);
    std::shared_ptr<pugi::xml_node> setValue(ConfigVal option, const std::string& value = "");
    std::shared_ptr<pugi::xml_node> setValue(ConfigVal option, ConfigVal attr, const std::string& value);
    std::shared_ptr<pugi::xml_node> setValue(ConfigVal option, const std::string& key, const std::string& value);
    std::shared_ptr<pugi::xml_node> setValue(ConfigVal option, ConfigVal dict, ConfigVal attr, const std::string& value);

    static std::shared_ptr<pugi::xml_node> setValue(const std::shared_ptr<pugi::xml_node>& parent, ConfigVal option, const std::string& value);
    static std::map<GeneratorSections, std::string_view> sections;
};

#endif // GERBERA_CONFIG_GENERATOR_H
