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

using namespace zmm;
using namespace mxml;

void web::tasks::process()
{
    check_request();
    String action = param(_("action"));
    if (!string_ok(action))
        throw _Exception(_("web:tasks called with illegal action"));
    Ref<ContentManager> cm = ContentManager::getInstance();

    if (action == "list") {
        Ref<Element> tasksEl(new Element(_("tasks")));
        tasksEl->setArrayName(_("task"));
        root->appendElementChild(tasksEl); // inherited from WebRequestHandler
        Ref<Array<GenericTask>> taskList = cm->getTasklist();
        if (taskList == nullptr)
            return;
        int count = taskList->size();
        for (int i = 0; i < count; i++) {
            appendTask(tasksEl, taskList->get(i));
        }
    } else if (action == "cancel") {
        int taskID = intParam(_("task_id"));
        cm->invalidateTask(taskID);
    } else
        throw _Exception(_("web:tasks called with illegal action"));
}
