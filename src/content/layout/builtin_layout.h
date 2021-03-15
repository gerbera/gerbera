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

/// \file builtin_layout.h

#ifndef __BUILTIN_LAYOUT_H__
#define __BUILTIN_LAYOUT_H__

#include <map>
#include <memory>

#include "layout.h"

#ifdef ENABLE_PROFILING
#include "util/tools.h"
#endif

// forward declaration
class CdsObject;

class BuiltinLayout : public Layout {
public:
    explicit BuiltinLayout(std::shared_ptr<ContentManager> content);
#ifdef ENABLE_PROFILING
    virtual ~BuiltinLayout();
#endif

    void processCdsObject(std::shared_ptr<CdsObject> obj, fs::path rootpath) override;

protected:
    void add(const std::shared_ptr<CdsObject>& obj, const std::pair<int, bool>& parentID, bool use_ref = true);
    static std::string esc(std::string str);
    void addVideo(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath);
    void addImage(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath);
    void addAudio(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath);
    std::string mapGenre(const std::string& genre);
#ifdef SOPCAST
    void addSopCast(const std::shared_ptr<CdsObject>& obj);
#endif
#ifdef ATRAILERS
    void addATrailers(const std::shared_ptr<CdsObject>& obj);
#endif
    std::map<std::string, std::string> genreMap;
#ifdef ENABLE_PROFILING
    bool profiling_initialized;
    profiling_t layout_profiling;
#endif
};

#endif // __BUILTIN_LAYOUT_H__
