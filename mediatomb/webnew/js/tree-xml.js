/*MT_J*
    
    MediaTomb - http://www.mediatomb.cc/
    
    tree-xml.js - this file is part of MediaTomb.
    
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

Ext.tree.TreeLoaderXML = function(config) {
	this.baseParams = {};
	this.requestMethod = "GET"; 
    Ext.apply(this, config);
    
    this.events = {
        "beforeload" : true,
        "load" : true,
        "loadexception" : true
    };
};
Ext.extend(Ext.tree.TreeLoaderXML, Ext.tree.TreeLoader, {
	getParams: function(node){
        var buf = [], bp = this.baseParams;
        for(var key in bp){
            if(typeof bp[key] != "function"){
                buf.push(encodeURIComponent(key), "=", encodeURIComponent(bp[key]), "&");
            }
        }
        buf.push("parent_id=", encodeURIComponent(node.id));
        return buf.join("");
    },
	processResponse : function(response, node, callback) {
        try {
			var o = this.parseResponseXML(response.responseXML);
			for(var i = 0, len = o.length; i < len; i++) {
				var n = this.createNode(o[i]);
				if(n) {
					node.appendChild(n);
				}
			}
			if(typeof callback == "function") {
				callback(this, node);
			}
		} catch(e) {
			this.handleFailure(response);
		}
	},
	parseResponseXML : function(responseXML) {
		var o = [];
		
		var rootEl = MT.tools.xmlGetElement(responseXML, 'containers');
		var els = rootEl.getElementsByTagName('container');
		
		for(var i = 0, len = els.length; i < len; i++) {
			o.push({
				id: MT.tools.xmlGetAttribute(els[i], 'id'),
				text: MT.tools.xmlGetText(els[i]),
				leaf: (MT.tools.xmlGetAttribute(els[i], 'childCount')) > 0 ? false : true
			});
		}
		
		return o;
	}
});

// Extens tree with auto-refresh

Ext.override(Ext.tree.AsyncTreeNode, {
    startAutoRefresh : function(interval, refreshNow) {
        if(refreshNow) {
            this.reload();
        }
        if(this.autoRefreshProcId) {
            clearInterval(this.autoRefreshProcId);
        }
        this.autoRefreshProcId = setInterval(this.reload.createDelegate(this), interval*1000);
    },
    stopAutoRefresh : function() {
        if(this.autoRefreshProcId) {
            clearInterval(this.autoRefreshProcId);
        }
    }
});
