/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    mpegremux_processor.cc - this file is part of MediaTomb.
    
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

/// \file mpegremux_processor.cc

#ifdef HAVE_LIBDVDNAV

#include "mpegremux_processor.h"
#include "mpegdemux/mpegdemux.h"

using namespace zmm;

MPEGRemuxProcessor::MPEGRemuxProcessor(int in_fd, int out_fd, unsigned char keep_audio_id) : ThreadExecutor()
{
    if (in_fd < 0)
        throw _Exception(_("Got invalid input fd!"));

    if (out_fd < 0)
        throw _Exception(_("Got invalid input fd!"));

    this->in_fd = in_fd;
    this->out_fd = out_fd;
    this->keep_audio_id = keep_audio_id;
    startThread();
}

void MPEGRemuxProcessor::threadProc()
{
    log_debug("Starting remux thread...\n");
    status = remux_mpeg(in_fd, out_fd, 0xe0, keep_audio_id);
    threadRunning = false;
    log_debug("Done remuxing\n");
}

#endif//HAVE_LIBDVDNAV
