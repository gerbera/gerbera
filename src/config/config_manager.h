/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    config_manager.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/// \file config_manager.h
///\brief Definitions of the ConfigManager class.

#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

#include <filesystem>
#include <map>
#include <memory>
#include <pugixml.hpp>
namespace fs = std::filesystem;

#include "common.h"
#include "config.h"

// forward declaration
class AutoscanList;
class AutoscanDirectory;
enum class ScanMode;
class ConfigOption;
class Storage;
class TranscodingProfileList;

class ConfigManager : public Config {
public:
    ConfigManager(fs::path filename,
        const fs::path& userhome, const fs::path& config_dir,
        fs::path prefix_dir, fs::path magic_file,
        std::string ip, std::string interface, int port,
        bool debug_logging);
    virtual ~ConfigManager();

    /// \brief Returns the name of the config file that was used to launch the server.
    fs::path getConfigFilename() const override { return filename; }

    void load(const fs::path& filename, const fs::path& userHome);

    /// \brief returns a config option of type std::string
    /// \param option option to retrieve.
    std::string getOption(config_option_t option) override;

    /// \brief returns a config option of type int
    /// \param option option to retrieve.
    int getIntOption(config_option_t option) override;

    /// \brief returns a config option of type bool
    /// \param option option to retrieve.
    bool getBoolOption(config_option_t option) override;

    /// \brief returns a config option of type dictionary
    /// \param option option to retrieve.
    std::map<std::string, std::string> getDictionaryOption(config_option_t option) override;

    /// \brief returns a config option of type array of string
    /// \param option option to retrieve.
    std::vector<std::string> getArrayOption(config_option_t option) override;

    /// \brief returns a config option of type AutoscanList
    /// \param option to retrieve
    std::shared_ptr<AutoscanList> getAutoscanListOption(config_option_t option) override;

    /// \brief returns a config option of type TranscodingProfileList
    /// \param option to retrieve
    std::shared_ptr<TranscodingProfileList> getTranscodingProfileListOption(config_option_t option) override;

    static bool isDebugLogging() { return debug_logging; }

protected:
    fs::path filename;
    fs::path prefix_dir;
    fs::path magic_file;
    std::string ip;
    std::string interface;
    int port;
    static bool debug_logging;

    std::unique_ptr<pugi::xml_document> xmlDoc;

    std::unique_ptr<std::vector<std::shared_ptr<ConfigOption>>> options;

    /// \brief Returns a config option with the given xpath, if option does not exist a default value is returned.
    /// \param xpath option xpath
    /// \param def default value if option not found
    ///
    /// The xpath parameter has XPath syntax:
    /// "/path/to/option" will return the text value of the given "option" element
    /// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
    std::string getOption(std::string xpath, std::string def) const;

    /// \brief same as getOption but returns an integer value of the option
    int getIntOption(std::string xpath, int def) const;

    /// \brief Returns a config option with the given xpath, an exception is raised if option does not exist.
    /// \param xpath option xpath.
    ///
    /// The xpath parameter has XPath syntax:
    /// "/path/to/option" will return the text value of the given "option" element
    /// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
    std::string getOption(std::string xpath) const;

    /// \brief Returns an integer value of the option with the given xpath, an exception is raised if option does not exist.
    /// \param xpath option xpath.
    ///
    /// The xpath parameter has XPath syntax:
    /// "/path/to/option" will return the text value of the given "option" element
    /// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
    int getIntOption(std::string xpath) const;

    /// \brief Returns a config XML element with the given xpath, an exception is raised if element does not exist.
    /// \param xpath option xpath.
    ///
    /// The xpath parameter has XPath syntax:
    /// "/path/to/element" will return the text value of the given "element" element
    pugi::xml_node getElement(std::string xpath) const;

    /// \brief resolve path against home, an exception is raised if path does not exist on filesystem.
    /// \param path path to be resolved
    /// \param isFile file or directory
    /// \param mustExist file/directory must exist
    fs::path resolvePath(fs::path path, bool isFile = false, bool mustExist = true);

    /// \brief Creates a dictionary from an XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param nodeName name of each node in the set
    /// \param keyAttr attribute name to be used as a key
    /// \param valAttr attribute name to be used as value
    ///
    /// The basic idea is the following:
    /// You have a piece of XML that looks like this
    /// <some-section>
    ///    <map from="1" to="2"/>
    ///    <map from="3" to="4"/>
    /// </some-section>
    ///
    /// This function will create a dictionary with the following
    /// key:value paris: "1":"2", "3":"4"
    static std::map<std::string, std::string> createDictionaryFromNode(const pugi::xml_node& element,
        const std::string& nodeName, const std::string& keyAttr, const std::string& valAttr, bool tolower = false);

    /// \brief Creates an array of AutoscanDirectory objects from a XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param scanmode add only directories with the specified scanmode to the array
    std::shared_ptr<AutoscanList> createAutoscanListFromNode(const std::shared_ptr<Storage>& storage, const pugi::xml_node& element,
        ScanMode scanmode);

    /// \brief Creates ab aray of TranscodingProfile objects from an XML
    /// nodeset.
    /// \param element starting element of the nodeset.
    static std::shared_ptr<TranscodingProfileList> createTranscodingProfileListFromNode(const pugi::xml_node& element);

    /// \brief Creates an array of strings from an XML nodeset.
    /// \param element starting element of the nodeset.
    /// \param nodeName name of each node in the set
    /// \param attrName name of the attribute, the value of which shouldbe extracted
    ///
    /// Similar to \fn createDictionaryFromNodeset() this one extracts
    /// data from the following XML:
    /// <some-section>
    ///     <tag attr="data"/>
    ///     <tag attr="otherdata"/>
    /// <some-section>
    ///
    /// This function will create an array like that: ["data", "otherdata"]
    static std::vector<std::string> createArrayFromNode(const pugi::xml_node& element, const std::string& nodeName, const std::string& attrName);

    void dumpOptions();
};

#endif // __CONFIG_MANAGER_H__
