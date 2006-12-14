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

