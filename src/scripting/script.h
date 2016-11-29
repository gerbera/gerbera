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

#include <jsapi.h>
#include "common.h"
#include "runtime.h"
#include "cds_objects.h"
#include "string_converter.h"

// perform garbage collection after script has been run for x times
#define JS_CALL_GC_AFTER_NUM    (1000)

typedef enum
{
    S_IMPORT = 0,
    S_PLAYLIST,
    S_DVD
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
    JSRuntime *rt;
    JSContext *cx;
    JSObject  *glob;
    JSObject *script;
    JSObject *common_script;
    
public:
    Script(zmm::Ref<Runtime> runtime);
    virtual ~Script();
    
    zmm::String getProperty(JSObject *obj, zmm::String name);
    int getBoolProperty(JSObject *obj, zmm::String name);
    int getIntProperty(JSObject *obj, zmm::String name, int def);
    JSObject *getObjectProperty(JSObject *obj, zmm::String name);
    
    void setProperty(JSObject *obj, zmm::String name, zmm::String value);
    void setIntProperty(JSObject *obj, zmm::String name, int value);
    void setObjectProperty(JSObject *parent, zmm::String name, JSObject *obj);
    
    void deleteProperty(JSObject *obj, zmm::String name);
    
    JSObject *getGlobalObject();
    void setGlobalObject(JSObject *glob);
    
    JSContext *getContext();
    
    void defineFunction(zmm::String name, JSNative function, uint32_t numParams);
    void defineFunctions(JSFunctionSpec *functions);
    void load(zmm::String scriptPath);
    void load(zmm::String scriptText, zmm::String scriptPath);
    
    zmm::Ref<CdsObject> jsObject2cdsObject(JSObject *js, zmm::Ref<CdsObject> pcd);
    void cdsObject2jsObject(zmm::Ref<CdsObject> obj, JSObject *js);
    
    virtual script_class_t whoami() = 0;

    zmm::Ref<CdsObject> getProcessedObject(); 

    zmm::String convertToCharset(zmm::String str, charset_convert_t chr);
    
protected:
    void execute();
    int gc_counter;

    // object that is currently being processed by the script (set in import
    // script)
    zmm::Ref<CdsObject> processed;
    
private:
    JSObject *common_root;

    void initGlobalObject();
    JSObject *_load(zmm::String scriptPath);
    void _execute(JSObject *scr);
    zmm::Ref<StringConverter> _p2i;
    zmm::Ref<StringConverter> _j2i;
    zmm::Ref<StringConverter> _f2i;
    zmm::Ref<StringConverter> _m2i;
    zmm::Ref<StringConverter> _i2i;
};

#endif // __SCRIPTING_SCRIPT_H__
