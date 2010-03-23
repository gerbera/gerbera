/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    hash.h - this file is part of MediaTomb.
    
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

/// \file hash.h

#ifndef __HASH_H__
#define __HASH_H__

#define HASH_PRIME 29
#define HASH_MAX_ITERATIONS 100

#include "common.h"

struct hash_slot_struct
{
    int dummy;
};
typedef struct hash_slot_struct *hash_slot_t;

#include "hash/db_hash.h"
#include "hash/dbr_hash.h"
#include "hash/dbb_hash.h"
#include "hash/dbo_hash.h"
#include "hash/dsb_hash.h"
#include "hash/dso_hash.h"

#endif // __HASH_H__
