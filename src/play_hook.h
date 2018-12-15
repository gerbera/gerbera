/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    play_hook.h - this file is part of MediaTomb.
    
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

/// \file play_hook.h
/// \brief Definition of the PlayHook class.
//
// \todo this should be solved via an observer model

#ifndef __PLAY_HOOK_H__
#define __PLAY_HOOK_H__

#include "cds_objects.h"
#include "common.h"
#include "singleton.h"

class PlayHook : public Singleton<PlayHook, std::mutex> {
public:
    void trigger(zmm::Ref<CdsObject> obj);
    zmm::String getName() override { return _("PlayHook"); }
};

#endif //__PLAY_HOOK_H__
