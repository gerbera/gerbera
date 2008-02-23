/*MT_J*
    
    MediaTomb - http://www.mediatomb.cc/
    
    session.js - this file is part of MediaTomb.
    
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

MT.session.sid = null;
MT.session.token = null;

MT.session.activeGrid = 'fs';

MT.session.pollWhenIdle = null;
MT.session.pollInterval = null;
MT.session.haveInotify = null;
MT.session.defaultItemsPerPage = null;
MT.session.itemsPerPage = null;

MT.session.cookieProvider = new Ext.state.CookieProvider({
	expires: new Date(new Date().getTime()+(1000*60*60*24)) // 24 hours
});

Ext.state.Manager.setProvider(MT.session.cookieProvider);


// global Ajax request options

Ext.Ajax.on('beforerequest', function(ajax, options) {
	if(Ext.isReady) {
		Ext.get('status-div').addClass('loading');
	}
	
	if(options && options.params) {
		Ext.apply(options.params, { count: options.params.limit });
		delete options.params.limit;
	}
});

Ext.Ajax.on('requestcomplete', function(ajax, response, options) {
	if(Ext.isReady) {
		Ext.get('status-div').removeClass('loading');
	}
});


MT.session.start = function() {
    // authentication

	Ext.Ajax.request({
		method: 'GET',
		url: MT.tools.generateURL('auth'),
		success: MT.session.onAuthSuccess,
		failure: MT.session.onAuthFailure,
		params: { checkSID: 1 }
	});
}

MT.session.onSessionStarted = function() {
	// start trees
	
	Ext.EventManager.onDocumentReady(MT.trees.dbTree.init, MT.trees.dbTree, true);
	Ext.EventManager.onDocumentReady(MT.trees.fsTree.init, MT.trees.fsTree, true);
	Ext.EventManager.onDocumentReady(MT.layout.DataGrid.init, MT.layout.DataGrid, true);
}

MT.session.onAuthSuccess = function(response) {
    var xml = response.responseXML;
	
    if(!MT.tools.xmlErrorCheck(xml)) {
		return;
	}
    
    var rootEl = MT.tools.xmlGetElement(xml, "root");
    
    MT.session.token = MT.tools.xmlGetElementText(rootEl, "token");
    MT.session.sid = MT.tools.xmlGetAttribute(rootEl, "sid");
    
    if (!MT.session.sid) {
		Ext.MessageBox.alert('Error', 'Could not obtain session id.');
	} else {
        MT.session.cookieProvider.set('SID', MT.session.sid);
	}
	
	// get configurations
	
	Ext.Ajax.request({
		method: 'GET',
		url: MT.tools.generateURL('auth'),
		success: MT.session.onGetConfigSuccess,
		failure: MT.session.onGetConfigFailure,
		params: { getConfig: 1 }
	});
}

MT.session.onAuthFailure = function(response) {
	// TODO :: handle auth failure
}

MT.session.onGetConfigSuccess = function(response) {
	var xml = response.responseXML;
	
    if(!MT.tools.xmlErrorCheck(xml)) {
		return;
	}
    
    var rootEl = MT.tools.xmlGetElement(xml, "root");
	var configEl = MT.tools.xmlGetElement(xml, "config");
	var itemsEl = MT.tools.xmlGetElement(xml, "items-per-page");
	
	MT.session.pollWhenIdle = parseInt(MT.tools.xmlGetAttribute(configEl, "poll-when-idle"));
	MT.session.pollInterval = parseInt(MT.tools.xmlGetAttribute(configEl, "poll-interval"));
	MT.session.haveInotify = parseInt(MT.tools.xmlGetAttribute(configEl, "have-inotify"));
	MT.session.defaultItemsPerPage = parseInt(MT.tools.xmlGetAttribute(itemsEl, "default"));
	
	var els = itemsEl.getElementsByTagName('option');
	if(els.length > 0) {
		MT.session.itemsPerPage = new Array();
		
		for(var i = 0, len = els.length; i < len; i++) {
			MT.session.itemsPerPage.push([parseInt(MT.tools.xmlGetText(els[i])), MT.tools.xmlGetText(els[i])]);
		}	
	}
	
	MT.session.onSessionStarted();
}

MT.session.onGetConfigFailure = function(response) {
	// TODO :: handle config failure
}

