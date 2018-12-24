/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    request_handler.cc - this file is part of MediaTomb.
    
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

/// \file request_handler.cc

#include "request_handler.h"
#include "tools.h"

using namespace zmm;

void RequestHandler::splitUrl(const char *url, char separator, String &path, String &parameters)
{
    int i1;

    String url_s = url;

    if (separator == '/')
        i1 = url_s.rindex(separator);
    else if (separator == '?')
        i1 = url_s.index(separator);
    else
        throw _Exception(String("Forbidden separator: ") + separator);

    if (i1 < 0) {
        path = url_s;
        parameters = _("");
    } else {
        parameters = url_s.substring(i1 + 1);
        path = url_s.substring(0, i1);
    }
}
