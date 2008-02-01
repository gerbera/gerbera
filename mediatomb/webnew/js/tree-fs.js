/*MT_J*/

MT.trees.fsTree = function() {
    var tree;
    return {
        init : function() {
			var Tree = Ext.tree;
    
			tree = new Tree.TreePanel('tree-fs', {
		        animate: true, 
		        loader: new Tree.TreeLoaderXML({ dataUrl: MT.tools.generateURL('directories', { select_it: 0 }) }),
		        enableDD: false,
				selModel: new Tree.DefaultSelectionModel(),
		        containerScroll: true,
				pathSeparator: ' > '
	    	});
	
		    // set the root node
		    var root = new Tree.AsyncTreeNode({
		        text: 'Filesystem',
		        draggable: false,
		        id:'0'
		    });
		    tree.setRootNode(root);
			
			// setup tree events
			tree.getSelectionModel().on({
		        selectionchange: { fn: this.onSelectionChange }
		    });
		
		    // render the tree
		    tree.render();
		    root.expand();
		},
		getTree : function() {
			return tree;
		},
		onSelectionChange : function(tree, node) {
			MT.layout.DataGrid.useFsGrid();
			MT.layout.DataGrid.setDataSourceURL(MT.tools.generateURL('files', { parent_id: node.id }));
			MT.layout.DataGrid.loadData({ parent_id: node.id, start: 0, limit: MT.session.defaultItemsPerPage });
			
			MT.layout.GridToolbar.disableButtons();
		}
	};
}();
