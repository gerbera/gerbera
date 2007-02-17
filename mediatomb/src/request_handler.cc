/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    request_handler.cc - this file is part of MediaTomb.
    
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

/// \file request_handler.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "request_handler.h"
#include "tools.h"

using namespace zmm;

void RequestHandler::split_url(const char *url, char separator, String &path, String &parameters)
{
    String url_s = unescape_amp(String(url));
    url_s = url_unescape(url_s);

    int i1 = url_s.rindex(separator);

    if (i1 < 0)
    {
        path = url_s;
        parameters = _("");
    }
    else
    {
        parameters = url_s.substring(i1 + 1);
        path = url_s.substring(0, i1);
    }
}


RequestHandler::RequestHandler() : zmm::Object()
{
}

void RequestHandler::get_info(IN const char *filename, OUT struct File_Info *info)
{

}
Ref<IOHandler> RequestHandler::open(IN const char *filename, OUT struct File_Info *info,
        IN enum UpnpOpenFileMode mode)
{
    return nil;
}

