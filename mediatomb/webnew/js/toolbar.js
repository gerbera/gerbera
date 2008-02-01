/*MT_J*/
MT.layout.TreeFsToolbar = function() {
	var toolbar;
	var menu;
	
	var refreshButton;
	
	return {
        init : function() {
			Ext.QuickTips.init();
		
		    menu = new Ext.menu.Menu({
		        id: 'treeFsMenu'
		    });
		
		    toolbar = new Ext.Toolbar('tree-fs-toolbar');
				
			refreshButton = new Ext.Toolbar.Button({
		        cls: 'x-btn-icon',
				iconCls: 'button-refresh',
		        tooltip: '<b>Refresh</b><br/>Forces the tree to refresh.',
				handler: this.onClickRefreshButton
		    });
				
			toolbar.add(refreshButton);
			
			// this.disableButtons();
		},
		getRefreshButton : function() {
			return refreshButton;
		},
		disableButtons : function() {
			getRefreshButton.disable();
		},
		onClickRefreshButton : function() {
			MT.trees.dbTree.getTree().getRootNode().reload();
		}
	};
}();

MT.layout.TreeDbToolbar = function() {
	var toolbar;
	var menu;
	
	var refreshButton;
	
	return {
        init : function() {
			Ext.QuickTips.init();
		
		    menu = new Ext.menu.Menu({
		        id: 'treeDbMenu'
		    });
		
		    toolbar = new Ext.Toolbar('tree-db-toolbar');
				
			refreshButton = new Ext.Toolbar.Button({
		        cls: 'x-btn-icon',
				iconCls: 'button-refresh',
		        tooltip: '<b>Refresh</b><br/>Forces the tree to refresh.',
				handler: this.onClickRefreshButton
		    });
				
			toolbar.add(refreshButton);
			
			// this.disableButtons();
		},
		getRefreshButton : function() {
			return refreshButton;
		},
		disableButtons : function() {
			getRefreshButton.disable();
		},
		onClickRefreshButton : function() {
			MT.trees.dbTree.getTree().getRootNode().reload();
		}
	};
}();

MT.layout.GridToolbar = function() {
	var toolbar;
	var menu;
	
	var addButton;
	var editButton;
	var removeButton;
	
	return {
        init : function() {
			Ext.QuickTips.init();
		
		    menu = new Ext.menu.Menu({
		        id: 'gridMenu'
		    });
		
		    toolbar = new Ext.Toolbar('grid-toolbar');
				
			addButton = new Ext.Toolbar.Button({
		        cls: 'x-btn-icon',
				iconCls: 'button-add',
		        tooltip: '<b>Add</b><br/>Adds the selection to the database.',
				handler: this.onClickAddButton
		    });
			
			editButton = new Ext.Toolbar.Button({
				cls: 'x-btn-icon',
				iconCls: 'button-edit',
		        tooltip: '<b>Tooltip</b><br/>Edit',
				handler: this.onClickEditButton
			});
			
			removeButton = new Ext.Toolbar.Button({
				cls: 'x-btn-icon',
				iconCls: 'button-remove',
		        tooltip: '<b>Tooltip</b><br/>Remove',
				handler: this.onClickRemoveButton
			});
						
			toolbar.addFill();
			toolbar.addText('<b>Actions</b> ');
			toolbar.addSeparator();
			
			toolbar.add(addButton);
			toolbar.addSeparator();
			toolbar.add(editButton);
			toolbar.addSeparator();
			toolbar.add(removeButton);
			
			this.disableButtons();
		},
		getAddButton : function() {
			return addButton;
		},
		getEditButton : function() {
			return editButton;
		},
		getRemoveButton : function() {
			return removeButton;
		},
		disableButtons : function() {
			addButton.disable();
			editButton.disable();
			removeButton.disable();
		},
		onClickAddButton : function() {
			var nodeIds = MT.layout.DataGrid.getSelectedNodeIds();
			if(nodeIds) {
				MT.items.addItems(nodeIds);
			}
		},
		onClickEditButton : function() {
			MT.dialogs.EditItem.showDialog(MT.layout.DataGrid.getSelectedNodeIds()[0]);
		},
		onClickRemoveButton : function() {
			var nodeIds = MT.layout.DataGrid.getSelectedNodeIds();
			if(nodeIds) {
				MT.items.removeItems(nodeIds);
			}
		}
	};
}();

Ext.EventManager.onDocumentReady(MT.layout.TreeFsToolbar.init, MT.layout.TreeFsToolbar, true);
Ext.EventManager.onDocumentReady(MT.layout.TreeDbToolbar.init, MT.layout.TreeDbToolbar, true);
Ext.EventManager.onDocumentReady(MT.layout.GridToolbar.init, MT.layout.GridToolbar, true);
