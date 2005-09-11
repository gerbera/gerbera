/*  config_manager.cc - this file is part of MediaTomb.
                                                                                
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

#include <stdio.h>
#include "uuid/uuid.h"
#include "common.h"
#include "config_manager.h"
#include "storage.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "tools.h"

using namespace zmm;
using namespace mxml;

static Ref<ConfigManager> instance;

String ConfigManager::construct_path(String path)
{
    String home = getOption(_("/server/home"));
#ifndef __CYGWIN__
    if (path.charAt(0) == '/')
        return path;
#else
    if (path.charAt(1) == ':')
        return path;
#endif
    if (home == "." && path.charAt(0) == '.')
        return path;
    
    if (home == "")
        return _(".") + DIR_SEPARATOR + path;
    else
        return home + DIR_SEPARATOR + path;
}

ConfigManager::ConfigManager() : Object()
{
}

void ConfigManager::init(String filename, String userhome)
{
    instance = Ref<ConfigManager>(new ConfigManager());

    String error_message;
    String home;
    bool home_ok = true;

    if (filename == nil)
    {
        // we are looking for ~/.mediatomb
        if (home_ok && (!check_path(userhome + DIR_SEPARATOR + DEFAULT_CONFIG_HOME + DIR_SEPARATOR + DEFAULT_CONFIG_NAME)))
        {
            home_ok = false;
        }
        else
        {
            home = userhome + DIR_SEPARATOR + DEFAULT_CONFIG_HOME;
            filename = home + DIR_SEPARATOR + DEFAULT_CONFIG_NAME;
        }
        if (!home_ok)
        {
            if (!check_path(_("") + DIR_SEPARATOR + DEFAULT_ETC + DIR_SEPARATOR + DEFAULT_SYSTEM_HOME + DIR_SEPARATOR + DEFAULT_CONFIG_NAME))
            {
                throw Exception(_("\nThe server configuration file could not be found in ~/.mediatomb and\n") +
                                       "/etc/mediatomb. Please run the tomb-install script or specify an alternative\n" +  
                                       "configuration file on the command line. For a list of options use: mediatomb -h\n");
            }

            home = _("") + DIR_SEPARATOR + DEFAULT_ETC + DIR_SEPARATOR + DEFAULT_SYSTEM_HOME;
            filename = home + DIR_SEPARATOR + DEFAULT_CONFIG_NAME;
        }

        if (home_ok)
            log_info(("Loading configuration from ~/.mediatomb/config.xml\n"));
        else
            log_info(("Loading configuration from /etc/mediatomb/config.xml\n"));

//        instance->load(userhome + DIR_SEPARATOR + DEFAULT_CONFIG_HOME + DIR_SEPARATOR + DEFAULT_CONFIG_NAME);
    }
//    else

    log_info(("Loading configuration from: %s\n", filename.c_str()));
    instance->load(filename);

    instance->prepare_udn();    
    instance->validate(home);    
}

void ConfigManager::validate(String serverhome)
{
    String temp;

    log_info(("Checking configuration...\n"));
   
    // first check if the config file itself looks ok, it must have a config
    // and a server tag
    if (root->getName() != "config")
        throw Exception(_("Error in config file: <config> tag not found"));

    if (root->getChild(_("server")) == nil)
        throw Exception(_("Error in config file: <server> tag not found"));

    // now go through the mandatory parameters, if something is missing
    // here we will not start the server
//    temp = checkOption_("/server/home");
//    check_path_ex(temp, true);

    getOption(_("/server/home"), serverhome); 

    prepare_path(_("/server/home"), true);
    
//    temp = checkOption_("/server/webroot");
//    check_path_ex(construct_path(temp), true);

    prepare_path(_("/server/webroot"), true);
    
    if (string_ok(getOption(_("/server/servedir"), _(""))))
        prepare_path(_("/server/servedir"), true);
    
    // udn should be already prepared
    checkOptionString(_("/server/udn"));

    checkOptionString(_("/server/storage/attribute::driver"));

    String dbDriver = getOption(_("/server/storage/attribute::driver"));

    // checking database driver options
    do
    {
#ifdef HAVE_SQLITE3	    
        if (dbDriver == "sqlite3")
        {
            prepare_path(_("/server/storage/database-file"));            
            break;
        }
#endif
#ifdef HAVE_MYSQL
        if (dbDriver == "mysql")
        {
            checkOptionString(_("/server/storage/host"));
            checkOptionString(_("/server/storage/database"));
            checkOptionString(_("/server/storage/username"));
            if (getElement(_("/server/storage/password")) == nil)
                throw Exception(_("/server/storage/password option not found")); 
            break;
        }
#endif
        // other database types...
        throw Exception(_("Unknown storage driver: ") + dbDriver);
    }
    while (false);

    
//    temp = checkOption_("/server/storage/database-file");
//    check_path_ex(construct_path(temp));

    // now go through the optional settings and fix them if anything is missing
   
    temp = getOption(_("/server/ui/attribute::enabled"),
                     _(DEFAULT_UI_VALUE));
    if ((temp != "yes") && (temp != "no"))
        throw Exception(_("Error in config file: incorrect parameter for <ui enabled=\")\" /> attribute"));

    getOption(_("/import/mappings/extension-mimetype/attribute::ignore-unknown"),
              _(DEFAULT_IGNORE_UNKNOWN_EXTENSIONS));

    getOption(_("/import/filesystem-charset"),
              _(DEFAULT_FILESYSTEM_CHARSET));
    getOption(_("/import/metadata-charset"),
              _(DEFAULT_FILESYSTEM_CHARSET));
    log_info(("checking ip..\n"));
    getOption(_("/server/ip"), _("")); // bind to any IP address
    getOption(_("/server/bookmark"), _(DEFAULT_BOOKMARK_FILE));
    getOption(_("/server/name"), _(DESC_FRIENDLY_NAME));

/*
#ifdef HAVE_JS
    try
    {
        temp = getOption(_("/import/script"));
    }
    catch (Exception ex)
    {
        temp = "";
    }

    if (string_ok(temp))
            check_path_ex(construct_path(temp));

    Ref<Element> script = getElement("/import/script");
    if (script != nil)
            script->setText(construct_path(temp));
#endif
*/

#ifdef HAVE_JS
    prepare_path(_("/import/script"));
#endif

    getIntOption(_("/server/port"), 0); // 0 means, that the SDK will any free port itself
    getIntOption(_("/server/alive"), DEFAULT_ALIVE_INTERVAL);


    Ref<Element> el = getElement(_("/import/mappings/mimetype-upnpclass"));
    if (el == nil)
    {
//        Ref<Dictionary> dict = createDictionaryFromNodeset(el, "map", "from", "to");
        getOption(_("/import/mappings/mimetype-upnpclass"), _(""));
    }
   
#ifdef HAVE_EXIF    

    el = getElement(_("/import/library-options/libexif/auxdata"));
    if (el == nil)
    {
    //    Ref<Array<StringBase> > arr = createArrayFromNodeset(el, "add", "tag");
        getOption(_("/import/library-options/libexif/auxdata"),
                  _(""));
        
    }

#endif // HAVE_EXIF

#ifdef HAVE_EXTRACTOR

    el = getElement(_("/import/library-options/libextractor/auxdata"));
    if (el == nil)
    {
    //    Ref<Array<StringBase> > arr = createArrayFromNodeset(el, "add", "tag");
        getOption(_("/import/library-options/libextractor/auxdata"),
                  _(""));
    }
#endif // HAVE_EXTRACTOR

#ifdef HAVE_MAGIC
    if (string_ok(getOption(_("/import/magic-file"), _(""))))
    {
        prepare_path(_("/import/magic-file"));
    }
#endif

    log_info(("Configuration check succeeded.\n"));

    log_debug(("Config file dump after validation: \n%s\n", root->print().c_str()));
}

void ConfigManager::prepare_udn()
{
    bool need_to_save = false;
    
    Ref<Element> element = root->getChild(_("server"))->getChild(_("udn"));
    if (element->getText() == nil || element->getText() == "")
    {
        char   uuid_str[37];
        uuid_t uuid;
                
        uuid_generate(uuid);
        uuid_unparse(uuid, uuid_str);

        log_info(("UUID GENERATED!: %s\n", uuid_str));
        
        element->setText(_("uuid:") + uuid_str);

        need_to_save = true;
    }
    
    if (need_to_save)
        save();
}

void ConfigManager::prepare_path(String xpath, bool needDir)
{
    String temp;

    temp = checkOptionString(xpath);
    
    temp = construct_path(temp);

    check_path_ex(temp, needDir);

    Ref<Element> script = getElement(xpath);
    if (script != nil)
        script->setText(temp);
}

Ref<ConfigManager> ConfigManager::getInstance()
{
    return instance;
}


void ConfigManager::save()
{
    save(filename);
}

void ConfigManager::save(String filename)
{
    String content = root->print();

    FILE *file = fopen(filename.c_str(), "wb");
    if (file == NULL)
    {
        throw Exception(_("could not open config file ") +
                        filename + " for writing : " + strerror(errno));
    }

    int bytesWritten = fwrite(content.c_str(), sizeof(char),
                              content.length(), file);
    if (bytesWritten < content.length())
    {
        throw Exception(_("could not write to config file ") +
                        filename + " : " + strerror(errno));
    }
    
    fclose(file);
}

void ConfigManager::load(String filename)
{
    this->filename = filename;
    Ref<Parser> parser(new Parser());
    root = parser->parseFile(filename);
}

String ConfigManager::getOption(String xpath, String def)
{      
    Ref<XPath> rootXPath(new XPath(root));
    String value = rootXPath->getText(xpath);
    if (string_ok(value))
        return trim_string(value);

    log_info(("Config: option not found: %s using default value: %s\n",
           xpath.c_str(), def.c_str()));
    
    String pathPart = XPath::getPathPart(xpath);
    String axisPart = XPath::getAxisPart(xpath);

    Ref<Array<StringBase> >parts = split_string(pathPart, '/');
    
    Ref<Element> cur = root;
    String attr = nil;
    
    int i;
    Ref<Element> child;
    for (i = 0; i < parts->size(); i++)
    {
        String part = parts->get(i);
        child = cur->getChild(part);
        if (child == nil)
            break;
        cur = child;
    }
    // here cur is the last existing element in the path
    for (; i < parts->size(); i++)
    {
        String part = parts->get(i);
        child = Ref<Element>(new Element(part));
        cur->appendChild(child);
        cur = child;
    }
    
    if (axisPart != nil)
    {
        String axis = XPath::getAxis(axisPart);
        String spec = XPath::getSpec(axisPart);
        if (axis != "attribute")
        {
            throw Exception(_("ConfigManager::getOption: only attribute:: axis supported"));
        }
        cur->setAttribute(spec, def);
    } 
    else
        cur->setText(def);

    return def;
}

int ConfigManager::getIntOption(String xpath, int def)
{
    String sDef;

    sDef = String::from(def);
    
    String sVal = getOption(xpath, sDef);
    return sVal.toInt();
}

String ConfigManager::getOption(String xpath)
{      
    Ref<XPath> rootXPath(new XPath(root));
    String value = rootXPath->getText(xpath);

    /// \TODO is this ok?
//    if (string_ok(value))
//        return value;
    if (value != nil)
        return trim_string(value);
    throw Exception(_("Config: option not found: ") + xpath);
}

int ConfigManager::getIntOption(String xpath)
{
    String sVal = getOption(xpath);
    int val = sVal.toInt();
    return val;
}


Ref<Element> ConfigManager::getElement(String xpath)
{      
    Ref<XPath> rootXPath(new XPath(root));
    return rootXPath->getElement(xpath);
}

void ConfigManager::writeBookmark(String ip, String port)
{
    FILE    *f;
    String  filename;
    String  path;
    String  data; 
    int     size; 
  
    String value = ConfigManager::getInstance()->getOption(_("/server/ui/attribute::enabled"));
    if (value != "yes")
    {
        data = http_redirect_to(ip, port, _("disabled.html"));
    }
    else
    {
        data = http_redirect_to(ip, port);
    }

    filename = getOption(_("/server/bookmark"));
    path = construct_path(filename);
    
        
    f = fopen(path.c_str(), "w");
    if (f == NULL)
    {
        throw Exception(_("writeBookmark: failed to open: ") + path);
    }

    size = fwrite(data.c_str(), sizeof(char), data.length(), f);
    fclose(f);

    if (size < data.length())
        throw Exception(_("write_Bookmark: failed to write to: ") + path);

}

String ConfigManager::checkOptionString(String xpath)
{
    String temp = getOption(xpath);
    if (!string_ok(temp))
        throw Exception(_("Config: value of ") + xpath + " tag is invalid");

    return temp;
}

Ref<Dictionary> ConfigManager::createDictionaryFromNodeset(Ref<Element> element, String nodeName, String keyAttr, String valAttr)
{
    Ref<Dictionary> dict(new Dictionary());
    String key;
    String value;

    for (int i = 0; i < element->childCount(); i++)
    {
        Ref<Element> child = element->getChild(i);
        if (child->getName() == nodeName)
        {
            key = child->getAttribute(keyAttr);
            value = child->getAttribute(valAttr);

            if (string_ok(key) && string_ok(value))
                dict->put(key, value);
        }
        
    }

    return dict;
}

Ref<Array<StringBase> > ConfigManager::createArrayFromNodeset(Ref<mxml::Element> element, String nodeName, String attrName)
{
    String attrValue;
    Ref<Array<StringBase> > arr(new Array<StringBase>());

    for (int i = 0; i < element->childCount(); i++)
    {
        Ref<Element> child = element->getChild(i);
        if (child->getName() == nodeName)
        {
            attrValue = child->getAttribute(attrName);

            if (string_ok(attrValue))
                arr->append(attrValue);
        }
    }

    return arr;
}
