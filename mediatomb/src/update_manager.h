/*  update_manager.h - this file is part of MediaTomb.
                                                                                
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

#ifndef __UPDATE_MANAGER_H__
#define __UPDATE_MANAGER_H__

#include <pthread.h>

#include "common.h"
#include "hash.h"
#include "sync.h"

#define FLUSH_ASAP 2
#define FLUSH_SPEC 1

class UpdateManager : public zmm::Object
{
public:
    UpdateManager();
    virtual ~UpdateManager();
    
    void shutdown();
    
    void containerChanged(int objectID, int flushPolicy = FLUSH_SPEC);
    
    static zmm::Ref<UpdateManager> getInstance();
    
protected:
    void init();
    static zmm::Ref<UpdateManager> instance;
    pthread_t updateThread;
    static Mutex mutex;
    pthread_cond_t updateCond;
    
    zmm::Ref<DBRHash<int> > objectIDHash;
    
    bool shutdownFlag;
    int flushPolicy;
    
    int lastContainerChanged;
    
    static inline void lock() { mutex.lock(); }
    static inline void unlock() { mutex.unlock(); }
    static void *staticThreadProc(void *arg);
    void threadProc();
    
    inline bool haveUpdates() { return (objectIDHash->size() > 0); }
};


#endif // __UPDATE_MANAGER_H__

