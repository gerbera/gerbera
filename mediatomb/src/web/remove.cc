/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    remove.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.org>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>,
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

/// \file remove.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "server.h"
#include <stdio.h>
#include "common.h"
#include "content_manager.h"
#include "pages.h"
#include "tools.h"
#include "storage.h"

using namespace zmm;
using namespace mxml;

web::remove::remove() : WebRequestHandler()
{}

void web::remove::process()
{
    log_debug("remove: start\n");
    
    check_request();
    
    int objectID = intParam(_("object_id"));
    bool all = intParam(_("all"));
    
    ContentManager::getInstance()->removeObject(objectID, true, all);
    
    log_debug("remove: returning\n");
}

