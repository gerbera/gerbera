/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    process.h - this file is part of MediaTomb.
    
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

/// \file process.h

#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "zmm/zmmf.h"

void init_process();
zmm::String run_simple_process(zmm::String prog, zmm::String param, zmm::String input);

void run_process(zmm::String prog, zmm::String param);

bool is_alive(pid_t pid, int* exit_status = NULL);

bool kill_proc(pid_t kill_pid);

#endif // __PROCESS_H__
