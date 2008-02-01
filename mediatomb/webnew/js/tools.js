/*MT_J*/

MT.tools.generateURL = function(req_type, param, get_update_ids) {
	var url = '/content/interface?req_type=' + encodeURIComponent(req_type) + '&sid=' + MT.session.sid;
	
    if(param) {
		for (var key in param) {
            url += '&' + encodeURIComponent(key) + '=' + encodeURIComponent(param[key]);
        }
	}
	
    if(get_update_ids /*&& isTypeDb() && dbStuff.treeShown*/) {
        url += '&get_update_ids=1';
	}
	
    return url;
};

MT.tools.xmlErrorCheck = function(xml, noredirect) {
    if (!xml) {
        return false;
    }
    var rootEl = MT.tools.xmlGetElement(xml, 'root');
    
    var uiDisabled = MT.tools.xmlGetAttribute(rootEl, 'ui_disabled');
    if (uiDisabled) {
        window.location = "/disabled.html";
        return false;
    }
    
    var redirect = MT.tools.xmlGetElementText(xml, 'redirect');
    if(redirect) {
        var now = new Date();
        var expire = new Date();
        expire.setTime(now.getTime() - 3600000 * 24 * 360);
        MT.session.cookieProvider.set('SID', null, expire);
        if (MT.session.sid && !noredirect) {
            MT.session.sid = null;
            window.location = redirect;
        }
        return false;
    }
    
    var error = MT.tools.xmlGetElementText(xml, 'error');
    if (error) {
        Ext.Ext.MessageBox.alert('Error', error);
        return false;
    }
    return true;
}

MT.tools.xmlGetElement = function(parent, name) {
	var returnVal = parent.getElementsByTagName(name);
    
    if (!returnVal || !returnVal.length) {
        return null;
	} else {
        return returnVal[0];
	}
};

MT.tools.xmlGetElementText = function(parent, name) {
    var el = MT.tools.xmlGetElement(parent, name);
    return MT.tools.xmlGetText(el);
};

MT.tools.xmlGetText = function(el) {
    if (!el || el.childNodes.length != 1) {
        return "";
	}
    return el.firstChild.nodeValue;
};

MT.tools.xmlGetAttribute = function(parent, name) {
    var a = parent.attributes;
    if (! a) {
        return null;
	}
    for (var i = 0; i < a.length; i++) {
        if (a[i].name == name) {
            return a[i].value;
		}
    }
    return null;
};
