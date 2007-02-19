/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    autoscan.js - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

/* not completely finished...

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
    autoscanHTMLel.setAttribute("class", "itemsTable");
    for (var i = 0; i < autoscans.length; i++)
    {
        var autoscanHTMLrow = rightDocument.createElement("div");
        autoscanHTMLrow.setAttribute("class", (i % 2 == 0 ? "itemRowA" : "itemRowB"));
        
        var spaceCell = rightDocument.createElement("div");
        spaceCell.setAttribute("class", "spaceCell");
        
        var itemButtons = rightDocument.createElement("div");
        itemButtons.setAttribute("class", "itemButtons");
        
        var autoscanXMLel = autoscans[i];
        
        var asEntry = rightDocument.createElement("div");
        asEntry.setAttribute("class", "itemEntry");
        //var linkEl = rightDocument.createElement("a");
        //linkEl.setAttribute("href", "javascript:parent.selectNode('d"+autoscanXMLel.getAttribute("objectID")+"');");
        var textEl = rightDocument.createTextNode(xmlGetElementText(autoscanXMLel, "location"));
        asEntry.appendChild(textEl);
        //linkEl.appendChild(textEl);
        
        autoscanHTMLrow.appendChild(asEntry);
        autoscanHTMLrow.appendChild(spaceCell);
        autoscanHTMLrow.appendChild(itemButtons);
        
        _addLink(itemButtons, true, "javascript:parent.changeAutoscanDirectory('remove', '"+autoscanXMLel.getAttribute("objectID")+"', false);", "remove", iconRemoveThis);
        
        autoscanHTMLel.appendChild(autoscanHTMLrow);
    }
    
    Element.hide(itemRoot);
    itemRoot = rightDocument.getElementById('autoscan_list_div');
    itemRoot.replaceChild(autoscanHTMLel, itemRoot.firstChild);
    Element.show(itemRoot);
}
*/

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
    scanLevelChanged();
    Element.hide(itemRoot);
    itemRoot = rightDocument.getElementById('autoscan_div');
    Element.show(itemRoot);
}

function updateAutoscanEditFields(autoscan)
{
    autoscanId = xmlGetElementText(autoscan, 'object_id');
    autoscanFromFs = xmlGetElementText(autoscan, 'from_fs') == '1';
    var elements = rightDocument.forms['autoscanForm'].elements;
    var scan_level_checked = 'scan_level_' + xmlGetElementText(autoscan, 'scan_level');
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
    if (args['scan_level'] == 'none')
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
    var elements = rightDocument.forms['autoscanForm'].elements;
    if (elements['scan_level_none'].checked)
    {
        elements['recursive'].disabled = true;
        elements['hidden'].disabled = true;
        elements['interval'].disabled = true;
    }
    else
    {
        elements['recursive'].disabled = false;
        elements['hidden'].disabled = false;
        elements['interval'].disabled = false;
    }
}
