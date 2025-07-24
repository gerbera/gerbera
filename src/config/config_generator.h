/*GRB*

    Gerbera - https://gerbera.io/

    config_generator.h - this file is part of Gerbera.

    Copyright (C) 2016-2025 Gerbera Contributors

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

#include "config/config.h"

#include <pugixml.hpp>
#include <utility>

// forward declarations
class ConfigDefinition;
class ConfigSetup;

/// \brief list of section ids available for direct printing
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

/// \brief Class to generate default or example configuration when called with respective command line
/// also used in tests
class ConfigGenerator {
public:
    ConfigGenerator(
        std::shared_ptr<ConfigDefinition> definition,
        std::string version,
        bool example,
        int sections = 0)
        : definition(std::move(definition))
        , version(std::move(version))
        , example(example)
        , generateSections(sections)
    {
    }
    /// \brief gerate full output
    std::string generate(
        const fs::path& userHome,
        const fs::path& configDir,
        const fs::path& dataDir,
        const fs::path& magicFile);

    /// \brief directly generate server section
    void generateServer(const fs::path& userHome, const fs::path& configDir, const fs::path& dataDir);
    /// \brief directly generate ui section
    void generateUi();
    /// \brief directly generate extended-runtime section
    void generateExtendedRuntime();
    /// \brief directly generate dynamic section
    void generateDynamics();
    /// \brief directly generate database section
    void generateDatabase(const fs::path& prefixDir);
    /// \brief directly generate import section
    void generateImport(const fs::path& prefixDir, const fs::path& configDir, const fs::path& magicFile);
    /// \brief directly generate mapping section
    void generateMappings();
    /// \brief directly generate box-layout section
    void generateBoxlayout(ConfigVal option);
    /// \brief directly generate online-content section
    void generateOnlineContent();
    /// \brief directly generate transcoding section
    void generateTranscoding();
    /// \brief directly generate udn value
    void generateUdn(bool doExport = true);

    /// \brief get initial item (root)
    std::shared_ptr<pugi::xml_node> init();

    /// \brief query node stored under tag
    std::shared_ptr<pugi::xml_node> getNode(const std::string& tag) const;

    /// \brief check selected sections
    bool isGenerated(GeneratorSections section);
    /// \brief extract section id from string argument
    static int remapGeneratorSections(const std::string& arg);
    /// \brief print section names for given id
    static std::string printSections(int section);
    /// \brief convert section string to section id bit field
    static int makeSections(const std::string& optValue);

protected:
    std::shared_ptr<ConfigDefinition> definition;
    /// @brief version of gerbera
    std::string version;
    /// \brief activate generation of full example with all defaults
    bool example { false };
    /// \brief bitfield with all sections to generate
    int generateSections { 0 };
    /// \brief dictionary with all generated nodes providing easy access to add subnotes
    std::map<std::string, std::shared_ptr<pugi::xml_node>> generated;
    /// \brief root xml document
    pugi::xml_document doc;

    /// \brief generate list of options
    void generateOptions(const std::vector<std::pair<ConfigVal, bool>>& options);
    /// \brief generate all server options
    void generateServerOptions(
        std::shared_ptr<pugi::xml_node>& server,
        const fs::path& userHome,
        const fs::path& configDir,
        const fs::path& dataDir);
    /// \brief generate all import options
    void generateImportOptions(
        const fs::path& prefixDir,
        const fs::path& configDir,
        const fs::path& magicFile);

    /// \brief create xml entry based on xpath in tag
    std::shared_ptr<pugi::xml_node> setValue(
        const std::string& tag,
        const std::shared_ptr<ConfigSetup>& cs = {},
        const std::string& value = "",
        bool makeLastChild = false);
    /// \brief create xml entry based on option and value
    std::shared_ptr<pugi::xml_node> setValue(ConfigVal option, const std::string& value = "");
    std::shared_ptr<pugi::xml_node> setValue(ConfigVal option, ConfigVal attr, const std::string& value);
    std::shared_ptr<pugi::xml_node> setValue(ConfigVal option, const std::string& key, const std::string& value);
    std::shared_ptr<pugi::xml_node> setValue(ConfigVal option, ConfigVal dict, ConfigVal attr, const std::string& value);
    /// \brief create xml subtree from dictionary option defaults
    std::shared_ptr<pugi::xml_node> setDictionary(ConfigVal option);
    /// \brief create xml subtree from vetcor option defaults
    std::shared_ptr<pugi::xml_node> setVector(ConfigVal option);

    /// \brief create subentry of xml node
    std::shared_ptr<pugi::xml_node> setXmlValue(const std::shared_ptr<pugi::xml_node>& parent, ConfigVal option, const std::string& value);

    /// \brief dictionary with section names
    static std::map<GeneratorSections, std::string_view> sections;
};

#endif // GERBERA_CONFIG_GENERATOR_H
