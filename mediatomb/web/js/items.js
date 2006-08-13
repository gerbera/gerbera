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
    if (true || ! items)
    {
        items = xmlGetElement(xml, "files");
        if (true || ! items)
        {
            alert("no items or files tag found");
            //DEBUG:
            alert(items);
            //alert(ajaxRequest.responseText);
            var itemsEl = document.createElement("div");
            itemsEl.appendChild(document.createTextNode(ajaxRequest.responseText));
            itemRoot.replaceChild(itemsEl, itemRoot.firstChild);
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
            
            itemLink.setAttribute("href", xmlGetElementText(item, "res"));
        }
        var itemText = document.createTextNode(useFiles ? item.firstChild.nodeValue : xmlGetElementText(item, "dc:title"));
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
    updateItemAddFields();
    Element.hide(itemRoot);
    itemRoot=$('item_add_div');
    Element.show(itemRoot);
}

function updateItemAddFields()
{
    var currentTypeOption;
    currentTypeOption = document.forms['addItem'].elements['objType'].value;
    if (!currentTypeOption) currentTypeOption = '1';
    
    var inputsDiv = document.createElement('div');
    
    /*var formEl = document.createElement('form');
    formEl.setAttribute('name', 'addItem');
    formEl.setAttribute('action', 'javascript:itemAddSubmit();');
    
    var selectEl = document.createElement('select');
    selectEl.setAttribute('onchange', 'updateItemAddFields()');
    selectEl.setAttribute('name', 'objType');
    selectEl.setAttribute('size', '1');
    formEl.appendChild(selectEl);
    */
    
    var selectEl = document.forms['addItem'].elements['objType'];
    
    //formEl.appendChild(document.createTextNode("<select name='objType' size='1' onchange='updateItemAddFields();'></select>");
    
    if (!selectEl.options[0])
    {
        // ATTENTION: These values need to be changed in src/cds_objects.h too.
        var objTypeOptions = new Array('Container', 'Item', 'Active Item', 'External Link (URL)', 'Internal Link (Local URL)');
        var objTypeOptionsIds = new Array('1', '2', '4', '8', '16');
    
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
    else if (currentTypeOption == '4')
    {
        fieldAr = new Array('Title', 'Location', 'Class', 'Description', 'Mimetype', 'Action Script', 'State');
        fieldNameAr = new Array('title', 'location', 'class', 'description', 'mime-type', 'action', 'state');
        defaultsAr = new Array('', '', 'object.item.activeItem', '', '', '', '');
    }
    else if (currentTypeOption == '8' || currentTypeOption == '16')
    {
        fieldAr = new Array('Title', 'URL', 'Protocol', 'Class', 'Description', 'Mimetype');
        fieldNameAr = new Array('title', 'location', 'protocol', 'class', 'description', 'mime-type');
        defaultsAr = new Array('', '', 'http-get', 'object.item', '', '');
    }
    
    
    if (fieldAr && defaultsAr)
    {
        for (var i = 0; i < fieldAr.length; ++i)
        {
            inputsDiv.appendChild(document.createTextNode(fieldAr[i]+": "));
            var inputEl = document.createElement('input');
            inputEl.setAttribute('type', 'text');
            inputEl.setAttribute('name', fieldNameAr[i]);
            inputEl.setAttribute('value', defaultsAr[i]);
            inputsDiv.appendChild(inputEl);
            addBr(inputsDiv);
        }
    }
   
    /*var submitEl = document.createElement('input');
    submitEl.setAttribute('type', 'submit');
    
    formEl.appendChild(submitEl);*/
    
    //addItemDiv.appendChild(formEl);
    //var objectTypeDiv = document.createElement('div');
    //addItemDiv.appendChild(objectTypeDiv);

    var itemRoot = $('item_add_inputs');
    itemRoot.replaceChild(inputsDiv, itemRoot.firstChild);
}

function itemAddSubmit()
{
    var form = document.forms['addItem'];
    var args = new Object();
    args["object_id"] = lastNodeDb.substr(1);
    for (var i = 0; i < form.length; ++i)
    {
        var element = form.elements[i];
        args[element.name] = element.value;
    }
    
    var url = link("add_object", args);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: addedItem
        });
}

function removeItem(itemId)
{
    var url = link("remove", {object_id: itemId});
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
