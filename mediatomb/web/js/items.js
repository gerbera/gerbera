var itemRoot;

function itemInit()
{
    var newNodeDb = document.createElement('div');
    var newNodeFs = document.createElement('div');
    var newNodeAdd = document.createElement('div');
    $('item_db_div').appendChild(newNodeDb);
    $('item_fs_div').appendChild(newNodeFs);
    $('item_add_div').appendChild(newNodeAdd);
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
            alert(ajaxRequest.responseText);
            return;
        }
        useFiles = true;
        childrenTag = "file";
    }
    
    var children = items.getElementsByTagName(childrenTag);
    var items = document.createElement("div");
    
    for (var i = 0; i < children.length; i++)
    {
        var item = children[i];
        var itemEntry = document.createElement("div");
        var itemLink = document.createElement("a");
        itemEntry.appendChild(itemLink);
        
        if (useFiles)
        {
            itemEntry.appendChild(document.createTextNode(" - "));
            var buttons = document.createElement("a");
            buttons.setAttribute("href", "javascript:addItem(\""+item.getAttribute("id")+"\");");
            buttons.appendChild(document.createTextNode("add"));
            itemEntry.appendChild(buttons);
        }
        else
        {
            itemLink.setAttribute("href", xmlGetElementText(item, "res"));
            
        }
        var itemText = document.createTextNode(useFiles ? item.firstChild.nodeValue : xmlGetElementText(item, "dc:title"));
        itemLink.appendChild(itemText);
        items.appendChild(itemEntry);
    }
    itemRoot.replaceChild(items, itemRoot.firstChild);
    
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
    if (document.forms['addItem'] && document.forms['addItem'].elements['objType'])
        currentTypeOption = document.forms['addItem'].elements['objType'].value;
    else
        currentTypeOption = '1';
    
    var addItemDiv = document.createElement('div');
    
    var formEl = document.createElement('form');
    addItemDiv.appendChild(formEl);
    formEl.setAttribute('name', 'addItem');
    formEl.setAttribute('action', 'javascript:itemAddSubmit();');
    var selectEl = document.createElement('select');
    formEl.appendChild(selectEl);
    selectEl.setAttribute('name', 'objType');
    selectEl.setAttribute('size', '1');
    selectEl.setAttribute('onChange', 'updateItemAddFields()');
    
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
    
    addBr(formEl);
    
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
        fieldNameAr = new Array('title', 'location', 'class', 'description', 'mimetype');
        defaultsAr = new Array('', '', 'object.item', '', '');
    }
    else if (currentTypeOption == '4')
    {
        fieldAr = new Array('Title', 'Location', 'Class', 'Description', 'Mimetype', 'Action Script', 'State');
        fieldNameAr = new Array('title', 'location', 'class', 'description', 'mimetype', 'script', 'state');
        defaultsAr = new Array('', '', 'object.item.activeItem', '', '', '', '');
    }
    else if (currentTypeOption == '8' || currentTypeOption == '16')
    {
        fieldAr = new Array('Title', 'URL', 'Protocol', 'Class', 'Description', 'Mimetype');
        fieldNameAr = new Array('title', 'location', 'protocol', 'class', 'description', 'mimetype');
        defaultsAr = new Array('', '', 'http-get', 'object.item', '', '');
    }
    
    if (fieldAr && defaultsAr)
    {
        for (var i = 0; i < fieldAr.length; ++i)
        {
            formEl.appendChild(document.createTextNode(fieldAr[i]+": "));
            var inputEl = document.createElement('input');
            formEl.appendChild(inputEl);
            inputEl.setAttribute('type', 'text');
            inputEl.setAttribute('name', fieldNameAr[i]);
            inputEl.setAttribute('value', defaultsAr[i]);
            addBr(formEl);
        }
    }
    
    
    var submitEl = document.createElement('input');
    formEl.appendChild(submitEl);
    submitEl.setAttribute('type', 'submit');
    
    var objectTypeDiv = document.createElement('div');
    addItemDiv.appendChild(objectTypeDiv);
    
    var itemRoot = $('item_add_div');
    itemRoot.replaceChild(addItemDiv, itemRoot.firstChild);
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

