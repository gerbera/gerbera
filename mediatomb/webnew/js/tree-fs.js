/*MT_J*
    
    MediaTomb - http://www.mediatomb.cc/
    
    tree-fs.js - this file is part of MediaTomb.
    
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
