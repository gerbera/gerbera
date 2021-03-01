/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    layout.h - this file is part of MediaTomb.
    
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

/// \file layout.h

#ifndef __LAYOUT_H__
#define __LAYOUT_H__

#include <filesystem>
#include <memory>
namespace fs = std::filesystem;

#include "context.h"

// forward declaration
class CdsObject;
class ContentManager;

class Layout {
public:
    explicit Layout(std::shared_ptr<ContentManager> content);

    virtual ~Layout() = default;
    virtual void processCdsObject(std::shared_ptr<CdsObject> obj, fs::path rootpath) = 0;

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Database> database;
    std::shared_ptr<ContentManager> content;
};

#endif // __LAYOUT_H__
