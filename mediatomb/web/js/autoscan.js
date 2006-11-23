function changeAutoscanDirectory(doAction, objId)
{
    var url = link('autoscan', {action: doAction, object_id: objId}, true);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: changeAutoscanCallback
        });
}

function changeAutoscanCallback(ajaxRequest)
{
    errorCheck(ajaxRequest.responseXML);
    folderChange(selectedNode);
}

