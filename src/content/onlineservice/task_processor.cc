/*MT*

    MediaTomb - http://www.mediatomb.cc/

    task_processor.cc - this file is part of MediaTomb.

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

/// \file task_processor.cc

#ifdef ONLINE_SERVICES
#include "task_processor.h" // API

#include "content/content_manager.h"
#include "content/layout/layout.h"

void TaskProcessor::run()
{
    threadRunner = std::make_unique<StdThreadRunner>("TaskProcessorThread", TaskProcessor::staticThreadProc, this, config);

    // wait for thread to become ready
    threadRunner->waitForReady();

    if (!threadRunner->isAlive())
        throw_std_runtime_error("Failed to task processor thread");
}

void TaskProcessor::shutdown()
{
    log_debug("Shutting down TaskProcessor");
    shutdownFlag = true;
    threadRunner->notify();
    threadRunner->join();
}

void* TaskProcessor::staticThreadProc(void* arg)
{
    auto inst = static_cast<TaskProcessor*>(arg);
    inst->threadProc();
    return nullptr;
}

void TaskProcessor::threadProc()
{
    StdThreadRunner::waitFor("TaskProcessor", [this] { return threadRunner != nullptr; });

    std::shared_ptr<GenericTask> task;
    auto lock = threadRunner->uniqueLockS("threadProc");
    // tell run() that we are ready
    threadRunner->setReady();
    working = true;

    while (!shutdownFlag) {
        currentTask = nullptr;

        task = nullptr;
        if (!taskQueue.empty()) {
            task = taskQueue.front();
            taskQueue.pop_front();
        }

        if (task == nullptr) {
            working = false;
            threadRunner->wait(lock);
            working = true;
            continue;
        }

        currentTask = task;
        lock.unlock();

        try {
            if (task->isValid())
                task->run();
        } catch (const ServerShutdownException& se) {
            shutdownFlag = true;
        } catch (const std::runtime_error& e) {
            log_error("Exception caught: {}", e.what());
        }

        if (!shutdownFlag) {
            lock.lock();
        }
    }
}

void TaskProcessor::addTask(const std::shared_ptr<GenericTask>& task)
{
    auto lock = threadRunner->lockGuard();

    task->setID(taskID++);

    taskQueue.push_back(task);
    threadRunner->notify();
}

std::shared_ptr<GenericTask> TaskProcessor::getCurrentTask()
{
    std::shared_ptr<GenericTask> task;
    auto lock = threadRunner->lockGuard();
    task = currentTask;
    return task;
}

void TaskProcessor::invalidateTask(unsigned int taskID)
{
    auto lock = threadRunner->lockGuard();
    auto tc = getCurrentTask();
    if (tc != nullptr) {
        if ((tc->getID() == taskID) || (tc->getParentID() == taskID)) {
            tc->invalidate();
        }
    }

    for (auto&& tq : taskQueue) {
        if ((tq->getID() == taskID) || (tq->getParentID() == taskID)) {
            tq->invalidate();
        }
    }
}

std::deque<std::shared_ptr<GenericTask>> TaskProcessor::getTasklist()
{
    std::deque<std::shared_ptr<GenericTask>> taskList;

    auto lock = threadRunner->lockGuard();
    auto tc = getCurrentTask();

    // if there is no current task, then the queues are empty
    // and we do not have to allocate the array
    if (tc == nullptr)
        return taskList;

    taskList.push_back(tc);

    std::copy_if(taskQueue.begin(), taskQueue.end(), std::back_inserter(taskList), [](auto&& task) { return task->isValid(); });

    return taskList;
}

TPFetchOnlineContentTask::TPFetchOnlineContentTask(std::shared_ptr<ContentManager> content,
    std::shared_ptr<TaskProcessor> task_processor,
    std::shared_ptr<Timer> timer,
    std::shared_ptr<OnlineService> service,
    std::shared_ptr<Layout> layout,
    bool cancellable,
    bool unscheduled_refresh)
    : GenericTask(TaskProcessorTask)
    , content(std::move(content))
    , task_processor(std::move(task_processor))
    , timer(std::move(timer))
    , service(std::move(service))
    , layout(std::move(layout))
    , unscheduled_refresh(unscheduled_refresh)
{
    this->cancellable = cancellable;
    this->taskType = FetchOnlineContent;
}

void TPFetchOnlineContentTask::run()
{
    if (this->service == nullptr) {
        log_debug("No service specified");
        return;
    }

    try {
        //cm->_fetchOnlineContent(service, getParentID(), unscheduled_refresh);
        if (service->refreshServiceData(layout) && (isValid())) {
            log_debug("Scheduling another task for online service: {}",
                service->getServiceName().c_str());

            if ((service->getRefreshInterval() > std::chrono::seconds::zero()) || unscheduled_refresh) {
                auto t = std::make_shared<TPFetchOnlineContentTask>(
                    content, task_processor, timer, service, layout, cancellable, unscheduled_refresh);
                task_processor->addTask(t);
            }
        } else {
            content->cleanupOnlineServiceObjects(service);
        }
    } catch (const std::runtime_error& ex) {
        log_error("{}", ex.what());
    }
    service->decTaskCount();
    if (service->getTaskCount() == 0) {
        if ((service->getRefreshInterval() > std::chrono::seconds::zero()) && !unscheduled_refresh) {
            timer->addTimerSubscriber(
                content.get(),
                service->getRefreshInterval(),
                service->getTimerParameter(), true);
        }
    }
}
#endif // ONLINE_SERVICES
