/*  fs_storage.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "common.h"
#include "config_manager.h"
#include "content_manager.h"
#include "filesystem.h"
#include "mxml/mxml.h"
#include "tools.h"

using namespace zmm;
using namespace mxml;

#ifdef __CYGWIN__
#define FS_ROOT_DIRECTORY "/cygdrive/"
#else
#define FS_ROOT_DIRECTORY "/"
#endif

int FsObjectComparator(void *arg1, void *arg2)
{
    return strcmp(((StringBase *)arg1)->data, ((StringBase *)arg2)->data);
    FsObject *o1 = (FsObject *)arg1;
    FsObject *o2 = (FsObject *)arg2;
    if (o1->isDirectory)
    {
        if (o2->isDirectory)
            return strcmp(o1->filename.c_str(), o2->filename.c_str());
        else
            return -1;
    }
    else
    {
        if (o2->isDirectory)
            return 1;
        else
            return strcmp(o1->filename.c_str(), o2->filename.c_str());
    }
}


Filesystem::Filesystem() : Object()
{
    includeRules = Ref<Array<RExp> >(new Array<RExp>());
    Ref<ConfigManager> cm = ConfigManager::getInstance();
    Ref<Element> rules = cm->getElement("filter");
    if (rules == nil)
        return;
    for (int i = 0; i < rules->childCount(); i++)
    {
        Ref<Element> rule = rules->getChild(i);
        String pat = rule->getAttribute("pattern");
        /// \todo make patterns from simple wildcards instead of
        /// taking the pattern attribute directly as regexp
        Ref<RExp> pattern(new RExp());
        try
        {
            pattern->compile(pat);
            includeRules->append(pattern);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
}

Ref<Array<FsObject> > Filesystem::readDirectory(String path)
{
    if (path.charAt(0) != '/')
    {
        throw StorageException(String("relative paths not allowed: ") + path);
    }
    if (! fileAllowed(path))
        throw Exception("this file is blocked from viewing from filesystem browser");

    struct stat statbuf;
    int ret;

    Ref<Array<FsObject> > files(new Array<FsObject>());

    DIR *dir;
    struct dirent *dent;

    dir = opendir(path.c_str());
    if (! dir)
    {
        throw Exception(String("could not list directory ")+
                        path + " : " + strerror(errno));
    }

    while ((dent = readdir(dir)) != NULL)
    {
        char *name = dent->d_name;
        if (name[0] == '.')
        {
            if (name[1] == 0)
            {
                continue;
            }
            else if (name[1] == '.' && name[2] == 0)
            {
                continue;
            }
        }
        String childPath;
        if (path == FS_ROOT_DIRECTORY)
            childPath = path + name;
        else
            childPath = path + "/" + name;
        if (fileAllowed(childPath))
        {
            Ref<FsObject> obj(new FsObject());
            obj->filename = name;

            ret = stat(childPath.c_str(), &statbuf);
            if (ret != 0)
                continue;
            if (S_ISREG(statbuf.st_mode))
            {}
            else if (S_ISDIR(statbuf.st_mode))
            {
                obj->isDirectory = true;
            }
            else
                continue; // special file
            files->append(obj);
        }
    }
    closedir(dir);

    quicksort((COMPARABLE *)files->getObjectArray(), files->size(),
              FsObjectComparator);

    return files;
}

bool Filesystem::fileAllowed(String path)
{
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

