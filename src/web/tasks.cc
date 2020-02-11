/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    tasks.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
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

/// \file tasks.cc

#include "common.h"
#include "content_manager.h"
#include "pages.h"

web::tasks::tasks(std::shared_ptr<ConfigManager> config, std::shared_ptr<Storage> storage,
    std::shared_ptr<ContentManager> content, std::shared_ptr<SessionManager> sessionManager)
    : WebRequestHandler(config, storage, content, sessionManager)
{
}

void web::tasks::process()
{
    check_request();
    std::string action = param("action");
    if (!string_ok(action))
        throw std::runtime_error("web:tasks called with illegal action");

    auto root = xmlDoc->document_element();

    if (action == "list") {
        auto tasksEl = root.append_child("tasks");
        xml2JsonHints->setArrayName(tasksEl, "tasks");
        auto taskList = content->getTasklist();
        for (const auto& i : taskList) {
            appendTask(i, &tasksEl);
        }
    } else if (action == "cancel") {
        int taskID = intParam("task_id");
        content->invalidateTask(taskID);
    } else
        throw std::runtime_error("web:tasks called with illegal action");
}
