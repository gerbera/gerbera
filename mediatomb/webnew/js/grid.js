/*MT_J*
    
    MediaTomb - http://www.mediatomb.cc/
    
    grid.js - this file is part of MediaTomb.
    
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

// add enable/disable functions to paging toolbar
Ext.override(Ext.PagingToolbar, {
    // enable toolbar elements
    enable: function(disable) {
        // setup variables
        disable = disable ? true : false;
        var d = this.getPageData();
        var ap = d.activePage;
        var ps = d.pages;

        // mask/unmask
        if(disable) {
            this.getEl().mask();
        } 
        else {
            this.getEl().unmask();
        }

        this.first.setDisabled(!disable ? ap == 1 : disable);
        this.prev.setDisabled(!disable ? ap == 1 : disable);
        this.next.setDisabled(!disable ? ap == ps : disable);
        this.last.setDisabled(!disable ? ap == ps : disable);
        this.field.dom.disabled = disable;
        this.loading.disabled = disable;
        this.loading.el.disabled = disable;

        // an optional filter field
        if(this.filter) {
            this.filter.dom.disabled = disable;
        }
    }
    // disable toolbar components
    , disable: function() {
    	this.enable(true);
    }
});

MT.layout.DataGrid = function() {
	var grid;
    var dsFs;
	var dsFb;
	var cmFs;
	var cmDb;
	var paging;
	
    return {
        init : function() {
			// create the filesystem data store
			dsFs = new Ext.data.Store({
				id: 'fs',
		        proxy: new Ext.data.HttpProxy({
					method: 'GET',
					url: MT.tools.generateURL('files')
				}),
		        reader: new Ext.data.XmlReader({
							record: 'file',
							id: '@id'
						}, [ '/' ]
				)
			});
			
			// create the database data store
			dsDb = new Ext.data.Store({
				id: 'db',
		        proxy: new Ext.data.HttpProxy({
					method: 'GET',
					url: MT.tools.generateURL('items')
				}),
		        reader: new Ext.data.XmlReader({
							record: 'item',
							id: '@id',
							totalRecords: 'items@totalMatches'
						}, [ 'title', 'res', 'virtual' ]
				)
			});
	
			// create the filesystem column model
		    cmFs = new Ext.grid.ColumnModel([
				{ id: 'item', header: "Files", dataIndex: '/' }
			]);
			
			// create the database column model
			cmDb = new Ext.grid.ColumnModel([
				{ id: 'item', header: "Item", dataIndex: 'title' }
			]);
			
			// selection model
			var selModel = new Ext.grid.RowSelectionModel({singleSelect:false});
			
			selModel.on('selectionchange', this.onSelectionChange);
		    
		    // create the grid
		    grid = new Ext.grid.Grid('data-grid', {
				ds: dsFs,
		        cm: cmFs,
		        selModel: selModel,
				monitorWindowResize: true,
            	autoExpandColumn: 'item',
				autoExpandMax: 1600,
		        enableColLock: false,
				enableColumnHide: false,
				enableColumnMove: false,
		        loadMask: true
		    });
			
		    // render it
		    grid.render();
		
		    var gridFoot = grid.getView().getFooterPanel(true);
		
		    // add a paging toolbar to the grid's footer
		    paging = new Ext.PagingToolbar(gridFoot, dsDb, {
		        pageSize: MT.session.defaultItemsPerPage,
		        displayInfo: true,
		        displayMsg: 'Displaying items {0} - {1} of {2}',
		        emptyMsg: "No items to display"
		    });
			
			// add pageSize selection to paging toolbar
		    var ppstore = new Ext.data.SimpleStore({
		        fields: ['num', 'txt'],
		        data : MT.session.itemsPerPage
		    });
		
		    var perPage = new Ext.form.ComboBox({
		        store: ppstore,
		        displayField: 'txt',
		        typeAhead: true,
		        mode: 'local',
		        triggerAction: 'all',
		        selectOnFocus: true,
		        resizable: false,
		        listWidth: 50,
		        width: 50,
		        value: MT.session.defaultItemsPerPage
		    });
		
		    function setPerPage() {
				paging.pageSize = parseInt(perPage.getValue());
				MT.session.defaultItemsPerPage = paging.pageSize;

				paging.cursor = 0;
				grid.dataSource.load({params:{start: paging.cursor, limit: paging.pageSize}});
		    }
		    perPage.on("select", setPerPage);
		
			paging.addSeparator();
			paging.addText("Per Page");
			paging.addField(perPage);
			
			paging.disable();
		},
		getGrid : function() {
			return grid;
		},
		getPagingCursor : function() {
			return paging.cursor;
		},
		getPagingSize : function() {
			return paging.pageSize;
		},
		onSelectionChange : function(selModel) {
			var selections = selModel.getSelections();
			var count = selModel.getCount();
			
			if(count > 0) {
				if(grid.getDataSource().id == 'fs') {
					MT.layout.GridToolbar.getAddButton().enable();
				} else {
					MT.layout.GridToolbar.getAddButton().disable();
					MT.layout.GridToolbar.getRemoveButton().enable();
					
					if(count == 1) {
						MT.layout.GridToolbar.getEditButton().enable();
					} else {
						MT.layout.GridToolbar.getEditButton().disable();
					}
				}
			} else {
				MT.layout.GridToolbar.disableButtons();
			}
		},
		getSelectedNodeIds : function() {
			var sel = grid.getSelectionModel().getSelections();
			
			if(sel) {
				var a = [];
				
				for(var i = 0, len = sel.length; i < len; i++) {
					a.push(MT.tools.xmlGetAttribute(sel[i].node, 'id'));	
				}
				
				return a;
			} else {
				return null;
			}
		},
		setDataSourceURL : function(url) {
			grid.getDataSource().proxy.conn.url = url;
		},
		useFsGrid : function() {
			if(MT.session.activeGrid != 'fs') {
				grid.reconfigure(dsFs, cmFs);
				this.disablePaging();
				
				MT.session.activeGrid = 'fs';
			}
		},
		useDbGrid : function() {
			if(MT.session.activeGrid != 'db') {
				grid.reconfigure(dsDb, cmDb);
				this.enablePaging();
				
				MT.session.activeGrid = 'db';
			}
		},
		enablePaging : function() {
			paging.enable();
		},
		disablePaging : function() {
			paging.disable();
		},
		loadData : function(params) {
			var parent_id;
			
			if(!params) {
				var params = {};
			}
			if(MT.session.activeGrid == 'db') {
				parent_id = MT.trees.dbTree.getTree().getSelectionModel().getSelectedNode().id;
			} else {
				parent_id = MT.trees.fsTree.getTree().getSelectionModel().getSelectedNode().id;
			}
			Ext.applyIf(params, {
				parent_id: parent_id,
				start: paging.cursor,
				limit: paging.pageSize
			});

			grid.getDataSource().load({ params: params });
		}
	};
}();

