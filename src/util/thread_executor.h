/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    thread_executor.h - this file is part of MediaTomb.
    
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

/// \file thread_executor.h

#ifndef __THREAD_EXECUTOR_H__
#define __THREAD_EXECUTOR_H__

#include <condition_variable>
#include <mutex>
#include <pthread.h>

#include "common.h"
#include "executor.h"

/// \brief an executor which runs a thread
class ThreadExecutor : public Executor {
public:
    /// \brief initialize the mutex and the cond
    ThreadExecutor();
    virtual ~ThreadExecutor();
    virtual bool isAlive() { return threadRunning; };

    /// \brief kill the thread (pthread_join)
    /// \return always true - this function only returns after the thread has died
    virtual bool kill();

    /// \brief the exit status of the thread - needs to be overridden
    virtual int getStatus() = 0;

protected:
    bool threadShutdown;
    /// \brief if the thread is currently running
    bool threadRunning;

    std::condition_variable cond;
    std::mutex mutex;

    /// \brief abstract thread method, which needs to be overridden
    virtual void threadProc() = 0;

    /// \brief start the thread
    void startThread();

    /// \brief check if the thread should shutdown
    /// should be called by the threadProc in short intervals
    bool threadShutdownCheck() { return threadShutdown; };

private:
    pthread_t thread;

    static void* staticThreadProc(void* arg);
};

#endif // __THREAD_EXECUTOR_H__
