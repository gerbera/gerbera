/*  file_io_handler.cc - this file is part of MediaTomb.
 
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
 
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/// \file file_io_handler.cc

#include "server.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "storage.h"
#include "cds_objects.h"
#include "process.h"
#include "update_manager.h"
#include <upnp/ixml.h>
#include "file_io_handler.h"
#include "dictionary.h"

using namespace zmm;
using namespace mxml;

FileIOHandler::FileIOHandler(String filename) : IOHandler()
{
    this->filename = filename;
}

void FileIOHandler::open(IN enum UpnpOpenFileMode mode)
{
    f = fopen(filename.c_str(), "rb");
    if (f == NULL)
    {
        throw Exception(_("FileIOHandler::open: failed to open: ") + filename.c_str());
    }
}

int FileIOHandler::read(OUT char *buf, IN size_t length)
{
    int ret = 0;

    ret = fread(buf, sizeof(char), length, f);

    if (ret <= 0)
    {
        if (feof(f)) return 0;
        if (ferror(f)) return -1;
    }

    return ret;
 }
                                                                                                                                                                         
void FileIOHandler::seek(IN long offset, IN int whence)
{
    if (fseek(f, offset, whence) != 0)
    {
        throw Exception(_("fseek failed"));
    }
}

void FileIOHandler::close()
{
    if (fclose(f) != 0)
    {
        throw Exception(_("fclose failed"));
    }
}


