/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    play_hook.cc - this file is part of MediaTomb.
    
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

/// \file play_hook.cc
/// \brief Definition of the PlayHook class.
//
// \todo this should be solved via an observer model

#include "play_hook.h"
#include "config_manager.h"
#include "content_manager.h"

#ifdef HAVE_LASTFMLIB
#include "lastfm_scrobbler.h"
#endif

using namespace zmm;

void PlayHook::trigger(zmm::Ref<CdsObject> obj)
{
    log_debug("start\n");
    Ref<ConfigManager> cfg = ConfigManager::getInstance();

    if (cfg->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_ENABLED) && !obj->getFlag(OBJECT_FLAG_PLAYED)) {
        Ref<Array<StringBase>> mark_list = cfg->getStringArrayOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_CONTENT_LIST);

        for (int i = 0; i < mark_list->size(); i++) {
            if (RefCast(obj, CdsItem)->getMimeType().startsWith(mark_list->get(i))) {
                obj->setFlag(OBJECT_FLAG_PLAYED);

                bool supress = cfg->getBoolOption(CFG_SERVER_EXTOPTS_MARK_PLAYED_ITEMS_SUPPRESS_CDS_UPDATES);

                Ref<ContentManager> cm = ContentManager::getInstance();
                log_debug("Marking object %s as played\n", obj->getTitle().c_str());
                cm->updateObject(obj, !supress);
                break;
            }
        }
    }

#ifdef HAVE_LASTFMLIB
    if (cfg->getBoolOption(CFG_SERVER_EXTOPTS_LASTFM_ENABLED) && (RefCast(obj, CdsItem)->getMimeType().startsWith("audio")))
        LastFm::getInstance()->startedPlaying(RefCast(obj, CdsItem));
#endif
    log_debug("end\n");
}
