/*MT*

    MediaTomb - http://www.mediatomb.cc/

    process_io_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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
#define LOG_FAC log_facility_t::iohandler

#include "process_io_handler.h" // API

#include <csignal>
#include <utility>

#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>

#include "content/content_manager.h"

// after MAX_TIMEOUTS we will tell libupnp to check the socket,
// this will make sure that we do not block the read and allow libupnp to
// call our close() callback

#define MAX_TIMEOUTS 2 // maximum allowed consecutive timeouts

ProcListItem::ProcListItem(std::shared_ptr<Executor> exec, bool abortOnDeath)
    : executor(std::move(exec))
    , abort(abortOnDeath)
{
}

std::shared_ptr<Executor> ProcListItem::getExecutor() const
{
    return executor;
}

bool ProcListItem::abortOnDeath() const
{
    return abort;
}

bool ProcessIOHandler::abort() const
{
    return std::any_of(procList.begin(), procList.end(),
        [=](auto&& proc) { auto exec = proc->getExecutor();
            return exec && !exec->isAlive() && proc->abortOnDeath(); });
}

void ProcessIOHandler::killAll() const
{
    for (auto&& i : procList) {
        auto exec = i->getExecutor();
        if (exec)
            exec->kill();
    }
}

void ProcessIOHandler::registerAll()
{
    if (mainProc)
        content->registerExecutor(mainProc);

    for (auto&& i : procList) {
        auto exec = i->getExecutor();
        if (exec)
            content->registerExecutor(exec);
    }
}

void ProcessIOHandler::unregisterAll()
{
    if (mainProc)
        content->unregisterExecutor(mainProc);

    for (auto&& i : procList) {
        auto exec = i->getExecutor();
        if (exec)
            content->unregisterExecutor(exec);
    }
}

ProcessIOHandler::ProcessIOHandler(std::shared_ptr<ContentManager> content,
    fs::path filename,
    std::shared_ptr<Executor> mainProc,
    std::vector<std::unique_ptr<ProcListItem>> procList,
    bool ignoreSeek)
    : content(std::move(content))
    , procList(std::move(procList))
    , mainProc(std::move(mainProc))
    , path(std::move(filename))
    , ignoreSeek(ignoreSeek)
{
    if (this->mainProc && (!this->mainProc->isAlive() || abort())) {
        killAll();
        throw_std_runtime_error("process terminated early");
    }
    registerAll();
}

void ProcessIOHandler::open(enum UpnpOpenFileMode mode)
{
    if (mainProc && (!mainProc->isAlive() || abort())) {
        killAll();
        throw_std_runtime_error("process terminated early");
    }

    if (mode == UPNP_READ)
#ifdef __linux__
        fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
#else
        fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
#endif
    else if (mode == UPNP_WRITE)
#ifdef __linux__
        fd = ::open(path.c_str(), O_WRONLY | O_NONBLOCK | O_CLOEXEC);
#else
        fd = ::open(path.c_str(), O_WRONLY | O_NONBLOCK);
#endif
    else
        fd = -1;

    if (fd == -1) {
        if (errno == ENXIO) {
            throw TryAgainException(fmt::format("open failed: {}", std::strerror(errno)));
        }

        killAll();
        if (mainProc)
            mainProc->kill();
        fs::remove(path);
        throw_std_runtime_error("open: failed to open: {}", path.c_str());
    }
}

std::size_t ProcessIOHandler::read(std::byte* buf, std::size_t length)
{
    fd_set readSet;
    struct timespec timeout = { FIFO_READ_TIMEOUT, 0 };
    std::size_t numBytes = 0;
    auto pBuffer = buf;
    int exitStatus = EXIT_SUCCESS;
    int ret;
    int timeoutCount = 0;

    while (true) {
        FD_ZERO(&readSet);
        FD_SET(fd, &readSet);

        ret = pselect(fd + 1, &readSet, nullptr, nullptr, &timeout, nullptr);
        if ((ret == -1) && (errno == EINTR))
            continue;

        // timeout
        if (ret == 0) {
            if (mainProc) {
                bool mainOk = mainProc->isAlive();
                if (!mainOk || abort()) {
                    if (!mainOk) {
                        exitStatus = mainProc->getStatus();
                        log_debug("process exited with status {}", exitStatus);
                        killAll();
                        return (exitStatus == EXIT_SUCCESS) ? 0 : -1;
                    }
                    mainProc->kill();
                    killAll();
                    return -1;
                }
            } else {
                killAll();
                return 0;
            }

            timeoutCount++;
            if (timeoutCount > MAX_TIMEOUTS) {
                log_debug("max timeouts, checking socket!");
                return CHECK_SOCKET;
            }
        }

        if (FD_ISSET(fd, &readSet)) {
            timeoutCount = 0;
            ssize_t bytesRead = ::read(fd, pBuffer, length);
            if (bytesRead == 0)
                break;

            if (bytesRead < 0) {
                log_debug("aborting read!!!");
                return -1;
            }

            numBytes = numBytes + bytesRead;
            length = length - bytesRead;

            if (length == 0)
                break;

            pBuffer = buf + numBytes;
        }
    }

    if (numBytes == 0) {
        // not sure what we return here since no way of knowing about feof
        // actually that will depend on the ret code of the process
        ret = -1;

        if (mainProc) {
            if (mainProc->isAlive())
                mainProc->kill();
            if (mainProc->getStatus() == EXIT_SUCCESS)
                ret = 0;
        } else
            ret = 0;

        killAll();
        return ret;
    }

    return numBytes;
}

std::size_t ProcessIOHandler::write(std::byte* buf, std::size_t length)
{
    fd_set writeSet;
    struct timespec timeout = { FIFO_WRITE_TIMEOUT, 0 };
    ssize_t bytesWritten;
    std::size_t numBytes = 0;
    auto pBuffer = buf;
    int exitStatus = EXIT_SUCCESS;
    int ret;

    while (true) {
        FD_ZERO(&writeSet);
        FD_SET(fd, &writeSet);

        ret = pselect(fd + 1, nullptr, &writeSet, nullptr, &timeout, nullptr);
        if ((ret == -1) && (errno == EINTR))
            continue;

        // timeout
        if (ret == 0) {
            if (mainProc) {
                bool mainOk = mainProc->isAlive();
                if (!mainOk || abort()) {
                    if (!mainOk) {
                        exitStatus = mainProc->getStatus();
                        log_debug("process exited with status {}", exitStatus);
                        killAll();
                        return (exitStatus == EXIT_SUCCESS) ? 0 : -1;
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
            bytesWritten = ::write(fd, pBuffer, length);
            if (bytesWritten == 0)
                break;

            if (bytesWritten < 0) {
                log_debug("aborting write!!!");
                return -1;
            }

            numBytes = numBytes + bytesWritten;
            length = length - bytesWritten;
            if (length == 0)
                break;

            pBuffer = buf + numBytes;
        }
    }

    if (numBytes == 0) {
        // not sure what we return here since no way of knowing about feof
        // actually that will depend on the ret code of the process
        ret = -1;

        if (mainProc) {
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

    return numBytes;
}

void ProcessIOHandler::seek(off_t offset, int whence)
{
    // we know we can not seek in a fifo, but the PS3 asks for a hack...
    if (!ignoreSeek)
        throw_std_runtime_error("fseek failed");
}

void ProcessIOHandler::close()
{
}

ProcessIOHandler::~ProcessIOHandler()
{
    log_debug("Destroying ProcessIOHandler: terminating process, closing {}", this->path.c_str());
    unregisterAll();

    bool killed;
    if (mainProc) {
        killed = mainProc->kill();
    } else {
        killed = true;
    }

    killAll();

    ::close(fd);

    fs::remove(path);

    if (!killed) {
        log_warning("~ProcessIOHandler: Failed to kill process");
    }
}
