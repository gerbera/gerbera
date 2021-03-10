/*MT*
*/

#ifndef __TASK_PROCESSOR_H__
#define __TASK_PROCESSOR_H__

#include <condition_variable>
#include <deque>
#include <memory>

#include "common.h"
#include "util/generic_task.h"

// forward declaration
class ContentManager;
class OnlineService;
class Layout;
class Timer;

class TaskProcessor {
public:
    void run();
    virtual ~TaskProcessor() = default;
    void shutdown();

    void addTask(const std::shared_ptr<GenericTask>& task);
    std::deque<std::shared_ptr<GenericTask>> getTasklist();
    std::shared_ptr<GenericTask> getCurrentTask();
    void invalidateTask(unsigned int taskID);

protected:
    pthread_t taskThread { 0 };
    std::condition_variable cond;
    std::mutex mutex;
    using AutoLock = std::lock_guard<decltype(mutex)>;
    using AutoLockU = std::unique_lock<decltype(mutex)>;

    bool shutdownFlag { false };
    bool working { false };
    unsigned int taskID { 1 };
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
    void run() override;

protected:
    std::shared_ptr<ContentManager> content;
    std::shared_ptr<TaskProcessor> task_processor;
    std::shared_ptr<Timer> timer;

    std::shared_ptr<OnlineService> service;
    std::shared_ptr<Layout> layout;
    bool unscheduled_refresh;
};

#endif //__TASK_PROCESSOR_H__
