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

#include <condition_variable>
#include <memory>
#include <unordered_set>
#include <vector>

#include "common.h"
#include "singleton.h"

#define FLUSH_ASAP 2
#define FLUSH_SPEC 1

class UpdateManager : public Singleton<UpdateManager> {
public:
    UpdateManager();
    virtual void shutdown() override;
    virtual void init() override;
    zmm::String getName() override { return _("Update Manager"); }

    void containerChanged(int objectID, int flushPolicy = FLUSH_SPEC);
    void containersChanged(const std::vector<int>& objectIDs, int flushPolicy = FLUSH_SPEC);

protected:
    pthread_t updateThread;
    std::condition_variable cond;

    std::shared_ptr<std::unordered_set<int>> objectIDHash;

    bool shutdownFlag;
    int flushPolicy;

    int lastContainerChanged;

    static void* staticThreadProc(void* arg);
    void threadProc();

    inline bool haveUpdates() { return (objectIDHash->size() > 0); }
};

#endif // __UPDATE_MANAGER_H__
