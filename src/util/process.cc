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
#include <sstream>
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
    std::FILE* file;
    int fd;

    /* creating input file */
    char tempIn[] = "mt_in_XXXXXX";
    char tempOut[] = "mt_out_XXXXXX";

    std::string inputFile = tempName(cfg->getOption(CFG_SERVER_TMPDIR), tempIn);
#ifdef __linux__
    fd = open(inputFile.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, S_IRUSR | S_IWUSR);
#else
    fd = open(inputFile.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
#endif
    if (fd == -1) {
        log_debug("Failed to open input file {}: {}", inputFile, std::strerror(errno));
        throw_std_runtime_error("Failed to open input file {}: {}", inputFile, std::strerror(errno));
    }
    std::size_t ret = write(fd, input.c_str(), input.length());
    close(fd);
    if (ret < input.length()) {
        log_debug("Failed to write to {}: {}", input, std::strerror(errno));
        throw_std_runtime_error("Failed to write to {}: {}", input, std::strerror(errno));
    }

    /* touching output file */
    std::string outputFile = tempName(cfg->getOption(CFG_SERVER_TMPDIR), tempOut);
#ifdef __linux__
    fd = open(outputFile.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, S_IRUSR | S_IWUSR);
#else
    fd = open(outputFile.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
#endif
    if (fd == -1) {
        log_debug("Failed to open output file {}: {}", outputFile, std::strerror(errno));
        throw_std_runtime_error("Failed to open output file {}: {}", outputFile, std::strerror(errno));
    }
    close(fd);

    /* executing script */
    auto command = fmt::format("{} {} < {} > {}", prog, param, inputFile, outputFile);
    log_debug("running {}", command);
    int sysret = std::system(command.c_str());
    if (sysret == -1) {
        log_debug("Failed to execute: {}", command);
        throw_std_runtime_error("Failed to execute: {}", command);
    }

    /* reading output file */
#ifdef __linux__
    file = std::fopen(outputFile.c_str(), "re");
#else
    file = std::fopen(outputFile.c_str(), "r");
#endif
    if (!file) {
        log_debug("Could not open output file {}: {}", outputFile, std::strerror(errno));
        throw_std_runtime_error("Failed to open output file {}: {}", outputFile, std::strerror(errno));
    }

    std::ostringstream output;
    int bytesRead;
    char buf[BUF_SIZE];
    while (true) {
        bytesRead = std::fread(buf, 1, BUF_SIZE, file);
        if (bytesRead > 0)
            output << std::string(buf, bytesRead);
        else
            break;
    }
    std::fclose(file);

    /* removing input and output files */
    unlink(inputFile.c_str());
    unlink(outputFile.c_str());

    return output.str();
}

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
