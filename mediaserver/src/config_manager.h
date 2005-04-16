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
    zmm::String home;
    zmm::String fs_charset;
public:
    ConfigManager();

    void ConfigManager::load(zmm::String filename);
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
    zmm::String ConfigManager::checkOptionString(zmm::String xpath);

    /// \brief adds path to server home and returns the result.
    /// \param path path that should be constructed
    ///
    /// This function does the following: we have a "server home",
    /// (usually ~/.mediatomb), this server home has other subdirectories
    /// or files. The function returns a string similar to "server home"/"path",
    /// constructing an absolute path.
    ///
    /// \return constructed path.
    zmm::String constructPath(zmm::String path);
    
    // call this function to initialize global ConfigManager object
    static void init(zmm::String filename, zmm::String userhome);

    static zmm::Ref<ConfigManager> getInstance();
protected:
    // will be used only internally
    void save(zmm::String filename);
    void create();
    void validate();
    void prepare();
};

#endif // __CONFIG_MANAGER_H__

