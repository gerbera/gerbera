/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    process_io_handler.cc - this file is part of MediaTomb.
    
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

/// \file process_io_handler.cc

#include "process_io_handler.h"
#include "common.h"
#include "content_manager.h"
#include "util/process.h"
#include <csignal>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

// after MAX_TIMEOUTS we will tell libupnp to check the socket,
// this will make sure that we do not block the read and allow libupnp to
// call our close() callback

#define MAX_TIMEOUTS 2 // maximum allowe consecutive timeouts

ProcListItem::ProcListItem(std::shared_ptr<Executor> exec, bool abortOnDeath)
{
    executor = std::move(exec);
    abort = abortOnDeath;
}

std::shared_ptr<Executor> ProcListItem::getExecutor()
{
    return executor;
}

bool ProcListItem::abortOnDeath() const
{
    return abort;
}

bool ProcessIOHandler::abort() const
{
    bool abort = false;

    if (procList.empty())
        return abort;

    for (const auto& i : procList) {
        auto exec = i->getExecutor();
        if ((exec != nullptr) && (!exec->isAlive())) {
            if (i->abortOnDeath())
                abort = true;
            break;
        }
    }

    return abort;
}

void ProcessIOHandler::killAll() const
{
    for (const auto& i : procList) {
        auto exec = i->getExecutor();
        if (exec != nullptr)
            exec->kill();
    }
}

void ProcessIOHandler::registerAll()
{
    if (mainProc != nullptr)
        content->registerExecutor(mainProc);

    for (const auto& i : procList) {
        auto exec = i->getExecutor();
        if (exec != nullptr)
            content->registerExecutor(exec);
    }
}

void ProcessIOHandler::unregisterAll()
{
    if (mainProc != nullptr)
        content->unregisterExecutor(mainProc);

    for (const auto& i : procList) {
        auto exec = i->getExecutor();
        if (exec != nullptr)
            content->unregisterExecutor(exec);
    }
}

ProcessIOHandler::ProcessIOHandler(std::shared_ptr<ContentManager> content,
    const fs::path& filename,
    const std::shared_ptr<Executor>& mainProc,
    std::vector<std::shared_ptr<ProcListItem>> procList,
    bool ignoreSeek)
{
    this->content = std::move(content);
    this->filename = filename;
    this->procList = std::move(procList);
    this->mainProc = mainProc;
    this->ignoreSeek = ignoreSeek;

    if ((mainProc != nullptr) && ((!mainProc->isAlive() || abort()))) {
        killAll();
        throw std::runtime_error("process terminated early");
    }
    /*
    if (mkfifo(filename.c_str(), O_RDWR) == -1)
    {
        log_error("Failed to create fifo: {}", strerror(errno));
        killAll();
        if (main_proc != nullptr)
            main_proc->kill();

        throw std::runtime_error("Could not create reader fifo!\n");
    }
*/
    registerAll();
}

void ProcessIOHandler::open(enum UpnpOpenFileMode mode)
{
    if ((mainProc != nullptr) && ((!mainProc->isAlive() || abort()))) {
        killAll();
        throw std::runtime_error("process terminated early");
    }

    if (mode == UPNP_READ)
        fd = ::open(filename.c_str(), O_RDONLY | O_NONBLOCK);
    else if (mode == UPNP_WRITE)
        fd = ::open(filename.c_str(), O_WRONLY | O_NONBLOCK);
    else
        fd = -1;

    if (fd == -1) {
        if (errno == ENXIO) {
            throw TryAgainException(std::string("open failed: ") + strerror(errno));
        }

        killAll();
        if (mainProc != nullptr)
            mainProc->kill();
        unlink(filename.c_str());
        throw std::runtime_error("open: failed to open: " + filename.string());
    }
}

size_t ProcessIOHandler::read(char* buf, size_t length)
{
    fd_set readSet;
    struct timeval timeout;
    ssize_t bytes_read = 0;
    size_t num_bytes = 0;
    char* p_buffer = buf;
    int exit_status = EXIT_SUCCESS;
    int ret = 0;
    int timeout_count = 0;

    while (true) {
        FD_ZERO(&readSet);
        FD_SET(fd, &readSet);

        timeout.tv_sec = FIFO_READ_TIMEOUT;
        timeout.tv_usec = 0;

        ret = select(fd + 1, &readSet, nullptr, nullptr, &timeout);
        if (ret == -1) {
            if (errno == EINTR)
                continue;
        }

        // timeout
        if (ret == 0) {
            if (mainProc != nullptr) {
                bool main_ok = mainProc->isAlive();
                if (!main_ok || abort()) {
                    if (!main_ok) {
                        exit_status = mainProc->getStatus();
                        log_debug("process exited with status {}", exit_status);
                        killAll();
                        return (exit_status == EXIT_SUCCESS) ? 0 : -1;
                    }
                    mainProc->kill();
                    killAll();
                    return -1;
                }
            } else {
                killAll();
                return 0;
            }

            timeout_count++;
            if (timeout_count > MAX_TIMEOUTS) {
                log_debug("max timeouts, checking socket!");
                return CHECK_SOCKET;
            }
        }

        if (FD_ISSET(fd, &readSet)) {
            timeout_count = 0;
            bytes_read = ::read(fd, p_buffer, length);
            if (bytes_read == 0)
                break;

            if (bytes_read < 0) {
                log_debug("aborting read!!!");
                return -1;
            }

            num_bytes = num_bytes + bytes_read;
            length = length - bytes_read;

            if (length == 0)
                break;

            p_buffer = buf + num_bytes;
        }
    }

    if (num_bytes < 0) {
        // not sure what we return here since no way of knowing about feof
        // actually that will depend on the ret code of the process
        ret = -1;

        if (mainProc != nullptr) {
            if (!mainProc->isAlive()) {
                if (mainProc->getStatus() == EXIT_SUCCESS)
                    ret = 0;

            } else {
                mainProc->kill();
            }
        } else
            ret = 0;

        killAll();
        return ret;
    }

    return num_bytes;
}

size_t ProcessIOHandler::write(char* buf, size_t length)
{
    fd_set writeSet;
    struct timeval timeout;
    ssize_t bytes_written = 0;
    size_t num_bytes = 0;
    char* p_buffer = buf;
    int exit_status = EXIT_SUCCESS;
    int ret = 0;

    while (true) {
        FD_ZERO(&writeSet);
        FD_SET(fd, &writeSet);

        timeout.tv_sec = FIFO_WRITE_TIMEOUT;
        timeout.tv_usec = 0;

        ret = select(fd + 1, nullptr, &writeSet, nullptr, &timeout);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
        }

        // timeout
        if (ret == 0) {
            if (mainProc != nullptr) {
                bool main_ok = mainProc->isAlive();
                if (!main_ok || abort()) {
                    if (!main_ok) {
                        exit_status = mainProc->getStatus();
                        log_debug("process exited with status {}", exit_status);
                        killAll();
                        return (exit_status == EXIT_SUCCESS) ? 0 : -1;
                    }

                    mainProc->kill();
                    killAll();
                    return -1;
                }
            } else {
                killAll();
                return 0;
            }
        }

        if (FD_ISSET(fd, &writeSet)) {
            bytes_written = ::write(fd, p_buffer, length);
            if (bytes_written == 0)
                break;

            if (bytes_written < 0) {
                log_debug("aborting write!!!");
                return -1;
            }

            num_bytes = num_bytes + bytes_written;
            length = length - bytes_written;
            if (length == 0)
                break;

            p_buffer = buf + num_bytes;
        }
    }

    if (num_bytes < 0) {
        // not sure what we return here since no way of knowing about feof
        // actually that will depend on the ret code of the process
        ret = -1;

        if (mainProc != nullptr) {
            if (!mainProc->isAlive()) {
                if (mainProc->getStatus() == EXIT_SUCCESS)
                    ret = 0;

            } else {
                mainProc->kill();
            }
        } else
            ret = 0;

        killAll();
        return ret;
    }

    return num_bytes;
}

void ProcessIOHandler::seek(off_t offset, int whence)
{
    // we know we can not seek in a fifo, but the PS3 asks for a hack...
    if (!ignoreSeek)
        throw std::runtime_error("fseek failed");
}

void ProcessIOHandler::close()
{
    bool ret;

    log_debug("terminating process, closing {}", this->filename.c_str());
    unregisterAll();

    if (mainProc != nullptr) {
        ret = mainProc->kill();
    } else
        ret = true;

    killAll();

    ::close(fd);

    unlink(filename.c_str());

    if (!ret)
        throw std::runtime_error("failed to kill process!");
}

ProcessIOHandler::~ProcessIOHandler()
{
    try {
        close();
    } catch (const std::runtime_error& ex) {
    }
}
