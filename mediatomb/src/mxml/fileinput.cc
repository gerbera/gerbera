/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    fileinput.cc - this file is part of MediaTomb.
    
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
    
    $Id$
*/

/// \file fileinput.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "fileinput.h"

#include <string.h>
#include <errno.h>

using namespace mxml;
using namespace zmm;

FileInput::FileInput(String filename) : Input()
{
    file = fopen(filename.c_str(), "r");
    if (file == NULL)
    {
        throw _Exception(_("could not open file ") + filename + " : " + strerror(errno));
    }
}

FileInput::~FileInput()
{
    if (file) fclose(file);
}

int FileInput::readChar()
{
    int ret = fgetc(file);
    if(ret < 0)
    {
        fclose(file);
        file = NULL;
        return -1;
    }
    return ret;
}
