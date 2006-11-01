var itemRoot;
var rightDocument;
var viewItems = 5;

function itemInit()
{
    rightDocument = frames["rightF"].document;
    itemChangeType();
}

function itemChangeType()
{
    var oldItemRoot = itemRoot;
    var dbItemRoot = rightDocument.getElementById("item_db_div");
    var fsItemRoot = rightDocument.getElementById("item_fs_div");
    
    itemRoot = isTypeDb() ? dbItemRoot : fsItemRoot;
    if (oldItemRoot && oldItemRoot!=itemRoot) Element.hide(oldItemRoot);
    Element.show(itemRoot);
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
    
    var children = items.getElementsByTagName(childrenTag);
    var itemsEl = rightDocument.createElement("div");
    
    var ofId = items.getAttribute("ofId");
    if (useFiles)
    {
        var topDiv = rightDocument.createElement("div");
        topDiv.appendChild(rightDocument.createTextNode("Folder: "));
        
        //if (lastNodeFs == 'f0')
        //{
        var link = rightDocument.createElement("a");
        topDiv.appendChild(link);
        link.setAttribute("href", "javascript:parent.addItem('"+ofId+"');");
        link.appendChild(rightDocument.createTextNode("add"));
        //}
        
        itemsEl.appendChild(topDiv);
    }
    else
    {
        var topDiv = rightDocument.createElement("div");
        topDiv.appendChild(rightDocument.createTextNode("Container: "));
        
        var link = rightDocument.createElement("a");
        topDiv.appendChild(link);
        link.setAttribute("href", "javascript:parent.userAddItemStart();");
        link.appendChild(rightDocument.createTextNode("add Item"));
        
        if (lastNodeDb !== 'd0')
        {
            topDiv.appendChild(rightDocument.createTextNode(", "));
            link = rightDocument.createElement("a");
            topDiv.appendChild(link);
            link.setAttribute("href", "javascript:parent.userEditItemStart('"+ofId+"');");
            link.appendChild(rightDocument.createTextNode("edit"));
            
            topDiv.appendChild(rightDocument.createTextNode(", "));
            link = rightDocument.createElement("a");
            topDiv.appendChild(link);
            link.setAttribute("href", "javascript:parent.removeItem('"+ofId+"', false);");
            link.appendChild(rightDocument.createTextNode("remove"));
        }
        
        itemsEl.appendChild(topDiv);
    }
    for (var i = 0; i < children.length; i++)
    {
        var item = children[i];
        var itemEntry = rightDocument.createElement("div");
        var itemLink = rightDocument.createElement("a");
        itemEntry.appendChild(itemLink);
        
        if (useFiles)
        {
            itemEntry.appendChild(rightDocument.createTextNode(" - "));
            
            var linkEl = rightDocument.createElement("a");
            linkEl.setAttribute("href", "javascript:parent.addItem(\""+item.getAttribute("id")+"\");");
            linkEl.appendChild(rightDocument.createTextNode("add"));
            itemEntry.appendChild(linkEl);
        }
        else
        {
            itemEntry.appendChild(rightDocument.createTextNode(" - "));
            
            var linkEl = rightDocument.createElement("a");
            linkEl.setAttribute("href", "javascript:parent.removeItem(\""+item.getAttribute("id")+"\", false);");
            linkEl.appendChild(rightDocument.createTextNode("remove this"));
            itemEntry.appendChild(linkEl);
            
            itemEntry.appendChild(rightDocument.createTextNode(", "));
            
            linkEl = rightDocument.createElement("a");
            linkEl.setAttribute("href", "javascript:parent.removeItem(\""+item.getAttribute("id")+"\", true);");
            linkEl.appendChild(rightDocument.createTextNode("remove all"));
            itemEntry.appendChild(linkEl);
            
            itemEntry.appendChild(rightDocument.createTextNode(", "));
            
            linkEl = rightDocument.createElement("a");
            linkEl.setAttribute("href", "javascript:parent.userEditItemStart('"+item.getAttribute("id")+"');");
            linkEl.appendChild(rightDocument.createTextNode("edit"));
            itemEntry.appendChild(linkEl);
            
            itemLink.setAttribute("href", xmlGetElementText(item, "res"));
            itemLink.setAttribute("target", "_blank");
        }
        var itemText = rightDocument.createTextNode(useFiles ? item.firstChild.nodeValue : xmlGetElementText(item, "title"));
        itemLink.appendChild(itemText);
        itemsEl.appendChild(itemEntry);
    }
    var showMaxPages = 10;
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
    var pagingLink;
    
    if (thisPage > showMaxPages / 2)
    {
        pagingLink = rightDocument.createElement("a");
        pagingLink.setAttribute("href", "javascript:parent.loadItems('"+loadItemId+"','0');");
        pagingLink.appendChild(rightDocument.createTextNode("first"));
        itemsEl.appendChild(pagingLink);
        itemsEl.appendChild(rightDocument.createTextNode(" "));
    }
    
    if (prevPageStart >= 0)
    {
        pagingLink = rightDocument.createElement("a");
        pagingLink.setAttribute("href", "javascript:parent.loadItems('"+loadItemId+"','"+prevPageStart+"');");
        pagingLink.appendChild(rightDocument.createTextNode("prev"));
        itemsEl.appendChild(pagingLink);
        itemsEl.appendChild(rightDocument.createTextNode(" "));
    }
    
    for (var i = pagesFrom; i <= pagesTo; i++)
    {
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
        itemsEl.appendChild(pagingLink);
        itemsEl.appendChild(rightDocument.createTextNode(" "));
    }
    
    if (nextPageStart < totalMatches)
    {
        pagingLink = rightDocument.createElement("a");
        pagingLink.setAttribute("href", "javascript:parent.loadItems('"+loadItemId+"','"+nextPageStart+"');");
        pagingLink.appendChild(rightDocument.createTextNode("next"));
        itemsEl.appendChild(pagingLink);
        itemsEl.appendChild(rightDocument.createTextNode(" "));
    }
    
    if (thisPage < totalPages - showMaxPages / 2)
    {
        pagingLink = rightDocument.createElement("a");
        pagingLink.setAttribute("href", "javascript:parent.loadItems('"+loadItemId+"','"+((totalPages - 1) * viewItems)+"');");
        pagingLink.appendChild(rightDocument.createTextNode("last"));
        itemsEl.appendChild(pagingLink);
        itemsEl.appendChild(rightDocument.createTextNode(" "));
    }
    itemsEl.appendChild(rightDocument.createTextNode("totalPages: "+totalPages+"; thisPage: "+thisPage+"; total: "+items.getAttribute("totalMatches")));
    
    itemRoot.replaceChild(itemsEl, itemRoot.firstChild);
    
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
    
    var inputsDiv = rightDocument.createElement('div');
    
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
    
    if (fieldAr && defaultsAr)
    {
        for (var i = 0; i < fieldAr.length; ++i)
        {
            inputsDiv.appendChild(rightDocument.createTextNode(fieldAr[i]+": "));
            var inputEl = rightDocument.createElement('input');
            inputEl.setAttribute('type', 'text');
            inputEl.setAttribute('name', fieldNameAr[i]);
            if (!editItem)
                inputEl.setAttribute('value', defaultsAr[i]);
            else
                inputEl.setAttribute('value', xmlGetElementText(editItem, fieldNameAr[i]));
            inputsDiv.appendChild(inputEl);
            addBr(rightDocument, inputsDiv);
        }
    }
    
    var itemRoot = rightDocument.getElementById("item_add_edit_inputs");
    itemRoot.replaceChild(inputsDiv, itemRoot.firstChild);
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
    for (var i = 0; i < form.length; ++i)
    {
        var element = form.elements[i];
        if (element.name != 'submit')
            args[element.name] = element.value;
    }
    
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
    var xml = ajaxRequest.responseXML;
    if (!errorCheck(xml)) return;
    
    /*
    var updateContainer = xmlGetElement(xml, 'updateContainer');
    if (updateContainer)
    {
        var node = getTreeNode("d"+xmlGetText(updateContainer));
        if (!node.hasChildren() && updateContainer.getAttribute("add"))
        {
            node.setHasChildren(true);
            refreshNode(node.getParent());
        }
        
        selectNode(node.getID());
        node.childrenHaveBeenFetched=false;
        node.resetChildren();
        
        fetchChildren(node);
        //itemChangeType();
    }
    else
    */
    folderChange(selectedNode);
}

function removeItem(itemId, all)
{
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

