/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dvd_handler.h - this file is part of MediaTomb.
    
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

/// \file dvd_handler.h
/// \brief Definition of the DVD class.

#ifndef __METADATA_DVD_H__
#define __METADATA_DVD_H__

#include "metadata_handler.h"

#define DVD_AUXDATA_TITLE_COUNT           "DVD_TITLES"
#define DVD_AUXDATA_CHAPTERS              "DVD_CHAPTERS"
#define DVD_AUXDATA_AUDIO_TRACKS          "DVD_AUDIO_TRACKS"



typedef enum
{
    DVD_Title,
    DVD_TitleDuration,
    DVD_TitleCount,

    DVD_Chapter,
    DVD_ChapterCount,
    DVD_ChapterDuration,
    DVD_ChapterRestDuration,
   
    DVD_AudioStreamID,
    DVD_AudioTrack,
    DVD_AudioTrackCount,
    DVD_AudioTrackFormat,
    DVD_AudioTrackStreamID,
    DVD_AudioTrackChannels,
    DVD_AudioTrackSampleFreq,
    DVD_AudioTrackLanguage,
} dvd_aux_key_names_t;


/// \brief This class is responsible for parsing DVDs and DVD ISO images
class DVDHandler : public MetadataHandler
{
public:
    DVDHandler();
    virtual void fillMetadata(zmm::Ref<CdsItem> item);
    virtual zmm::Ref<IOHandler> serveContent(zmm::Ref<CdsItem> item, int resNum, off_t *data_size);

    /// \brief Helper function to construct the key names for DVD aux data
    static zmm::String renderKey(dvd_aux_key_names_t name, int title_idx = 0, 
                                 int chapter_idx = 0, int audio_track_idx = 0);
};

#endif // __METADATA_DVD_H__
