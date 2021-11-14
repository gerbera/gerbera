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

#include "thread_executor.h"

using ThreadProc = void* (*)(void* target);

template <class Condition, class Mutex>
class ThreadRunner final {
public:
    ThreadRunner(std::string name, ThreadProc targetProc, void* target)
        : name(std::move(name))
    {
        log_debug("ThreadRunner: Creating {}", this->name);

        try {
            thread = std::thread(targetProc, target);
        } catch (const std::exception& err) {
            log_error("Could not start thread {}: {}", name, err.what());
        }
        isRunning = true;
    }

    ~ThreadRunner()
    {
        log_debug("ThreadRunner: Destroying {}", name);
        if (thread.joinable()) {
            thread.join();
        }
    }

    ThreadRunner(const ThreadRunner&) = delete;
    ThreadRunner& operator=(const ThreadRunner&) = delete;

    void join()
    {
        log_debug("ThreadRunner: Waiting for join {}", name);
        if (thread.joinable()) {
            thread.join();
        }
        isRunning = false;
        thread = {};
    }

    using AutoLock = std::lock_guard<Mutex>;
    using AutoLockU = std::unique_lock<Mutex>;

    void setReady()
    {
        log_debug("ThreadRunner: Setting {} ready", name);
        isReady = true;
        cond.notify_one();
    }

    bool isAlive()
    {
        return isRunning;
    }

    void wait(std::unique_lock<Mutex>& lck)
    {
        log_debug("ThreadRunner: Waiting for {}", name);
        cond.wait(lck);
    }

    void waitForReady()
    {
        auto lck = AutoLockU(mutex);
        log_debug("ThreadRunner: Waiting for {} to become ready", name);
        cond.wait(lck, [this] { return isReady; });
        lck.unlock();
        isReady = false;
    }

    template <class Predicate>
    void wait(std::unique_lock<Mutex>& lck, Predicate pred)
    {
        log_debug("ThreadRunner: Waiting for {}", name);
        cond.wait(lck, pred);
    }

    template <class Predicate>
    void wait(std::lock_guard<Mutex>& lck, Predicate pred)
    {
        log_debug("ThreadRunner: Waiting for {}", name);
        cond.wait(lck, pred);
    }

    std::cv_status waitFor(std::unique_lock<Mutex>& lck, std::chrono::milliseconds ms)
    {
        log_debug("ThreadRunner: Waiting for {}", name);
        return cond.wait_for(lck, std::chrono::milliseconds(ms));
    }

    void notify()
    {
        log_debug("ThreadRunner: Notifying {}", name);
        cond.notify_one();
    }

    void notifyAll()
    {
        log_debug("ThreadRunner: Notifying all {}", name);
        cond.notify_all();
    }

    AutoLock lockGuard([[maybe_unused]] const std::string& loc = "")
    {
        log_debug("ThreadRunner: Guard for {} - {}", name, loc);
        return AutoLock(mutex);
    }

    AutoLockU uniqueLockS([[maybe_unused]] const std::string& loc = "")
    {
        log_debug("ThreadRunner: Lock {} - {}", name, loc);
        return AutoLockU(mutex);
    }

    AutoLockU uniqueLock()
    {
        log_debug("ThreadRunner: Lock {}", name);
        return AutoLockU(mutex);
    }

    AutoLockU uniqueLock(std::defer_lock_t tag)
    {
        log_debug("ThreadRunner: Lock with tag {}", name);
        return AutoLockU(mutex, tag);
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

private:
    std::thread thread;
    std::string name;

    Condition cond;
    Mutex mutex;

    bool isRunning {};
    bool isReady {};
};

using StdThreadRunner = class ThreadRunner<std::condition_variable, std::mutex>;

#endif // __THREAD_RUNNER_H__
