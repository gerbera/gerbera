/*MT*

    MediaTomb - http://www.mediatomb.cc/

    thread_executor.cc - this file is part of MediaTomb.

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

/// \file thread_executor.cc

#include "thread_executor.h" // API

#include "config/config.h"

ThreadExecutor::~ThreadExecutor()
{
    kill();
}

void ThreadExecutor::startThread()
{
    int ret = pthread_create(
        &thread,
        nullptr,
        ThreadExecutor::staticThreadProc,
        this);

    if (ret != 0) {
        log_error("Could not start thread: {}", std::strerror(errno));
    } else {
        threadRunning = true;
    }
}

bool ThreadExecutor::kill()
{
    if (!threadRunning)
        return true;

    std::unique_lock<std::mutex> lock(mutex);
    threadShutdown = true;
    cond.notify_one();
    lock.unlock();

    if (thread) {
        threadRunning = false;
        pthread_join(thread, nullptr);
        thread = 0;
    }
    return true;
}

void* ThreadExecutor::staticThreadProc(void* arg)
{
    auto inst = static_cast<ThreadExecutor*>(arg);
    inst->threadProc();
    pthread_exit(nullptr);
}

void* ThreadRunner::staticThreadProc(void* arg)
{
    auto inst = static_cast<ThreadRunner*>(arg);
    inst->targetProc(inst->target);
    pthread_exit(nullptr);
}

void ThreadRunner::startThread()
{
    if (config->getBoolOption(CFG_THREAD_SCOPE_SYSTEM)) {
        attr = new pthread_attr_t;
        pthread_attr_init(attr);
        // pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setscope(attr, PTHREAD_SCOPE_SYSTEM);
    }

    int ret = pthread_create(
        &thread,
        attr,
        ThreadRunner::staticThreadProc,
        this);

    if (ret != 0) {
        log_error("Could not start thread {}: {}", threadName.c_str(), std::strerror(errno));
    } else {
        threadRunning = true;
    }

    if (attr != nullptr) {
        pthread_attr_destroy(attr);
        delete attr;
        attr = nullptr;
    }
}

void ThreadRunner::join()
{
    log_debug("Waiting for thread {}", threadName.c_str());
    if (thread) {
        pthread_join(thread, nullptr);
    }

    threadRunning = false;
    thread = 0;
}
