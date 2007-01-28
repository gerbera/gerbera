/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    auth.js - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.org>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.org>,
                            Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

function authenticate()
{
    
    // fetch authentication token
    var url = link('auth');
    
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: gotToken
        });
}

function gotToken(ajaxRequest)
{
    var xml = ajaxRequest.responseXML;
    if (!errorCheck(xml)) return;
    
    var rootEl = xmlGetElement(xml, "root");
    
    var token = xmlGetElementText(rootEl, "token");
    SID = xmlGetAttribute(rootEl, "sid");
    
    if (!SID)
        alert('could not obtain session id');
    else
        setCookie("SID", SID);
    
    var username = frames["topF"].document.login_form.username.value;
    var password = frames["topF"].document.login_form.password.value;
    
    // create authentication password
    password = hex_md5(token + password);
    // try to login
    var url = link('auth', {auth: 1, username: username, password: password});
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: checkLogin
        });
}

function checkLogin(ajaxRequest)
{
    var xml = ajaxRequest.responseXML;
    if (!errorCheck(xml)) return;
    
    var topDocument = frames["topF"].document;
    Element.hide(topDocument.getElementById("loginDiv"));
    Element.show(topDocument.getElementById("topDiv"));
    Element.show(frames["leftF"].document.getElementById("treeDiv"));
    Element.show(frames["topleftF"].document.getElementById("db_fs_selector"));
    loggedIn = true;
    initLoggedIn();
    updateTreeAfterLogin();
}

function checkSID()
{
    var url = link('auth', {checkSID: 1});
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            asynchronous: false,
            onComplete: checkSIDcallback
        });
}

function checkSIDcallback(ajaxRequest)
{
    var xml = ajaxRequest.responseXML;
    errorCheck(xml, true);
    var rootEl = xmlGetElement(xml, "root");
    var newSID = xmlGetAttribute(rootEl, "sid");
    if (newSID)
    {
        SID = newSID;
        setCookie("SID", SID);
    }
    var accountsStr = xmlGetElementText(rootEl, "accounts");
    ACCOUNTS = (accountsStr && accountsStr == "1");
    
    var configEl = xmlGetElement(rootEl, "config");
    if (configEl)
    {
        pollIntervalTime = xmlGetAttribute(configEl, "poll-interval") * 1000;
        pollWhenIdle = (xmlGetAttribute(configEl, "poll-when-idle") == "yes");
        if (pollWhenIdle)
            startPollInterval();
    }
}

function logout()
{
    var url = link('auth', {logout: 1});
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            asynchronous: false,
            onComplete: handleLogout
        });
}

function handleLogout(ajaxRequest)
{
    errorCheck(ajaxRequest.responseXML);
}
