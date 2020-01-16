/*MT*
*/

#ifndef __TASK_PROCESSOR_H__
#define __TASK_PROCESSOR_H__

#include <memory>
#include <deque>

#include "common.h"
#include "generic_task.h"
#include "onlineservice/online_service.h"
#include <condition_variable>

// forward declaration
class ContentManager;

class TaskProcessor {
public:
    TaskProcessor();
    void init();
    virtual ~TaskProcessor();
    void shutdown();

    void addTask(std::shared_ptr<GenericTask> task);
    std::deque<std::shared_ptr<GenericTask>> getTasklist();
    std::shared_ptr<GenericTask> getCurrentTask();
    void invalidateTask(unsigned int taskID);

protected:
    pthread_t taskThread;
    std::condition_variable cond;
    std::mutex mutex;
    using AutoLock = std::lock_guard<decltype(mutex)>;
    using AutoLockU = std::unique_lock<decltype(mutex)>;

    bool shutdownFlag;
    bool working;
    unsigned int taskID;
    std::deque<std::shared_ptr<GenericTask>> taskQueue;
    std::shared_ptr<GenericTask> currentTask;

    static void* staticThreadProc(void* arg);

    void threadProc();
};

class TPFetchOnlineContentTask : public GenericTask {
public:
    TPFetchOnlineContentTask(std::shared_ptr<ContentManager> content,
        std::shared_ptr<TaskProcessor> task_processor,
        std::shared_ptr<Timer> timer,
        std::shared_ptr<OnlineService> service,
        std::shared_ptr<Layout> layout, bool cancellable,
        bool unscheduled_refresh);
    virtual void run();

protected:
    std::shared_ptr<ContentManager> content;
    std::shared_ptr<TaskProcessor> task_processor;
    std::shared_ptr<Timer> timer;

    std::shared_ptr<OnlineService> service;
    std::shared_ptr<Layout> layout;
    bool unscheduled_refresh;
};

#endif //__TASK_PROCESSOR_H__
