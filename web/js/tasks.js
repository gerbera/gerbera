/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    tasks.js - this file is part of MediaTomb.
    
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

/*
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
*/

var currentTaskID = -1;

var pollInterval;

// will be set by getConfigCallback() (auth.js)
var pollWhenIdle = false;
var pollIntervalTime = 2000;
var showTooltips = true;

function updateCurrentTask(taskEl)
{
    var taskID = -1;
    if (taskEl)
        taskID = taskEl.getAttribute('id');
    if (taskID != currentTaskID)
    {
        currentTaskID = taskID;
        var topDocument = frames["topF"].document;
        
        var currentTaskTdEl = topDocument.getElementById("currentTaskTd");
        if (taskID == -1)
        {
            if (! pollWhenIdle)
                clearPollInterval();
            Element.hide(currentTaskTdEl);
        }
        else
        {
            var currentTaskTxtEl = topDocument.getElementById("currentTaskText").firstChild;
            currentTaskTxtEl.replaceData(0, currentTaskTxtEl.length, taskEl.firstChild.data);
            var cancelCurrentTaskEl = topDocument.getElementById("cancelCurrentTask");
            var cancelCurrentTaskPlaceholderEl = topDocument.getElementById("cancelCurrentTaskPlaceholder");
            if (taskEl.getAttribute("cancellable") == "1")
            {
                Element.hide(cancelCurrentTaskPlaceholderEl);
                Element.show(cancelCurrentTaskEl);
            }
            else
            {
                Element.hide(cancelCurrentTaskEl);
                Element.show(cancelCurrentTaskPlaceholderEl);
            }
            Element.show(currentTaskTdEl);
            if (! pollWhenIdle) // will be started by getConfigCallback() (auth.js) otherwise
                startPollInterval();
        }
    }
}

function clearPollInterval()
{
    if (pollInterval)
    {
        window.clearInterval(pollInterval);
        pollInterval = false;
    }
}

function startPollInterval()
{
    if (! pollInterval)
        pollInterval = window.setInterval("getUpdates(false)", pollIntervalTime);
}

/*
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
    if (tasks.length <= 0)
        return;
    currentTaskTxtEl.insertData(0, tasks[0].firstChild.data);
}
*/

function cancelCurrentTask()
{
    if (currentTaskID != -1)
        cancelTask(currentTaskID);
}

function cancelTask(taskID)
{
    var url = link('tasks', {action: 'cancel', task_id: taskID}, false);
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
    if (! errorCheck(xml))
        return;
}
