/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    mt_inotify.cc - this file is part of MediaTomb.
    
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

/*
    The original version of the next_events function was taken from the 
    inotify-tools package, http://inotify-tools.sf.net/
    The original author is Rohan McGovern  <rohan@mcgovern.id.au>
    inotify tools are licensed under GNU GPL Version 2
*/

/// \file mt_inotify.cc

#ifdef HAVE_INOTIFY
#include "mt_inotify.h"

#include <array>
#include <cassert>
#include <cerrno>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#ifdef SOLARIS
#include <sys/filio.h> // FIONREAD
#endif

#include "tools.h"

#define MAX_EVENTS 4096
#define MAX_STRLEN 4096

Inotify::Inotify()
#ifdef __linux__
    : inotify_fd(inotify_init1(IN_CLOEXEC))
#else
    : inotify_fd(inotify_init())
#endif
{
    if (inotify_fd < 0)
        throw_std_runtime_error("Unable to initialize inotify");

#ifdef __linux__
    if (pipe2(stop_fds_pipe, IN_CLOEXEC) < 0)
#else
    if (pipe(stop_fds_pipe) < 0)
#endif
        throw_std_runtime_error("Unable to create pipe");

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
    int test_fd = inotify_init1(IN_CLOEXEC);
#else
    int test_fd = inotify_init();
#endif
    if (test_fd < 0)
        return false;

    close(test_fd);
    return true;
}

int Inotify::addWatch(const fs::path& path, int events) const
{
    int wd = inotify_add_watch(inotify_fd, path.c_str(), events);
    if (wd < 0 && errno != ENOENT) {
        if (errno == ENOSPC)
            throw_std_runtime_error("The user limit on the total number of inotify watches was reached or the kernel failed to allocate a needed resource.");
        if (errno == EACCES) {
            log_warning("Cannot add inotify watch for {}: {}", path.c_str(), std::strerror(errno));
            return -1;
        }
        throw_std_runtime_error(std::strerror(errno));
    }
    return wd;
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
    static struct inotify_event* ret;
    static int first_byte = 0;
    static ssize_t bytes;

    int fd_max;

    // first_byte is index into event buffer
    if (first_byte != 0
        && first_byte <= int(bytes - sizeof(struct inotify_event))) {

        ret = reinterpret_cast<struct inotify_event*>(reinterpret_cast<char*>(&event[0]) + first_byte);
        first_byte += sizeof(struct inotify_event) + ret->len;

        // if the pointer to the next event exactly hits end of bytes read,
        // that's good.  next time we're called, we'll read.
        if (first_byte == bytes) {
            first_byte = 0;
        } else if (first_byte > bytes) {
            // oh... no.  this can't be happening.  An incomplete event.
            // Copy what we currently have into first element, call self to
            // read remainder.
            // oh, and they BETTER NOT overlap.
            // Boy I hope this code works.
            // But I think this can never happen due to how inotify is written.
            assert(long(reinterpret_cast<char*>(&event[0]) + sizeof(struct inotify_event) + event[0].len) <= long(ret));

            // how much of the event do we have?
            bytes = reinterpret_cast<char*>(&event[0]) + bytes - reinterpret_cast<char*>(ret);
            memcpy(&event[0], ret, bytes);
            return nextEvent();
        }
        return ret;
    }

    if (first_byte == 0) {
        bytes = 0;
    }

    static ssize_t this_bytes;
    static unsigned int bytes_to_read;
    static int rc;
    static fd_set read_fds;

    FD_ZERO(&read_fds);

    FD_SET(inotify_fd, &read_fds);

    fd_max = inotify_fd;

    FD_SET(stop_fd_read, &read_fds);

    if (stop_fd_read > fd_max)
        fd_max = stop_fd_read;

    rc = select(fd_max + 1, &read_fds,
        nullptr, nullptr, nullptr);
    if (rc < 0) {
        return nullptr;
    }
    if (rc == 0) {
        // timeout
        return nullptr;
    }

    if (FD_ISSET(stop_fd_read, &read_fds)) {
        char buf;
        if (read(stop_fd_read, &buf, 1) == -1) {
            log_error("Inotify: could not read stop: {}", std::strerror(errno));
        }
    }

    if (FD_ISSET(inotify_fd, &read_fds)) {

        // wait until we have enough bytes to read
        do {
            rc = ioctl(inotify_fd, FIONREAD, &bytes_to_read);
        } while (!rc && bytes_to_read < sizeof(struct inotify_event));

        if (rc == -1) {
            return nullptr;
        }

        this_bytes = read(inotify_fd, &event[0] + bytes,
            sizeof(struct inotify_event) * MAX_EVENTS - bytes);
        if (this_bytes < 0) {
            return nullptr;
        }
        if (this_bytes == 0) {
            log_error("Inotify reported end-of-file.  Possibly too many "
                      "events occurred at once.\n");
            return nullptr;
        }
        bytes += this_bytes;

        ret = &event[0];
        first_byte = int(sizeof(struct inotify_event) + ret->len);
        assert(first_byte <= bytes);

        if (first_byte == bytes) {
            first_byte = 0;
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

#endif //HAVE_INOTIFY
