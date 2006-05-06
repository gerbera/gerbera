var itemRoot;

function itemInit()
{
    itemRoot = $('item_div');
    var newNode = document.createElement('div');
    itemRoot.appendChild(newNode);
}

function folderChange(id)
{
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
    if (!xml)
    {
        alert ("could not fetch XML");
        return;
    }
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
        var itemText = document.createTextNode(useFiles ? item.firstChild.nodeValue : xmlGetElementText(item, "dc:title"));
        itemEntry.appendChild(itemText);
        items.appendChild(itemEntry);
    }
    itemRoot.replaceChild(items, itemRoot.firstChild);
    
}

