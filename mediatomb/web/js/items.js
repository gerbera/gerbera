var itemRoot;

function itemInit()
{
    /*
    var newNodeDb = document.createElement('div');
    var newNodeFs = document.createElement('div');
    var newNodeAdd = document.createElement('div');
    $('item_db_div').appendChild(newNodeDb);
    $('item_fs_div').appendChild(newNodeFs);
    $('item_add_inputs').appendChild(newNodeAdd);
    */
    itemChangeType();
}

function itemChangeType()
{
    var oldItemRoot = itemRoot;
    itemRoot = isTypeDb() ? $('item_db_div') : $('item_fs_div');
    if (oldItemRoot && oldItemRoot!=itemRoot) Element.hide(oldItemRoot);
    Element.show(itemRoot);
}

function folderChange(id)
{
    itemChangeType()
    var type = id.substring(0, 1);
    id = id.substring(1);
    var itemLink = type == 'd' ? 'items' : 'files';
    var url = link(itemLink, {parent_id: id});
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
    var itemsEl = document.createElement("div");
    
    for (var i = 0; i < children.length; i++)
    {
        var item = children[i];
        var itemEntry = document.createElement("div");
        var itemLink = document.createElement("a");
        itemEntry.appendChild(itemLink);
        
        if (useFiles)
        {
            itemEntry.appendChild(document.createTextNode(" - "));
            
            var linkEl = document.createElement("a");
            linkEl.setAttribute("href", "javascript:addItem(\""+item.getAttribute("id")+"\");");
            linkEl.appendChild(document.createTextNode("add"));
            itemEntry.appendChild(linkEl);
        }
        else
        {
            itemEntry.appendChild(document.createTextNode(" - "));
            
            var linkEl = document.createElement("a");
            linkEl.setAttribute("href", "javascript:removeItem(\""+item.getAttribute("id")+"\");");
            linkEl.appendChild(document.createTextNode("remove"));
            itemEntry.appendChild(linkEl);
            
            itemEntry.appendChild(document.createTextNode(", "));
            
            linkEl = document.createElement("a");
            linkEl.setAttribute("href", "javascript:userEditItemStart(\""+item.getAttribute("id")+"\");");
            linkEl.appendChild(document.createTextNode("edit"));
            itemEntry.appendChild(linkEl);
            
            itemLink.setAttribute("href", xmlGetElementText(item, "res"));
        }
        var itemText = document.createTextNode(useFiles ? item.firstChild.nodeValue : xmlGetElementText(item, "title"));
        itemLink.appendChild(itemText);
        itemsEl.appendChild(itemEntry);
    }
    itemRoot.replaceChild(itemsEl, itemRoot.firstChild);
    
}

function addItem(itemId)
{
    var url = link("add", {object_id: itemId});
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
    itemRoot=$('item_add_edit_div');
    Element.show(itemRoot);
}

function userEditItemStart(objectId)
{
    var url = link("edit_load", {object_id: objectId});
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
    itemRoot=$('item_add_edit_div');
    Element.show(itemRoot);
}

function updateItemAddEditFields(editItem)
{
    var currentTypeOption;
    var form = document.forms['addEditItem'];
    var selectEl = form.elements['objType'];
    var submitEl = form.elements['submit'];
    if (!editItem)
    {
        selectEl.disabled = false;
        submitEl.value = 'Add item...';
        currentTypeOption = selectEl.value;
        if (!currentTypeOption) currentTypeOption = '1';
        form.action = 'javascript:itemAddEditSubmit();';
    }
    else
    {
        selectEl.disabled = true;
        submitEl.value = 'Update item...';
        currentTypeOption = xmlGetElementText(editItem, 'objType');
        var objectId = editItem.getAttribute('object_id');
        selectEl.value = currentTypeOption;
        form.action = 'javascript:itemAddEditSubmit('+objectId+');';
    }
    
    var inputsDiv = document.createElement('div');
    
    if (!selectEl.options[0])
    {
        // ATTENTION: These values need to be changed in src/cds_objects.h too.
        // Note: 'Active Item', 'External Link (URL)', 'Internal Link (Local URL)'
        // are also 'Items', so they have the item flag set too.
        var objTypeOptions = new Array('Container', 'Item', 'Active Item', 'External Link (URL)', 'Internal Link (Local URL)');
        var objTypeOptionsIds = new Array('1', '2', '6', '10', '18');
        
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
    else if (currentTypeOption == '18')
    {
        fieldAr = new Array('Title', 'URL', 'Class', 'Description', 'Mimetype');
        fieldNameAr = new Array('title', 'location', 'class', 'description', 'mime-type');
        defaultsAr = new Array('', '', 'object.item', '', '');
    }
    
    if (fieldAr && defaultsAr)
    {
        for (var i = 0; i < fieldAr.length; ++i)
        {
            inputsDiv.appendChild(document.createTextNode(fieldAr[i]+": "));
            var inputEl = document.createElement('input');
            inputEl.setAttribute('type', 'text');
            inputEl.setAttribute('name', fieldNameAr[i]);
            if (!editItem)
                inputEl.setAttribute('value', defaultsAr[i]);
            else
                inputEl.setAttribute('value', xmlGetElementText(editItem, fieldNameAr[i]));
            inputsDiv.appendChild(inputEl);
            addBr(inputsDiv);
        }
    }
    
    var itemRoot = $('item_add_edit_inputs');
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
    var form = document.forms['addEditItem'];
    for (var i = 0; i < form.length; ++i)
    {
        var element = form.elements[i];
        args[element.name] = element.value;
    }
    
    var url = link(req_type, args);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: addedItem
        });
}

function removeItem(itemId)
{
    var url = link('remove', {object_id: itemId});
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: removedItem
        });
}

function removedItem(ajaxRequest)
{
    var xml = ajaxRequest.responseXML;
    if (!errorCheck(xml)) return;
    
    //todo: feedback, reload?
}
