/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    fileinput.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
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

