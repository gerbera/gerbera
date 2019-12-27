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
#include "string_converter.h"

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

class Script : public zmm::Object
{
public:
    zmm::Ref<Runtime> runtime;
    
public:
    virtual ~Script();
    
    std::string getProperty(std::string name);
    int getBoolProperty(std::string name);
    int getIntProperty(std::string name, int def);
    
    void setProperty(std::string name, std::string value);
    void setIntProperty(std::string name, int value);
    
    void defineFunction(std::string name, duk_c_function function, uint32_t numParams);
    void defineFunctions(duk_function_list_entry *functions);
    void load(std::string scriptPath);
    void load(std::string scriptText, std::string scriptPath);
    
    zmm::Ref<CdsObject> dukObject2cdsObject(zmm::Ref<CdsObject> pcd);
    void cdsObject2dukObject(zmm::Ref<CdsObject> obj);
    
    virtual script_class_t whoami() = 0;

    zmm::Ref<CdsObject> getProcessedObject(); 

    std::string convertToCharset(std::string str, charset_convert_t chr);
    
    static Script *getContextScript(duk_context *ctx);

protected:
    Script(zmm::Ref<Runtime> runtime, std::string name);
    void execute();
    int gc_counter;

    // object that is currently being processed by the script (set in import
    // script)
    zmm::Ref<CdsObject> processed;
    
    using AutoLock = std::lock_guard<std::recursive_mutex>;
    duk_context *ctx;

private:
    std::string name;
    void _load(std::string scriptPath);
    void _execute();
    zmm::Ref<StringConverter> _p2i;
    zmm::Ref<StringConverter> _j2i;
    zmm::Ref<StringConverter> _f2i;
    zmm::Ref<StringConverter> _m2i;
    zmm::Ref<StringConverter> _i2i;
};

#endif // __SCRIPTING_SCRIPT_H__
