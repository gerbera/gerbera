/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    stringtokenizer.cc - this file is part of MediaTomb.
    
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

/// \file stringtokenizer.cc

#include "stringtokenizer.h"

#include <string.h>

using namespace zmm;

StringTokenizer::StringTokenizer(String str)
{
    this->str = str;
    pos = 0;
    len = str.length();
}
String StringTokenizer::nextToken(String seps)
{
    char *cstr = str.c_str();
    char *cseps = seps.c_str();
    while(pos < len && strchr(cseps, cstr[pos]))
        pos++;
    if(pos < len)
    {
        int start = pos;
        while(pos < len && ! strchr(cseps, cstr[pos]))
            pos++;
        return str.substring(start, pos - start);
    }
    return nil;
}
