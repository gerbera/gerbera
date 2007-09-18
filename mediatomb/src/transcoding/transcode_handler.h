/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    transcode_handler.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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
    
    $Id: transcode_handler.h 1436 2007-08-17 17:46:29Z lww $
*/

/// \file transcode_handler.h
/// \brief Definition of the TranscodeRequest class.
#ifndef __TRANSCODE_HANDLER_H__
#define __TRANSCODE_HANDLER_H__

#include "common.h"
#include "io_handler.h"
#include "transcoding.h"
#include "upnp.h"

class TranscodeHandler : public zmm::Object
{
public:
//    TranscodeHandler();
    virtual zmm::Ref<IOHandler> open(zmm::Ref<TranscodingProfile> profile, 
                                     zmm::String location,
                                     int objectType, 
                                     struct File_Info *info) = 0;
};


#endif // __TRANSCODE_HANDLER_H__
