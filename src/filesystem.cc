/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    filesystem.cc - this file is part of MediaTomb.
    
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

/// \file filesystem.cc

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"
#include "config_manager.h"
#include "content_manager.h"
#include "filesystem.h"
#include "mxml/mxml.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

int FsObjectComparator(void* arg1, void* arg2)
{
    auto* o1 = (FsObject*)arg1;
    auto* o2 = (FsObject*)arg2;
    if (o1->isDirectory) {
        if (o2->isDirectory)
            return strcmp(o1->filename.c_str(), o2->filename.c_str());
        else
            return -1;
    } else {
        if (o2->isDirectory)
            return 1;
        else
            return strcmp(o1->filename.c_str(), o2->filename.c_str());
    }
}

Filesystem::Filesystem()
    : Object()
{
    includeRules = Ref<Array<RExp>>(new Array<RExp>());
    Ref<ConfigManager> cm = ConfigManager::getInstance();
    /*    
    Ref<Element> rules = cm->getElement(_("filter"));
    if (rules == nullptr)
        return;
    for (int i = 0; i < rules->childCount(); i++)
    {
        Ref<Element> rule = rules->getChild(i);
        String pat = rule->getAttribute(_("pattern"));
        /// \todo make patterns from simple wildcards instead of
        /// taking the pattern attribute directly as regexp
        Ref<RExp> pattern(new RExp());
        try
        {
            pattern->compile(pat);
            includeRules->append(pattern);
        }
        catch (const Exception & e)
        {
            e.printStackTrace();
        }
    }
    */
}

Ref<Array<FsObject>> Filesystem::readDirectory(String path, int mask,
    int childMask)
{
    if (path.charAt(0) != '/') {
        throw _Exception(_("Filesystem: relative paths not allowed: ") + path);
    }
    if (!fileAllowed(path))
        throw _Exception(_("Filesystem: file blocked: ") + path);

    struct stat statbuf;
    int ret;

    Ref<Array<FsObject>> files(new Array<FsObject>());

    DIR* dir;
    struct dirent* dent;

    dir = opendir(path.c_str());
    if (!dir) {
        throw _Exception(_("could not list directory ") + path + " : " + strerror(errno));
    }

    while ((dent = readdir(dir)) != nullptr) {
        char* name = dent->d_name;
        if (name[0] == '.') {
            if (name[1] == 0) {
                continue;
            } else if (name[1] == '.' && name[2] == 0) {
                continue;
            } else if (!(mask & FS_MASK_HIDDEN))
                continue;
        }
        String childPath;
        if (path == FS_ROOT_DIRECTORY)
            childPath = path + name;
        else
            childPath = path + "/" + name;
        if (fileAllowed(childPath)) {
            bool isDirectory = false;
            bool hasContent = false;
            ret = stat(childPath.c_str(), &statbuf);
            if (ret != 0)
                continue;
            if (S_ISREG(statbuf.st_mode)) {
                if (!(mask & FS_MASK_FILES))
                    continue;
            } else if (S_ISDIR(statbuf.st_mode)) {
                if (!(mask & FS_MASK_DIRECTORIES))
                    continue;
                isDirectory = true;
                if (childMask) {
                    try {
                        hasContent = have(childPath, childMask);
                    } catch (const Exception& e) {
                        //continue;
                    }
                }
            } else
                continue; // special file

            Ref<FsObject> obj(new FsObject());
            obj->filename = name;
            obj->isDirectory = isDirectory;
            obj->hasContent = hasContent;
            files->append(obj);
        }
    }
    closedir(dir);

    quicksort((COMPARABLE*)files->getObjectArray(), files->size(),
        FsObjectComparator);

    return files;
}

bool Filesystem::have(String path, int mask)
{
    /*
    if (path.charAt(0) != '/')
    {
        throw _Exception(_("Filesystem relative paths not allowed: ") +
                        path);
    }
    */
    if (!fileAllowed(path))
        return false;

    struct stat statbuf;
    int ret;

    Ref<Array<FsObject>> files(new Array<FsObject>());

    DIR* dir;
    struct dirent* dent;

    dir = opendir(path.c_str());
    if (!dir) {
        throw _Exception(_("could not list directory ") + path + " : " + strerror(errno));
    }

    bool result = false;
    while ((dent = readdir(dir)) != nullptr) {
        char* name = dent->d_name;
        if (name[0] == '.') {
            if (name[1] == 0) {
                continue;
            } else if (name[1] == '.' && name[2] == 0) {
                continue;
            } else if (!(mask & FS_MASK_HIDDEN))
                continue;
        }
        String childPath;
        if (path == FS_ROOT_DIRECTORY)
            childPath = path + name;
        else
            childPath = path + "/" + name;
        if (fileAllowed(childPath)) {
            ret = stat(childPath.c_str(), &statbuf);
            if (ret != 0)
                continue;
            if (S_ISREG(statbuf.st_mode) && mask & FS_MASK_FILES) {
                result = true;
                break;
            } else if (S_ISDIR(statbuf.st_mode) && mask & FS_MASK_DIRECTORIES) {
                result = true;
                break;
            } else
                continue; // special file
        }
    }
    closedir(dir);
    return result;
}

bool Filesystem::haveFiles(String dir)
{
    return have(dir, FS_MASK_FILES);
}
bool Filesystem::haveDirectories(String dir)
{
    return have(dir, FS_MASK_DIRECTORIES);
}

bool Filesystem::fileAllowed(String path)
{
    if (path == ConfigManager::getInstance()->getConfigFilename())
        return false;

    return true;
    /*
    for (int i = 0; i < include_rules->size(); i++)
    {
        Ref<RExp> rule = include_rules->get(i);
        if (rule->matches(path))
            return true;
    }
    return false;
    */
}
