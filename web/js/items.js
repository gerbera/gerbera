/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    items.js - this file is part of MediaTomb.
    
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

var itemRoot;
var topItemRoot
var rightDocument;
var topRightDocument;

var dbItemRoot;
var fsItemRoot;

// will be overridden by getConfigCallback() (auth.js)
// dann hast du nicht irgendwelche schrittlaenge und scheisse...
var itemOptions = new Array(10, 25, 50, 100);
var viewItemsMin = 10;
var viewItems = -1;
var defaultViewItems = 25;
var showAddPages = 3;

function itemInit()
{
    rightDocument = frames["rightF"].document;
    topRightDocument = frames["toprightF"].document;
    dbItemRoot = rightDocument.getElementById("item_db_div");
    fsItemRoot = rightDocument.getElementById("item_fs_div");
    itemChangeType();
    if (viewItems == -1)
        viewItems = defaultViewItems;
}

function itemChangeType()
{
    var oldItemRoot = itemRoot;
    
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

var lastFolder;
var lastItemStart;

function folderChange(id)
{
    //if (itemRoot != dbItemRoot && itemRoot != fsItemRoot)
    //    return;
    if (id == lastFolder)
        loadItems(id, lastItemStart);
    else
        loadItems(id, 0);
}

function loadItems(id, start)
{
    if (start % viewItems != 0)
        start = Math.floor((start / viewItems)) * viewItems;
    
    lastItemStart = start;
    lastFolder = id;
    
    //itemChangeType();
    var type = id.substring(0, 1);
    id = id.substring(1);
    var itemLink = type == 'd' ? 'items' : 'files';
    var url = link(itemLink, {parent_id: id, start: start, count: viewItems}, true);
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
    
    var ofId = items.getAttribute("parent_id");
    var success = xmlGetElement(xml, 'root').getAttribute("success");
    //alert(success);
    //alert(xml);
    if (! success || success != "1")
    {
        if (ofId == '0')
        {
            alert("Oops, your database seems to be corrupt. Please report this problem.");
            return;
        }
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
    var autoscanType = items.getAttribute("autoscan_type");
    var autoscanMode = items.getAttribute("autoscan_mode");
    var path = items.getAttribute("location");
    var loadItemId = (useFiles ? 'f' : 'd') + ofId;
    var totalMatches = parseInt(items.getAttribute("total_matches"));
    var isProtected = (items.getAttribute("protect_container") == '1');
    var itemsProtected = (items.getAttribute("protect_items") == '1');
    var totalPages = Math.ceil(totalMatches / viewItems);
    var start = parseInt(items.getAttribute("start"));
    var thisPage = Math.abs(start / viewItems);
    var nextPageStart = (thisPage + 1) * viewItems;
    var prevPageStart = (thisPage - 1) * viewItems;
    var showPaging = (! useFiles && totalMatches > viewItemsMin);
    var showPagingPages = (totalPages > 1);
    if (showPaging)
    {
        if (showPagingPages)
        {
            var pagesFrom;
            var pagesTo
            if (thisPage <= showAddPages + 1)
            {
                pagesFrom = 0;
                pagesTo = showAddPages * 2 + 1;
            }
            else if (thisPage < totalPages - showAddPages - 1)
            {
                pagesFrom = thisPage - showAddPages;
                pagesTo = thisPage + showAddPages;
            }
            else
            {
                pagesFrom = totalPages - showAddPages * 2 - 2;
                pagesTo = totalPages - 1;
            }
            
            if (pagesFrom == 2)
                pagesFrom--;
            if (pagesFrom < 0)
                pagesFrom = 0;
            if (pagesTo == totalPages - 3)
                pagesTo++;
            
            if (pagesTo >= totalPages)
                pagesTo = totalPages - 1;
        }
        
        var pagingTab1 = rightDocument.createElement("table");
        pagingTab1.setAttribute("align", "center");
        var pagingTbody1 = rightDocument.createElement("tbody");
        var pagingRow = rightDocument.createElement("tr");
        var pagingCellLeft = rightDocument.createElement("td");
        pagingCellLeft.setAttribute("align", "right");
        var pagingCellCenter = rightDocument.createElement("td");
        pagingCellCenter.setAttribute("align", "center");
        var pagingCellRight = rightDocument.createElement("td");
        pagingCellRight.setAttribute("align", "left");
        pagingTab1.appendChild(pagingTbody1);
        pagingTbody1.appendChild(pagingRow);
        pagingRow.appendChild(pagingCellLeft);
        pagingRow.appendChild(pagingCellCenter);
        pagingRow.appendChild(pagingCellRight);
        
        var first = true;
        
        /*
        if (thisPage > showMaxPages / 2)
            first = _addLink(pagingPar, first, "javascript:parent.loadItems('"+loadItemId+"','0');", "first", iconFirst, " ");
        */
        
        if (prevPageStart >= 0)
        {
            _addLink(rightDocument, pagingCellLeft, false, "javascript:parent.loadItems('"+loadItemId+"','0');", "first", iconFirst, " ");
            _addLink(rightDocument, pagingCellLeft, false, "javascript:parent.loadItems('"+loadItemId+"','"+prevPageStart+"');", "previous", iconPrevious, " ");
        }
        else
        {
            appendImgNode(rightDocument, pagingCellLeft, "", iconArrowReplacement);
            appendImgNode(rightDocument, pagingCellLeft, "", iconArrowReplacement);
        }
        
        if (nextPageStart < totalMatches)
        {
            _addLink(rightDocument, pagingCellRight, false, "javascript:parent.loadItems('"+loadItemId+"','"+nextPageStart+"');", "next", iconNext, " ");
            _addLink(rightDocument, pagingCellRight, false, "javascript:parent.loadItems('"+loadItemId+"','"+((totalPages - 1) * viewItems)+"');", "last", iconLast, " ");
        }
        else
        {
            appendImgNode(rightDocument, pagingCellRight, "", iconArrowReplacement);
            appendImgNode(rightDocument, pagingCellRight, "", iconArrowReplacement);
        }
        
        if (showPagingPages)
        {
            var pagingTab2 = rightDocument.createElement("table");
            pagingTab2.setAttribute("align", "center");
            var pagingTbody2 = rightDocument.createElement("tbody");
            pagingTab2.appendChild(pagingTbody2);
            var pagingPagesRow = rightDocument.createElement("tr");
            var pagingPagesCell = rightDocument.createElement("td");
            pagingPagesCell.setAttribute("align", "center");
            pagingPagesCell.setAttribute("colspan", "3");
            pagingTbody2.appendChild(pagingPagesRow);
            pagingPagesRow.appendChild(pagingPagesCell);
            
            var first = true;
            
            if (pagesFrom > 0)
            {
                first = false;
                var pagingLink = rightDocument.createElement("a");
                pagingLink.setAttribute("href", "javascript:parent.loadItems('"+loadItemId+"','0');");
                pagingLink.appendChild(rightDocument.createTextNode(1));
                pagingPagesCell.appendChild(pagingLink);
                if (pagesFrom > 1)
                    pagingPagesCell.appendChild(rightDocument.createTextNode(" ..."));
            }
            
            for (var i = pagesFrom; i <= pagesTo; i++)
            {
                if (first)
                    first = false;
                else
                    pagingPagesCell.appendChild(rightDocument.createTextNode(" "));
                
                var pagingLink;
                
                if (i == thisPage)
                {
                    pagingLink = rightDocument.createElement("strong");
                }
                else
                {
                    pagingLink = rightDocument.createElement("a");
                    pagingLink.setAttribute("href", "javascript:parent.loadItems('"+loadItemId+"','"+(i * viewItems)+"');");
                }
                pagingLink.appendChild(rightDocument.createTextNode( i + 1));
                pagingPagesCell.appendChild(pagingLink);
            }
            
            if (pagesTo < totalPages - 1)
            {
                var pagingLink = rightDocument.createElement("a");
                pagingLink.setAttribute("href", "javascript:parent.loadItems('"+loadItemId+"','"+((totalPages - 1) * viewItems)+"');");
                pagingLink.appendChild(rightDocument.createTextNode(totalPages));
                if (pagesTo < totalPages - 2)
                    pagingPagesCell.appendChild(rightDocument.createTextNode(" ... "));
                else
                    pagingPagesCell.appendChild(rightDocument.createTextNode(" "));
                pagingPagesCell.appendChild(pagingLink);
                
            }
        }
        
        /*
        if (thisPage < totalPages - showMaxPages / 2)
            first = _addLink(pagingPagesCell, first, "javascript:parent.loadItems('"+loadItemId+"','"+((totalPages - 1) * viewItems)+"');", "last", iconLast, " ");
        */
    }
    
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
    leftDiv.setAttribute("valign", "middle");
    
    var contIcon = topRightDocument.createElement("img");
    leftDiv.appendChild(contIcon);
    //contIcon.setAttribute("style", "vertical-align:middle;");
    
    var pathEl = topRightDocument.createElement("span");
    pathEl.setAttribute("class", "contText");
    //pathEl.setAttribute("style", "vertical-align:middle;");
    leftDiv.appendChild(pathEl);
    
    var buttons = topRightDocument.createElement("td");
    buttons.setAttribute("class", "itemButtons");
    buttons.setAttribute("align", "right");
    
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
        first = _addLink(topRightDocument, buttons, first, "javascript:parent.editLoadAutoscanDirectory('"+ofId+"', true);", "add as autoscan dir", iconAddAutoscan);
    }
    else
    {
        var iconSrc = iconContainer;
        if (autoscanType == 'ui')
        {
            if (autoscanMode == 'inotify')
                iconSrc = iconContainerAutoscanInotify;
            else
                iconSrc = iconContainerAutoscanTimed;
        }
        
        if (autoscanType == 'persistent')
        {
            if (autoscanMode == 'inotify')
                iconSrc = iconContainerAutoscanInotifyConfig;
            else
                iconSrc = iconContainerAutoscanTimedConfig;
        }
        
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
        else
        {
            if (isVirtual)
            {
                addLink = true;
                editLink = true;
                if (! isProtected)
                {
                    removeThisLink = true;
                    removeAllLink = true;
                }
            }
            else
            if (! isProtected)
            {
                removeThisLink = true;
                autoscanLink = true;
            }
        }
        
        if (autoscanType != 'none')
            autoscanLink = true;
        
        if (addLink)
            first = _addLink(topRightDocument, buttons, first, "javascript:parent.userAddItemStart();", "add Item", iconNewItem);
        if (editLink)
            first = _addLink(topRightDocument, buttons, first, "javascript:parent.userEditItemStart('"+ofId+"');", "edit", iconEdit);
        if (removeThisLink)
            first = _addLink(topRightDocument, buttons, first, "javascript:parent.removeItem('"+ofId+"', false);", "remove", iconRemoveThis);
        if (removeAllLink)
            first = _addLink(topRightDocument, buttons, first, "javascript:parent.removeItem('"+ofId+"', true);", "remove all", iconRemoveAll);
        if (autoscanLink)
            first = _addLink(topRightDocument, buttons, first,  "javascript:parent.editLoadAutoscanDirectory('"+ofId+"', false);", "change autoscan dir", iconEditAutoscan);
    }
    
    if (showPaging)
    {
        var pagingForm = rightDocument.createElement("form");
        pagingForm.setAttribute("name", "itemsPerPageForm1");
        var pagingSelect = rightDocument.createElement("select");
        pagingForm.appendChild(pagingSelect);
        pagingSelect.setAttribute("size", "1");
        pagingSelect.setAttribute("onchange", "parent.changeItemsPerPage(1)");
        pagingSelect.setAttribute("name", "itemsPerPage1");
        
        // deactivated for MSIE for now..
        if (! isMSIE)
            pagingCellCenter.appendChild(pagingForm);
        
        itemsEl.appendChild(pagingTab1.cloneNode(true));
        if (showPagingPages)
            itemsEl.appendChild(pagingTab2.cloneNode(true));
        
        pagingForm.setAttribute("name", "itemsPerPageForm2");
        pagingSelect.setAttribute("onchange", "parent.changeItemsPerPage(2)");
        pagingSelect.setAttribute("name", "itemsPerPage2");
    }
    
    var itemsTable = rightDocument.createElement("table");
    itemsTable.setAttribute("width", "100%");
    itemsTable.setAttribute("cellspacing", "0");
    var itemsTableBody = rightDocument.createElement("tbody");
    itemsTable.appendChild(itemsTableBody);
    itemsEl.appendChild(itemsTable);
    for (var i = 0; i < children.length; i++)
    {
        var itemRow = rightDocument.createElement("tr");
        itemRow.setAttribute("class", (i % 2 == 0 ? "itemRowA" : "itemRowB"));
        var item = children[i];
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
        var itemLink = rightDocument.createElement("a");
        itemEntry.appendChild(itemLink);
        
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
        
        var itemText = rightDocument.createTextNode(useFiles ? item.firstChild.nodeValue : xmlGetElementText(item, "title"));
        itemLink.appendChild(itemText);
        
        itemRow.appendChild(itemEntryTd);
        itemRow.appendChild(itemButtonsTd);
        
        if (useFiles)
        {
            //itemEntry.appendChild(rightDocument.createTextNode(" - "));
            
            _addLink(rightDocument, itemButtons, true, "javascript:parent.addItem(\""+item.getAttribute("id")+"\");", "add", iconAdd);
        }
        else
        {
            //itemEntry.appendChild(rightDocument.createTextNode(" - "));
            
            if (! itemsProtected)
            {
                _addLink(rightDocument, itemButtons, true, "javascript:parent.removeItem(\""+item.getAttribute("id")+"\", false);", "remove this", iconRemoveThis);
                if (isVirtual)
                {
                    _addLink(rightDocument, itemButtons, false, "javascript:parent.removeItem(\""+item.getAttribute("id")+"\", true);", "remove all", iconRemoveAll);
                }
            }
            
            _addLink(rightDocument, itemButtons, false, "javascript:parent.userEditItemStart('"+item.getAttribute("id")+"');", "edit", iconEdit);
            
            itemLink.setAttribute("href", xmlGetElementText(item, "res"));
            
            // the target is needed, because otherwise we probably mess up one of
            // our frames
            itemLink.setAttribute("target", "mediatomb_target");
        }
        itemsTableBody.appendChild(itemRow);
    }
    
    if (showPaging)
    {
        if (showPagingPages)
            itemsEl.appendChild(pagingTab2);
        itemsEl.appendChild(pagingTab1);
    }
    
    //itemsEl.appendChild(rightDocument.createTextNode("total: "+totalMatches));
    
    if (useFiles)
        fsItemRoot.replaceChild(itemsEl, fsItemRoot.firstChild);
    else
        dbItemRoot.replaceChild(itemsEl, dbItemRoot.firstChild);
    topItemRoot.replaceChild(topItemsEl, topItemRoot.firstChild);
    
    /* partly works with IE...
    for (var i = 0; i < rightDocument.forms.length; i++)
    {
        var form = rightDocument.forms[i];
        if (form.name == 'itemsPerPageForm1')
            _addItemsPerPage(form.elements, 1);
        if (form.name == 'itemsPerPageForm2')
            _addItemsPerPage(form.elements, 2);
    }
    */
    
    if (showPaging)
    {
        // deactivated for MSIE for now...
        if (! isMSIE)
        {
            _addItemsPerPage(rightDocument.forms['itemsPerPageForm1'].elements['itemsPerPage1']);
            _addItemsPerPage(rightDocument.forms['itemsPerPageForm2'].elements['itemsPerPage2']);
        }
    }
}

function _addItemsPerPage(itemsPerPageEl) //IE itemsPerPageFormElements, formId)
{
    /* partly works with IE...
    var itemsPerPageEl;
    for (var i = 0; i < itemsPerPageFormElements.length; i ++)
    {
        if (itemsPerPageFormElements[i].name == 'itemsPerPage'+formId)
            itemsPerPageEl = itemsPerPageFormElements[i];
    }
    */
    
    if (itemsPerPageEl)
    {
        //itemsPerPageEl.onchange='parent.changeItemsPerPage('+formId+')';
        for (var i = 0; i < itemOptions.length; i ++)
        {
            var itemCount = itemOptions[i];
            itemsPerPageEl.options[i] = new Option(
                itemCount,
                itemCount,
                false,
                itemCount == viewItems
            );
        }
    }
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
    
    addUpdateTimer();
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
    itemRoot = rightDocument.getElementById('item_add_edit_div');
    Element.show(itemRoot);
    use_inactivity_timeout_short = true;
}

function updateItemAddEditFields(editItem)
{
    var currentTypeOption;
    var form = rightDocument.forms['addEditItem'];
    var selectEl = form.elements['obj_type'];
    var submitEl = form.elements['submit'];
    if (!editItem)
    {
        selectEl.disabled = false;
        submitEl.value = 'Add item...';
        currentTypeOption = selectEl.value;
        if (!currentTypeOption) currentTypeOption = 'container';
        form.action = 'javascript:parent.itemAddEditSubmit();';
    }
    else
    {
        selectEl.disabled = true;
        submitEl.value = 'Update item...';
        currentTypeOption = xmlGetElementText(editItem, 'obj_type');
        var objectId = editItem.getAttribute('object_id');
        selectEl.value = currentTypeOption;
        form.action = 'javascript:parent.itemAddEditSubmit('+objectId+');';
    }
    
    if (!selectEl.options[0])
    {
        // ATTENTION: These values need to be changed in src/cds_objects.h too.
        // Note: 'Active Item', 'External Link (URL)', 'Internal Link (Local URL)'
        // are also 'Items', so they have the item flag set too.
        var objTypeOptionsText = new Array('Container', 'Item', 'Active Item', 'External Link (URL)', 'Internal Link (Local URL)');
        var objTypeOptionsValue = new Array('container', 'item', 'active_item', 'external_url', 'internal_url');
        
        for (var i = 0; i < objTypeOptionsValue.length; ++i)
            selectEl.options[i] = new Option(
                objTypeOptionsText[i],
                objTypeOptionsValue[i],
                false, 
                (currentTypeOption && objTypeOptionsValue[i] == currentTypeOption)
                );
    }
    
    var fieldAr;
    var fieldNameAr;
    var defaultsAr;
    
    // using "if" instead of "switch" for compatibility reasons...
    if (currentTypeOption == 'container')
    {
        fieldAr = new Array('Title', 'Class');
        fieldNameAr = new Array('title', 'class');
        defaultsAr = new Array('', 'object.container');
    }
    else if (currentTypeOption == 'item')
    {
        fieldAr = new Array('Title', 'Location', 'Class', 'Description', 'Mimetype');
        fieldNameAr = new Array('title', 'location', 'class', 'description', 'mime-type');
        defaultsAr = new Array('', '', 'object.item', '', '');
    }
    else if (currentTypeOption == 'active_item')
    {
        fieldAr = new Array('Title', 'Location', 'Class', 'Description', 'Mimetype', 'Action Script', 'State');
        fieldNameAr = new Array('title', 'location', 'class', 'description', 'mime-type', 'action', 'state');
        defaultsAr = new Array('', '', 'object.item.activeItem', '', '', '', '');
    }
    else if (currentTypeOption == 'external_url')
    {
        fieldAr = new Array('Title', 'URL', 'Protocol', 'Class', 'Description', 'Mimetype');
        fieldNameAr = new Array('title', 'location', 'protocol', 'class', 'description', 'mime-type');
        defaultsAr = new Array('', '', 'http-get', 'object.item', '', '');
    }
    else if (currentTypeOption == 'internal_url')
    {
        fieldAr = new Array('Title', 'URL', 'Class', 'Description', 'Mimetype');
        fieldNameAr = new Array('title', 'location', 'class', 'description', 'mime-type');
        defaultsAr = new Array('', '', 'object.item', '', '');
    }
    
    var itemTbody = rightDocument.createElement('tbody');
    //itemRoot = rightDocument.getElementById("item_add_edit_inputs");
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
            inputEl.setAttribute('size', '40');
            if (!editItem)
                inputEl.setAttribute('value', defaultsAr[i]);
            else
            {
                var xmlElement = xmlGetElement(editItem, fieldNameAr[i]);
                inputEl.setAttribute('value', xmlGetText(xmlElement));
                if(xmlElement.getAttribute('editable') != '1')
                    inputEl.setAttribute('disabled', 'disabled');
            }
            
            inputTd.appendChild(inputEl);
        }
    }
    var oldNode = rightDocument.getElementById("item_add_edit_tbody");
    oldNode.parentNode.replaceChild(itemTbody, oldNode);
    itemTbody.setAttribute("id", "item_add_edit_tbody");
}

function itemAddEditSubmit(objectId)
{
    itemChangeType();
    
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
    
    window.setTimeout("getUpdates(true)", 800);
    folderChange(selectedNode);
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
    
    use_inactivity_timeout_short = true;
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: addEditRemoveSubmitted
        });
}

function changeItemsPerPage(formId)
{
    var newViewItems = rightDocument.forms['itemsPerPageForm'+formId].elements['itemsPerPage'+formId].value;
    if (newViewItems != viewItems)
    {
        viewItems = newViewItems;
        setCookie("viewItems", viewItems);
        folderChange(selectedNode);
    }
}

function cancelButtonPressed()
{
    itemChangeType();
}
