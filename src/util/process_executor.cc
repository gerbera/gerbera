/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    process_executor.cc - this file is part of MediaTomb.
    
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

/// \file process_executor.cc

#include "process_executor.h" // API

#include <csignal>
#include <unistd.h>

#include "exceptions.h"
#include "logger.h"
#include "process.h"

ProcessExecutor::ProcessExecutor(const std::string& command, const std::vector<std::string>& arglist)
{
#define MAX_ARGS 255
    const char* argv[MAX_ARGS];

    argv[0] = command.c_str();
    int apos = 0;

    for (const auto& i : arglist) {
        argv[++apos] = i.c_str();
        if (apos >= MAX_ARGS - 2)
            break;
    }
    argv[++apos] = nullptr;

    exit_status = 0;

    process_id = fork();

    switch (process_id) {
    case -1:
        throw_std_runtime_error("Failed to launch process {}", command.c_str());

    case 0:
        sigset_t mask_set;
        pthread_sigmask(SIG_SETMASK, &mask_set, nullptr);
        log_debug("Launching process: {}", command.c_str());
        execvp(command.c_str(), const_cast<char**>(argv));
        break;
    default:
        break;
    }

    log_debug("Launched process {}, pid: {}", command.c_str(), process_id);
}

bool ProcessExecutor::isAlive()
{
    return is_alive(process_id, &exit_status);
}

bool ProcessExecutor::kill()
{
    return kill_proc(process_id);
}

int ProcessExecutor::getStatus()
{
    is_alive(process_id, &exit_status);
    return exit_status;
}

ProcessExecutor::~ProcessExecutor()
{
    kill();
}
