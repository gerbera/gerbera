/*MT*

    MediaTomb - http://www.mediatomb.cc/

    mt_inotify.cc - this file is part of MediaTomb.

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

/*
    The original version of the next_events function was taken from the
    inotify-tools package, http://inotify-tools.sf.net/
    The original author is Rohan McGovern  <rohan@mcgovern.id.au>
    inotify tools are licensed under GNU GPL Version 2
*/

/// \file mt_inotify.cc
#define LOG_FAC log_facility_t::autoscan

#ifdef HAVE_INOTIFY
#include "mt_inotify.h"

#include <array>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <thread>
#include <unistd.h>
#ifdef SOLARIS
#include <sys/filio.h> // FIONREAD
#endif

#include "util/tools.h"

#define MAX_EVENTS 4096

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
        throw_fmt_system_error("Unable to create pipe");

    stop_fd_read = stop_fds_pipe[0];
    stop_fd_write = stop_fds_pipe[1];
}

Inotify::~Inotify()
{
    if (inotify_fd >= 0)
        close(inotify_fd);
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
    static std::array<inotify_event, MAX_EVENTS> event;
    static struct inotify_event* ret = nullptr;
    static ssize_t firstByte = 0;
    static ssize_t bytes = 0;

    // firstByte is index into event buffer
    if (firstByte != 0
        && firstByte <= bytes - ssize_t(sizeof(struct inotify_event))) {
        ret = reinterpret_cast<struct inotify_event*>(reinterpret_cast<char*>(event.data()) + firstByte);
        firstByte += sizeof(struct inotify_event) + ret->len;

        // if the pointer to the next event exactly hits end of bytes read,
        // that's good.  next time we're called, we'll read.
        if (firstByte == bytes) {
            firstByte = 0;
        } else if (firstByte > bytes) {
            // oh... no.  this can't be happening.  An incomplete event.
            // Copy what we currently have into first element, call self to
            // read remainder.
            // oh, and they BETTER NOT overlap.
            // Boy I hope this code works.
            // But I think this can never happen due to how inotify is written.
            assert(long(reinterpret_cast<char*>(event.data()) + sizeof(struct inotify_event) + event[0].len) <= long(ret));

            // how much of the event do we have?
            bytes = reinterpret_cast<char*>(event.data()) + bytes - reinterpret_cast<char*>(ret);
            std::memcpy(event.data(), ret, bytes);
            return nextEvent();
        }
        return ret;
    }

    if (firstByte == 0) {
        bytes = 0;
    }

    static ssize_t bytesToRead = 0;
    static int rc;
    static fd_set readFds;

    FD_ZERO(&readFds);

    FD_SET(inotify_fd, &readFds);

    auto fdMax = inotify_fd;

    FD_SET(stop_fd_read, &readFds);

    if (stop_fd_read > fdMax)
        fdMax = stop_fd_read;

    rc = select(fdMax + 1, &readFds, nullptr, nullptr, nullptr);
    if (rc < 0) {
        return nullptr;
    }
    if (rc == 0) {
        // timeout
        return nullptr;
    }

    if (FD_ISSET(stop_fd_read, &readFds)) {
        char buf;
        if (read(stop_fd_read, &buf, 1) == -1) {
            log_error("Inotify: could not read stop: {}", std::strerror(errno));
        }
    }

    if (FD_ISSET(inotify_fd, &readFds)) {
        static ssize_t thisBytes = 0;
        // wait until we have enough bytes to read
        do {
            rc = ioctl(inotify_fd, FIONREAD, &bytesToRead);
        } while (!rc && bytesToRead < ssize_t(sizeof(struct inotify_event)));

        if (rc == -1) {
            return nullptr;
        }

        thisBytes = read(inotify_fd, event.data() + bytes,
            sizeof(struct inotify_event) * MAX_EVENTS - bytes);
        if (thisBytes < 0) {
            return nullptr;
        }
        if (thisBytes == 0) {
            log_error("Inotify reported end-of-file.  Possibly too many "
                      "events occurred at once.\n");
            return nullptr;
        }
        bytes += thisBytes;

        ret = event.data();
        firstByte = sizeof(struct inotify_event) + ret->len;
        assert(firstByte <= bytes);

        if (firstByte == bytes) {
            firstByte = 0;
        }

        return ret;
    }

    return nullptr;
}

void Inotify::stop() const
{
    char stop = 's';
    if (write(stop_fd_write, &stop, 1) == -1) {
        log_error("inotify: could not send stop: {}", std::strerror(errno));
    }
}

#endif // HAVE_INOTIFY
