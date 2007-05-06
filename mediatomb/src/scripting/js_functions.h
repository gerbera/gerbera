/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    js_functions.h - this file is part of MediaTomb.
    
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

/// \file js_functions.h 
/// \brief These functions can be called from scripts.

#ifndef __SCRIPTING_JS_FUNCTIONS_H__
#define __SCRIPTING_JS_FUNCTIONS_H__

#ifdef __APPLE__
    #define XP_MAC 1
#else
    #define XP_UNIX 1
#endif

#include <jsapi.h>

extern "C" {

/// \brief Log output.
JSBool js_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

/// \brief Adds an object to the database.
JSBool js_addCdsObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

/// \brief Reads a line from a text file.
JSBool js_readln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

/// \brief Makes a copy of an CDS object.
JSBool js_copyObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

} // extern "C"

#endif//__SCRIPTING_JS_FUNCTIONS_H__
