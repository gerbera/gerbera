/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    thread_executor.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file thread_executor.h

#ifndef __THREAD_EXECUTOR_H__
#define __THREAD_EXECUTOR_H__

#include <pthread.h>

#include "common.h"
#include "executor.h"
#include "sync.h"

class ThreadExecutor : public Executor
{
public:
    ThreadExecutor();
    virtual ~ThreadExecutor();
    virtual bool isAlive() { return threadRunning; };
    virtual bool kill();
    virtual int getStatus() = 0;
protected:
    bool threadShutdown;
    zmm::Ref<Cond> cond;
    zmm::Ref<Mutex> mutex;
    virtual void threadProc() = 0;
    void startThread();
    bool threadShutdownCheck() { return threadShutdown; };
    
private:
    pthread_t thread;
    bool threadRunning;
    static void *staticThreadProc(void *arg);
};

#endif // __THREAD_EXECUTOR_H__
