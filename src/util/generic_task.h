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
    task_type_t taskType;
    task_owner_t taskOwner;
    unsigned int parentTaskID;
    unsigned int taskID;
    bool valid;
    bool cancellable;

public:
    GenericTask(task_owner_t taskOwner);
    virtual void run() = 0;
    inline void setDescription(const std::string& description) { this->description = description; }
    inline std::string getDescription() const { return description; }
    inline task_type_t getType() const { return taskType; }
    inline unsigned int getID() const { return taskID; }
    inline unsigned int getParentID() const { return parentTaskID; }
    inline void setID(unsigned int taskID) { this->taskID = taskID; }
    inline void setParentID(unsigned int parentTaskID = 0) { this->parentTaskID = parentTaskID; }
    inline bool isValid() const { return valid; }
    inline bool isCancellable() const { return cancellable; }
    inline void invalidate() { valid = false; }
    inline task_owner_t getOwner() const { return taskOwner; }

    virtual ~GenericTask() = default;
};

#endif //__GENERIC_TASK_H__
