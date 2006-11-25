var autoscanAddId;

function changeAutoscanDirectory(doAction, objId)
{
    if (doAction == 'add')
    {
        autoscanAddId = objId;
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
    formToArray(form, args);
    var url = link('autoscan', args);
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: changeAutoscanCallback
        });
}

