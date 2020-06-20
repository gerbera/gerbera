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

#include "storage.h" // API

#include <utility>

#include "config/config_manager.h"
#include "storage/mysql/mysql_storage.h"
#include "storage/sqlite3/sqlite3_storage.h"
#include "util/tools.h"

Storage::Storage(std::shared_ptr<Config> config)
    : config(std::move(config))
{
}

std::shared_ptr<Storage> Storage::createInstance(const std::shared_ptr<Config>& config, const std::shared_ptr<Timer>& timer)
{
    std::shared_ptr<Storage> storage;

    std::string type = config->getOption(CFG_SERVER_STORAGE_DRIVER);
    do {
        if (type == "sqlite3") {
            storage = std::static_pointer_cast<Storage>(std::make_shared<Sqlite3Storage>(config, timer));
            break;
        }

#ifdef HAVE_MYSQL
        if (type == "mysql") {
            storage = std::static_pointer_cast<Storage>(std::make_shared<MysqlStorage>(config));
            break;
        }
#endif
        // other database types...
        throw_std_runtime_error("Unknown storage type: " + type);
    } while (false);

    storage->init();
    storage->doMetadataMigration();

    return storage;
}

void Storage::stripAndUnescapeVirtualContainerFromPath(std::string virtualPath, std::string& first, std::string& last)
{
    if (virtualPath.at(0) != VIRTUAL_CONTAINER_SEPARATOR) {
        throw_std_runtime_error(fmt::format("Got non-absolute virtual path; needs to start with: {}", VIRTUAL_CONTAINER_SEPARATOR));
    }
    size_t sep = virtualPath.rfind(VIRTUAL_CONTAINER_SEPARATOR);
    if (sep == 0) {
        first = "/";
        last = virtualPath.substr(1);
    } else {
        while (sep != std::string::npos) {
            char beforeSep = virtualPath.at(sep - 1);
            if (beforeSep != VIRTUAL_CONTAINER_ESCAPE) {
                break;
            }
            sep = virtualPath.rfind(VIRTUAL_CONTAINER_SEPARATOR, sep - 1);
        }
        if (sep == 0) {
            first = "/";
        } else {
            first = virtualPath.substr(0, sep);
        }
        last = unescape(virtualPath.substr(sep + 1), VIRTUAL_CONTAINER_ESCAPE);
    }
}
