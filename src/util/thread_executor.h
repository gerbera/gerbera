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

class Config;

/// \brief an executor which runs a thread
class ThreadExecutor : public Executor {
public:
    ~ThreadExecutor() override;
    bool isAlive() override { return threadRunning; }

    /// \brief kill the thread (pthread_join)
    /// \return always true - this function only returns after the thread has died
    bool kill() override;

    /// \brief the exit status of the thread - needs to be overridden
    int getStatus() override = 0;

protected:
    bool threadShutdown { false };
    /// \brief if the thread is currently running
    bool threadRunning;

    std::condition_variable cond;
    std::mutex mutex;
    pthread_t thread { 0 };

    /// \brief abstract thread method, which needs to be overridden
    virtual void threadProc() = 0;

    /// \brief start the thread
    virtual void startThread();

    /// \brief check if the thread should shutdown
    /// should be called by the threadProc in short intervals
    bool threadShutdownCheck() const { return threadShutdown; }

private:
    static void* staticThreadProc(void* arg);
};

typedef void* (*ThreadProc)(void* target);

class ThreadRunner : public ThreadExecutor {
public:
    ThreadRunner(const std::string name, ThreadProc targetProc, void* target, const std::shared_ptr<Config>& config)
        : config(config)
        , attr(nullptr)
        , threadName(name)
        , targetProc(targetProc)
        , target(target)
    {
        ThreadRunner::startThread();
    }

    ~ThreadRunner() override
    {
        log_debug("ThreadExecutor destroying {}", threadName.c_str());
        if (attr != nullptr) {
            pthread_attr_destroy(attr);
            delete attr;
        }
    }

    void join();
    /// \brief the exit status of the thread - needs to be overridden
    int getStatus() override { return 0; };

#ifdef TODO
    using AutoLock = std::lock_guard<decltype(mutex)>;
    using AutoLockU = std::unique_lock<decltype(mutex)>;
    void lock() { }
    void unlock() { }
    void wait() { }
    void notify() { cond.notify_one(); }
#endif
protected:
    std::shared_ptr<Config> config;

    /// \brief thread method is injected
    void threadProc() override
    {
        targetProc(target);
        log_debug("ThreadRunner terminating {}", threadName.c_str());
    };

    /// \brief start the thread
    void startThread() override;

private:
    pthread_attr_t* attr;
    std::string threadName;
    ThreadProc targetProc;
    void* target;

    static void* staticThreadProc(void* arg);
};

#endif // __THREAD_EXECUTOR_H__
