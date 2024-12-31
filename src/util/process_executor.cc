/*MT*

    MediaTomb - http://www.mediatomb.cc/

    process_executor.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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
#define GRB_LOG_FAC GrbLogFacility::proc

#include "process_executor.h" // API

#include <array>
#include <csignal>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>

#include "exceptions.h"
#include "logger.h"

#include "fmt/core.h"

ProcessExecutor::ProcessExecutor(const std::string& command, const std::vector<std::string>& arglist, const std::map<std::string, std::string>& env, std::vector<fs::path> tempPaths)
    : tempPaths(std::move(tempPaths))
{
#define MAX_ARGS 255
    std::array<const char*, MAX_ARGS> argv;

    argv.front() = command.c_str();
    int apos = 0;

    for (auto&& i : arglist) {
        argv.at(++apos) = i.c_str();
        if (apos >= MAX_ARGS - 2)
            break;
    }
    argv.at(++apos) = nullptr;

    pid = fork();
    switch (pid) {
    case -1:
        throw_std_runtime_error("Failed to launch process {}", command);

    case 0:
        sigset_t maskSet;
        pthread_sigmask(SIG_SETMASK, &maskSet, nullptr);
        for (auto&& [eName, eValue] : env) {
            setenv(eName.c_str(), eValue.c_str(), 1);
            log_debug("setenv: {}='{}'", eName, eValue);
        }
        log_debug("Launching process: {} {}", command, fmt::join(arglist, " "));
        if (execvp(command.c_str(), const_cast<char**>(argv.data())))
            log_error("Failed to execvp {} {}", command, fmt::join(arglist, " "));
        break;
    default:
        break;
    }

    log_debug("Launched process {} {}, pid: {}", command, fmt::join(arglist, " "), pid);
}

bool ProcessExecutor::isAlive()
{
    return (waitpid(pid, &exitStatus, WNOHANG) == 0);
}

bool ProcessExecutor::kill()
{
    if (!isAlive()) {
        log_debug("He's dead, Jim: {}", pid);
        return true;
    }

    log_debug("Sending SIGTERM to PID: {}", pid);
    ::kill(pid, SIGTERM);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!isAlive())
        return true;

    log_debug("Sending SIGINT to PID: {}", pid);
    ::kill(pid, SIGINT);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!isAlive())
        return true;

    log_debug("Sending SIGKILL to PID: {}", pid);
    ::kill(pid, SIGKILL);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return !isAlive();
}

int ProcessExecutor::getStatus()
{
    isAlive();
    return exitStatus;
}

ProcessExecutor::~ProcessExecutor()
{
    kill();
    for (const auto& path : tempPaths) {
        log_debug("Cleaning up: {}", path.string());
        int retries = 3;
        bool failed = false;
        while (retries--) {
            try {
                if (failed) {
                    log_debug("Sleeping before attempting to delete file again...");
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                fs::remove(path);
                break;
            } catch (const fs::filesystem_error& err) {
                failed = true;
                log_warning("Failed to remove: {}: {}", path.string(), err.what());
            }
        }
        if (failed) {
            log_warning("Failed to remove temp path: {}: giving up!", path.string());
        }
    }
}
