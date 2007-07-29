/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    file_io_handler.cc - this file is part of MediaTomb.
    
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

/// \file file_io_handler.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

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
#include "ixml.h"
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
    printf("%s: OPEN on file %s\n", __func__, this->filename.c_str());
    f = fopen(filename.c_str(), "rb");
    if (f == NULL)
    {
        throw _Exception(_("FileIOHandler::open: failed to open: ") + filename.c_str());
    }
}

int FileIOHandler::read(OUT char *buf, IN size_t length)
{
    int ret = 0;

    printf("%s: READ on file %s\n", __func__, this->filename.c_str());
    ret = fread(buf, sizeof(char), length, f);

    if (ret <= 0)
    {
        if (feof(f)) return 0;
        if (ferror(f)) return -1;
    }

    return ret;
 }
                                                                                                                                                                         
void FileIOHandler::seek(IN off_t offset, IN int whence)
{
    printf("%s: SEEK on file %s\n", __func__, this->filename.c_str());
    if (fseeko(f, offset, whence) != 0)
    {
        throw _Exception(_("fseek failed"));
    }
}

void FileIOHandler::close()
{
    printf("CLOSING FILE: %s\n", this->filename.c_str());
    if (fclose(f) != 0)
    {
        throw _Exception(_("fclose failed"));
    }
    f = NULL;
}
