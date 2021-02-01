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

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <thread>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config/config_manager.h"
#include "util/tools.h"

#define BUF_SIZE 256

std::string run_simple_process(const std::shared_ptr<Config>& cfg, const std::string& prog, const std::string& param, const std::string& input)
{
    FILE* file;
    int fd;

    /* creating input file */
    char temp_in[] = "mt_in_XXXXXX";
    char temp_out[] = "mt_out_XXXXXX";

    std::string input_file = tempName(cfg->getOption(CFG_SERVER_TMPDIR), temp_in);
#ifdef __linux__
    fd = open(input_file.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, S_IRUSR | S_IWUSR);
#else
    fd = open(input_file.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
#endif
    if (fd == -1) {
        log_debug("Failed to open input file {}: {}", input_file.c_str(), std::strerror(errno));
        throw_std_runtime_error("Failed to open input file {}: {}", input_file.c_str(), std::strerror(errno));
    }
    size_t ret = write(fd, input.c_str(), input.length());
    close(fd);
    if (ret < input.length()) {

        log_debug("Failed to write to {}: {}", input.c_str(), std::strerror(errno));
        throw_std_runtime_error("Failed to write to {}: {}", input.c_str(), std::strerror(errno));
    }

    /* touching output file */
    std::string output_file = tempName(cfg->getOption(CFG_SERVER_TMPDIR), temp_out);
#ifdef __linux__
    fd = open(output_file.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, S_IRUSR | S_IWUSR);
#else
    fd = open(output_file.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
#endif
    if (fd == -1) {
        log_debug("Failed to open output file {}: {}", output_file.c_str(), std::strerror(errno));
        throw_std_runtime_error("Failed to open output file {}: {}", output_file.c_str(), std::strerror(errno));
    }
    close(fd);

    /* executing script */
    std::string command = prog + " " + param + " < " + input_file + " > " + output_file;
    log_debug("running {}", command.c_str());
    int sysret = system(command.c_str());
    if (sysret == -1) {
        log_debug("Failed to execute: {}", command.c_str());
        throw_std_runtime_error("Failed to execute: {}", command.c_str());
    }

    /* reading output file */
#ifdef __linux__
    file = ::fopen(output_file.c_str(), "re");
#else
    file = ::fopen(output_file.c_str(), "r");
#endif
    if (!file) {
        log_debug("Could not open output file {}: {}", output_file.c_str(), std::strerror(errno));
        throw_std_runtime_error("Failed to open output file {}: {}", output_file.c_str(), std::strerror(errno));
    }
    std::ostringstream output;

    int bytesRead;
    char buf[BUF_SIZE];
    while (true) {
        bytesRead = fread(buf, 1, BUF_SIZE, file);
        if (bytesRead > 0)
            output << std::string(buf, bytesRead);
        else
            break;
    }
    fclose(file);

    /* removing input and output files */
    unlink(input_file.c_str());
    unlink(output_file.c_str());

    return output.str();
}

bool is_alive(pid_t pid, int* status)
{
    return waitpid(pid, status, WNOHANG) == 0;
}

bool kill_proc(pid_t kill_pid)
{
    if (!is_alive(kill_pid))
        return true;

    log_debug("KILLING TERM PID: {}", kill_pid);
    kill(kill_pid, SIGTERM);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!is_alive(kill_pid))
        return true;

    log_debug("KILLING INT PID: {}", kill_pid);
    kill(kill_pid, SIGINT);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!is_alive(kill_pid))
        return true;

    log_debug("KILLING KILL PID: {}", kill_pid);
    kill(kill_pid, SIGKILL);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return !is_alive(kill_pid);
}
