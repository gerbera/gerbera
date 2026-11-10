/*GRB*

    Gerbera - https://gerbera.io/

    cm_task.cc - this file is part of Gerbera.

    Copyright (C) 2023-2026 Gerbera Contributors

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

/// @file content/cm_task.cc
#define GRB_LOG_FAC GrbLogFacility::content

#include "cm_task.h" // API

#include "config/result/autoscan.h"
#include "content_manager.h"
#include "context.h"
#include "database/database.h"
#include "util/logger.h"

#ifdef ONLINE_SERVICES
#include "onlineservice/task_processor.h"
#endif

#include <exception>

/// @brief Complete the autoscan task count on every exit and persist its scan time on success.
template <typename Run, typename DescribeFailure>
static void runAutoscanTask(const std::shared_ptr<ContentManager>& content,
    const std::shared_ptr<AutoscanDirectory>& adir, const Run& run, const DescribeFailure& describeFailure)
{
    struct TaskCountCompletion {
        const std::shared_ptr<AutoscanDirectory>& directory;

        ~TaskCountCompletion()
        {
            if (directory)
                directory->decTaskCount();
        }
    };

    try {
        const TaskCountCompletion completion { adir };
        run();
    } catch (const std::exception& ex) {
        log_debug("{}: {}", describeFailure(), EX(ex));
        throw;
    } catch (...) {
        log_debug("{}", describeFailure());
        throw;
    }

    if (adir && adir->updateLMT()) {
        log_debug("Updating last_modified for autoscan directory {}", adir->getLocation().c_str());
        content->getContext()->getDatabase()->updateAutoscanDirectory(adir);
    }
}

CMAddFileTask::CMAddFileTask(std::shared_ptr<ContentManager> content,
    fs::directory_entry dirEnt, fs::path rootpath, AutoScanSetting asSetting, bool cancellable)
    : GenericTask(TaskOwner::ContentManagerTask)
    , content(std::move(content))
    , dirEnt(std::move(dirEnt))
    , rootpath(std::move(rootpath))
    , asSetting(std::move(asSetting))
{
    this->cancellable = cancellable;
    this->taskType = TaskType::AddFile;
    if (this->asSetting.adir)
        this->asSetting.adir->incTaskCount();
}

fs::path CMAddFileTask::getPath() const { return dirEnt.path(); }

fs::path CMAddFileTask::getRootPath() const { return rootpath; }

void CMAddFileTask::run()
{
    log_debug("running add file task with path {} recursive: {}", dirEnt.path().c_str(), asSetting.recursive);
    auto self = shared_from_this();
    const auto describeFailure = [&] {
        return fmt::format("Add file task failed for {}", dirEnt.path().c_str());
    };
    runAutoscanTask(content, asSetting.adir, [&] { content->_addFile(dirEnt, rootpath, asSetting, self); }, describeFailure);
}

CMRemoveObjectTask::CMRemoveObjectTask(std::shared_ptr<ContentManager> content, std::shared_ptr<AutoscanDirectory> adir,
    std::shared_ptr<CdsObject> object, fs::path path, bool rescanResource, bool all)
    : GenericTask(TaskOwner::ContentManagerTask)
    , content(std::move(content))
    , adir(std::move(adir))
    , object(std::move(object))
    , path(std::move(path))
    , all(all)
    , rescanResource(rescanResource)
{
    this->taskType = TaskType::RemoveObject;
    this->cancellable = false;
}

void CMRemoveObjectTask::run()
{
    content->_removeObject(adir, object, path, rescanResource, all);
}

CMRescanDirectoryTask::CMRescanDirectoryTask(std::shared_ptr<ContentManager> content,
    std::shared_ptr<AutoscanDirectory> adir, int containerId, bool cancellable, bool refreshExisting)
    : GenericTask(TaskOwner::ContentManagerTask)
    , content(std::move(content))
    , adir(std::move(adir))
    , containerID(containerId)
    , refreshExisting(refreshExisting)
{
    this->cancellable = cancellable;
    this->taskType = TaskType::RescanDirectory;
}

void CMRescanDirectoryTask::run()
{
    if (!adir)
        return;

    auto self = shared_from_this();
    const auto describeFailure = [&] {
        return fmt::format("Rescan task failed for {} (container {})", adir->getLocation().c_str(), containerID);
    };
    runAutoscanTask(content, adir, [&] { content->_rescanDirectory(adir, containerID, self, refreshExisting); }, describeFailure);
}

#ifdef ONLINE_SERVICES
CMFetchOnlineContentTask::CMFetchOnlineContentTask(std::shared_ptr<ContentManager> content,
    std::shared_ptr<TaskProcessor> taskProcessor, std::shared_ptr<Timer> timer,
    std::shared_ptr<OnlineService> service, std::shared_ptr<Layout> layout, bool cancellable, bool unscheduledRefresh)
    : GenericTask(TaskOwner::ContentManagerTask)
    , content(std::move(content))
    , task_processor(std::move(taskProcessor))
    , timer(std::move(timer))
    , service(std::move(service))
    , layout(std::move(layout))
    , unscheduled_refresh(unscheduledRefresh)
{
    this->cancellable = cancellable;
    this->taskType = TaskType::FetchOnlineContent;
}

void CMFetchOnlineContentTask::run()
{
    if (!this->service) {
        log_debug("Received invalid service!");
        return;
    }
    try {
        auto t = std::make_shared<TPFetchOnlineContentTask>(content, task_processor, timer, service, layout, cancellable, unscheduled_refresh);
        task_processor->addTask(std::move(t));
    } catch (const std::runtime_error& ex) {
        log_error("{}", EX(ex));
    }
}
#endif // ONLINE_SERVICES
