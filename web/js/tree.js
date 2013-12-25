/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    tree.js - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
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


var iconArray = new Array(iconTreeClosed,iconTreeOpen);
var autoscanTimedIconArray = new Array(iconTreeAutoscanTimedClosed, iconTreeAutoscanTimedOpen);
var autoscanTimedConfigIconArray = new Array(iconTreeAutoscanTimedConfigClosed, iconTreeAutoscanTimedConfigOpen);
var autoscanInotifyIconArray = new Array(iconTreeAutoscanInotifyClosed, iconTreeAutoscanInotifyOpen);
var autoscanInotifyConfigIconArray = new Array(iconTreeAutoscanInotifyConfigClosed, iconTreeAutoscanInotifyConfigOpen);
var lastNodeDb = 'd0';
var lastNodeFs = 'f0';
var lastNodeDbWish;
var lastNodeFsWish;

var imageExtension = '.png';
var imageStyle = 'style="width:24px;height:26px;vertical-align:middle;"';
var imageStyleIcon = "width:24px;height:26px;";
var imageLastnode = 'lastnode';
var imageWhite = 'white';
var imageTNoRoot = 't_no_root';
var imageMinus = 'minus';
var imageMinusNoRoot = 'minus_no_root';
var imageMinusLastNoRoot = 'minus_last_no_root';
var imageMinusLast = 'minus_last';
var imagePlusNoRoot = 'plus_no_root';
var imagePlusLastNoRoot = 'plus_last_no_root';
var imagePlus = 'plus';
var imagePlusLast = 'plus_last';

sortNodes = false;
showRootNode = false;

var dbStuff =
{
    container: false,
    rootNode: false,
    treeShown: false,
    tombRootNode: false,
    refreshQueue: new Array()
};
var fsStuff =
{
    container: false,
    rootNode: false,
    treeShown: false,
    tombRootNode: false,
    refreshQueue: new Array()
};

/* nanotree.js overrides... */

function getTreeNode(nodeID) {
    var type = nodeID.substr(0,1);
    if (type == 'd')
        return findNodeWithID(dbStuff.rootNode,nodeID);
    if (type == 'f')
        return findNodeWithID(fsStuff.rootNode,nodeID);
}

/* overrides END */

function treeInit()
{
    treeDocument = frames["leftF"].document;
    treeWindow = frames["leftF"].window;
    
    documentID = "mediatombUI";
    
    var rootContainer = treeDocument.getElementById('treeDiv');
    dbStuff.container = treeDocument.createElement("div");
    fsStuff.container = treeDocument.createElement("div");
    
    Element.hide(dbStuff.container);
    Element.hide(fsStuff.container);
    rootContainer.appendChild(dbStuff.container);
    rootContainer.appendChild(fsStuff.container);
    
    dbStuff.rootNode = new TreeNode(-1,"Database", iconArray);
    dbStuff.tombRootNode = new TreeNode('d0', "Database", iconArray);
    dbStuff.tombRootNode.setHasChildren(true);
    dbStuff.tombRootNode.addOpenEventListener("openEventListener");
    dbStuff.rootNode.addChild(dbStuff.tombRootNode);
    //writeStates('d0','open');
    fsStuff.rootNode = new TreeNode(-2,"Filesystem", iconArray);
    fsStuff.tombRootNode = new TreeNode('f0', "Filesystem", iconArray);
    fsStuff.tombRootNode.setHasChildren(true);
    fsStuff.tombRootNode.addOpenEventListener("openEventListener");
    fsStuff.rootNode.addChild(fsStuff.tombRootNode);
    //writeStates('f0','open');
    treeChangeType();
    treeDocument.onkeydown = keyDown;
    treeDocument.onmousedown = mouseDownHandler;
}

function updateTreeAfterLogin()
{
    if (isTypeDb() && getState(dbStuff.tombRootNode.getID()) == "open") fetchChildren(dbStuff.tombRootNode);
    if (!isTypeDb() && getState(fsStuff.tombRootNode.getID()) == "open") fetchChildren(fsStuff.tombRootNode);
    selectLastNode();
}

function treeChangeType()
{
    var type = isTypeDb() ? dbStuff : fsStuff; 
    var newContainer = type.container;
    if (container != newContainer)
    {
        if (container)
        {
            Element.hide(container);
        }
        container = newContainer;
        Element.show(container);
        rootNode = type.rootNode;
    }
    if (!type.treeShown)
    {
        readStates();
        if (isTypeDb())
        {
            writeStates('d0','open');
            //refreshNode(getTreeNode('d0'));
        }
        else
        {
            writeStates('f0','open');
            //refreshNode(getTreeNode('f0'));
        }
        showTree('icons/nanotree/');
        type.treeShown = true;
    }
    while (type.refreshQueue.length > 0)
    {
        refreshNode(type.refreshQueue.pop());
    }
    if (loggedIn)
    {
        selectLastNode();
        if (isTypeDb())
            getUpdates(true);
        else
            setStatus("no_updates");
    }
}

function refreshOrQueueNode(node)
{
    var id = node.getID();
    var type = id.substr(0,1);
     if (isTypeDb())
    {
        if (type == 'd')
            refreshNode(node);
        else
            fsStuff.refreshQueue.push(node);
    }
    else
    {
        if (type == 'f')
            refreshNode(node);
        else
            dbStuff.refreshQueue.push(node);
    }
}

function selectLastNode()
{
    if (isTypeDb())
    {
        var saveLastNodeDbWish = lastNodeDbWish;
        if (lastNodeDb)
            selectNode(lastNodeDb);
        lastNodeDbWish = saveLastNodeDbWish
    }
    else
    {
        var saveLastNodeFsWish = lastNodeFsWish;
        if (lastNodeFs)
            selectNode(lastNodeFs);
        lastNodeFsWish = saveLastNodeFsWish
    }
}

function selectNodeIfVisible(nodeID)
{
    var type = nodeID.substr(0,1);
    if (isTypeDb())
    {
        if (type == 'd')
            selectNode(nodeID);
    }
    else
    {
        if (type == 'f')
            selectNode(nodeID);
    }
}

function standardClick(treeNode)
{
    userActivity();
    var id = treeNode.getID();
    if (isTypeDb())
    {
        if (id.substr(0,1) == 'd')
        {
            setCookie('lastNodeDb', id);
            lastNodeDb = id;
            lastNodeDbWish = null;
            folderChange(id);
        }
    }
    else
    {
        if (id.substr(0,1) == 'f')
        {
            setCookie('lastNodeFs', id);
            lastNodeFs = id;
            lastNodeFsWish = null;
            folderChange(id);
        }
    }
}

function openEventListener(id)
{
    var node = getTreeNode(id);
    if (!node.childrenHaveBeenFetched)
    {
        fetchChildren(node);
    }
}

function fetchChildren(node, uiUpdate, selectIt)
{
    var id = node.getID();
    var type = id.substring(0, 1);
    id = id.substring(1);
    var linkType = (type == 'd') ? 'containers' : 'directories';
    var url = link(linkType, {parent_id: id, select_it: (selectIt ? '1' : '0')});
    var async = ! uiUpdate;
    
    //DEBUG?
    //async = false;
    
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: updateTree,
            asynchronous: async
        });
}

function updateTree(ajaxRequest)
{
    /*
    var xml = fetchXML(url);
    if (! xml)
    {
        alert("could not fetch XML - server not reachable?");
        return;
    }
    */
    
    var xml = ajaxRequest.responseXML;
    if (!errorCheck(xml)) return;
    
    var containers = xmlGetElement(xml, "containers");
    if (! containers)
    {
        alert("no containers tag found");
        return;
    }
    var type = xmlGetAttribute(containers, "type");
    if (type == 'filesystem')
        type = 'f';
    else if (type == 'database')
        type = 'd';
    var ofId = xmlGetAttribute(containers, "parent_id");
    var parentId = type+ofId;
    
    var node = getTreeNode(parentId);
    var success = xmlGetElement(xml, 'root').getAttribute("success");
    //alert(success);
    if (! success || success != "1")
    {
        if (ofId == '0')
        {
            alert("Oops, your database seems to be corrupt. Please report this problem.");
            return;
        }
        var parNode = node.getParent();
        parNode.childrenHaveBeenFetched=false;
        parNode.resetChildren();
        fetchChildren(parNode, true, true);
        //selectNode(parNode.getID());
        //alert("no success!");
        return;
    }
    
    var selectItStr = containers.getAttribute("select_it");
    var selectIt = (selectItStr && selectItStr == '1');
    
    if (node.childrenHaveBeenFetched)
    {
        return;
    }
    var cts = containers.getElementsByTagName("container");
    if (! cts)
    {
        alert("no containers found");
        return;
    }
    
    var i;
    var len = cts.length;
    var doSelectLastNode = false;
    
    for (var i = 0; i < len; i++)
    {
        var c = cts[i];
        var id = type + xmlGetAttribute(c, "id");
        
        //TODO: childCount unnecessary? - hasChildren instead?
        var childCount = xmlGetAttribute(c, "child_count");
        if (childCount)
            childCount = parseInt(childCount);
        var expandable = childCount ? true : false;
        
        var autoscanType = xmlGetAttribute(c, "autoscan_type");
        var autoscanMode = xmlGetAttribute(c, "autoscan_mode");
        
        var thisIconArray = iconArray;
        if (autoscanType == 'ui')
        {
            if (autoscanMode == 'inotify')
                thisIconArray = autoscanInotifyIconArray;
            else
                thisIconArray = autoscanTimedIconArray;
        }
        
        if (autoscanType == 'persistent')
        {
            if (autoscanMode == 'inotify')
                thisIconArray = autoscanInotifyConfigIconArray;
            else
                thisIconArray = autoscanTimedConfigIconArray;
        }
        
        var title = xmlGetText(c);
        var child = new TreeNode(id, title, thisIconArray);
        
        //TODO:
        //child.setEditable(true);
        
        child.setHasChildren(expandable);
        try
        {
            node.addChild(child);
        }
        catch (e)
        {
            return;
        }
        child.addOpenEventListener("openEventListener");
        
        if (id == lastNodeDbWish)
        {
            lastNodeDbWish=null;
            lastNodeDb = id;
            doSelectLastNode = true;
        }
        else if (id == lastNodeFsWish)
        {
            lastNodeFsWish=null;
            lastNodeFs = id;
            doSelectLastNode = true;
        }
        
        //recurse immediately:
        //fetchChildren(openEventListener, child);
    }
    node.childrenHaveBeenFetched = true;
    refreshOrQueueNode(node);
    if (doSelectLastNode)
        selectLastNode();
    else if (selectIt)
    {
        //alert("selecting: " + parentId);
        selectNodeIfVisible(parentId);
        //selectNode("d0");
        //alert("selected! " + parentId);
    }
}
