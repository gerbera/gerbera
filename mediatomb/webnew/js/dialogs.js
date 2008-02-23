/*MT_J*
    
    MediaTomb - http://www.mediatomb.cc/
    
    dialogs.js - this file is part of MediaTomb.
    
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

MT.dialogs.EditItem = function(){
    var dialog;
	var form;
	
	var object_id;

    return {
        init : function() {
			form = new Ext.form.Form({
				labelWidth: 75,
				method: 'GET'
			});
			
			form.add(
		        new Ext.form.TextField({
		            fieldLabel: 'Title',
		            name: 'title',
		            width: 200
		        }),
		
		        new Ext.form.TextField({
		            fieldLabel: 'Location',
		            name: 'location',
		            width: 200
		        }),
		
		        new Ext.form.TextField({
		            fieldLabel: 'Class',
		            name: 'class',
		            width: 200,
					allowBlank: false
		        }),
		
		        new Ext.form.TextField({
		            fieldLabel: 'Description',
		            name: 'description',
		            width: 200
		        }),
		
		        new Ext.form.TextField({
		            fieldLabel: 'Mimetype',
		            name: 'mime-type',
		            width: 200
		        })
		    );
		
		    form.render('form-editItem');
        },
        showDialog : function(id) {
			object_id = id;
			
			if(id) {
				Ext.Ajax.request({
					method: 'GET',
					url: MT.tools.generateURL('edit_load'),
					success: this.onLoadSuccess,
					failure: this.onLoadFailure,
					params: { object_id: id }
				});
				
				if(!dialog){ // lazy initialize the dialog and only create it once
	                dialog = new Ext.BasicDialog("dlg-editItem", {
						autoTabs: false,
						width: 320,
						height: 240,
						resizable: false,
						shadow: true,
						minWidth: 300,
						minHeight: 250,
						proxyDrag: true,
						modal: false
	                });
					
	                dialog.addKeyListener(27, dialog.hide, dialog);
	                dialog.addButton('Update item', this.onClickUpdateIem, dialog);
	                dialog.addButton('Cancel', dialog.hide, dialog);
	            }
			}
        },
		onClickUpdateIem : function() {
			Ext.Ajax.request({
					method: 'GET',
					url: MT.tools.generateURL('edit_save', { object_id: object_id }),
					success: MT.dialogs.EditItem.onSaveItemSuccess,
					failure: MT.dialogs.EditItem.onSaveItemFailure,
					params: form.getValues()
			});
		},
		onLoadSuccess : function(response) {
			var xml = response.responseXML;
			
			if(!MT.tools.xmlErrorCheck(xml)) {
				this.onLoadFailure();
				return;
			}
			
			var rootEl = MT.tools.xmlGetElement(xml, "root");
			var itemEl = MT.tools.xmlGetElement(xml, "item");
			
			var title = MT.tools.xmlGetElement(itemEl, 'title');
			var object_class = MT.tools.xmlGetElement(itemEl, 'class');
			var description = MT.tools.xmlGetElement(itemEl, 'description');
			var location = MT.tools.xmlGetElement(itemEl, 'location');
			var mimetype = MT.tools.xmlGetElement(itemEl, 'mime-type');
			
			form.findField('title').setValue(MT.tools.xmlGetText(title));
			form.findField('class').setValue(MT.tools.xmlGetText(object_class));
			form.findField('description').setValue(MT.tools.xmlGetText(description));
			form.findField('location').setValue(MT.tools.xmlGetText(location));
			form.findField('mime-type').setValue(MT.tools.xmlGetText(mimetype));
			
			if(MT.tools.xmlGetAttribute(title, 'editable') == '0') {
				form.findField('title').disable();
			}
			if(MT.tools.xmlGetAttribute(object_class, 'editable') == '0') {
				form.findField('class').disable();
			}
			if(MT.tools.xmlGetAttribute(description, 'editable') == '0') {
				form.findField('desciption').disable();
			}
			if(MT.tools.xmlGetAttribute(location, 'editable') == '0') {
				form.findField('location').disable();
			}
			if(MT.tools.xmlGetAttribute(mimetype, 'editable') == '0') {
				form.findField('mimetype').disable();
			}
			
			dialog.show();
		},
		onLoadFailure : function(response) {
			Ext.MessageBox.alert('Error', 'Could not edit item. :-(');
		},
		onSaveItemSuccess : function(response) {
			MT.layout.DataGrid.loadData();
			
			dialog.hide();
		},
		onSaveItemFailure : function(response) {
			Ext.MessageBox.alert('Error', 'Could not save item. :-(');
		}
    };
}();

Ext.onReady(MT.dialogs.EditItem.init, MT.dialogs.EditItem, true);
