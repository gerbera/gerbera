/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    script.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

#ifdef __APPLE__
    #define XP_MAC 1
#else
    #define XP_UNIX 1
#endif

#include <jsapi.h>
#include "common.h"
#include "runtime.h"
#include "cds_objects.h"

class Script : public zmm::Object
{
public:
    zmm::Ref<Runtime> runtime;
    JSRuntime *rt;
    JSContext *cx;
    JSObject  *glob;
	JSScript *script;
public:
	Script(zmm::Ref<Runtime> runtime, JSContext *cx = NULL);
	virtual ~Script();
    
    zmm::String getProperty(JSObject *obj, zmm::String name);
    bool getBoolProperty(JSObject *obj, zmm::String name);
    int getIntProperty(JSObject *obj, zmm::String name, int def);
    JSObject *getObjectProperty(JSObject *obj, zmm::String name);

    void setProperty(JSObject *obj, zmm::String name, zmm::String value);
    void setIntProperty(JSObject *obj, zmm::String name, int value);
    void setObjectProperty(JSObject *parent, zmm::String name, JSObject *obj);

    void deleteProperty(JSObject *obj, zmm::String name);

    JSObject *getGlobalObject();
    void setGlobalObject(JSObject *glob);

    JSContext *getContext();

    void initGlobalObject();

    void defineFunctions(JSFunctionSpec *functions);
    void load(zmm::String scriptPath);
    void load(zmm::String scriptText, zmm::String scriptPath);
    void execute();

    /// \todo can those two functions really stay here, or do we need
    /// a class inbetween to keep a nice separation?
    zmm::Ref<CdsObject> jsObject2cdsObject(JSObject *js);
    void cdsObject2jsObject(zmm::Ref<CdsObject> obj, JSObject *js);

};

#endif // __SCRIPTING_SCRIPT_H__

