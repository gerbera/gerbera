/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dvd_io_handler.cc - this dvd is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2008 Gena Batyan <bgeradz@mediatomb.cc>,
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
    
    $Id: dvd_io_handler.cc 1698 2008-02-23 20:48:30Z lww $
*/

/// \dvd dvd_io_handler.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_LIBDVDNAV

#include "server.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "dvd_io_handler.h"

using namespace zmm;
using namespace mxml;

DVDIOHandler::DVDIOHandler(String dvdname, int track, int chapter, 
                           int audio_stream_id) : IOHandler()
{
    this->dvdname = dvdname;
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
    return (int) dvd->readSector((unsigned char *)buf, length); 
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
}

off_t DVDIOHandler::length()
{
    return -1;
}

#endif//HAVE_LIBDVDNAV
