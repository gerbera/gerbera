/*MT*
*/

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef ONLINE_SERVICES

#include "task_processor.h"
#include "layout/layout.h"
#include "content_manager.h"

#define TP_INITIAL_QUEUE_SIZE 4

using namespace zmm;

SINGLETON_MUTEX(TaskProcessor, false);

TaskProcessor::TaskProcessor() : Singleton<TaskProcessor>()
{
    cond = Ref<Cond>(new Cond(mutex));
    taskID = 1;
    working = false;
    shutdownFlag = false;

    taskQueue = Ref<ObjectQueue<GenericTask> >(new ObjectQueue<GenericTask>(TP_INITIAL_QUEUE_SIZE));
}

void TaskProcessor::init()
{
    int ret;

    ret = pthread_create(&taskThread, NULL, TaskProcessor::staticThreadProc,
          this);

    if (ret != 0)
    {
        throw _Exception(_("Could not launch task processor thread!"));
    }
}

void TaskProcessor::shutdown()
{
    log_debug("Shutting down TaskProcessor\n");
    shutdownFlag = true;
    cond->signal();
    if (taskThread)
        pthread_join(taskThread, NULL);
    taskThread = 0;
}

void *TaskProcessor::staticThreadProc(void *arg)
{
    TaskProcessor *inst = (TaskProcessor *)arg;
    inst->threadProc();
    pthread_exit(NULL);
    return NULL;
}

void TaskProcessor::threadProc()
{
    Ref<GenericTask> task;
    //Ref<TaskProcessor> this_ref(this);
    AUTOLOCK(mutex);
    working = true;

    while (!shutdownFlag)
    {
        currentTask = nil;
        if ((task = taskQueue->dequeue()) == nil)
        {
            working = false;
            cond->wait();
            working = true;
            continue;
        }
        else
        {
            currentTask = task;
        }
        AUTOUNLOCK();

        try
        {
            if (task->isValid())
                task->run();
        }
        catch (ServerShutdownException se)
        {
            shutdownFlag = true;
        }
        catch (Exception e)
        {
            log_error("Exception caught: %s\n", e.getMessage().c_str());
            e.printStackTrace();
        }

        if (!shutdownFlag)
        {
            AUTORELOCK();
        }
    }
}

void TaskProcessor::addTask(Ref<GenericTask> task)
{
    AUTOLOCK(mutex);

    task->setID(taskID++);

    taskQueue->enqueue(task);
    cond->signal();
}

Ref<GenericTask> TaskProcessor::getCurrentTask()
{
    Ref<GenericTask> task;
    AUTOLOCK(mutex);
    task = currentTask;
    return task;
}

void TaskProcessor::invalidateTask(unsigned int taskID)
{
    int i;

    AUTOLOCK(mutex);
    Ref<GenericTask> t = getCurrentTask();
    if (t != nil)
    {
        if ((t->getID() == taskID) || (t->getParentID() == taskID))
        {
            t->invalidate();
        }
    }

    int qsize = taskQueue->size();

    for (i = 0; i < qsize; i++)
    {
        Ref<GenericTask> t = taskQueue->get(i);
        if ((t->getID() == taskID) || (t->getParentID() == taskID))
        {
            t->invalidate();
        }
    }
}

Ref<Array<GenericTask> > TaskProcessor::getTasklist()
{
    int i;
    Ref<Array<GenericTask> > taskList = nil;

    AUTOLOCK(mutex);
    Ref<GenericTask> t = getCurrentTask();

    // if there is no current task, then the queues are empty
    // and we do not have to allocate the array
    if (t == nil)
        return nil;

    taskList = Ref<Array<GenericTask> >(new Array<GenericTask>());
    taskList->append(t);

    int qsize = taskQueue->size();

    for (i = 0; i < qsize; i++)
    {
        Ref<GenericTask> t = taskQueue->get(i);
        if (t->isValid())
            taskList->append(t);
    }

    return taskList;
}

TaskProcessor::~TaskProcessor()
{}

TPFetchOnlineContentTask::TPFetchOnlineContentTask(Ref<OnlineService> service,
                                                   Ref<Layout> layout,
                                                   bool cancellable,
                                                   bool unscheduled_refresh) : GenericTask(TaskProcessorTask)
{
    this->service = service;
    this->layout = layout;
    this->taskType = FetchOnlineContent;
    this->cancellable = cancellable;
    this->unscheduled_refresh = unscheduled_refresh;
}

void TPFetchOnlineContentTask::run()
{
    if (this->service == nil)
    {
        log_debug("No service specified\n");
        return;
    }

    try
    {
        //cm->_fetchOnlineContent(service, getParentID(), unscheduled_refresh);
        if (service->refreshServiceData(layout) && (isValid()))
        {
            log_debug("Scheduling another task for online service: %s\n",
                    service->getServiceName().c_str());

            if ((service->getRefreshInterval() > 0) || unscheduled_refresh)
            {
                Ref<GenericTask> t(new TPFetchOnlineContentTask(service, layout, cancellable, unscheduled_refresh));
                TaskProcessor::getInstance()->addTask(t);
            }
        }
        else
        {
            ContentManager::getInstance()->cleanupOnlineServiceObjects(service);
        }
    }
    catch (Exception ex)
    {
        log_error("%s\n", ex.getMessage().c_str());
    }
    service->decTaskCount();
    if (service->getTaskCount() == 0)
    {
        if ((service->getRefreshInterval() > 0) && !unscheduled_refresh)
        {
            Timer::getInstance()->addTimerSubscriber(AS_TIMER_SUBSCRIBER_SINGLETON_FROM_REF(ContentManager::getInstance()),
                    service->getRefreshInterval(),
                    service->getTimerParameter(), true);
        }
    }
    
}
#endif // ONLINE_SERVICES

