/*MT*
 */

#include "common.h"

#ifndef __GENERIC_TASK_H__
#define __GENERIC_TASK_H__

enum task_type_t {
    Invalid,
    AddFile,
    RemoveObject,
    LoadAccounting,
    RescanDirectory,
    FetchOnlineContent
};

enum task_owner_t {
    ContentManagerTask,
    TaskProcessorTask
};

class GenericTask {
protected:
    std::string description;
    task_type_t taskType { Invalid };
    task_owner_t taskOwner;
    unsigned int parentTaskID { 0 };
    unsigned int taskID { 0 };
    bool valid { true };
    bool cancellable { true };

public:
    explicit GenericTask(task_owner_t taskOwner);
    virtual void run() = 0;
    void setDescription(const std::string& description) { this->description = description; }
    std::string getDescription() const { return description; }
    task_type_t getType() const { return taskType; }
    unsigned int getID() const { return taskID; }
    unsigned int getParentID() const { return parentTaskID; }
    void setID(unsigned int taskID) { this->taskID = taskID; }
    void setParentID(unsigned int parentTaskID = 0) { this->parentTaskID = parentTaskID; }
    bool isValid() const { return valid; }
    bool isCancellable() const { return cancellable; }
    void invalidate() { valid = false; }
    task_owner_t getOwner() const { return taskOwner; }

    virtual ~GenericTask() = default;
};

#endif //__GENERIC_TASK_H__
