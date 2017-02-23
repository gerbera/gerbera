/*MT*
 */

#include "common.h"

#ifndef __GENERIC_TASK_H__
#define __GENERIC_TASK_H__

enum task_type_t
{
    Invalid,
    AddFile,
    RemoveObject,
    LoadAccounting,
    RescanDirectory,
    FetchOnlineContent
};

enum task_owner_t
{
    ContentManagerTask,
    TaskProcessorTask
};

class GenericTask : public zmm::Object
{
protected:
    zmm::String description;
    task_type_t taskType;
    task_owner_t taskOwner;
    unsigned int parentTaskID;
    unsigned int taskID;
    bool valid;
    bool cancellable;

public:
    GenericTask(task_owner_t taskOwner);
    virtual void run() = 0;
    inline void setDescription(zmm::String description) { this->description = description; };
    inline zmm::String getDescription() { return description; };
    inline task_type_t getType() { return taskType; };
    inline unsigned int getID() { return taskID; };
    inline unsigned int getParentID() { return parentTaskID; };
    inline void setID(unsigned int taskID) { this->taskID = taskID; };
    inline void setParentID(unsigned int parentTaskID = 0) { this->parentTaskID = parentTaskID; };
    inline bool isValid() { return valid; };
    inline bool isCancellable() { return cancellable; };
    inline void invalidate() { valid = false; };
    inline task_owner_t getOwner() { return taskOwner; };
};

#endif//__GENERIC_TASK_H__

