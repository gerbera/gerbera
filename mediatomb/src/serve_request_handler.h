/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    serve_request_handler.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
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

/// \file serve_request_handler.h
/// \brief Definition of the ServeRequestHandler class.
#ifndef __SERVE_REQUEST_HANDLER_H__
#define __SERVE_REQUEST_HANDLER_H__

#include "common.h"
#include "request_handler.h"
#include "dictionary.h"

class ServeRequestHandler : public RequestHandler
{
public:
    ServeRequestHandler();
    virtual void get_info(IN const char *filename, OUT struct File_Info *info);
    virtual zmm::Ref<IOHandler> open(IN const char *filename,  OUT struct File_Info *info,
                                     IN enum UpnpOpenFileMode mode);
};


#endif // __SERVE_REQUEST_HANDLER_H__

