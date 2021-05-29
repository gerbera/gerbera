/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    mt_inotify.h - this file is part of MediaTomb.
    
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

/// \file mt_inotify.h

#ifndef __MT_INOTIFY_H__
#define __MT_INOTIFY_H__

#ifdef HAVE_INOTIFY

#include <filesystem>
#include <string>
#include <sys/inotify.h>
namespace fs = std::filesystem;

/// \brief Inotify interface.
class Inotify {
public:
    Inotify();
    virtual ~Inotify();

    Inotify(const Inotify&) = delete;
    Inotify& operator=(const Inotify&) = delete;

    /// \brief Puts a file or directory on the inotify watch list.
    /// \param path file or directory to monitor.
    /// \param events inotify event mask
    /// \return watch descriptor or a negative value on error
    int addWatch(const fs::path& path, int events) const;

    /// \brief Removes a previously added file or directory from the watch list
    /// \param wd watch descriptor that was returned by the add_watch function
    void removeWatch(int wd) const;

    /// \brief Returns the next inotify event.
    ///
    /// This function will return the next inotify event that occurs, in case
    /// that there are no events the function will block indefinetely. It can
    /// be unblocked by the stop function.
    struct inotify_event* nextEvent();

    /// \brief Unblock the next_event function.
    void stop() const;

    /// \brief Checks if inotify is supported on the system.
    static bool supported();

private:
    int inotify_fd;
    int stop_fds_pipe[2];
    int stop_fd_read;
    int stop_fd_write;
};

#endif

#endif // __INOTIFY_H__
