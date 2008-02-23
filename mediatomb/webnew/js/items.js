/*MT_J*
    
    MediaTomb - http://www.mediatomb.cc/
    
    items.js - this file is part of MediaTomb.
    
    Copyright (C) 2007-2008 Jan Habermann <jan.habermann@gmail.com>
    
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
