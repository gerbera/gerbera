/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    script.h - this file is part of MediaTomb.
    
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

/// \file script.h

#ifndef __SCRIPTING_SCRIPT_H__
#define __SCRIPTING_SCRIPT_H__

#include <mutex>
#include "duktape.h"
#include "common.h"
#include "runtime.h"
#include "cds_objects.h"
#include "util/string_converter.h"

// perform garbage collection after script has been run for x times
#define JS_CALL_GC_AFTER_NUM    (1000)

typedef enum
{
    S_IMPORT = 0,
    S_PLAYLIST
} script_class_t;

typedef enum
{
    M2I,
    F2I,
    J2I,
    P2I,
    I2I,
} charset_convert_t;

// forward declaration
class ConfigManager;
class ContentManager;

class Script
{
public:
    virtual ~Script();

    std::string getProperty(const std::string& name);
    int getBoolProperty(const std::string& name);
    int getIntProperty(const std::string& name, int def);

    void setProperty(const std::string& name, const std::string& value);
    void setIntProperty(const std::string& name, int value);

    void defineFunction(const std::string& name, duk_c_function function, uint32_t numParams);
    void defineFunctions(duk_function_list_entry *functions);
    void load(const std::string& scriptPath);
    void load(std::string scriptText, std::string scriptPath);

    std::shared_ptr<CdsObject> dukObject2cdsObject(const std::shared_ptr<CdsObject>& pcd);
    void cdsObject2dukObject(const std::shared_ptr<CdsObject>& obj);

    virtual script_class_t whoami() = 0;

    std::shared_ptr<CdsObject> getProcessedObject();

    std::string convertToCharset(const std::string& str, charset_convert_t chr);

    static Script *getContextScript(duk_context *ctx);

    std::shared_ptr<ConfigManager> getConfig() { return config; }
    std::shared_ptr<Storage> getStorage() { return storage; }
    std::shared_ptr<ContentManager> getContent() { return content; }

protected:
    Script(const std::shared_ptr<ConfigManager>& config,
        std::shared_ptr<Storage> storage,
        std::shared_ptr<ContentManager> content,
        const std::shared_ptr<Runtime>& runtime, const std::string& name);
    void execute();
    int gc_counter;

    // object that is currently being processed by the script (set in import
    // script)
    std::shared_ptr<CdsObject> processed;
    
    duk_context *ctx;

protected:
    std::shared_ptr<ConfigManager> config;
    std::shared_ptr<Storage> storage;
    std::shared_ptr<ContentManager> content;
    std::shared_ptr<Runtime> runtime;

private:
    std::string name;
    void _load(const std::string& scriptPath);
    void _execute();
    std::unique_ptr<StringConverter> _p2i;
    std::unique_ptr<StringConverter> _j2i;
    std::unique_ptr<StringConverter> _f2i;
    std::unique_ptr<StringConverter> _m2i;
    std::unique_ptr<StringConverter> _i2i;
};

#endif // __SCRIPTING_SCRIPT_H__
