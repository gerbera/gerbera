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

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <pthread.h>

#include "config/config.h"
#include "thread_executor.h"

using ThreadProc = void* (*)(void* target);

template <class Condition, class Mutex>
class ThreadRunner : public ThreadExecutor {
public:
    ThreadRunner(std::string name, ThreadProc targetProc, void* target, std::shared_ptr<Config> config)
        : config(std::move(config))
        , threadName(std::move(name))
        , targetProc(targetProc)
        , target(target)
    {
        log_debug("ThreadRunner: Creating {}", this->threadName);
        try {
            ThreadRunner::startThread();
        } catch (const std::runtime_error& ex) {
            log_error("{}", ex.what());
        }
    }

    ~ThreadRunner() override
    {
        log_debug("ThreadRunner: Destroying {}", threadName);
        if (attr) {
            pthread_attr_destroy(attr);
            delete attr;
        }
    }

    ThreadRunner(const ThreadRunner&) = delete;
    ThreadRunner& operator=(const ThreadRunner&) = delete;

    void join()
    {
        log_debug("ThreadRunner: Waiting for join {}", threadName);
        if (thread) {
            pthread_join(thread, nullptr);
        }
        threadRunning = false;
        thread = 0;
    }

    /// \brief the exit status of the thread - needs to be overridden
    int getStatus() override { return 0; }

    void setReady()
    {
        log_debug("ThreadRunner: Setting {} ready", threadName);
        isReady = true;
        cond.notify_one();
    }

    void wait(std::unique_lock<Mutex>& lck)
    {
        log_debug("ThreadRunner: Waiting for {}", threadName);
        cond.wait(lck);
    }
    void waitForReady()
    {
        auto lck = std::unique_lock(mutex);
        log_debug("ThreadRunner: Waiting for {} to become ready", threadName);
        cond.wait(lck, [this] { return isReady; });
        lck.unlock();
        isReady = false;
    }
    template <class Predicate>
    void wait(std::unique_lock<Mutex>& lck, Predicate pred)
    {
        log_debug("ThreadRunner: Waiting for {}", threadName);
        cond.wait(lck, pred);
    }
    template <class Predicate>
    void wait(std::scoped_lock<Mutex>& lck, Predicate pred)
    {
        log_debug("ThreadRunner: Waiting for {}", threadName);
        cond.wait(lck, pred);
    }
    std::cv_status waitFor(std::unique_lock<Mutex>& lck, std::chrono::milliseconds ms)
    {
        log_debug("ThreadRunner: Waiting for {}", threadName);
        return cond.wait_for(lck, std::chrono::milliseconds(ms));
    }
    void notify()
    {
        log_debug("ThreadRunner: Notifying {}", threadName);
        cond.notify_one();
    }
    void notifyAll()
    {
        log_debug("ThreadRunner: Notifying all {}", threadName);
        cond.notify_all();
    }
    auto lockGuard([[maybe_unused]] const std::string& loc = "")
    {
        log_debug("ThreadRunner: Guard for {} - {}", threadName, loc);
        return std::scoped_lock(mutex);
    }
    auto uniqueLockS([[maybe_unused]] const std::string& loc = "")
    {
        log_debug("ThreadRunner: Lock {} - {}", threadName, loc);
        return std::unique_lock(mutex);
    }
    auto uniqueLock()
    {
        log_debug("ThreadRunner: Lock {}", threadName);
        return std::unique_lock(mutex);
    }
    auto uniqueLock(std::defer_lock_t tag)
    {
        log_debug("ThreadRunner: Lock with tag {}", threadName);
        return std::unique_lock(mutex, tag);
    }
    template <class Predicate>
    static void waitFor(const std::string_view& threadName, Predicate pred, int max_count = 10)
    {
        int count = 0;
        while (!(pred()) && count < max_count) {
            log_debug("ThreadRunner: wait for pred {}", threadName);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            count++;
        }
        if (count >= max_count) {
            log_error("ThreadRunner: broke lock for {}", threadName);
        }
    }

protected:
    std::shared_ptr<Config> config;

    /// \brief thread method is injected
    void threadProc() override
    {
        targetProc(target);
        log_debug("ThreadRunner: Terminating {}", threadName);
    }

    /// \brief start the thread
    void startThread() override
    {
#if !defined(SOLARIS) && !defined(__HAIKU__)
        // default scoping on Solaroid systems is in fact PTHREAD_SCOPE_SYSTEM
        // plus, setting PTHREAD_EXPLICIT_SCHED requires elevated privileges
        // while Haiku doesn't implement pthread_attr_setinheritsched yet
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
            [](void* arg) -> void* {
                auto inst = static_cast<ThreadRunner<Condition, Mutex>*>(arg);
                inst->targetProc(inst->target);
                pthread_exit(nullptr);
            },
            this);

        if (ret != 0) {
            log_error("Could not start thread {}: {}", threadName, std::strerror(ret));
        } else {
            threadRunning = true;
        }
    }

private:
    pthread_attr_t* attr {};
    std::string threadName;
    ThreadProc targetProc;
    void* target;
    Condition cond;
    Mutex mutex;
    bool isReady {};
};

using StdThreadRunner = class ThreadRunner<std::condition_variable, std::mutex>;

#endif // __THREAD_RUNNER_H__
