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

#include "process.h"

#include <csignal>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "config_manager.h"

using namespace zmm;

#define BUF_SIZE 256

String run_simple_process(String prog, String param, String input)
{
    FILE* file;
    int fd;

    /* creating input file */
    char temp_in[] = "mt_in_XXXXXX";
    char temp_out[] = "mt_out_XXXXXX";

    Ref<ConfigManager> cfg = ConfigManager::getInstance();
    String input_file = tempName(cfg->getOption(CFG_SERVER_TMPDIR), temp_in);
    fd = open(input_file.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        log_debug("Failed to open input file %s: %s\n", input_file.c_str(),
            strerror(errno));
        throw _Exception(_("Failed to open input file ") + input_file + _(" ") + strerror(errno));
    }
    ssize_t ret = write(fd, input.c_str(), input.length());
    close(fd);
    if (ret < input.length()) {

        log_debug("Failed to write to %s: %s\n", input.c_str(),
            strerror(errno));
        throw _Exception(_("Failed to write to ") + input + ": " + strerror(errno));
    }

    /* touching output file */
    String output_file = tempName(cfg->getOption(CFG_SERVER_TMPDIR), temp_out);
    fd = open(output_file.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        log_debug("Failed to open output file %s: %s\n", output_file.c_str(),
            strerror(errno));
        throw _Exception(_("Failed to open output file ") + input_file + _(" ") + strerror(errno));
    }
    close(fd);

    /* executing script */
    String command = prog + " " + param + " < " + input_file + " > " + output_file;
    log_debug("running %s\n", command.c_str());
    int sysret = system(command.c_str());
    if (sysret == -1) {
        log_debug("Failed to execute: %s\n", command.c_str());
        throw _Exception(_("Failed to execute: ") + command);
    }

    /* reading output file */
    file = fopen(output_file.c_str(), "r");
    if (!file) {
        log_debug("Could not open output file %s: %s\n", output_file.c_str(),
            strerror(errno));
        throw _Exception(_("Failed to open output file ") + output_file + _(" ") + strerror(errno));
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
    if (waitpid(pid, status, WNOHANG) == 0)
        return true;

    return false;
}

bool kill_proc(pid_t kill_pid)
{
    if (is_alive(kill_pid)) {
        log_debug("KILLING TERM PID: %d\n", kill_pid);
        kill(kill_pid, SIGTERM);
        sleep(1);
    } else
        return true;

    if (is_alive(kill_pid)) {
        log_debug("KILLING INT PID: %d\n", kill_pid);
        kill(kill_pid, SIGINT);
        sleep(1);
    } else
        return true;

    if (is_alive(kill_pid)) {
        log_debug("KILLING KILL PID: %d\n", kill_pid);
        kill(kill_pid, SIGKILL);
        sleep(1);
    } else
        return true;

    if (is_alive(kill_pid))
        return false;

    return true;
}
