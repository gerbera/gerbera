/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    transcode_process_io_handler.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file transcode_process_io_handler.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef TRANSCODING

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <unistd.h>
#include "common.h"
#include "transcode_process_io_handler.h"
#include "process.h"
#include "content_manager.h"

using namespace zmm;

TranscodeProcessIOHandler::TranscodeProcessIOHandler(String filename, pid_t kill_pid) : IOHandler()
{
    int exit_status = EXIT_SUCCESS;

    if (!is_alive(kill_pid, &exit_status))
    {
        unlink(filename.c_str());
        log_debug("transcoder exit status %d\n", exit_status);
        throw _Exception(_("transcoder terminated early with status: ") + String::from(exit_status));
    }
    this->kill_pid = kill_pid;
    this->filename = filename;
    ContentManager::getInstance()->registerTranscoder(kill_pid, filename);
}

void TranscodeProcessIOHandler::open(IN enum UpnpOpenFileMode mode)
{
    int exit_status = EXIT_SUCCESS;
    if (!is_alive(kill_pid, &exit_status))
    {
        log_debug("transcoder exit status %d\n", exit_status);
        throw _Exception(_("transcoder terminated with code: ") + String::from(exit_status));
    }

    fd = ::open(filename.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd == -1)
        throw _Exception(_("open: failed to open: ") + filename.c_str());
}

int TranscodeProcessIOHandler::read(OUT char *buf, IN size_t length)
{
    fd_set readSet;
    struct timeval timeout;
    ssize_t bytes_read = 0;
    int num_bytes = 0;
    char* p_buffer = buf;
    int exit_status = EXIT_SUCCESS;
    int ret = 0;

    FD_ZERO(&readSet);
    FD_SET(fd, &readSet);

    timeout.tv_sec = FIFO_READ_TIMEOUT;
    timeout.tv_usec = 0;

    while (true)
    {
        ret = select(fd + 1, &readSet, NULL, NULL, &timeout);
        if (ret == -1)
        {
            if (errno == EINTR)
                continue;
        }

        // timeout
        if (ret == 0)
        {
            /// \todo check exit status and return error if appropriate
            if (!is_alive(kill_pid, &exit_status))
            {
                log_debug("transcoder exited with status %d\n", exit_status);
                if (exit_status == EXIT_SUCCESS)
                    return 0; 
                else
                    return -1;
            }
        }

        if (FD_ISSET(fd, &readSet))
        {
            timeout.tv_sec = FIFO_READ_TIMEOUT;
            timeout.tv_usec = 0;

            bytes_read = ::read(fd, p_buffer, length);
            if (bytes_read == 0)
                break;

            if (bytes_read < 0)
            {
                log_debug("aborting read!!!\n");
                return -1;
            }

            num_bytes = num_bytes + bytes_read;
            length = length - bytes_read;

            if (length <= 0)
                break;

            p_buffer = buf + num_bytes;
        }
    }

    if (num_bytes < 0)
    {
        // not sure what we return here since no way of knowing about feof
        // actually that will depend onthe ret code of the process
//        if (feof(f)) return 0;
//        if (ferror(f)) return -1;
        log_debug("aborting read 2!!!!\n");
        return -1;
    }

    return num_bytes;
}

void TranscodeProcessIOHandler::seek(IN off_t offset, IN int whence)
{   
    // we know we can not seek in a fifo
    throw _Exception(_("fseek failed"));
}

void TranscodeProcessIOHandler::close()
{
    bool ret;
   

    log_debug("terminating transcoder %d, closing %s\n", kill_pid,
                this->filename.c_str());

    ContentManager::getInstance()->unregisterTranscoder(kill_pid);
    ret = kill_proc(kill_pid);
    
    ::close(fd);
    unlink(filename.c_str());

    if (!ret)
        throw _Exception(_("failed to kill transcoding process!"));
}

#endif // TRANSCODING

