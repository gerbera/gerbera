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
    /// \param name option name
    /// \param def default value if option not found
    ///
    /// the name parameter has the XPath syntax.
    /// Currently only two xpath constructs are supported:
    /// "/path/to/option" will return the text value of the given "option" element
    /// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
    zmm::String getOption(zmm::String name, zmm::String def);    
    
    /// \brief same as getOption but returns an integer value of the option
    int getIntOption(zmm::String name, int def);
    
    /// \brief Returns a config option with the given path, an exception is raised if option does not exist.
    /// \param name option name
    ///
    /// the name parameter has the XPath syntax.
    /// Currently only two xpath constructs are supported:
    /// "/path/to/option" will return the text value of the given "option" element
    /// "/path/to/option/attribute::attr" will return the value of the attribute "attr"
    zmm::String getOption(zmm::String name);    
    
    /// \brief same as getOption but returns an integer value of the option
    int getIntOption(zmm::String name);    
   
//    zmm::String getStorageOptions(zmm::String &type, zmm::Ref<Dictionary> &params);

    /// \brief Creates a html file that is a redirector to the current server instance
    void writeBookmark(zmm::String ip, zmm::String port);

    // call this function to initialize global ConfigManager object
    static void init(zmm::String filename, zmm::String userhome);

    static zmm::Ref<ConfigManager> getInstance();
protected:
    // will be used only internally
    void save(zmm::String filename);
    void create();
    void validate();
    void prepare();
    zmm::String construct_path(zmm::String path);
    void validate_paths(char **valid, bool isDir);
};

#endif // __CONFIG_MANAGER_H__

