/*MT*

    MediaTomb - http://www.mediatomb.cc/

    task_processor.h - this file is part of MediaTomb.

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

#ifndef __TASK_PROCESSOR_H__
#define __TASK_PROCESSOR_H__

#include "util/generic_task.h"
#include "util/thread_runner.h"

#include <deque>
#include <memory>

// forward declaration
class Config;
class Content;
class OnlineService;
class Layout;
class Timer;

class TaskProcessor {
public:
    explicit TaskProcessor(std::shared_ptr<Config> config);

    void run();
    void shutdown();

    void addTask(std::shared_ptr<GenericTask> task);
    std::deque<std::shared_ptr<GenericTask>> getTasklist() const;
    std::shared_ptr<GenericTask> getCurrentTask() const;
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
    TPFetchOnlineContentTask(std::shared_ptr<Content> content,
        std::shared_ptr<TaskProcessor> taskProcessor,
        std::shared_ptr<Timer> timer,
        std::shared_ptr<OnlineService> service,
        std::shared_ptr<Layout> layout, bool cancellable,
        bool unscheduledRefresh);
    void run() override;

protected:
    std::shared_ptr<Content> content;
    std::shared_ptr<TaskProcessor> task_processor;
    std::shared_ptr<Timer> timer;

    std::shared_ptr<OnlineService> service;
    std::shared_ptr<Layout> layout;
    bool unscheduled_refresh;
};

#endif //__TASK_PROCESSOR_H__
