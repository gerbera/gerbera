/*MT*
*/

#ifndef __TASK_PROCESSOR_H__
#define __TASK_PROCESSOR_H__

#include "common.h"
#include "generic_task.h"
#include "online_service.h"
#include "singleton.h"
#include <condition_variable>

class TPFetchOnlineContentTask : public GenericTask {
public:
    TPFetchOnlineContentTask(zmm::Ref<OnlineService> service,
        zmm::Ref<Layout> layout, bool cancellable,
        bool unscheduled_refresh);
    virtual void run();

protected:
    zmm::Ref<OnlineService> service;
    zmm::Ref<Layout> layout;
    bool unscheduled_refresh;
};

class TaskProcessor : public Singleton<TaskProcessor> {
public:
    TaskProcessor();
    virtual ~TaskProcessor();
    void addTask(zmm::Ref<GenericTask> task);
    virtual void init() override;
    virtual void shutdown() override;
    zmm::Ref<zmm::Array<GenericTask>> getTasklist();
    zmm::Ref<GenericTask> getCurrentTask();
    void invalidateTask(unsigned int taskID);
    zmm::String getName() override { return _("Task Processor"); }

protected:
    pthread_t taskThread;
    std::condition_variable cond;
    bool shutdownFlag;
    bool working;
    unsigned int taskID;
    zmm::Ref<zmm::ObjectQueue<GenericTask>> taskQueue;
    zmm::Ref<GenericTask> currentTask;

    static void* staticThreadProc(void* arg);

    void threadProc();
};

#endif //__TASK_PROCESSOR_H__
