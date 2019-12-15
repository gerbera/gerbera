/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    runtime.h - this file is part of MediaTomb.
    
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

/// \file runtime.h

#ifndef __SCRIPTING_RUNTIME_H__
#define __SCRIPTING_RUNTIME_H__

#include "duktape.h"
#include <pthread.h>
#include "common.h"
#include "singleton.h"

/// \brief Runtime class definition.
class Runtime : public Singleton<Runtime, std::recursive_mutex>
{
protected:
    duk_context *ctx;

public:
    Runtime();
    virtual ~Runtime();
    std::string getName() override { return "Runtime"; }
    
    /// \brief Returns a new (sub)context. !!! Not thread-safe !!!
    duk_context *createContext(std::string name);
    void destroyContext(std::string name);
    std::recursive_mutex& getMutex() { return mutex; }
};

#endif // __SCRIPTING_RUNTIME_H__
