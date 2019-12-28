/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    executor.h - this file is part of MediaTomb.
    
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

/// \file executor.h

#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__

#include "zmm/zmmf.h"

/// \brief wraps something executable e.g. a thread or a process
class Executor : public zmm::Object {
public:
    /// \brief destructor of the executor, has to make sure that the executor is dead
    virtual ~Executor() {};

    /// \brief method to check if the executor is still running
    /// \return true if the executor is still running, false otherwise
    virtual bool isAlive() = 0;

    /// \brief kill the executor
    /// \return true if the executor was killed successfully, false otherwise
    virtual bool kill() = 0;

    /// \brief get the status, exit or return code of the executor
    /// \return the status, exit or return code. 0 for normal shutdown
    virtual int getStatus() = 0;
};

#endif // __EXECUTOR_H__
