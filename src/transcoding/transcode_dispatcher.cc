/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    transcode_dispatcher.cc - this file is part of MediaTomb.
    
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

/// \file transcode_dispatcher.cc

#ifdef EXTERNAL_TRANSCODING

#include "common.h"
#include "cds_objects.h"
#include "transcoding.h"
#include "transcode_dispatcher.h"
#include "transcode_ext_handler.h"
#include "tools.h"

using namespace zmm;

TranscodeDispatcher::TranscodeDispatcher() : TranscodeHandler()
{
}

Ref<IOHandler> TranscodeDispatcher::open(Ref<TranscodingProfile> profile, 
                                         String location, 
                                         Ref<CdsObject> obj,
                                         String range)
{
    if (profile == nil)
        throw _Exception(_("Transcoding of file ") + location +
                           "requested but no profile given ");
   
//    check_path_ex(location);

    if (profile->getType() == TR_External)
    {
        Ref<TranscodeExternalHandler> tr_ext(new TranscodeExternalHandler());
        return tr_ext->open(profile, location, obj, range);
    }
    else
        throw _Exception(_("Unknown transcoding type for profile ") + 
                         profile->getName());
}

#endif//EXTERNAL_TRANSCODING
