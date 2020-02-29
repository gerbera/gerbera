/*MT*
*/

#ifdef ONLINE_SERVICES

#include "task_processor.h"

#include "content_manager.h"
#include "layout/layout.h"
#include <utility>

TaskProcessor::TaskProcessor()
    : taskThread(0)
    , shutdownFlag(false)
    , working(false)
    , taskID(1)
{
}

void TaskProcessor::run()
{
    int ret;
    ret = pthread_create(&taskThread, nullptr, TaskProcessor::staticThreadProc,
        this);

    if (ret != 0) {
        throw std::runtime_error("Could not launch task processor thread!");
    }
}

void TaskProcessor::shutdown()
{
    log_debug("Shutting down TaskProcessor");
    shutdownFlag = true;
    cond.notify_one();
    if (taskThread)
        pthread_join(taskThread, nullptr);
    taskThread = 0;
}

void* TaskProcessor::staticThreadProc(void* arg)
{
    auto inst = static_cast<TaskProcessor*>(arg);
    inst->threadProc();
    pthread_exit(nullptr);
}

void TaskProcessor::threadProc()
{
    std::shared_ptr<GenericTask> task;
    AutoLockU lock(mutex);
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
            cond.wait(lock);
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
    AutoLock lock(mutex);

    task->setID(taskID++);

    taskQueue.push_back(task);
    cond.notify_one();
}

std::shared_ptr<GenericTask> TaskProcessor::getCurrentTask()
{
    std::shared_ptr<GenericTask> task;
    AutoLock lock(mutex);
    task = currentTask;
    return task;
}

void TaskProcessor::invalidateTask(unsigned int taskID)
{
    AutoLock lock(mutex);
    auto t = getCurrentTask();
    if (t != nullptr) {
        if ((t->getID() == taskID) || (t->getParentID() == taskID)) {
            t->invalidate();
        }
    }

    for (const auto& t : taskQueue) {
        if ((t->getID() == taskID) || (t->getParentID() == taskID)) {
            t->invalidate();
        }
    }
}

std::deque<std::shared_ptr<GenericTask>> TaskProcessor::getTasklist()
{
    std::deque<std::shared_ptr<GenericTask>> taskList;

    AutoLock lock(mutex);
    auto t = getCurrentTask();

    // if there is no current task, then the queues are empty
    // and we do not have to allocate the array
    if (t == nullptr)
        return taskList;

    taskList.push_back(t);

    for (const auto& t : taskQueue) {
        if (t->isValid())
            taskList.push_back(t);
    }

    return taskList;
}

TaskProcessor::~TaskProcessor() = default;

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
{
    this->cancellable = cancellable;
    this->unscheduled_refresh = unscheduled_refresh;
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

            if ((service->getRefreshInterval() > 0) || unscheduled_refresh) {
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
        if ((service->getRefreshInterval() > 0) && !unscheduled_refresh) {
            timer->addTimerSubscriber(
                content.get(),
                service->getRefreshInterval(),
                service->getTimerParameter(), true);
        }
    }
}
#endif // ONLINE_SERVICES
