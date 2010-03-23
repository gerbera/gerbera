/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    fallback_layout.h - this file is part of MediaTomb.
    
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

/// \file fallback_layout.h

#ifndef __FALLBACK_LAYOUT_H__
#define __FALLBACK_LAYOUT_H__

#include "layout.h"
#include "cds_objects.h"

#ifdef ENABLE_PROFILING
    #include "tools.h"
#endif
class FallbackLayout : public Layout
{
public:
    FallbackLayout();
    virtual void processCdsObject(zmm::Ref<CdsObject> obj, zmm::String rootpath);
#ifdef ENABLE_PROFILING
    virtual ~FallbackLayout();
#endif
protected:
    void add(zmm::Ref<CdsObject> obj, int parentID, bool use_ref = true);
    zmm::String esc(zmm::String str);
    void addVideo(zmm::Ref<CdsObject> obj, zmm::String rootpath);
    void addImage(zmm::Ref<CdsObject> obj, zmm::String rootpath);
    void addAudio(zmm::Ref<CdsObject> obj);
#ifdef HAVE_LIBDVDNAV
    zmm::Ref<CdsObject> prepareChapter(zmm::Ref<CdsObject> obj, int title_idx,
                                       int chapter_idx);
    void addDVD(zmm::Ref<CdsObject> obj);
    zmm::String mpeg_mimetype;
#endif
#ifdef YOUTUBE
    void addYouTube(zmm::Ref<CdsObject> obj);
#endif
#ifdef SOPCAST
    void addSopCast(zmm::Ref<CdsObject> obj);
#endif
#ifdef WEBORAMA
    void addWeborama(zmm::Ref<CdsObject> obj);
#endif
#ifdef ATRAILERS
    void addATrailers(zmm::Ref<CdsObject> obj);
#endif
#ifdef ENABLE_PROFILING
    bool profiling_initialized;
    profiling_t layout_profiling;
#endif
};

#endif // __FALLBACK_LAYOUT_H__
