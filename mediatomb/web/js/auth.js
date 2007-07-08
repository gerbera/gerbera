/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    auth.js - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
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
    
    var username = frames["rightF"].document.login_form.username.value;
    var password = frames["rightF"].document.login_form.password.value;
    
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
    var rightDocument = frames["rightF"].document;
    var leftDocument = frames["leftF"].document;
    Element.hide(rightDocument.getElementById("loginDiv"));
    Element.hide(leftDocument.getElementById("leftLoginDiv"));
    //Element.show(topDocument.getElementById("topDiv"));
    Element.show(topDocument.getElementById("statusDiv"));
    Element.show(frames["leftF"].document.getElementById("treeDiv"));
    Element.show(frames["topleftF"].document.getElementById("db_fs_selector"));
    if (ACCOUNTS)
        Element.show(topDocument.getElementById("logout_link"));
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

function getConfig()
{
    var url = link('auth', {getConfig: 1});
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            asynchronous: false,
            onComplete: getConfigCallback
        });
}

function getConfigCallback(ajaxRequest)
{
    var xml = ajaxRequest.responseXML;
    errorCheck(xml, true);
    var rootEl = xmlGetElement(xml, "root");
    var configEl = xmlGetElement(rootEl, "config");
    if (configEl)
    {
        pollIntervalTime = xmlGetAttribute(configEl, "poll-interval") * 1000;
        pollWhenIdle = (xmlGetAttribute(configEl, "poll-when-idle") == "yes");
        if (pollWhenIdle)
            startPollInterval();
        var itemsPerPageOptionsEl = xmlGetElement(configEl, "items-per-page");
        if (itemsPerPageOptionsEl)
        {
            var newDefaultViewItems = xmlGetAttribute(itemsPerPageOptionsEl, "default");
            var options = itemsPerPageOptionsEl.getElementsByTagName("option");
            var newItemOptions = new Array();
            var newViewItemsMin = -1;
            var defaultViewItemsFound = false;
            var currentViewItemsFound = false;
            for (var i = 0; i < options.length; i++)
            {
                var itemOption = options[i].firstChild.nodeValue;
                newItemOptions[i] = itemOption;
                if (itemOption == newDefaultViewItems)
                    defaultViewItemsFound = true;
                if (viewItems != -1 && itemOption == viewItems)
                    currentViewItemsFound = true;
                if (newViewItemsMin == -1 || itemOption < newViewItemsMin)
                    newViewItemsMin = itemOption;
            }
            if (defaultViewItemsFound)
            {
                itemOptions = newItemOptions;
                viewItemsMin = newViewItemsMin;
                if (! currentViewItemsFound)
                    viewItems = newDefaultViewItems;
            }
        }
        var haveInotify = (xmlGetAttribute(configEl, "have-inotify") == "1");
        if (haveInotify)
        {
            Element.show(frames["rightF"].document.getElementById("scan_mode_inotify"));
            Element.show(frames["rightF"].document.getElementById("scan_mode_inotify_label"));
        }
    }
}
