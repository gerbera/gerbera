/*MT*

    MediaTomb - http://www.mediatomb.cc/

    update_manager.h - this file is part of MediaTomb.

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

/// \file update_manager.h

#ifndef __UPDATE_MANAGER_H__
#define __UPDATE_MANAGER_H__

#include <memory>
#include <unordered_set>
#include <vector>

#include "common.h"
#include "util/thread_runner.h"

// forward declaration
class Config;
class Database;
class Server;

#define FLUSH_ASAP 2
#define FLUSH_SPEC 1

class UpdateManager {
public:
    UpdateManager(std::shared_ptr<Config> config, std::shared_ptr<Database> database, std::shared_ptr<Server> server);
    virtual ~UpdateManager();

    UpdateManager(const UpdateManager&) = delete;
    UpdateManager& operator=(const UpdateManager&) = delete;

    void run();
    void shutdown();

    void containerChanged(int objectID, int flushPolicy = FLUSH_SPEC);
    void containersChanged(const std::vector<int>& objectIDs, int flushPolicy = FLUSH_SPEC);

protected:
    std::shared_ptr<Config> config;
    std::shared_ptr<Database> database;
    std::shared_ptr<Server> server;

    std::unique_ptr<StdThreadRunner> threadRunner;

    std::unique_ptr<std::unordered_set<int>> objectIDHash;

    bool shutdownFlag;
    int flushPolicy;

    int lastContainerChanged;

    static void* staticThreadProc(void* arg);
    void threadProc();

    bool haveUpdates() const { return !objectIDHash->empty(); }
};

#endif // __UPDATE_MANAGER_H__
