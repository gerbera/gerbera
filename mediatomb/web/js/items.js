/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    items.js - this file is part of MediaTomb.
    
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

var itemRoot;
var topItemRoot
var rightDocument;
var topRightDocument;
var viewItems = 50;
var showMaxPages = 10;

function itemInit()
{
    rightDocument = frames["rightF"].document;
    topRightDocument = frames["toprightF"].document;
    itemChangeType();
}

function itemChangeType()
{
    var oldItemRoot = itemRoot;
    var dbItemRoot = rightDocument.getElementById("item_db_div");
    var fsItemRoot = rightDocument.getElementById("item_fs_div");
    
    itemRoot = isTypeDb() ? dbItemRoot : fsItemRoot;
    var dummy;
    if (oldItemRoot && oldItemRoot!=itemRoot) dummy = Element.hide(oldItemRoot);
    dummy = Element.show(itemRoot);
    
    var oldTopItemRoot = topItemRoot;
    var dbTopItemRoot = topRightDocument.getElementById("item_db_div");
    var fsTopItemRoot = topRightDocument.getElementById("item_fs_div");
    
    topItemRoot = isTypeDb() ? dbTopItemRoot : fsTopItemRoot;
    if (oldTopItemRoot && oldTopItemRoot!=topItemRoot) Element.hide(oldTopItemRoot);
    Element.show(topItemRoot);
}

function folderChange(id)
{
    loadItems(id, 0);
}

function loadItems(id, start, count)
{
    if (! count)
        count = viewItems;
    itemChangeType();
    var type = id.substring(0, 1);
    id = id.substring(1);
    var itemLink = type == 'd' ? 'items' : 'files';
    var url = link(itemLink, {parent_id: id, start: start, count: count}, true);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: updateItems
        });
}

function updateItems(ajaxRequest)
{
    var xml = ajaxRequest.responseXML;
    if (!errorCheck(xml)) return;
    
    var items = xmlGetElement(xml, "items");
    var useFiles = false;
    var childrenTag = "item";
    if (! items)
    {
        items = xmlGetElement(xml, "files");
        if (! items)
        {
            alert("no items or files tag found");
            return;
        }
        useFiles = true;
        childrenTag = "file";
    }
    
    var ofId = items.getAttribute("ofId");
    var success = items.getAttribute("success");
    if (! success || success != "1")
    {
        var prefix = (useFiles ? 'f' : 'd');
        var node = getTreeNode(prefix + ofId);
        var parNode = node.getParent();
        parNode.childrenHaveBeenFetched=false;
        parNode.resetChildren();
        fetchChildren(parNode, true, true);
        //selectNode(parNode.getID());
        //alert("no success!");
        return;
    }
    
    var isVirtual = (items.getAttribute("virtual") == '1');
    var autoscanType = items.getAttribute("autoscan");
    var path = items.getAttribute("location");
    var loadItemId = (useFiles ? 'f' : 'd') + ofId;
    var totalMatches = parseInt(items.getAttribute("totalMatches"));
    var totalPages = Math.ceil(totalMatches / viewItems);
    var start = parseInt(items.getAttribute("start"));
    var thisPage = Math.abs(start / viewItems);
    var nextPageStart = (thisPage + 1) * viewItems;
    var prevPageStart = (thisPage - 1) * viewItems;
    var pagesFrom = thisPage - showMaxPages / 2;
    if (pagesFrom < 0)
        pagesFrom = 0;
    var pagesTo = pagesFrom + showMaxPages - 1;
    if (pagesTo >= totalPages)
        pagesTo = totalPages - 1;
    // delete this:
    var pagingLink;
    var pagingPar = rightDocument.createElement("p");
    
    var first = true;
    
    /*
    if (thisPage > showMaxPages / 2)
        first = _addLink(pagingPar, first, "javascript:parent.loadItems('"+loadItemId+"','0');", "first", iconFirst, " ");
    */
    
    if (prevPageStart >= 0)
    {
        first = _addLink(rightDocument, pagingPar, first, "javascript:parent.loadItems('"+loadItemId+"','0');", "first", iconFirst, " ");
        first = _addLink(rightDocument, pagingPar, first, "javascript:parent.loadItems('"+loadItemId+"','"+prevPageStart+"');", "previous", iconPrevious, " ");
    }
    
    for (var i = pagesFrom; i <= pagesTo; i++)
    {
        if (first)
            first = false;
        else
            pagingPar.appendChild(rightDocument.createTextNode(" "));
        
        if (i == thisPage)
        {
            pagingLink = rightDocument.createElement("strong");
        }
        else
        {
            pagingLink = rightDocument.createElement("a");
            pagingLink.setAttribute("href", "javascript:parent.loadItems('"+loadItemId+"','"+(i * viewItems)+"');");
        }
        pagingLink.appendChild(rightDocument.createTextNode(i));
        pagingPar.appendChild(pagingLink);
    }
    
    if (nextPageStart < totalMatches)
    {
        first = _addLink(rightDocument, pagingPar, first, "javascript:parent.loadItems('"+loadItemId+"','"+nextPageStart+"');", "next", iconNext, " ");
        first = _addLink(rightDocument, pagingPar, first, "javascript:parent.loadItems('"+loadItemId+"','"+((totalPages - 1) * viewItems)+"');", "last", iconLast, " ");
    }
    
    /*
    if (thisPage < totalPages - showMaxPages / 2)
        first = _addLink(pagingPar, first, "javascript:parent.loadItems('"+loadItemId+"','"+((totalPages - 1) * viewItems)+"');", "last", iconLast, " ");
    */
    
    var children = items.getElementsByTagName(childrenTag);
    var itemsEl = rightDocument.createElement("div");
    var topItemsEl = topRightDocument.createElement("div");
    itemsEl.setAttribute("class", "itemsEl");
    
    var topTopDiv  = topRightDocument.createElement("div");
    topTopDiv.setAttribute("class", "topDiv");
    topItemsEl.appendChild(topTopDiv);
    
    var contTable = topRightDocument.createElement("table");
    contTable.setAttribute("width", "100%");
    contTable.setAttribute("class", "contTable");
    var contTableBody = topRightDocument.createElement("tbody");
    contTable.appendChild(contTableBody);
    topTopDiv.appendChild(contTable);
    var contRow = topRightDocument.createElement("tr");
    contTableBody.appendChild(contRow);
    
    var leftDiv = topRightDocument.createElement("td");
    leftDiv.setAttribute("class", "contEntry");
    
    var contIcon = topRightDocument.createElement("img");
    leftDiv.appendChild(contIcon);
    
    var pathEl = topRightDocument.createElement("span");
    pathEl.setAttribute("class", "contText");
    leftDiv.appendChild(pathEl);
    
    var buttons = topRightDocument.createElement("td");
    buttons.setAttribute("class", "itemButtons");
    
    contRow.appendChild(leftDiv);
    contRow.appendChild(buttons);
    
    if (useFiles)
    {
        contIcon.setAttribute("src", iconContainer.src);
        contIcon.setAttribute("alt", "directory:");
        contIcon.setAttribute("width", iconContainer.width);
        contIcon.setAttribute("height", iconContainer.height);
        
        pathEl.appendChild(topRightDocument.createTextNode(" /Filesystem" + path + (path.charAt(path.length - 1) != '/' ? '/' : '')));
        
        var first = true
        first = _addLink(topRightDocument, buttons, first, "javascript:parent.addItem('"+ofId+"');", "add", iconAdd);
        first = _addLink(topRightDocument, buttons, first, "javascript:parent.changeAutoscanDirectory('add','"+ofId+"', true);", "add as autoscan dir", iconAddAutoscan);
    }
    else
    {
        var iconSrc = iconContainer;
        if (autoscanType == '1')
            iconSrc = iconContainerAutoscan;
        if (autoscanType == '2')
            iconSrc = iconContainerAutoscanConfig;
        
        contIcon.setAttribute("src", iconSrc.src);
        contIcon.setAttribute("alt", "container:");
        contIcon.setAttribute("width", iconSrc.width);
        contIcon.setAttribute("height", iconSrc.height);
        
        pathEl.appendChild(topRightDocument.createTextNode(" /Database" + path + (path.charAt(path.length - 1) != '/' ? '/' : '')));
        
        var link;
        var first = true;
        var addLink = false;
        var editLink = false;
        var removeThisLink = false;
        var removeAllLink = false;
        var autoscanLink = false;
        
        
        if (lastNodeDb == 'd0')
        {
            addLink = true;
        }
        else if (lastNodeDb == 'd1')
        {
            editLink = true;
            autoscanLink = true;
        }
        else if (lastNodeDb !== 'd0')
        {
            if (isVirtual)
            {
                addLink = true;
                editLink = true;
                removeAllLink = true;
            }
            else
                autoscanLink = true;
            removeThisLink = true;
        }
        
        if (addLink)
            first = _addLink(topRightDocument, buttons, first, "javascript:parent.userAddItemStart();", "add Item", iconNewItem);
        if (editLink)
            first = _addLink(topRightDocument, buttons, first, "javascript:parent.userEditItemStart('"+ofId+"');", "edit", iconEdit);
        if (removeThisLink)
            first = _addLink(topRightDocument, buttons, first, "javascript:parent.removeItem('"+ofId+"', false);", "remove", iconRemoveThis);
        if (removeAllLink)
            first = _addLink(topRightDocument, buttons, first, "javascript:parent.removeItem('"+ofId+"', true);", "remove all", iconRemoveAll);
        if (autoscanLink)
        {
            if (autoscanType == "2")
            {
                //buttons.appendChild(topRightDocument.createTextNode(" (added as autoscan directory through config.xml)"));
            }
            else
            {
                var action = (autoscanType == "1" ? "remove" : "add");
                var icon = (autoscanType == "1" ? iconRemoveAutoscan : iconAddAutoscan);
                first = _addLink(topRightDocument, buttons, first,  "javascript:parent.changeAutoscanDirectory('"+action+"','"+ofId+"', false);", action+" as autoscan dir", icon);
            }
        }
    }
    itemsEl.appendChild(pagingPar);
    var itemsTable = rightDocument.createElement("table");
    itemsTable.setAttribute("width", "100%");
    var itemsTableBody = rightDocument.createElement("tbody");
    itemsTable.appendChild(itemsTableBody);
    itemsEl.appendChild(itemsTable);
    for (var i = 0; i < children.length; i++)
    {
        var itemRow = rightDocument.createElement("tr");
        itemRow.setAttribute("class", (i % 2 == 0 ? "itemRowA" : "itemRowB"));
        var item = children[i];
        var itemEntry = rightDocument.createElement("td");
        itemEntry.setAttribute("class", "itemEntry");
        var itemLink = rightDocument.createElement("a");
        itemEntry.appendChild(itemLink);
        
        var itemButtons = rightDocument.createElement("td");
        itemButtons.setAttribute("class", "itemButtons");
        
        var itemText = rightDocument.createTextNode(useFiles ? item.firstChild.nodeValue : xmlGetElementText(item, "title"));
        itemLink.appendChild(itemText);
        
        itemRow.appendChild(itemEntry);
        itemRow.appendChild(itemButtons);
        
        if (useFiles)
        {
            //itemEntry.appendChild(rightDocument.createTextNode(" - "));
            
            _addLink(rightDocument, itemButtons, true, "javascript:parent.addItem(\""+item.getAttribute("id")+"\");", "add", iconAdd);
        }
        else
        {
            //itemEntry.appendChild(rightDocument.createTextNode(" - "));
            
            _addLink(rightDocument, itemButtons, true, "javascript:parent.removeItem(\""+item.getAttribute("id")+"\", false);", "remove this", iconRemoveThis);
            if (isVirtual)
            {
                _addLink(rightDocument, itemButtons, false, "javascript:parent.removeItem(\""+item.getAttribute("id")+"\", true);", "remove all", iconRemoveAll);
                _addLink(rightDocument, itemButtons, false, "javascript:parent.userEditItemStart('"+item.getAttribute("id")+"');", "edit", iconEdit);
            }
            
            itemLink.setAttribute("href", xmlGetElementText(item, "res"));
            
            // the target is needed, because otherwise we probably mess up one of
            // our frames
            itemLink.setAttribute("target", "mediatomb_target");
        }
        itemsTableBody.appendChild(itemRow);
    }
    itemsEl.appendChild(pagingPar.cloneNode(true));
    
    //itemsEl.appendChild(rightDocument.createTextNode("total: "+totalMatches));
    
    itemRoot.replaceChild(itemsEl, itemRoot.firstChild);
    topItemRoot.replaceChild(topItemsEl, topItemRoot.firstChild);
}

function _addLink(useDocument, addToElement, first, href, text, icon, seperator, target)
{
    /*
    if (! first)
        addToElement.appendChild(rightDocument.createTextNode((seperator ? seperator : ", ")));
    */
    
    var link = useDocument.createElement("a");
    addToElement.appendChild(link);
    link.setAttribute("href", href);
    if (target)
        link.setAttribute("target", target);
    
    if (icon)
        appendImgNode(useDocument, link, text, icon);
    else
        link.appendChild(useDocument.createTextNode(text));
    return false; // to set the next "first"
}

function addItem(itemId)
{
    var url = link("add", {object_id: itemId}, true);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: addedItem
        });
}

function addedItem(ajaxRequest)
{
    var xml = ajaxRequest.responseXML;
    if (!errorCheck(xml)) return;
    
    //todo: feedback
}

function userAddItemStart()
{
    updateItemAddEditFields();
    Element.hide(itemRoot);
    itemRoot=rightDocument.getElementById('item_add_edit_div');
    Element.show(itemRoot);
}

function userEditItemStart(objectId)
{
    var url = link("edit_load", {object_id: objectId}, true);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: userEditItemCallback
        });
}

function userEditItemCallback(ajaxRequest)
{
    var xml = ajaxRequest.responseXML;
    if (!errorCheck(xml)) return;
    var item = xmlGetElement(xml, "item");
    updateItemAddEditFields(item);
    Element.hide(itemRoot);
    itemRoot=rightDocument.getElementById('item_add_edit_div');
    Element.show(itemRoot);
}

function updateItemAddEditFields(editItem)
{
    var currentTypeOption;
    var form = rightDocument.forms['addEditItem'];
    var selectEl = form.elements['objType'];
    var submitEl = form.elements['submit'];
    if (!editItem)
    {
        selectEl.disabled = false;
        submitEl.value = 'Add item...';
        currentTypeOption = selectEl.value;
        if (!currentTypeOption) currentTypeOption = '1';
        form.action = 'javascript:parent.itemAddEditSubmit();';
    }
    else
    {
        selectEl.disabled = true;
        submitEl.value = 'Update item...';
        currentTypeOption = xmlGetElementText(editItem, 'objType');
        var objectId = editItem.getAttribute('object_id');
        selectEl.value = currentTypeOption;
        form.action = 'javascript:parent.itemAddEditSubmit('+objectId+');';
    }
    
    if (!selectEl.options[0])
    {
        // ATTENTION: These values need to be changed in src/cds_objects.h too.
        // Note: 'Active Item', 'External Link (URL)', 'Internal Link (Local URL)'
        // are also 'Items', so they have the item flag set too.
        var objTypeOptions = new Array('Container', 'Item', 'Active Item', 'External Link (URL)', 'Internal Link (Local URL)');
        var objTypeOptionsIds = new Array('1', '2', '6', '10', '26');
        
        for (var i = 0; i < objTypeOptions.length; ++i)
            selectEl.options[i] = new Option(
                objTypeOptions[i],
                objTypeOptionsIds[i],
                false, 
                (currentTypeOption && objTypeOptionsIds[i] == currentTypeOption)
                );
    }
    
    var fieldAr;
    var fieldNameAr;
    var defaultsAr;
    
    // using "if" instead of "switch" for compatibility reasons...
    if (currentTypeOption == '1')
    {
        fieldAr = new Array('Title', 'Class');
        fieldNameAr = new Array('title', 'class');
        defaultsAr = new Array('', 'object.container');
    }
    else if (currentTypeOption == '2')
    {
        fieldAr = new Array('Title', 'Location', 'Class', 'Description', 'Mimetype');
        fieldNameAr = new Array('title', 'location', 'class', 'description', 'mime-type');
        defaultsAr = new Array('', '', 'object.item', '', '');
    }
    else if (currentTypeOption == '6')
    {
        fieldAr = new Array('Title', 'Location', 'Class', 'Description', 'Mimetype', 'Action Script', 'State');
        fieldNameAr = new Array('title', 'location', 'class', 'description', 'mime-type', 'action', 'state');
        defaultsAr = new Array('', '', 'object.item.activeItem', '', '', '', '');
    }
    else if (currentTypeOption == '10')
    {
        fieldAr = new Array('Title', 'URL', 'Protocol', 'Class', 'Description', 'Mimetype');
        fieldNameAr = new Array('title', 'location', 'protocol', 'class', 'description', 'mime-type');
        defaultsAr = new Array('', '', 'http-get', 'object.item', '', '');
    }
    else if (currentTypeOption == '26')
    {
        fieldAr = new Array('Title', 'URL', 'Class', 'Description', 'Mimetype');
        fieldNameAr = new Array('title', 'location', 'class', 'description', 'mime-type');
        defaultsAr = new Array('', '', 'object.item', '', '');
    }
    
    var itemTbody = rightDocument.createElement('tbody');
    var itemRoot = rightDocument.getElementById("item_add_edit_inputs");
    if (fieldAr && defaultsAr)
    {
        for (var i = 0; i < fieldAr.length; ++i)
        {
            var inputTr = rightDocument.createElement('tr');
            itemTbody.appendChild(inputTr);
            var inputTd = rightDocument.createElement('td');
            inputTr.appendChild(inputTd);
            inputTd.setAttribute("align", "right");
            inputTd.appendChild(rightDocument.createTextNode(fieldAr[i]+": "));
            
            var inputTd = rightDocument.createElement('td');
            inputTr.appendChild(inputTd);
            var inputEl = rightDocument.createElement('input');
            inputEl.setAttribute('type', 'text');
            inputEl.setAttribute('name', fieldNameAr[i]);
            if (!editItem)
                inputEl.setAttribute('value', defaultsAr[i]);
            else
                inputEl.setAttribute('value', xmlGetElementText(editItem, fieldNameAr[i]));
            
            inputTd.appendChild(inputEl);
        }
    }
    var oldNode = rightDocument.getElementById("item_add_edit_tbody");
    oldNode.parentNode.replaceChild(itemTbody, oldNode);
    itemTbody.setAttribute("id", "item_add_edit_tbody");
}

function itemAddEditSubmit(objectId)
{
    var req_type;
    var args = new Object();
    if (objectId)
    {
        req_type = 'edit_save';
        args['object_id'] = objectId;
    }
    else
    {
        req_type = 'add_object';
        args['parent_id'] = lastNodeDb.substr(1);
    }
    var form = rightDocument.forms['addEditItem'];
    
    formToArray(form, args);
    
    var url = link(req_type, args);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: addEditRemoveSubmitted
        });
}

function addEditRemoveSubmitted(ajaxRequest)
{
    //alert(ajaxRequest.responseText);
    if (!errorCheck(ajaxRequest.responseXML)) return;
    
    folderChange(selectedNode);
    window.setTimeout("getUpdates(false)", 1000);
}

function removeItem(itemId, all)
{
    if (itemId == '0')
    {
        alert("Root container cannot be removed!");
        return;
    }
    
    if ('d'+itemId == selectedNode)
    {
        //alert("current selected");
        // current container will be removed, selecting parent...
        selectNode(getTreeNode(selectedNode).getParent().getID());
    }
    if (all)
        all_send="1";
    else
        all_send="0";
    var url = link('remove', {object_id: itemId, all: all_send}, true);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: addEditRemoveSubmitted
        });
}
