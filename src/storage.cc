/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    storage.cc - this file is part of MediaTomb.
    
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

/// \file storage.cc

#include "config_manager.h"

#if !defined(HAVE_MYSQL) && !defined(HAVE_SQLITE3)
#error "need at least one storage (mysql or sqlite3)"
#endif

#include "storage/mysql/mysql_storage.h"
#include "storage/sqlite3/sqlite3_storage.h"

#include "tools.h"

using namespace zmm;

Ref<Storage> Storage::getInstance()
{
    if (!instance->singletonActive)
        throw _Exception(_("singleton is currently inactive!"));
    if (instance == nullptr) {
        // extra scope added so that lock can be released prior to metadata upgrade
        // when flushInsertBuffer() will try taking this lock
        {
            AutoLock lock(mutex);
            if (!instance->singletonActive)
                throw _Exception(_("singleton is currently inactive!"));
            if (instance == nullptr) {
                Ref<Storage> tmpInstance = createInstance();
                tmpInstance->init();
                tmpInstance->registerSingleton();
                instance = tmpInstance;
            }
        }
        if (instance != nullptr)
            instance->doMetadataMigration();
    }
    return instance;
}

Ref<Storage> Storage::createInstance()
{
    String type;
    Ref<Storage> storage;

    Ref<ConfigManager> config = ConfigManager::getInstance();
    type = config->getOption(CFG_SERVER_STORAGE_DRIVER);

    do {
#ifdef HAVE_SQLITE3
        if (type == "sqlite3") {
            storage = Ref<Storage>(new Sqlite3Storage());
            break;
        }
#endif

#ifdef HAVE_MYSQL
        if (type == "mysql") {
            storage = Ref<Storage>(new MysqlStorage());
            break;
        }
#endif
        // other database types...
        throw _Exception(_("Unknown storage type: ") + type);
    } while (false);

    return storage;
}

void Storage::stripAndUnescapeVirtualContainerFromPath(String path, String& first, String& last)
{
    if (path.charAt(0) != VIRTUAL_CONTAINER_SEPARATOR) {
        throw _Exception(_("got non-absolute virtual path; needs to start with: ") + VIRTUAL_CONTAINER_SEPARATOR);
    }
    int sep = path.rindex(VIRTUAL_CONTAINER_SEPARATOR);
    if (sep == 0) {
        first = _("/");
        last = path.substring(1);
    } else {
        while (sep > 0) {
            char beforeSep = path.charAt(sep - 1);
            if (beforeSep != VIRTUAL_CONTAINER_ESCAPE) {
                break;
            }
            sep = path.rindex(sep - 1, VIRTUAL_CONTAINER_SEPARATOR);
        }
        if (sep == 0) {
            first = _("/");
            last = unescape(path.substring(sep + 1), VIRTUAL_CONTAINER_ESCAPE);
        } else {
            first = path.substring(0, sep);
            last = unescape(path.substring(sep + 1), VIRTUAL_CONTAINER_ESCAPE);
        }
    }
}
