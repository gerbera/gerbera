/*MT*
    
    MediaTomb - http://www.mediatomb.org/
    
    tools.js - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>
    Copyright (C) 2006 Gena Batyan <bgeradz@mediatomb.org>,
                       Sergey Bostandzhyan <jin@mediatomb.org>,
                       Leonhard Wimmer <leo@mediatomb.org>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    $Id$
*/

function link(req_type, param, get_update_ids)
{
    var url = "/content/interface?req_type="+ req_type +"&sid="+ SID;
    if (param)
        for (var key in param)
        {
            url += "&" + key +"="+ param[key];
        }
    if (get_update_ids && isTypeDb() && dbStuff.treeShown)
        url += "&get_update_ids=1";
    return url;
}

function isTypeDb()
{
    return (TYPE == "db");
}

function xmlGetElement(parent, name)
{
    var returnVal = parent.getElementsByTagName(name)
    
    if (!returnVal || !returnVal.length)
        return null;
    else
        return returnVal[0];
}

function xmlGetElementText(parent, name)
{
    var el = xmlGetElement(parent, name);
    return xmlGetText(el);
}
function xmlGetText(el)
{
    if (!el || el.childNodes.length != 1)
        return null;
    return el.firstChild.nodeValue;
}

function xmlGetAttribute(parent, name)
{
    var a = parent.attributes;
    if (! a)
        return null;
    for (var i = 0; i < a.length; i++)
    {
        if (a[i].name == name)
            return a[i].value;
    }
    return null;
}

function errorCheck(xml, noredirect)
{
    if (!xml)
    {
        //alert ("could not fetch XML");
        return false;
    }
    var rootEl = xmlGetElement(xml, 'root');
    var uiDisabled = xmlGetAttribute(rootEl, 'ui_disabled');
    if (uiDisabled)
    {
        window.location = "/disabled.html";
        return false;
    }
    var redirect = xmlGetElementText(xml, 'redirect');
    if (redirect)
    {
        var now = new Date();
        var expire = new Date();
        expire.setTime(now.getTime() - 3600000 * 24 * 360);
        setCookie('SID', null, expire);
        if (SID)
        {
            SID = null;
            if (!noredirect) window.location = redirect;
        }
        return false;
    }
    
    var updateIDsEl = xmlGetElement(xml, 'updateIDs');
    if (updateIDsEl)
    {
        var updateIDStr = xmlGetText(updateIDsEl);
        var savedlastNodeDbID = lastNodeDb;
        var savedlastNodeDbIDParent;
        if (savedlastNodeDbID != 'd0')
            savedlastNodeDbIDParent = getTreeNode(savedlastNodeDbID).getParent().getID();
        //alert('before: lastid: ' + savedlastNodeDbID);
        //alert('before: node: ' + getTreeNode(savedlastNodeDbID));
        selectNode('d0');
        var updateAll = false;
        if (updateIDStr != 'all')
        {
            var updateIDsArr = updateIDStr.split(",");
            for (var i = 0; i < updateIDsArr.length; i++)
            {
                if (updateIDsArr[i] == 0)
                    updateAll = true;
            }
        }
        else
            updateAll = true;
        if (! updateAll)
        {
            for (var i = 0; i < updateIDsArr.length; i++)
            {
                var node = getTreeNode('d' + updateIDsArr[i]);
                if (node)
                {
                    var parNode = node.getParent();
                    if (parNode)
                    {
                        parNode.childrenHaveBeenFetched=false;
                        parNode.resetChildren();
                        fetchChildren(parNode, true);
                    }
                }
            }
        }
        else
        {
            var node = getTreeNode('d0');
            node.childrenHaveBeenFetched=false;
            node.resetChildren();
            fetchChildren(node, true);
        }
        var savedlastNodeDb = getTreeNode(savedlastNodeDbID);
        //alert('lastid: ' + savedlastNodeDbID);
        //alert('node: ' + savedlastNodeDb);
        if (savedlastNodeDb)
            selectNode(savedlastNodeDbID);
        else if (savedlastNodeDbIDParent)
        {
            savedlastNodeDb = getTreeNode(savedlastNodeDbIDParent);
            if (savedlastNodeDb)
                selectNode(savedlastNodeDbIDParent);
        }
    }
    var error = xmlGetElementText(xml, 'error');
    if (error)
    {
        alert(error);
        return false;
    }
    return true;
}

function addBr(useDocument, element)
{
    var newBr = useDocument.createElement('br');
    element.appendChild(newBr);
}

function getTopOffset(el)
{
    var offset = 0;
    while (el != null)
    {
        offset += el.offsetTop;
        el = el.offsetParent;
    }
    return offset;
}

function ensureElementVisibility(win, doc, el, bottomAdd)
{
    var viewheight;
    if (win.innerHeight) // all except IE
    {
        viewheight = win.innerHeight;
    }
    else if (doc.documentElement && doc.documentElement.clientHeight)
        // IE 6 Strict Mode
    {
        viewheight = doc.documentElement.clientHeight;
    }
    else if (doc.body) // other IEs
    {
        viewheight = doc.body.clientHeight;
    }
    
    var scrollpos;
    var xoffset;
    if (win.pageYOffset) // all except IE
    {
        xoffset = win.pageXOffset;
        scrollpos = win.pageYOffset;
    }
    else if (doc.documentElement && doc.documentElement.scrollTop)
        // IE 6 Strict
    {
        xoffset = doc.documentElement.scrollLeft;
        scrollpos = doc.documentElement.scrollTop;
    }
    else if (document.body) // all other IEs
    {
        xoffset = doc.body.scrollLeft;
        scrollpos = doc.body.scrollTop;
    }
    
    var elpos = getTopOffset(el);
    
    if (elpos<scrollpos)
    {
        win.scrollTo(xoffset, elpos);
    }
    else if (elpos>scrollpos+viewheight-bottomAdd)
    {
        win.scrollTo(xoffset, elpos-viewheight+bottomAdd);
    }
}

