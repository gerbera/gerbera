/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    autoscan.js - this file is part of MediaTomb.
    
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

var autoscanId;
var autoscanFromFs;
var autoscanPersistent;

function showAutoscanDirs()
{
    setType('db');
    var url = link('autoscan', {action: 'list'}, true);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: showAutoscanCallback
        });
}

function showAutoscanCallback(ajaxRequest)
{
    var xml = ajaxRequest.responseXML
    if (! errorCheck(xml)) return;
    var autoscansXMLel = xmlGetElement(xml, "autoscans");
    if (! autoscansXMLel) return;
    var autoscans = autoscansXMLel.getElementsByTagName("autoscan");
    if (autoscans.length <= 0) return;
    
    var autoscanHTMLel = rightDocument.createElement("div");
    autoscanHTMLel.setAttribute("class", "itemsEl");
    var itemsTable = rightDocument.createElement("table");
    itemsTable.setAttribute("width", "100%");
    itemsTable.setAttribute("cellspacing", "0");
    var itemsTableBody = rightDocument.createElement("tbody");
    itemsTable.appendChild(itemsTableBody);
    autoscanHTMLel.appendChild(itemsTable);
    
    for (var i = 0; i < autoscans.length; i++)
    {
        var itemRow = rightDocument.createElement("tr");
        itemRow.setAttribute("class", (i % 2 == 0 ? "itemRowA" : "itemRowB"));
        
        var itemEntryTd = rightDocument.createElement("td");
        itemEntryTd.setAttribute("class", "itemEntry");
        
        var itemEntry;
        if (isMSIE)
        {
            itemEntry = rightDocument.createElement("div"); // another div only for IE...
            itemEntry.setAttribute("class", "itemLeft");
            itemEntryTd.appendChild(itemEntry);
        }
        else
        {
            itemEntry = itemEntryTd;
        }
        var autoscanXMLel = autoscans[i];
        
        var autoscanMode = xmlGetElementText(autoscanXMLel, "scan_mode");
        var autoscanFromConfig = xmlGetElementText(autoscanXMLel, "from_config") == "1";
        
        var icon;
        var altText;
        if (autoscanMode == "timed")
            appendImgNode(rightDocument, itemEntry, "Timed-Autoscan:", (autoscanFromConfig ? iconContainerAutoscanTimedConfig : iconContainerAutoscanTimed));
        else if (autoscanMode == "inotify")
            appendImgNode(rightDocument, itemEntry, "Inotify-Autoscan:", (autoscanFromConfig ? iconContainerAutoscanInotifyConfig : iconContainerAutoscanInotify));
        
        var itemText = rightDocument.createTextNode(" " + xmlGetElementText(autoscanXMLel, "location"));
        itemEntry.appendChild(itemText);
        
        var itemButtonsTd = rightDocument.createElement("td");
        itemButtonsTd.setAttribute("class", "itemButtons");
        itemButtonsTd.setAttribute("align", "right");
        var itemButtons;
        if (isMSIE)
        {
            itemButtons = rightDocument.createElement("div"); // another div only for IE...
            itemButtons.setAttribute("class", "itemRight");
            itemButtonsTd.appendChild(itemButtons);
        }
        else
        {
            itemButtons = itemButtonsTd;
        }
        
        itemRow.appendChild(itemEntryTd);
        itemRow.appendChild(itemButtonsTd);
        
        _addLink(rightDocument, itemButtons, true, "javascript:parent.editLoadAutoscanDirectory('"+autoscanXMLel.getAttribute("objectID")+"', false);", "edit", iconEditAutoscan);
        
        itemsTableBody.appendChild(itemRow);
    }
    
    Element.hide(itemRoot);
    itemRoot = rightDocument.getElementById('autoscan_list_div');
    itemRoot.replaceChild(autoscanHTMLel, itemRoot.firstChild);
    Element.show(itemRoot);
}

function editLoadAutoscanDirectory(objectId, fromFs)
{
    var url = link("autoscan", {action: 'as_edit_load', object_id: objectId, from_fs: fromFs}, true);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: editLoadAutoscanDirectoryCallback
        });
}

function editLoadAutoscanDirectoryCallback(ajaxRequest)
{
    var xml = ajaxRequest.responseXML;
    if (!errorCheck(xml)) return;
    var autoscan = xmlGetElement(xml, "autoscan");
    
    updateAutoscanEditFields(autoscan);
    scanModeChanged(true);
    
    if (autoscanPersistent)
    {
        Element.show(frames["rightF"].document.getElementById("autoscan_persistent_message"));
        Element.hide(frames["rightF"].document.getElementById("autoscan_set_button"));
    }
    else
    {
        Element.hide(frames["rightF"].document.getElementById("autoscan_persistent_message"));
        Element.show(frames["rightF"].document.getElementById("autoscan_set_button"));
    }
    
    Element.hide(itemRoot);
    itemRoot = rightDocument.getElementById('autoscan_div');
    Element.show(itemRoot);
}

function updateAutoscanEditFields(autoscan)
{
    autoscanId = xmlGetElementText(autoscan, 'object_id');
    autoscanFromFs = xmlGetElementText(autoscan, 'from_fs') == '1';
    var elements = rightDocument.forms['autoscanForm'].elements;
    var scan_mode_checked = 'scan_mode_' + xmlGetElementText(autoscan, 'scan_mode');
    var scan_level_checked = 'scan_level_' + xmlGetElementText(autoscan, 'scan_level');
    var persistent = xmlGetElementText(autoscan, 'persistent');
    if (persistent == '1')
        autoscanPersistent = true;
    else
        autoscanPersistent = false;
    elements[scan_mode_checked].checked = true;
    elements[scan_level_checked].checked = true;
    elements['recursive'].checked = xmlGetElementText(autoscan, 'recursive') == '1';
    elements['hidden'].checked = xmlGetElementText(autoscan, 'hidden') == '1';
    elements['interval'].value = xmlGetElementText(autoscan, 'interval');
}

function autoscanSubmit()
{
    itemChangeType();
    var form = rightDocument.forms['autoscanForm'];
    var args = new Object();
    args['action'] = 'as_edit_save';
    args['object_id'] = autoscanId;
    args['from_fs'] = (autoscanFromFs ? '1' : '0');
    formToArray(form, args);
    if (args['scan_mode'] == 'none')
        use_inactivity_timeout_short = true;
    var url = link('autoscan', args);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: autoscanSubmitCallback
        });
}

function autoscanSubmitCallback(ajaxRequest)
{
    if (errorCheck(ajaxRequest.responseXML))
        folderChange(selectedNode);
}

function scanLevelChanged()
{
    
}

function scanModeChanged(filled)
{
    var elements = rightDocument.forms['autoscanForm'].elements;
    if (autoscanPersistent)
    {
        elements['scan_mode_none'].disabled = true;
        elements['scan_mode_timed'].disabled = true;
        elements['scan_mode_inotify'].disabled = true;
    }
    else
    {
        elements['scan_mode_none'].disabled = false;
        elements['scan_mode_timed'].disabled = false;
        elements['scan_mode_inotify'].disabled = false;
    }
    
    var scan_level_text = rightDocument.getElementById("scan_level_text");
    if (autoscanPersistent || elements['scan_mode_none'].checked)
    {
        elements['scan_level_basic'].disabled = true;
        elements['scan_level_full'].disabled = true;
        elements['recursive'].disabled = true;
        elements['hidden'].disabled = true;
        elements['interval'].disabled = true;
        scan_level_text.replaceChild(rightDocument.createTextNode("Scan Level:"), scan_level_text.firstChild);
    }
    else
    {
        elements['scan_level_basic'].disabled = false;
        elements['scan_level_full'].disabled = false;
        var scan_interval_row = rightDocument.getElementById("scan_interval_row");
        if (elements['scan_mode_inotify'].checked)
        {
            scan_level_text.replaceChild(rightDocument.createTextNode("Initial Scan:"), scan_level_text.firstChild);
            if (! filled)
                elements['scan_level_basic'].checked = true;
            Element.hide(scan_interval_row);
        }
        else
        {
            scan_level_text.replaceChild(rightDocument.createTextNode("Scan Level:"), scan_level_text.firstChild);
            if (! filled)
                elements['scan_level_full'].checked = true;
            Element.show(scan_interval_row);
        }
        
        elements['recursive'].disabled = false;
        elements['hidden'].disabled = false;
        if (elements['scan_mode_timed'].checked)
        {
            elements['interval'].disabled = false;
            if (elements['interval'].value == '0')
                elements['interval'].value = '1800';
        }
        else
            elements['interval'].disabled = true;
    }
}
