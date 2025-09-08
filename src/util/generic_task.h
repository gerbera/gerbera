/*MT*

    MediaTomb - http://www.mediatomb.cc/

    generic_task.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

    $Id$
*/

/// @file util/generic_task.h

#ifndef __GENERIC_TASK_H__
#define __GENERIC_TASK_H__

#include <string>

/// @brief Type of tasks
enum class TaskType {
    Invalid,
    AddFile,
    RemoveObject,
    RescanDirectory,
    FetchOnlineContent
};

/// @brief Type of task owners
enum class TaskOwner {
    ContentManagerTask,
    TaskProcessorTask
};

/// @brief Abstract base class for tasks
class GenericTask {
protected:
    std::string description;
    TaskType taskType { TaskType::Invalid };
    TaskOwner taskOwner;
    unsigned int parentTaskID {};
    unsigned int taskID {};
    bool valid { true };
    bool cancellable { true };

public:
    explicit GenericTask(TaskOwner taskOwner);
    virtual ~GenericTask();
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
};

#endif //__GENERIC_TASK_H__
