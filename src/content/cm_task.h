/*GRB*

    Gerbera - https://gerbera.io/

    cm_task.h - this file is part of Gerbera.

    Copyright (C) 2023-2024 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// \file cm_task.h
#ifndef __CM_TASK_H__
#define __CM_TASK_H__

#include "common.h"

#include "config/directory_tweak.h"
#include "util/generic_task.h"
#include "util/grb_fs.h"

class AutoDirectory;
class CdsObject;
class ContentManager;
class Layout;
class TaskProcessor;
class Timer;

#ifdef ONLINE_SERVICES
class OnlineService;
#endif // ONLINE_SERVICES

class CMAddFileTask : public GenericTask, public std::enable_shared_from_this<CMAddFileTask> {
protected:
    std::shared_ptr<ContentManager> content;
    fs::directory_entry dirEnt;
    fs::path rootpath;
    AutoScanSetting asSetting;

public:
    CMAddFileTask(std::shared_ptr<ContentManager> content,
        fs::directory_entry dirEnt, fs::path rootpath, AutoScanSetting asSetting, bool cancellable = true);
    fs::path getPath() const;
    fs::path getRootPath() const;
    void run() override;
};

class CMRemoveObjectTask : public GenericTask {
protected:
    std::shared_ptr<ContentManager> content;
    std::shared_ptr<AutoscanDirectory> adir;
    std::shared_ptr<CdsObject> object;
    fs::path path;
    bool all;
    bool rescanResource;

public:
    CMRemoveObjectTask(std::shared_ptr<ContentManager> content, std::shared_ptr<AutoscanDirectory> adir,
        std::shared_ptr<CdsObject> object, const fs::path& path, bool rescanResource, bool all);
    void run() override;
};

class CMRescanDirectoryTask : public GenericTask, public std::enable_shared_from_this<CMRescanDirectoryTask> {
protected:
    std::shared_ptr<ContentManager> content;
    std::shared_ptr<AutoscanDirectory> adir;
    int containerID;

public:
    CMRescanDirectoryTask(std::shared_ptr<ContentManager> content,
        std::shared_ptr<AutoscanDirectory> adir, int containerId, bool cancellable);
    void run() override;
};

#ifdef ONLINE_SERVICES
class CMFetchOnlineContentTask : public GenericTask {
protected:
    std::shared_ptr<ContentManager> content;
    std::shared_ptr<TaskProcessor> task_processor;
    std::shared_ptr<Timer> timer;
    std::shared_ptr<OnlineService> service;
    std::shared_ptr<Layout> layout;
    bool unscheduled_refresh;

public:
    CMFetchOnlineContentTask(std::shared_ptr<ContentManager> content,
        std::shared_ptr<TaskProcessor> taskProcessor,
        std::shared_ptr<Timer> timer,
        std::shared_ptr<OnlineService> service, std::shared_ptr<Layout> layout,
        bool cancellable, bool unscheduledRefresh);
    void run() override;
};
#endif

#endif
