/*  scripting.h - this file is part of MediaTomb.
                                                                                
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
};

#endif // __SCRIPTING_SCRIPT_H__



