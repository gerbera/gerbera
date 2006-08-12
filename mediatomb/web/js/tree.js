var closedGif = '/icons/nanotree/images/folder_closed.gif';
var openGif = '/icons/nanotree/images/folder_open.gif';
var iconArray = new Array(closedGif,openGif);
var lastNodeDb = 'd0';
var lastNodeFs = 'f0';
sortNodes = false;
showRootNode = false;

var dbStuff =
{
    container: false,
    rootNode: false,
    treeShown: false,
    tombRootNode: false
};
var fsStuff =
{
    container: false,
    rootNode: false,
    treeShown: false,
    tombRootNode: false
};

function treeInit()
{
    var rootContainer = $('tree_div');
    
    dbStuff.container = document.createElement("div");
    fsStuff.container = document.createElement("div");
    
    Element.hide(dbStuff.container);
    Element.hide(fsStuff.container);
    rootContainer.appendChild(dbStuff.container);
    rootContainer.appendChild(fsStuff.container);
    
    dbStuff.rootNode = new TreeNode(-1,"Database", iconArray);
    dbStuff.tombRootNode = new TreeNode('d0', "Database", iconArray);
    dbStuff.tombRootNode.setHasChildren(true);
    dbStuff.tombRootNode.addOpenEventListener("openEventListener");
    dbStuff.rootNode.addChild(dbStuff.tombRootNode);
    fsStuff.rootNode = new TreeNode(-2,"Filesystem", iconArray);
    fsStuff.tombRootNode = new TreeNode('f0', "Filesystem", iconArray);
    fsStuff.tombRootNode.setHasChildren(true);
    fsStuff.tombRootNode.addOpenEventListener("openEventListener");
    fsStuff.rootNode.addChild(fsStuff.tombRootNode);
    
    treeChangeType();
}

function updateTreeAfterLogin()
{
    if (isTypeDb() && getState(dbStuff.tombRootNode.getID()) == "open") fetchChildren(dbStuff.tombRootNode);
    if (!isTypeDb() && getState(fsStuff.tombRootNode.getID()) == "open") fetchChildren(fsStuff.tombRootNode);
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
        showTree('/icons/nanotree/');
        type.treeShown = true;
    }
    
    if (isTypeDb())
    {
        if (lastNodeDb) selectNode(lastNodeDb);
    }
    else
    {
        if (lastNodeFs) selectNode(lastNodeFs);
    }
}

function standardClick(treeNode)
{
    var id = treeNode.getID()
    if (isTypeDb()) lastNodeDb = id;
    else lastNodeFs = id;
    folderChange(id);
}

function openEventListener(id)
{
    var node = getTreeNode(id);
    if (!node.childrenHaveBeenFetched)
    {
        fetchChildren(node);
    }
}

function fetchChildren(node)
{
    var id = node.getID();
    var type = id.substring(0, 1);
    id = id.substring(1);
    var linkType = (type == 'd') ? 'containers' : 'directories';
    var url = link(linkType, {parent_id: id});
    var myAjax = new Ajax.Request(
        url,
        {
            method: 'get',
            onComplete: updateTree
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
    var parentId = type+xmlGetAttribute(containers, "ofId");
    var node = getTreeNode(parentId);
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
    
    for (var i = 0; i < len; i++)
    {
        var c = cts[i];
        var id = type+xmlGetAttribute(c, "id");
        
        //TODO: childCount unnecessary? - hasChildren instead?
        var childCount = xmlGetAttribute(c, "childCount");
        if (childCount)
            childCount = parseInt(childCount);
        var expandable = childCount ? true : false;
        
        var title = xmlGetText(c);
        var child = new TreeNode(id, title, iconArray);
        
        //TODO:
        //child.setEditable(true);
        
        child.setHasChildren(expandable);
        node.addChild(child);
        child.addOpenEventListener("openEventListener");
        
        //recurse immediately:
        //fetchChildren(openEventListener, child);
    }
    node.childrenHaveBeenFetched=true;
    refreshNode(node);
}

