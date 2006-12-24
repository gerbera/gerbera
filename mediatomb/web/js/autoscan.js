/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    autoscan.js - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
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

var autoscanAddId;
var autoscanFromFs;

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

function changeAutoscanDirectory(doAction, objId, fromFs)
{
    if (doAction == 'add')
    {
        autoscanAddId = objId;
        autoscanFromFs = fromFs;
        Element.hide(itemRoot);
        itemRoot = rightDocument.getElementById('autoscan_div');
        Element.show(itemRoot);
    }
    else
    {
        var url = link('autoscan', {action: doAction, object_id: objId}, true);
        var myAjax = new Ajax.Request(
            url,
            {
                method: 'get',
                onComplete: changeAutoscanCallback
            });
    }
}

function changeAutoscanCallback(ajaxRequest)
{
    if (errorCheck(ajaxRequest.responseXML))
        folderChange(selectedNode);
}

function autoscanSubmit()
{
    var form = rightDocument.forms['autoscanForm'];
    var args = new Object();
    args['action'] = 'add';
    args['object_id'] = autoscanAddId;
    args['fromFs'] = (autoscanFromFs ? '1' : '0');
    formToArray(form, args);
    var url = link('autoscan', args);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: changeAutoscanCallback
        });
}

