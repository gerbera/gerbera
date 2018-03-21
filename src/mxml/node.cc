/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    node.cc - this file is part of MediaTomb.
    
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

/// \file node.cc

#include "mxml.h"

#include <cstring>

using namespace zmm;
using namespace mxml;

String Node::print()
{
    std::ostringstream buf;
    print_internal(buf, 0);
    return buf.str();
}

String Node::escape(String str)
{
    std::ostringstream buf;
    auto *ptr = (signed char *)str.c_str();
    while (ptr && *ptr)
    {
        switch (*ptr)
        {
            case '<' : buf << "&lt;"; break;
            case '>' : buf << "&gt;"; break;
            case '&' : buf << "&amp;"; break;
            case '"' : buf << "&quot;"; break;
            case '\'' : buf << "&apos;"; break;
                       // handle control codes
            default  : if (((*ptr >= 0x00) && (*ptr <= 0x1f) && 
                            (*ptr != 0x09) && (*ptr != 0x0d) && 
                            (*ptr != 0x0a)) || (*ptr == 0x7f))
                       {
                           buf << '.';
                       }
                       else
                           buf << *ptr;
                       break;
        }
        ptr++;
    }
    return buf.str();
}

/*
void Node::print_internal(Ref<StringBuffer> buf, int indent)
{
    static char *ind_str = "                                                               ";
    static char *ind = ind_str + strlen(ind_str);
    char *ptr = ind - indent * 2;
    *buf << ptr;
}
*/
