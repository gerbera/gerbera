/*MT*

    MediaTomb - http://www.mediatomb.cc/

    mt_inotify.cc - this file is part of MediaTomb.

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

/*
    The original version of the next_events function was taken from the
    inotify-tools package, http://inotify-tools.sf.net/
    The original author is Rohan McGovern  <rohan@mcgovern.id.au>
    inotify tools are licensed under GNU GPL Version 2
*/

/// @file content/inotify/mt_inotify.cc
#define GRB_LOG_FAC GrbLogFacility::autoscan

#ifdef HAVE_INOTIFY
#include "mt_inotify.h"

#include "exceptions.h"
#include "util/logger.h"

#include <algorithm>
#include <cstring>
#include <sys/inotify.h>
#include <sys/select.h>
#include <thread>
#include <unistd.h>

Inotify::Inotify()
#ifdef __linux__
    : inotify_fd(inotify_init1(IN_CLOEXEC))
#else
    : inotify_fd(inotify_init())
#endif
{
    if (inotify_fd < 0)
        throw_fmt_system_error("Unable to initialize inotify");

#ifdef __linux__
    if (pipe2(stop_fds_pipe, IN_CLOEXEC) < 0)
#else
    if (pipe(stop_fds_pipe) < 0)
#endif
    {
        int savedError = errno;
        close(inotify_fd);
        errno = savedError;
        throw_fmt_system_error("Unable to create pipe");
    }

    stop_fd_read = stop_fds_pipe[0];
    stop_fd_write = stop_fds_pipe[1];
}

Inotify::~Inotify()
{
    if (inotify_fd >= 0)
        close(inotify_fd);
    close(stop_fd_read);
    close(stop_fd_write);
}

bool Inotify::supported()
{
#ifdef __linux__
    int testFd = inotify_init1(IN_CLOEXEC);
#else
    int testFd = inotify_init();
#endif
    if (testFd < 0)
        return false;

    close(testFd);
    return true;
}

int Inotify::addWatch(const fs::path& path, InotifyFlags events, unsigned int retryCount) const
{
    for (int wd = -1; wd < 0;) {
        wd = inotify_add_watch(inotify_fd, path.c_str(), events);
        if (wd < 0 && errno != ENOENT) {
            if (errno == ENOSPC)
                throw_fmt_system_error("The user limit on the total number of inotify watches was reached or the kernel failed to allocate a needed resource.");
            if (errno == EACCES) {
                if (retryCount > 0) {
                    log_debug("Retrying {} to add inotify watch for {}: {}", retryCount, path.c_str(), std::strerror(errno));
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    retryCount--;
                    continue;
                } else {
                    log_warning("Cannot add inotify watch for {}: {}", path.c_str(), std::strerror(errno));
                    return -1;
                }
            }
            throw_fmt_system_error("Adding inotify watch failed");
        }
        log_debug("Add inotify watch for {}: {}", path.c_str(), wd);
        return wd;
    }
    return -1;
}

void Inotify::removeWatch(int wd) const
{
    if (inotify_rm_watch(inotify_fd, wd) < 0) {
        log_debug("Error removing watch: {}", std::strerror(errno));
    }
}

struct inotify_event* Inotify::nextEvent()
{
    if (nextEventOffset == eventBytes) {
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(inotify_fd, &readFds);
        FD_SET(stop_fd_read, &readFds);
        int rc = select(std::max(inotify_fd, stop_fd_read) + 1, &readFds, nullptr, nullptr, nullptr);
        if (rc <= 0)
            return nullptr;
        if (FD_ISSET(stop_fd_read, &readFds)) {
            char stop;
            if (read(stop_fd_read, &stop, 1) == -1)
                log_error("Inotify: could not read stop: {}", std::strerror(errno));
        }
        if (!FD_ISSET(inotify_fd, &readFds))
            return nullptr;

        // inotify reads contain complete events; each instance owns its batch.
        auto bytesRead = read(inotify_fd, eventBuffer.data(), sizeof(eventBuffer));
        if (bytesRead < 0)
            return nullptr;
        if (bytesRead == 0)
            throw_std_runtime_error("Inotify reported end-of-file");
        eventBytes = bytesRead;
        nextEventOffset = 0;
    }

    auto remaining = eventBytes - nextEventOffset;
    if (remaining < sizeof(inotify_event))
        throw_std_runtime_error("Inotify returned an incomplete event header");
    auto event = reinterpret_cast<inotify_event*>(reinterpret_cast<char*>(eventBuffer.data()) + nextEventOffset);
    if (event->len > remaining - sizeof(inotify_event))
        throw_std_runtime_error("Inotify returned an incomplete event name");
    nextEventOffset += sizeof(inotify_event) + event->len;
    return event;
}

void Inotify::stop() const
{
    char stop = 's';
    if (write(stop_fd_write, &stop, 1) == -1) {
        log_error("inotify: could not send stop: {}", std::strerror(errno));
    }
}

#endif // HAVE_INOTIFY
