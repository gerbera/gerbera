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
#include <signal.h>
#include <unistd.h>
#include "common.h"
#include "transcode_process_io_handler.h"
#include "process.h"

using namespace zmm;

TranscodeProcessIOHandler::TranscodeProcessIOHandler(String filename, pid_t kill_pid) : FileIOHandler(filename)
{
    if (kill_pid < 1)
    {
        unlink(filename.c_str());
        throw _Exception(_("invalid pid specified for killing"));
    }
    if (!is_alive(kill_pid))
    {
        unlink(filename.c_str());
        throw _Exception(_("transcoder terminated early"));
    }
    this->kill_pid = kill_pid;
}

void TranscodeProcessIOHandler::open(IN enum UpnpOpenFileMode mode)
{
    if (!is_alive(kill_pid))
        throw _Exception(_("transcoder terminated"));

    FileIOHandler::open(mode);

    if (!is_alive(kill_pid))
        throw _Exception(_("transcoder terminated"));
}

int TranscodeProcessIOHandler::read(OUT char *buf, IN size_t length)
{
    if (!is_alive(kill_pid))
        return 0; /// \todo check exit status and return error if appropriate

    return FileIOHandler::read(buf, length);
}

void TranscodeProcessIOHandler::seek(IN off_t offset, IN int whence)
{   
    // we know we can not seek in a fifo
    throw _Exception(_("fseek failed"));
}
void TranscodeProcessIOHandler::close()
{
    int ret;
    if (is_alive(kill_pid))
    {
        printf("KILLING TERM PID: %d\n", kill_pid);
        kill(kill_pid, SIGTERM);
        sleep(1);
    }

    if (is_alive(kill_pid))
    {
        printf("KILLING INT PID: %d\n", kill_pid);
        kill(kill_pid, SIGINT);
        sleep(1);
    }

    if (is_alive(kill_pid))
    {
        printf("KILLING KILL PID: %d\n", kill_pid);
        kill(kill_pid, SIGKILL);
    }

    printf("CLOSING FILE: %s\n", this->filename.c_str());
    fclose(f);
    unlink(filename.c_str());

    if (is_alive(kill_pid))
        throw _Exception(_("failed to kill transcoding process!"));
}

#endif // TRANSCODING

