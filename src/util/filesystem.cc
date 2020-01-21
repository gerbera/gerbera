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
#include "config/config_manager.h"
#include "content_manager.h"
#include "filesystem.h"
#include "tools.h"

static bool FsObjectComparator(std::shared_ptr<FsObject> o1, std::shared_ptr<FsObject> o2)
{
    if (o1->isDirectory) {
        if (o2->isDirectory)
            return strcmp(o1->filename.c_str(), o2->filename.c_str()) < 0;
        else
            return false;
    } else {
        if (o2->isDirectory)
            return true;
        else
            return strcmp(o1->filename.c_str(), o2->filename.c_str()) < 0;
    }
}

Filesystem::Filesystem(std::shared_ptr<ConfigManager> config)
    : config(config)
{
}

std::vector<std::shared_ptr<FsObject>> Filesystem::readDirectory(std::string path, int mask,
    int childMask)
{
    if (path.at(0) != '/') {
        throw std::runtime_error("Filesystem: relative paths not allowed: " + path);
    }
    if (!fileAllowed(path))
        throw std::runtime_error("Filesystem: file blocked: " + path);

    struct stat statbuf;
    int ret;

    std::vector<std::shared_ptr<FsObject>> files;

    DIR* dir;
    struct dirent* dent;

    dir = opendir(path.c_str());
    if (!dir) {
        throw std::runtime_error("could not list directory " + path + " : " + strerror(errno));
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
        std::string childPath;
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
                    } catch (const std::runtime_error& e) {
                        //continue;
                    }
                }
            } else
                continue; // special file

            auto obj = std::make_shared<FsObject>();
            obj->filename = name;
            obj->isDirectory = isDirectory;
            obj->hasContent = hasContent;
            files.push_back(obj);
        }
    }
    closedir(dir);

    std::sort(files.begin(), files.end(), FsObjectComparator);

    return files;
}

bool Filesystem::have(std::string path, int mask)
{
    /*
    if (path.charAt(0) != '/')
    {
        throw std::runtime_error("Filesystem relative paths not allowed: " +
                        path);
    }
    */
    if (!fileAllowed(path))
        return false;

    struct stat statbuf;
    int ret;

    DIR* dir;
    struct dirent* dent;

    dir = opendir(path.c_str());
    if (!dir) {
        throw std::runtime_error("could not list directory " + path + " : " + strerror(errno));
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
        std::string childPath;
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

bool Filesystem::haveFiles(std::string dir)
{
    return have(dir, FS_MASK_FILES);
}
bool Filesystem::haveDirectories(std::string dir)
{
    return have(dir, FS_MASK_DIRECTORIES);
}

bool Filesystem::fileAllowed(std::string path)
{
    if (path == config->getConfigFilename())
        return false;

    return true;
}
