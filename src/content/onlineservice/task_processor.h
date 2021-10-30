/*MT*
 */

#ifndef __TASK_PROCESSOR_H__
#define __TASK_PROCESSOR_H__

#include <deque>
#include <memory>

#include "common.h"
#include "util/generic_task.h"
#include "util/thread_runner.h"

// forward declaration
class Config;
class ContentManager;
class OnlineService;
class Layout;
class Timer;

class TaskProcessor {
public:
    explicit TaskProcessor(std::shared_ptr<Config> config)
        : config(std::move(config))
    {
    }

    void run();
    void shutdown();

    void addTask(const std::shared_ptr<GenericTask>& task);
    std::deque<std::shared_ptr<GenericTask>> getTasklist();
    std::shared_ptr<GenericTask> getCurrentTask();
    void invalidateTask(unsigned int taskID);

protected:
    std::shared_ptr<Config> config;
    std::unique_ptr<StdThreadRunner> threadRunner;

    bool shutdownFlag {};
    bool working {};
    unsigned int taskID { 1 };
    std::deque<std::shared_ptr<GenericTask>> taskQueue;
    std::shared_ptr<GenericTask> currentTask;

    void threadProc();
};

class TPFetchOnlineContentTask : public GenericTask {
public:
    TPFetchOnlineContentTask(std::shared_ptr<ContentManager> content,
        std::shared_ptr<TaskProcessor> taskProcessor,
        std::shared_ptr<Timer> timer,
        std::shared_ptr<OnlineService> service,
        std::shared_ptr<Layout> layout, bool cancellable,
        bool unscheduledRefresh);
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
