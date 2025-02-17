/*MT*

    MediaTomb - http://www.mediatomb.cc/

    web/tasks.cc - this file is part of MediaTomb.

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

/// \file web/tasks.cc
#define GRB_LOG_FAC GrbLogFacility::web

#include "pages.h" // API

#include "content/content.h"
#include "exceptions.h"
#include "util/generic_task.h"
#include "util/xml_to_json.h"

const std::string Web::Tasks::PAGE = "tasks";

void Web::Tasks::processPageAction(pugi::xml_node& element)
{
    std::string action = param("action");
    if (action.empty())
        throw_std_runtime_error("called with illegal action");

    if (action == "list") {
        auto tasksEl = element.append_child("tasks");
        xml2Json->setArrayName(tasksEl, "tasks");
        auto taskList = content->getTasklist();
        for (auto&& task : taskList) {
            appendTask(task, tasksEl);
        }
    } else if (action == "cancel") {
        int taskID = intParam("task_id");
        content->invalidateTask(taskID, TaskOwner::ContentManagerTask);
    } else
        throw_std_runtime_error("tasks called with illegal action {}", action);
}
