/*GRB*

    Gerbera - https://gerbera.io/

    thread_runner.h - this file is part of Gerbera.

    Copyright (C) 2021 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file thread_runner.h

#ifndef __THREAD_RUNNER_H__
#define __THREAD_RUNNER_H__

#include <condition_variable>
#include <mutex>
#include <pthread.h>

#include "config/config.h"
#include "thread_executor.h"

using ThreadProc = void* (*)(void* target);

template <class Condition, class Mutex>
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
        log_debug("ThreadRunner destroying {}", threadName.c_str());
        if (attr != nullptr) {
            pthread_attr_destroy(attr);
            delete attr;
        }
    }

    void join()
    {
        log_debug("Waiting for thread {}", threadName.c_str());
        if (thread) {
            pthread_join(thread, nullptr);
        }
        threadRunning = false;
        thread = 0;
    }

    using AutoLock = std::lock_guard<Mutex>;
    using AutoLockU = std::unique_lock<Mutex>;

    /// \brief the exit status of the thread - needs to be overridden
    int getStatus() override { return 0; };

    void wait(std::unique_lock<Mutex>& lck)
    {
        log_debug("ThreadRunner waiting for {}", threadName.c_str());
        cond.wait(lck);
    }
    template <class Predicate>
    void wait(std::unique_lock<Mutex>& lck, Predicate pred)
    {
        log_debug("ThreadRunner waiting for {}", threadName.c_str());
        cond.wait(lck, pred);
    }
    std::cv_status waitFor(std::unique_lock<Mutex>& lck, std::chrono::milliseconds ms)
    {
        log_debug("ThreadRunner waiting for {}", threadName.c_str());
        return cond.wait_for(lck, std::chrono::milliseconds(ms));
    }
    void notify()
    {
        log_debug("ThreadRunner notifying {}", threadName.c_str());
        cond.notify_one();
    }
    void notifyAll()
    {
        log_debug("ThreadRunner notifying all {}", threadName.c_str());
        cond.notify_all();
    }
    AutoLock lockGuard(const std::string& loc = "")
    {
        log_debug("ThreadRunner guard for {} - {}", threadName.c_str(), loc.c_str());
        return AutoLock(mutex);
    }
    AutoLockU uniqueLock()
    {
        log_debug("ThreadRunner lock {}", threadName.c_str());
        return AutoLockU(mutex);
    }
    AutoLockU uniqueLock(std::defer_lock_t tag)
    {
        log_debug("ThreadRunner lock with tag {}", threadName.c_str());
        return AutoLockU(mutex, tag);
    }

protected:
    std::shared_ptr<Config> config;

    /// \brief thread method is injected
    void threadProc() override
    {
        targetProc(target);
        log_debug("ThreadRunner terminating {}", threadName.c_str());
    };

    /// \brief start the thread
    void startThread() override
    {
#ifndef SOLARIS
        // default scoping on solaroid systems is in fact PTHREAD_SCOPE_SYSTEM
        // plus, setting PTHREAD_EXPLICIT_SCHED requires elevated priveleges
        if (config->getBoolOption(CFG_THREAD_SCOPE_SYSTEM)) {
            attr = new pthread_attr_t;
            pthread_attr_init(attr);
            // pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED);
            pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
            pthread_attr_setscope(attr, PTHREAD_SCOPE_SYSTEM);
        }
#endif

        int ret = pthread_create(
            &thread,
            attr,
            ThreadRunner::staticThreadProc,
            this);

        if (ret != 0) {
            log_error("Could not start thread {}: {}", threadName.c_str(), std::strerror(ret));
        } else {
            threadRunning = true;
        }

        if (attr != nullptr) {
            pthread_attr_destroy(attr);
            delete attr;
            attr = nullptr;
        }
    }

private:
    pthread_attr_t* attr;
    std::string threadName;
    ThreadProc targetProc;
    void* target;
    Condition cond;
    Mutex mutex;

    static void* staticThreadProc(void* arg)
    {
        auto inst = static_cast<ThreadRunner<Condition, Mutex>*>(arg);
        inst->targetProc(inst->target);
        pthread_exit(nullptr);
    }
};

using StdThreadRunner = class ThreadRunner<std::condition_variable, std::mutex>;

#endif // __THREAD_RUNNER_H__
