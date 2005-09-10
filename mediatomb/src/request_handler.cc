/*  request_handler.cc - this file is part of MediaTomb.
                                                                                
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

#include "request_handler.h"

using namespace zmm;

void RequestHandler::split_url(const char *url, String &path, String &parameters)
{
    char *i1 = index(url, '?');

    if (i1 == NULL)
    {
        path = String((char *)url);
        parameters = _("");
    }
    else
    {
        parameters = String(i1 + 1);
        path = String((char *)url, i1 - url);
    }
}


RequestHandler::RequestHandler() : zmm::Object()
{
}

void RequestHandler::get_info(IN const char *filename, OUT struct File_Info *info)
{

}
Ref<IOHandler> RequestHandler::open(IN const char *filename, IN enum UpnpOpenFileMode mode)
{
    return nil;
}

