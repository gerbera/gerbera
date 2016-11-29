/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dvd_io_handler.cc - this file is part of MediaTomb.
    
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

/// \file dvd_io_handler.cc

#ifdef HAVE_LIBDVDNAV

#include <stdint.h>

#include <sys/types.h>
#include <dvdnav/dvdnav.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "server.h"
#include "common.h"
#include "dvd_io_handler.h"

using namespace zmm;
using namespace mxml;

DVDIOHandler::DVDIOHandler(String dvdname, int track, int chapter, 
                           int audio_stream_id) : IOHandler()
{
    this->dvdname = dvdname;
    small_buffer = (unsigned char *)MALLOC(DVD_VIDEO_LB_LEN);
    if (small_buffer == NULL)
        throw _Exception(_("Could not allocate memory for DVD small databuffer!"));
    small_buffer[0] = '\0';
    small_buffer_pos = NULL;
    last_read = false;
    dvd = Ref<DVDNavReader>(new DVDNavReader(dvdname));
    dvd->selectPGC(track, chapter);
}

void DVDIOHandler::open(IN enum UpnpOpenFileMode mode)
{
    if (mode != UPNP_READ)
        throw _Exception(_("DVDIOHandler::open: write not supported!"));
}

int DVDIOHandler::read(OUT char *buf, IN size_t length)
{

    char *pbuf = buf;
    size_t count = 0;

    if (last_read)
        return 0;

    // dvdnav reader returns a minimum of 2048 bytes, so we have to account
    // for that
    //
    // rest buffer already has a remainder from the previous call
    if (small_buffer_pos != NULL)
    {
        size_t rest = (small_buffer + DVD_VIDEO_LB_LEN) - small_buffer_pos;
        if (rest >= length)
        {
            memcpy(pbuf, small_buffer_pos, length);
            small_buffer_pos = small_buffer_pos + length;

            if (rest == length)
                small_buffer_pos = NULL;

            return (int)length;
        } 
        else if (rest < length)
        {
            memcpy(pbuf, small_buffer_pos, rest);
            pbuf = pbuf + rest;
            count = count + rest;
            length = length - rest;
            small_buffer_pos = NULL;
        }
    }

    if ((length) < DVD_VIDEO_LB_LEN)
    {
        size_t bytes = dvd->readSector((unsigned char *)small_buffer, DVD_VIDEO_LB_LEN);
        if (bytes == 0)
        {
            last_read = true;
            return count;
        }
        else if (bytes < 0)
            return bytes;
        else
        {
            small_buffer_pos = small_buffer;

            memcpy(pbuf, small_buffer_pos, length);
            count = count + length;
            small_buffer_pos = small_buffer_pos + length;
            return (int)count;
        }
    }
    else
    {
        int ret = dvd->readSector((unsigned char *)pbuf, length);
        if (ret == 0)
        {
            last_read = true;
        }

        if (ret >= 0)
            ret = count + ret;
       
        return ret;
    }

    return -1;
}

int DVDIOHandler::write(IN char *buf, IN size_t length)
{
    throw _Exception(_("DVD write is not possible"));
}

void DVDIOHandler::seek(IN off_t offset, IN int whence)
{
}

void DVDIOHandler::close()
{
    dvd = nil;
    small_buffer_pos = NULL;
}

off_t DVDIOHandler::length()
{
    return -1;
}

DVDIOHandler::~DVDIOHandler()
{
    if (small_buffer)
        FREE(small_buffer);
}

#endif//HAVE_LIBDVDNAV
