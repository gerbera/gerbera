/*MT_J*/

MT.items.addItems = function(itemIds) {
	if(itemIds && itemIds.length > 0) {
		for(var i = 0, len = itemIds.length; i < len; i++) {
			MT.items.addItem(itemIds[i]);
		}
	}
}

MT.items.addItem = function(itemId) {
	Ext.Ajax.request({
		method: 'GET',
		url: MT.tools.generateURL('add'),
		success: MT.items.onAddItemSuccess,
		failure: MT.items.onAddItemFailure,
		params: { object_id: itemId }
	});
}

MT.items.onAddItemSuccess = function(response) {
    var xml = response.responseXML;
    if(!MT.tools.xmlErrorCheck(xml)) {
		return;	
	}
    
    Ext.MessageBox.alert('Item Added', 'The item has been successfully added.');
}

MT.items.onAddItemFailure = function(response) {
	Ext.MessageBox.alert('Error', 'Could not add item. :-(');
}

MT.items.removeItems = function(itemIds, all) {
	if(itemIds && itemIds.length > 0) {
		for(var i = 0, len = itemIds.length; i < len; i++) {
			MT.items.removeItem(itemIds[i], all);
		}
	}
}

MT.items.removeItem = function(itemId, all) {
	if(itemId == '0') {
		Ext.MessageBox.alert("Root container cannot be removed!");
        return;
    }
    
    if(all) {
		all_send = '1';
	} else {
		all_send = '0';
	}
    
	Ext.Ajax.request({
		method: 'GET',
		url: MT.tools.generateURL('remove', {object_id: itemId, all: all_send}, true),
		success: MT.items.onRemoveItemSuccess,
		failure: MT.items.onRemoveItemFailure
	});
}

MT.items.onRemoveItemSuccess = function(response) {
	MT.layout.DataGrid.loadData();
	MT.layout.GridToolbar.disableButtons();
}

MT.items.onRemoveItemFailure = function(response) {
	Ext.MessageBox.alert('Error', 'Could not remove item. :-(');
}
