/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    transcode_handler.h - this file is part of MediaTomb.
    
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

/// \file transcode_handler.h
/// \brief Definition of the TranscodeRequest class.
#ifndef __TRANSCODE_HANDLER_H__
#define __TRANSCODE_HANDLER_H__

#include "common.h"
#include "io_handler.h"
#include "transcoding.h"
#include "upnp.h"
#include "cds_objects.h"

class TranscodeHandler : public zmm::Object
{
public:
//    TranscodeHandler();
    virtual zmm::Ref<IOHandler> open(zmm::Ref<TranscodingProfile> profile, 
                                     zmm::String location,
                                     zmm::Ref<CdsObject> obj, 
                                     struct File_Info *info) = 0;
protected:
    enum { UNKNOWN_CONTENT_LENGTH = -1 };
};


#endif // __TRANSCODE_HANDLER_H__
