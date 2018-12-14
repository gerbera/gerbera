/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    cached_url.h - this file is part of MediaTomb.
    
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

/// \file cached_url.h
/// \brief Definition of the CachedURL class.

#ifndef __CACHED_URL_H__
#define __CACHED_URL_H__

#include "zmm/zmm.h"
#include "zmm/zmmf.h"
#include <mutex>

/// \brief Stores information about cached URLs
class CachedURL : public zmm::Object {
public:
    /// \brief Creates a cached url object.
    CachedURL(int object_id, zmm::String url);

    /// \brief Retrieves the object id.
    int getObjectID();

    /// \brief Retrieves the cached URL.
    zmm::String getURL();

    /// \brief Retrieves the time when the object was created.
    time_t getCreationTime();

    /// \brief Retrieves the time when the last access time of the data.
    time_t getLastAccessTime();

protected:
    int object_id;
    zmm::String url;
    time_t creation_time;
    time_t last_access_time;

    std::mutex mutex;
    using AutoLock = std::lock_guard<std::mutex>;
};

#endif //__CACHED_URL_H__
