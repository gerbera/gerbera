/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    cached_url.cc - this file is part of MediaTomb.
    
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

/// \file cached_url.cc

#include <ctime>

#include "cached_url.h"
#include "tools.h"

using namespace zmm;

CachedURL::CachedURL(int object_id, zmm::String url)
{
    this->object_id = object_id;
    this->url = url;
    this->creation_time = time(nullptr);
    if (this->creation_time == -1) {
        throw _Exception(_("Failed to get current time: ") + mt_strerror(errno));
    }
    this->last_access_time = creation_time;
}

int CachedURL::getObjectID()
{
    return object_id;
}

String CachedURL::getURL()
{
    AutoLock lock(mutex);
    last_access_time = time(nullptr);
    if (last_access_time == -1) {
        throw _Exception(_("Failed to get current time: ") + mt_strerror(errno));
    }
    return url;
}

time_t CachedURL::getCreationTime()
{
    return creation_time;
}

/// \brief Retrieves the time when the last access time of the data.
time_t CachedURL::getLastAccessTime()
{
    AutoLock lock(mutex);
    return last_access_time;
}
