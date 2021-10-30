/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    process.cc - this file is part of MediaTomb.
    
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

/// \file process.cc

#include "process.h" // API

#include <csignal>
#include <sstream>
#include <thread>

#include <sys/wait.h>

#include "util/tools.h"

bool is_alive(pid_t pid, int* status)
{
    return waitpid(pid, status, WNOHANG) == 0;
}

bool kill_proc(pid_t killPid)
{
    if (!is_alive(killPid))
        return true;

    log_debug("KILLING TERM PID: {}", killPid);
    kill(killPid, SIGTERM);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!is_alive(killPid))
        return true;

    log_debug("KILLING INT PID: {}", killPid);
    kill(killPid, SIGINT);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!is_alive(killPid))
        return true;

    log_debug("KILLING KILL PID: {}", killPid);
    kill(killPid, SIGKILL);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return !is_alive(killPid);
}
