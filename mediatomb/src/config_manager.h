/*  config_manager.h - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
///\file config_manager.h
///\brief Definitions of the ConfigManager class. 

#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

#include "common.h"
#include "dictionary.h"
#include "xpath.h"

class ConfigManager : public zmm::Object
{
protected:
    zmm::String filename;
    zmm::Ref<mxml::Element> root;

public:
    ConfigManager();

    /// \brief Returns the name of the config file that was used to launch the server.
    inline zmm::String getConfigFilename() { return filename; }

    void load(zmm::String filename);
    void save();

    /// \brief Returns a config option with the given path, if option does not exist a default value is returned.
    /// \param xpath option xpath
    /// \param def default value if option not found
    ///
    /// the name parameter has the XPath syntax.
    /// Currently only two xpath constructs are supported:
    /// "/path/to/option" will return the text value of the given "option" element
    /// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
    zmm::String getOption(zmm::String xpath, zmm::String def);    
    
    /// \brief same as getOption but returns an integer value of the option
    int getIntOption(zmm::String xpath, int def);
    
    /// \brief Returns a config option with the given path, an exception is raised if option does not exist.
    /// \param xpath option xpath.
    ///
    /// The xpath parameter has XPath syntax.
    /// Currently only two xpath constructs are supported:
    /// "/path/to/option" will return the text value of the given "option" element
    /// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
    zmm::String getOption(zmm::String xpath);
    
    /// \brief same as getOption but returns an integer value of the option
    int getIntOption(zmm::String xpath);

    /// \brief Returns a config XML element with the given path, an exception is raised if element does not exist.
    /// \param xpath option xpath.
    ///
    /// The xpath parameter has XPath syntax.
    /// "/path/to/element" will return the text value of the given "element" element
    zmm::Ref<mxml::Element> getElement(zmm::String xpath);
            
    /// \brief Creates a html file that is a redirector to the current server instance
    void writeBookmark(zmm::String ip, zmm::String port);

    /// \brief Checks if the string returned by getOption is valid.
    /// \param xpath xpath expression to the XML node
    zmm::String checkOptionString(zmm::String xpath);

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
    zmm::Ref<Dictionary> createDictionaryFromNodeset(zmm::Ref<mxml::Element> element, zmm::String nodeName, zmm::String keyAttr, zmm::String valAttr);
  
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
    zmm::Ref<zmm::Array<zmm::StringBase> > createArrayFromNodeset(zmm::Ref<mxml::Element> element, zmm::String nodeName, zmm::String attrName); 

    // call this function to initialize global ConfigManager object
    static void init(zmm::String filename, zmm::String userhome);

    static zmm::Ref<ConfigManager> getInstance();

protected:
    // will be used only internally
    void save(zmm::String filename);
    void validate(zmm::String serverhome);
    void prepare_udn();
    zmm::String construct_path(zmm::String path);
    void prepare_path(zmm::String path, bool needDir = false);
};

#endif // __CONFIG_MANAGER_H__

