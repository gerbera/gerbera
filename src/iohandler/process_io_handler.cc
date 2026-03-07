/*MT*

    MediaTomb - http://www.mediatomb.cc/

    process_io_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2026 Gerbera Contributors

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

/// @file iohandler/process_io_handler.cc
#define GRB_LOG_FAC GrbLogFacility::iohandler

#include "process_io_handler.h" // API

#include "content/content.h"
#include "exceptions.h"
#include "upnp/compat.h"

#include <algorithm>
#include <csignal>
#include <deque>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#include <utility>

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
        [=](auto&& proc) { auto exec = proc.getExecutor();
            return exec && !exec->isAlive() && proc.abortOnDeath(); });
}

void ProcessIOHandler::killAll() const
{
    for (auto&& i : procList) {
        auto exec = i.getExecutor();
        if (exec)
            exec->kill();
    }
}

void ProcessIOHandler::registerAll()
{
    if (mainProc)
        content->registerExecutor(mainProc);

    for (auto&& i : procList) {
        auto exec = i.getExecutor();
        if (exec)
            content->registerExecutor(exec);
    }
}

void ProcessIOHandler::unregisterAll()
{
    if (mainProc)
        content->unregisterExecutor(mainProc);

    for (auto&& i : procList) {
        auto exec = i.getExecutor();
        if (exec)
            content->unregisterExecutor(exec);
    }
}

ProcessIOHandler::ProcessIOHandler(
    const std::shared_ptr<Content>& content,
    fs::path filename,
    std::shared_ptr<Executor> mainProc,
    std::chrono::seconds timeout,
    unsigned int retryCount,
    std::vector<ProcListItem> procList,
    bool ignoreSeek)
    : content(content)
    , procList(std::move(procList))
    , mainProc(std::move(mainProc))
    , path(std::move(filename))
    , ignoreSeek(ignoreSeek)
    , timeout(timeout)
    , retryCount(retryCount)
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

    std::lock_guard<std::mutex> lock(mutex);

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

/// @brief check if writing to the socket is possible
static bool isSocketReadable(int sockfd, long to_val)
{
    fd_set readSet;
    fd_set exceptSet;
    struct timeval timeout;

    FD_ZERO(&readSet);
    FD_ZERO(&exceptSet);
    FD_SET(sockfd, &readSet);
    FD_SET(sockfd, &exceptSet);

    timeout.tv_sec = to_val;
    timeout.tv_usec = 0;

    int retCode = select(sockfd + 1, &readSet, NULL, &exceptSet, &timeout);

    if (FD_ISSET(sockfd, &readSet) || FD_ISSET(sockfd, &exceptSet)) {
        return true;
    }

    if (retCode == 0) { // timeout or fd ready for writing
        log_debug("timeout {}", sockfd);
        return false;
    }

    if (retCode == -1 && errno == EINTR) {
        log_debug("error {}", sockfd);
        return false;
    }

    return true;
}

grb_read_t ProcessIOHandler::read(std::byte* buf, std::size_t length)
{
    std::lock_guard<std::mutex> lock(mutex);

    fd_set readSet;
    struct timespec requestTimeout = { timeout.count(), 0 };
    std::deque<long> fibonaccis = { 2, 3 };
    std::size_t numBytes = 0;
    auto pBuffer = buf;
    int exitStatus = EXIT_SUCCESS;
    int ret = GRB_READ_ERROR;
    unsigned int timeoutCount = 0;

    while (true) {
        FD_ZERO(&readSet);
        FD_SET(fd, &readSet);

        ret = pselect(fd + 1, &readSet, nullptr, nullptr, &requestTimeout, nullptr);
        if (ret == -1 && errno == EINTR)
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
                        return (exitStatus == EXIT_SUCCESS) ? GRB_READ_END : GRB_READ_ERROR;
                    }
                    mainProc->kill();
                    killAll();
                    return GRB_READ_ERROR;
                }
            } else {
                killAll();
                return GRB_READ_END;
            }

            timeoutCount++;
            requestTimeout.tv_sec = timeout.count() * fibonaccis.front();
            fibonaccis.push_back(fibonaccis.front() + fibonaccis.back());
            fibonaccis.pop_front();
            log_info("Socket timeout adjusted to {}", requestTimeout.tv_sec);
            if (timeoutCount > retryCount) {
                // after retryCount we will check the socket,
                // this will make sure that we do not block the read and allow libupnp to
                // call our close() callback
                log_debug("max timeouts {}, checking socket!", retryCount);
                if (!isSocketReadable(fd, requestTimeout.tv_sec)) {
                    log_debug("socket broken aborting read!!!");
                    mainProc->kill();
                    killAll();
                    return GRB_READ_ERROR;
                }
                timeoutCount = 0;
                continue;
            }
        }

        if (FD_ISSET(fd, &readSet)) {
            timeoutCount = 0;
            ssize_t bytesRead = ::read(fd, pBuffer, length);
            if (bytesRead == 0)
                break;

            if (bytesRead < 0) {
                log_debug("aborting read!!!");
                return GRB_READ_ERROR;
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
        ret = GRB_READ_ERROR;

        if (mainProc) {
            if (mainProc->isAlive())
                mainProc->kill();
            if (mainProc->getStatus() == EXIT_SUCCESS)
                ret = GRB_READ_END;
        } else
            ret = GRB_READ_END;

        killAll();
        return ret;
    }
    timeout = std::chrono::seconds(requestTimeout.tv_sec);
    return numBytes;
}

std::size_t ProcessIOHandler::write(std::byte* buf, std::size_t length)
{
    std::lock_guard<std::mutex> lock(mutex);

    fd_set writeSet;
    struct timespec requestTimeout = { timeout.count(), 0 };
    ssize_t bytesWritten;
    std::size_t numBytes = 0;
    auto pBuffer = buf;
    int exitStatus = EXIT_SUCCESS;
    int ret;

    while (true) {
        FD_ZERO(&writeSet);
        FD_SET(fd, &writeSet);

        ret = pselect(fd + 1, nullptr, &writeSet, nullptr, &requestTimeout, nullptr);
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
