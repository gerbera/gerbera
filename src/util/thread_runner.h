/*GRB*

    Gerbera - https://gerbera.io/

    thread_runner.h - this file is part of Gerbera.

    Copyright (C) 2021-2023 Gerbera Contributors

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

using ThreadProc = std::function<void(void* target)>;

template <class Condition, class Mutex>
class ThreadRunner : public ThreadExecutor {
public:
    ThreadRunner(std::string name, ThreadProc targetProc, void* target)
        : threadName(std::move(name))
        , targetProc(std::move(targetProc))
        , target(target)
    {
        log_threading("ThreadRunner: Creating {}", this->threadName);
        try {
            ThreadRunner::startThread();
        } catch (const std::runtime_error& ex) {
            log_error("{}", ex.what());
        }
    }

    ~ThreadRunner() override
    {
        log_threading("ThreadRunner: Destroying {}", threadName);
        if (attr) {
            pthread_attr_destroy(attr);
            delete attr;
        }
    }

    ThreadRunner(const ThreadRunner&) = delete;
    ThreadRunner& operator=(const ThreadRunner&) = delete;

    void join()
    {
        log_threading("ThreadRunner: Waiting for join {}", threadName);
        if (thread) {
            pthread_join(thread, nullptr);
        }
        threadRunning = false;
        thread = 0;
    }

    using AutoLock = std::scoped_lock<Mutex>;
    using AutoLockU = std::unique_lock<Mutex>;

    /// \brief the exit status of the thread - needs to be overridden
    int getStatus() override { return 0; }

    void setReady()
    {
        log_threading("ThreadRunner: Setting {} ready", threadName);
        isReady = true;
        cond.notify_one();
    }

    void wait(std::unique_lock<Mutex>& lck)
    {
        log_threading("ThreadRunner: Waiting for {}", threadName);
        cond.wait(lck);
    }
    void waitForReady()
    {
        auto lck = AutoLockU(mutex);
        log_threading("ThreadRunner: Waiting for {} to become ready", threadName);
        cond.wait(lck, [this] { return isReady; });
        lck.unlock();
        isReady = false;
    }
    template <class Predicate>
    void wait(std::unique_lock<Mutex>& lck, Predicate pred)
    {
        log_threading("ThreadRunner: Waiting for {}", threadName);
        cond.wait(lck, pred);
    }
    template <class Predicate>
    void wait(std::scoped_lock<Mutex>& lck, Predicate pred)
    {
        log_threading("ThreadRunner: Waiting for {}", threadName);
        cond.wait(lck, pred);
    }
    std::cv_status waitFor(std::unique_lock<Mutex>& lck, std::chrono::milliseconds ms)
    {
        log_threading("ThreadRunner: Waiting for {}", threadName);
        return cond.wait_for(lck, std::chrono::milliseconds(ms));
    }
    void notify()
    {
        log_threading("ThreadRunner: Notifying {}", threadName);
        cond.notify_one();
    }
    void notifyAll()
    {
        log_threading("ThreadRunner: Notifying all {}", threadName);
        cond.notify_all();
    }
    AutoLock lockGuard([[maybe_unused]] const std::string& loc = "")
    {
        log_threading("ThreadRunner: Guard for {} - {}", threadName, loc);
        return AutoLock(mutex);
    }
    AutoLockU uniqueLockS([[maybe_unused]] const std::string& loc = "")
    {
        log_threading("ThreadRunner: Lock {} - {}", threadName, loc);
        return AutoLockU(mutex);
    }
    AutoLockU uniqueLock()
    {
        log_threading("ThreadRunner: Lock {}", threadName);
        return AutoLockU(mutex);
    }
    AutoLockU uniqueLock(std::defer_lock_t tag)
    {
        log_threading("ThreadRunner: Lock with tag {}", threadName);
        return AutoLockU(mutex, tag);
    }
    template <class Predicate>
    static void waitFor(const std::string_view& threadName, Predicate pred, int max_count = 10)
    {
        int count = 0;
        while (!(pred()) && count < max_count) {
            log_threading("ThreadRunner: wait for pred {}", threadName);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            count++;
        }
        if (count >= max_count) {
            log_error("ThreadRunner: broke lock for {}", threadName);
        }
    }

protected:
    /// \brief thread method is injected
    void threadProc() override
    {
        targetProc(target);
        log_threading("ThreadRunner: Terminating {}", threadName);
    }

    /// \brief start the thread
    void startThread() override
    {
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
    mutable Mutex mutex;
    bool isReady {};
};

using StdThreadRunner = class ThreadRunner<std::condition_variable, std::mutex>;

#endif // __THREAD_RUNNER_H__
