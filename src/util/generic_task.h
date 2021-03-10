/*MT*
 */


#include <future>
#include "common.h"

#ifndef __GENERIC_TASK_H__
#define __GENERIC_TASK_H__

enum class TaskType {
    Invalid,
    AddFile,
    RemoveObject,
    RescanDirectory,
    FetchOnlineContent
};

enum class TaskOwner {
    ContentManager,
    TaskProcessor
};

class GenericTask {
protected:
    std::string description;
    TaskType taskType;
    TaskOwner taskOwner;
    unsigned int parentTaskID;
    unsigned int taskID;
    bool valid;
    bool cancellable;
    std::promise<void> promise;

public:
    GenericTask(TaskOwner taskOwner, std::promise<void>& promise);
    explicit GenericTask(TaskOwner taskOwner);

    virtual void run() = 0;
    void setDescription(const std::string& description) { this->description = description; }
    std::string getDescription() const { return description; }
    TaskType getType() const { return taskType; }
    unsigned int getID() const { return taskID; }
    unsigned int getParentID() const { return parentTaskID; }
    void setID(unsigned int taskID) { this->taskID = taskID; }
    void setParentID(unsigned int parentTaskID = 0) { this->parentTaskID = parentTaskID; }
    bool isValid() const { return valid; }
    bool isCancellable() const { return cancellable; }
    void invalidate() { valid = false; }
    TaskOwner getOwner() const { return taskOwner; }

    virtual ~GenericTask() = default;
};

#endif //__GENERIC_TASK_H__
