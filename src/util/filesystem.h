/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    filesystem.h - this file is part of MediaTomb.
    
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

/// \file filesystem.h

#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include <memory>
#include "common.h"
#include "rexp.h"

#define FS_MASK_FILES 1
#define FS_MASK_DIRECTORIES 2
#define FS_MASK_HIDDEN 4 // accept hidden files (start with . "dot")

#define FS_ROOT_DIRECTORY "/"

class FsObject : public zmm::Object {
public:
    inline FsObject()
        : zmm::Object()
    {
        isDirectory = false;
        hasContent = false;
    }

public:
    std::string filename;
    bool isDirectory;
    bool hasContent;
};

// forward declaration
class ConfigManager;

class Filesystem : public zmm::Object {
public:
    Filesystem(std::shared_ptr<ConfigManager> config);

    zmm::Ref<zmm::Array<FsObject>> readDirectory(std::string path, int mask,
        int chldMask = 0);
    bool haveFiles(std::string dir);
    bool haveDirectories(std::string dir);
    bool fileAllowed(std::string path);

protected:
    std::shared_ptr<ConfigManager> config;
    zmm::Ref<zmm::Array<RExp>> includeRules;
    bool have(std::string dir, int mask);
};

#endif // __FILESYSTEM_H__
