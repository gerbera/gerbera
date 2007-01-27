/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    tasks.js - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.org>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>,
                            Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

function updateCurrentTask()
{
    var url = link('tasks', {action: 'current'}, false);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: updateCurrentTasksCallback
        });
}

function updateCurrentTasksCallback(ajaxRequest)
{
    var xml = ajaxRequest.responseXML
    if (! errorCheck(xml)) return;
    var tasksXMLel = xmlGetElement(xml, "tasks");
    if (! tasksXMLel) return;
    var tasks = tasksXMLel.getElementsByTagName("task");
    
    var topDocument = frames["topF"].document;
    var currentTaskTxtEl = topDocument.getElementById("currentTask").firstChild;
    currentTaskTxtEl.deleteData(0, currentTaskTxtEl.length);
    if (tasks.length <= 0) return;
    currentTaskTxtEl.insertData(0, tasks[0].firstChild.data);
}

function cancelTask(taskID)
{
    var url = link('tasks', {action: 'cancel', taskID: taskID}, false);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: cancelTaskCallback
        });
}

function cancelTaskCallback(ajaxRequest)
{
    var xml = ajaxRequest.responseXML
    if (! errorCheck(xml)) return;
}
