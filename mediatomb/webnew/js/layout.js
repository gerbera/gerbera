/*MT_J*
    
    MediaTomb - http://www.mediatomb.cc/
    
    layout.js - this file is part of MediaTomb.
    
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
Ext.namespace('MT', 'MT.layout', 'MT.tools', 'MT.trees', 'MT.session', 'MT.items', 'MT.dialogs');

Ext.apply(Ext, {
	BLANK_IMAGE_URL : "images/blank.gif"
});

MT.layout.Layout = function() {
    var layout;
    return {
        init : function(){
           layout = new Ext.BorderLayout(document.body, {
                north: {
                    split: false,
                    titlebar: false
                },
                west: {
                    split:true,
                    initialSize: 250,
                    minSize: 175,
                    maxSize: 400,
                    titlebar: false,
                    collapsible: false,
					autoScroll: true,
                    animate: true
                },
                center: {
                    titlebar: false,
                    autoScroll:true,
                    closeOnTab: true
                }
            });

            layout.beginUpdate();
			
            layout.add('north', new Ext.ContentPanel('top-bar', 'Mediatomb'));
			
            var contentLayout = new Ext.BorderLayout('content', {
				north: {
                    split: false,
                    titlebar: true
                },
                center: {
                    autoScroll: true,
					fitToFrame: true
                }
            });
			
			var fsTreeLayout = new Ext.BorderLayout('navFs', {
				north: {
                    split: false,
                    titlebar: true
                },
                center: {
                    autoScroll: true,
					fitToFrame: true
                }
            });
			
			var dbTreeLayout = new Ext.BorderLayout('navDb', {
				north: {
                    split: false,
                    titlebar: true
                },
                center: {
                    autoScroll: true,
					fitToFrame: true
                }
            });
			
			fsTreeLayout.add('north', new Ext.ContentPanel('tree-fs-toolbar-panel', 'Filesystem'));
			dbTreeLayout.add('north', new Ext.ContentPanel('tree-db-toolbar-panel', 'Database'));
			fsTreeLayout.add('center', new Ext.ContentPanel('nav-fs'));
            dbTreeLayout.add('center', new Ext.ContentPanel('nav-db'));

			contentLayout.add('north', new Ext.ContentPanel('grid-toolbar-panel', 'Content Panel'));
			contentLayout.add('center', new Ext.ContentPanel('main-panel'));
			
			layout.add('west', new Ext.NestedLayoutPanel(fsTreeLayout, 'Filesystem'));
			layout.add('west', new Ext.NestedLayoutPanel(dbTreeLayout, 'Database'));
            layout.add('center', new Ext.NestedLayoutPanel(contentLayout));
			
			layout.getRegion('west').showPanel('navFs');
            layout.getRegion('center').showPanel('content');
			
			layout.getRegion('west').on('resized', function(region, newSize) {
				MT.layout.DataGrid.getGrid().autoSize();
			});
			
            layout.endUpdate();
       }
 };
   
}();

Ext.EventManager.onDocumentReady(MT.layout.Layout.init, MT.layout.Layout, true);
