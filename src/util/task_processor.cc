/*MT*
*/

#ifdef ONLINE_SERVICES

#include "task_processor.h"
#include "content_manager.h"
#include "layout/layout.h"

#define TP_INITIAL_QUEUE_SIZE 4

using namespace zmm;
using namespace std;

TaskProcessor::TaskProcessor()
    : taskThread(0)
    , shutdownFlag(false)
    , working(false)
    , taskID(1)
{
    taskQueue = Ref<ObjectQueue<GenericTask>>(new ObjectQueue<GenericTask>(TP_INITIAL_QUEUE_SIZE));
}

void TaskProcessor::init()
{
    int ret;
    ret = pthread_create(&taskThread, nullptr, TaskProcessor::staticThreadProc,
        this);

    if (ret != 0) {
        throw _Exception("Could not launch task processor thread!");
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
    auto* inst = (TaskProcessor*)arg;
    inst->threadProc();
    pthread_exit(nullptr);
    return nullptr;
}

void TaskProcessor::threadProc()
{
    Ref<GenericTask> task;
    AutoLockU lock(mutex);
    working = true;

    while (!shutdownFlag) {
        currentTask = nullptr;
        if ((task = taskQueue->dequeue()) == nullptr) {
            working = false;
            cond.wait(lock);
            working = true;
            continue;
        } else {
            currentTask = task;
        }
        lock.unlock();

        try {
            if (task->isValid())
                task->run();
        } catch (const ServerShutdownException& se) {
            shutdownFlag = true;
        } catch (const Exception& e) {
            log_error("Exception caught: {}", e.getMessage().c_str());
        }

        if (!shutdownFlag) {
            lock.lock();
        }
    }
}

void TaskProcessor::addTask(Ref<GenericTask> task)
{
    AutoLock lock(mutex);

    task->setID(taskID++);

    taskQueue->enqueue(task);
    cond.notify_one();
}

Ref<GenericTask> TaskProcessor::getCurrentTask()
{
    Ref<GenericTask> task;
    AutoLock lock(mutex);
    task = currentTask;
    return task;
}

void TaskProcessor::invalidateTask(unsigned int taskID)
{
    int i;

    AutoLock lock(mutex);
    Ref<GenericTask> t = getCurrentTask();
    if (t != nullptr) {
        if ((t->getID() == taskID) || (t->getParentID() == taskID)) {
            t->invalidate();
        }
    }

    int qsize = taskQueue->size();

    for (i = 0; i < qsize; i++) {
        Ref<GenericTask> t = taskQueue->get(i);
        if ((t->getID() == taskID) || (t->getParentID() == taskID)) {
            t->invalidate();
        }
    }
}

Ref<Array<GenericTask>> TaskProcessor::getTasklist()
{
    int i;
    Ref<Array<GenericTask>> taskList = nullptr;

    AutoLock lock(mutex);
    Ref<GenericTask> t = getCurrentTask();

    // if there is no current task, then the queues are empty
    // and we do not have to allocate the array
    if (t == nullptr)
        return nullptr;

    taskList = Ref<Array<GenericTask>>(new Array<GenericTask>());
    taskList->append(t);

    int qsize = taskQueue->size();

    for (i = 0; i < qsize; i++) {
        Ref<GenericTask> t = taskQueue->get(i);
        if (t->isValid())
            taskList->append(t);
    }

    return taskList;
}

TaskProcessor::~TaskProcessor()
{
}

TPFetchOnlineContentTask::TPFetchOnlineContentTask(std::shared_ptr<ContentManager> content,
    std::shared_ptr<TaskProcessor> task_processor,
    std::shared_ptr<Timer> timer,
    Ref<OnlineService> service,
    Ref<Layout> layout,
    bool cancellable,
    bool unscheduled_refresh)
    : GenericTask(TaskProcessorTask)
    , content(content)
    , task_processor(task_processor)
    , timer(timer)
    , service(service)
    , layout(layout)
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
                Ref<GenericTask> t(
                    new TPFetchOnlineContentTask(content, task_processor, timer, service, layout, cancellable, unscheduled_refresh)
                );
                task_processor->addTask(t);
            }
        } else {
            content->cleanupOnlineServiceObjects(service);
        }
    } catch (const Exception& ex) {
        log_error("{}", ex.getMessage().c_str());
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
