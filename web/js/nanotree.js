/**
* Original Author of this file: Martin Mouritzen. (martin@nano.dk)
* 
* some changes (c) 2006 by Leonhard Wimmer (leo@mediatomb.org)
*
* according to http://nanotree.sourceforge.net/ this file is published under
* the LGPL License
*
* (Lack of) Documentation:
*
*
* If a finishedLoading method exists, it will be called when the tree is loaded.
* (good to display a div, etc.).
*
*
* You have to set the variable rootNode (as a TreeNode).
*
* You have to set a container element, this is the element in which the tree will be.
*
*
* TODO: 
* Save cookies better (only 1 cookie for each tree). Else the page will totally cookieclutter.
*
***********************************************************************
* Configuration variables.
************************************************************************/

// Should the rootNode be displayed.
var showRootNode = true;

// Should the nodes be sorted? (You can either specify a number, then it will be sorted by that, else it will
// be sorted alphabetically (by name).
var sortNodes = true;

// This is IMPORTANT... use an unique id for each document you use the tree in. (else they'll get mixed up).
var documentID = window.location.href;

// being read from cookie.
var nodesOpen = new Array();

// RootNode of the tree.
var rootNode;

// Container to display the Tree in.
var container;

// Shows/Hides subnodes on startup
var showAllNodesOnStartup = false;

// Is the roots dragable?
var dragable = false;

//set to the document/window the tree is in (needed for frame handling).
var treeDocument;
var treeWindow;


/************************************************************************
* The following is just instancevariables.
************************************************************************/
var href = '';

// rootNodeCallBack name (if null, it's not selectable).
var rootNodeCallBack = null;

// selectedNode
var selectedNode = null;

var states = '';
var statearray = new Array();

var treeNodeEdited = null;

var editaborted = false;

var floatDragElement = null;
var colouredElement = null;
var draggedNodeID = null;
var lastDraggedOnNodeID = null;


/**
* The TreeNode Object
* @param id unique id of this treenode
* @param name The title of this node
* @param icon The icon if this node (Can also be an array with 2 elements, the first one will represent the closed state, and the next one the open state)
* @param param A parameter, this can be pretty much anything. (eg. an array with information).
* @param orderNumber an orderNumber If one is given the nodes will be sorted by this (else they'll be sorted alphabetically (If sorting is on).
*/
function TreeNode(id,name,icon,param,orderNumber) {
	this.id = id;
	this.children = new Array();
	this.name = (name == null ? 'unset name' : name);
	this.icon = (icon == null ? '' : icon);
	this.parent = null;
	this.handler = null;
	this.param = (param == null ? '' : param);
	this.orderNumber = (orderNumber == null ? -1 : orderNumber);
	
	this.openeventlisteners = new Array();
	this.editeventlisteners = new Array();
	this.moveeventlisteners = new Array();
	this.haschildren = false;
	this.editable = false;
	this.linestring = '';
	
	this.nextSibling = null;
	this.prevSibling = null;
	
	this.childrenHaveBeenFetched = false;

	this.getID = function() {
		return this.id;
	}
	this.setName = function(newname) {
		this.name = newname;
	}
	this.getName = function() {
		return this.name;
	}
	this.getParam = function() {
		return this.param;
	}
	this.setIcon = function(icon) {
		this.icon = icon;
	}
	this.getIcon = function() {
		if (typeof(this.icon) == 'object') {
			return this.icon[0];
		}
		return this.icon;
	}
	this.getOpenIcon = function() {
		if (typeof(this.icon) == 'object') {
			return this.icon[1];
		}
		return this.icon;
	}
	this.hasIcon = function () {
		return this.icon != '';
	}
	this.getOrderNumber = function() {
		return this.orderNumber;
	}
	this.addOpenEventListener = function(event) {
		this.openeventlisteners[this.openeventlisteners.length] = event;
	}
	this.gotOpenEventListeners = function() {
		return (this.openeventlisteners.length > 0);
	}
	this.addEditEventListener = function(event) {
		this.editeventlisteners[this.editeventlisteners.length] = event;
	}
	this.gotEditEventListeners = function() {
		return (this.editeventlisteners.length > 0);
	}
	this.addMoveEventListener = function(event) {
		this.moveeventlisteners[this.moveeventlisteners.length] = event;
	}
	this.gotMoveEventListeners = function() {
		return (this.moveeventlisteners.length > 0);
	}
	this.addChild = function(childNode) {
		var possiblePrevNode = this.children[this.children.length - 1]
		if (possiblePrevNode) {
			possiblePrevNode.nextSibling = childNode;
			childNode.prevSibling = possiblePrevNode;
			//alert(childNode.prevSibling);
		}
        
		this.children[this.children.length] = childNode;
		childNode.setParent(this);

		if (sortNodes) {
			function sortByOrder(a,b) {
				var order1 = a.getOrderNumber();
				var order2 = b.getOrderNumber();
				if (order1 == -1 || order2 == -1) {
					return a.getName().toLowerCase() > b.getName().toLowerCase() ? 1 : -1;
				}
				else {
					if (order1 == order2) {
						// If they got the same order number, then we'll sort by their title.
						return a.getName().toLowerCase() > b.getName().toLowerCase() ? 1 : -1;
					}
					else {
						return order1 - order2;
					}
				}
			}
			this.children.sort(sortByOrder);
		}
	}
	this.removeChild = function(childNode) {
		var found = false;
		for (var i=0;i<this.children.length;i++) {
			if (found) {
				this.children[i] = this.children[i + 1];
			}
			if (this.children[i] == childNode) {
				if (i == (this.children.length - 1)) {
					this.children[i] = null;
				}
				else {
					this.children[i] = this.children[i + 1];
				}
				found = true;
			}
		}
		if (found) {
			this.children.length = this.children.length-1;
		}
	}
	this.resetChildren = function() {
		this.children = new Array();
	}
	this.setHasChildren = function(hasChildren) {
		this.haschildren = hasChildren;
	}
	this.hasChildren = function() {
		if (this.haschildren == true) {
			return true;
		}
		return (this.children.length > 0);
	}
	this.getChildCount = function() {
		return this.children.length;
	}
	this.getFirstChild = function() {
		if (this.hasChildren()) {
			return this.children[0];
		}
		return null;
	}
	this.gotHandler = function() {
		return this.handler != null;
	}
	this.setHandler = function(handler) {
		this.handler = handler;
	}
	this.getHandler = function() {
		return this.handler;
	}
	this.setParent = function(parent) {
		this.parent = parent;
	}
	this.getParent = function() {
		return this.parent;
	}
	this.getLineString = function() {
		return this.linestring;
	}
	this.setLineString = function(string) {
		this.linestring = string;
	}
	this.isEditable = function() {
		return this.editable;
	}
	this.setEditable = function(editable) {
		this.editable = editable;
	}
	
}

/* overridden in tree.js to serve use the correct rootNode
function getTreeNode(nodeID) {
	return findNodeWithID(rootNode,nodeID);
}
*/

function findNodeWithID(node,nodeID) {
	if (node.getID() == nodeID) {
		return node;
	}
	else {
		if (node.hasChildren()) {
			for(var i=0;i<node.getChildCount();i++) {
				var value = findNodeWithID(node.children[i],nodeID);
				if (value != false) {
					return value;
				}
			}
		}
		return false;
	}
}
function readStates() {
	//setCookie('tree' + documentID,'');
	states = getCookie('tree' + documentID);
	if (states != null) {
		var array = states.split(';');
		for(var i=0;i<array.length;i++) {
			var singlestate = array[i].split('|');
			statearray[i] = new Array();
			statearray[i]["key"] = singlestate[0];
			statearray[i]["state"]  = singlestate[1];
		}
	}
}
function getState(nodeID) {
	for(var i=0;i<statearray.length;i++) {
		if (statearray[i]["key"] == nodeID) {
			state = statearray[i]["state"];
			if (state == null || state == '') {
				state = 'closed';
			}
			return state;
		}
	}
	return "closed";
}
function writeStates(nodeID,newstate) {
	//alert(nodeID);
	var str = '';
	var found = false;
	for(var i=0;i<statearray.length;i++) {
		if (statearray[i]["key"] == nodeID) {
			statearray[i]["state"] = newstate;
			found = true;
		}
		if (statearray[i]["state"] != null) {
			str += statearray[i]["key"] + '|' + statearray[i]["state"] + ';';
		}
	}
	if (found == false) {
		statearray[statearray.length] = new Array();
		statearray[statearray.length - 1]["key"] = nodeID;
		statearray[statearray.length - 1]["state"] = newstate;
		if (newstate != null) {
			str += nodeID + '|' + newstate + ';';
		}
	}
	setCookie('tree' + documentID,str);
}
function showTree(path) {
	readStates();
	href = path;
	treeWindow.focus();
	treeWindow.onblur = blurSelection;
	treeWindow.onfocus = focusSelection;
	var str = '';
	str = '<div id="node' + rootNode.getID() + '" class="treetitle" style="display:' + (showRootNode == true ? 'block' : 'none') + '">';
	str += '<nobr>';
	if (rootNode.hasIcon()) {
		str += '<img src="' + rootNode.getIcon() + '" style="vertical-align:middle;' + imageStyleIcon + '">';
	}
	str += '<span style="vertical-align:middle;">&nbsp;' + rootNode.getName() + '</span>';
	str += '</nobr></div>';
	if (rootNode.hasChildren()) {
		for(i=0;i<rootNode.children.length;i++) {
			nodeContents = showNode(rootNode.children[i],(i == (rootNode.getChildCount() -1)));
			str = str + nodeContents;
		}
	}
	container.innerHTML = str;
	if (treeWindow.finishedLoading) {
		finishedLoading();
	}
}
/**
* Shows the given node, and subnodes.
*/
function showNode(treeNode,lastNode) {
	linestring = treeNode.getLineString();
	var state = getState(treeNode.getID());
	var str;
	str = '<div  ondragenter="parent.dragEnter(\'' + treeNode.getID() + '\');" ondragleave="parent.dragLeave();" ondragstart="parent.startDrag(\'' + treeNode.getID() + '\');" ondrag="parent.dragMove();" ondragend="parent.endDrag(\'' + treeNode.getID() + '\')" id="node' + treeNode.getID() + '">';
	str += '<nobr>';
	for(var y=0;y<linestring.length;y++) {
		if (linestring.charAt(y) == 'I') {
			str += '<img src="' + href + 'images/line' + imageExtension + '" ' + imageStyle + '>';
		}
		else if (linestring.charAt(y) == 'B') {
			str += '<img src="' + href + 'images/' + imageWhite + imageExtension + '" ' + imageStyle + '>';
		}
	}
	if (treeNode.hasChildren()) {
		// If this is the first child of the rootNode, and showRootNode is false, we want to display a different icon.
		if (!showRootNode && (treeNode.getParent() == rootNode) && (treeNode.getParent().getFirstChild() == treeNode)) {
			if (!lastNode) {
				str += '<img id="handler' + treeNode.getID() + '" src="' + href + 'images/' + (state == 'open' ? imageMinusNoRoot : imagePlusNoRoot) + imageExtension + '" class="nanotree_children_no_root" ' + imageStyle + ' OnClick="parent.handleNode(\'' + treeNode.getID() + '\');">';
			}
			else {
				str += '<img id="handler' + treeNode.getID() + '" src="' + href + 'images/' + (state == 'open' ? imageMinusLastNoRoot : imagePlusLastNoRoot) + imageExtension + '" class="nanotree_children_last_no_root" ' + imageStyle + ' OnClick="parent.handleNode(\'' + treeNode.getID() + '\');">';
			}
		}
		else {
			if (!lastNode) {
				str += '<img id="handler' + treeNode.getID() + '" src="' + href + 'images/' + (state == 'open' ? imageMinus : imagePlus) + imageExtension + '" class="nanotree_children" ' + imageStyle + ' OnClick="parent.handleNode(\'' + treeNode.getID() + '\');">';
			}
			else {
				str += '<img id="handler' + treeNode.getID() + '" src="' + href + 'images/' + (state == 'open' ? imageMinusLast : imagePlusLast) + imageExtension + '" class="nanotree_children_last" ' + imageStyle + ' OnClick="parent.handleNode(\'' + treeNode.getID() + '\');">';
			}
		}
	}
	else {
		// If this is the first child of the rootNode, and showRootNode is false, we want to display a different icon.
		if (!showRootNode && (treeNode.getParent() == rootNode) && (treeNode.getParent().getFirstChild() == treeNode)) {
			if (!lastNode) {
				str += '<img id="handler' + treeNode.getID() + '" src="' + href + 'images/' + imageTNoRoot + imageExtension + '" class="nanotree_no_root" ' + imageStyle + '>';
			}
			else {
				str += '<img id="handler' + treeNode.getID() + '" src="' + href + 'images/' + imageWhite + imageExtension + '" ' + imageStyle + '>';
			}
		}
		else {
			if (!lastNode) {
				str += '<img id="handler' + treeNode.getID() + '" src="' + href + 'images/' + 't' + imageExtension + '" ' + imageStyle + '>';
			}
			else {
				str += '<img id="handler' + treeNode.getID() + '" src="' + href + 'images/' + imageLastnode + '.png" class="nanotree_last" ' + imageStyle + '>';
			}
		}
	}
	iconStartImage = treeNode.getIcon();
	if (state != 'closed') {
		if (treeNode.hasChildren()) {
			iconStartImage = treeNode.getOpenIcon();
		}
	}
	
	str += '<img id="iconimage' + treeNode.getID() + '" src="' + iconStartImage + '" style="vertical-align:middle;' + imageStyleIcon + '" OnClick="parent.selectNode(\'' + treeNode.getID() + '\')" OnDblClick="parent.handleNode(\'' + treeNode.getID() + '\')">';
	str += '&nbsp;<span unselectable="ON" style="vertical-align:bottom;" class="treetitle" ID="title' + treeNode.getID() + '" OnDblClick="parent.handleNode(\'' + treeNode.getID() + '\')" OnClick="parent.selectNode(\'' + treeNode.getID() + '\')">';
	str += treeNode.getName();
	str += '</span>';
	str += '</nobr>';
	str += '</div>';

	if (treeNode.hasChildren()) {
		if (state == 'open') {
			str += '<div id="node' + treeNode.getID() + 'sub" style="display:block;">';
			fireOpenEvent(treeNode);
			// alert('openevent: ' + treeNode.getName());
		}
		else {
			str += '<div id="node' + treeNode.getID() + 'sub" style="display:' + (showAllNodesOnStartup == true ? 'block;' : 'none;') + ';">';
		}
		var subgroupstr = '';
		var newChar = '';

		if (!lastNode) {
			newChar = 'I';
		}
		else {
			newChar = 'B';
		}
		for(var z=0;z<treeNode.getChildCount();z++) {
			treeNode.children[z].setLineString(linestring + newChar);
		}
		for(var z=0;z<treeNode.getChildCount();z++) {
			subgroupstr += showNode(treeNode.children[z],(z == (treeNode.getChildCount() -1)));
		}
		str += subgroupstr;
		str += '</div>';
	}
	else {
		str += '<div id="node' + treeNode.getID() + 'sub" style="display:none;">';
		str += '</div>';
	}
	return str;
}
/*
function mouseMove() {
	if (dragging) {
		alert('bob');
	}
}
function mouseUp() {
	if (dragging) {
		alert('dropped on something!');
	}
}
*/
function startDrag(nodeID) {
	if (!dragable) {
		return;
	}
	draggedNodeID = nodeID;
	
	var srcObj = treeWindow.event.srcElement;
	while(srcObj.tagName != 'DIV') {
		srcObj = srcObj.parentElement;
	}
	floatDragElement = treeDocument.createElement('DIV');

	floatDragElement.innerHTML = srcObj.innerHTML;
	floatDragElement.childNodes[0].removeChild(floatDragElement.childNodes[0].childNodes[0]);
	
	treeDocument.body.appendChild(floatDragElement);
	floatDragElement.style.zIndex = 100;
	floatDragElement.style.position = 'absolute';
	floatDragElement.style.filter='progid:DXImageTransform.Microsoft.Alpha(1,opacity=60);';
}
function findSpanChild(element) {
	if (element.tagName == 'SPAN') {
		return element;
	}
	else {
		if (element.childNodes) {
			for(var i=0;i<element.childNodes.length;i++) {
				var value = findSpanChild(element.childNodes[i]);
				if (value != false) {
					return value;
				}
			}
			return false;
		}
	}
}
function dragEnter(nodeID) {
	if (!dragable) {
		return;
	}
	lastDraggedOnNodeID = nodeID;
	
	if (colouredElement) {
		findSpanChild(colouredElement).className = 'treetitle';
	}
	colouredElement = treeWindow.event.srcElement;
	while(colouredElement.tagName != 'DIV') {
		colouredElement = colouredElement.parentElement;
		if (colouredElement.tagName == 'BODY') {
			// Something gone seriously wrong.
			alert('Drag failure, reached <BODY>!');
			return;
		}
	}	
	findSpanChild(colouredElement).className = 'treetitleselectedfocused';
}
function dragLeave() {
	if (!dragable) {
		return;
	}
}
function endDrag(nodeID) {
	if (!dragable) {
		return;
	}
	if (lastDraggedOnNodeID != null) {
		fireMoveEvent(getTreeNode(lastDraggedOnNodeID),draggedNodeID,lastDraggedOnNodeID);
	}
}
function dragProceed() {
	if (!dragable) {
		return;
	}
	var dragged = getTreeNode(draggedNodeID);
	var newparent = getTreeNode(lastDraggedOnNodeID);

	var oldparent = dragged.getParent();
	
	oldparent.removeChild(dragged);
	newparent.addChild(dragged);
	
	refreshNode(oldparent);
	refreshNode(newparent);
	
	_dragClean()
}
function dragCancel() {
	if (!dragable) {
		return;
	}
	_dragClean()
}
/**
* Don't call this yourself.
*/
function _dragClean() {
	if (!dragable) {
		return;
	}
	if (colouredElement) {
		findSpanChild(colouredElement).className = 'treetitle';
	}
	
	floatDragElement.parentElement.removeChild(floatDragElement);
	floatDragElement = null;
	colouredElement = null;
	draggedNodeID = null;
	lastDraggedOnNodeID = null;
}
function dragMove() {
	if (!dragable) {
		return;
	}
	floatDragElement.style.top = treeWindow.event.clientY;
	floatDragElement.style.left = treeWindow.event.clientX;
}
function editEnded() {
	if (treeNodeEdited != null) {
		// treeNodeEdited.getID();
		var editTitle = treeDocument.getElementById('title' + treeNodeEdited.getID());
		var input = editTitle.childNodes[0];
	
		var newValue = input.value;
		
		if (newValue == treeNodeEdited.getName()) {
			editTitle.innerHTML = newValue;
			treeNodeEdited = null;
			return;
		}
	
		fireEditEvent(treeNodeEdited,newValue);
		
		if (!editaborted) {
			treeNodeEdited.setName(newValue);
			editTitle.innerHTML = newValue;
		}
	
		treeNodeEdited = null;
	}
}
function selectNode(nodeID) {
	var treeNode = getTreeNode(nodeID);

	if (selectedNode != null && selectedNode != nodeID) {
        
        /*
		if (selectedNode == nodeID) {
			if (treeNode.isEditable()) {
				if (treeNodeEdited == treeNode) {
					return;
				}
				treeNodeEdited = treeNode;
				var editTitle = treeDocument.getElementById('title' + treeNode.getID());
				editTitle.className = 'editednode';
				

				editTitle.innerHTML = '<input type="text" onKeypress="if (event.keyCode == 13) { this.onblur = null; editEnded(); }" name="editednode" class="editednodeinput">';
				var input = editTitle.childNodes[0];
				input.value = treeNode.getName();
				input.focus();
				input.select();
				input.onblur = editEnded;
			}
			return;
		}
		if (treeNodeEdited != null) {
			editEnded();
		}
        */
		var oldNodeTitle = treeDocument.getElementById('title' + selectedNode);
        if (oldNodeTitle)
            oldNodeTitle.className = 'treetitle';
	}
    if (selectedNode == null || selectedNode != nodeID)
    {
        selectedNode = nodeID;
	    var nodetitle = treeDocument.getElementById('title' + selectedNode);
        nodetitle.className = 'treetitleselectedfocused';
        ensureElementVisibility(treeWindow, treeDocument, nodetitle, 35);
    }
	
	if (treeNode.gotHandler()) {
		eval(treeNode.getHandler() + '(getTreeNode(' + nodeID + '));');
	}
	else {
		standardClick(treeNode);
	}
}
function refreshNode(treeNode) {
	var submenu = treeDocument.getElementById('node' + treeNode.getID() + 'sub');
	var str = '';
	for(var i=0;i<treeNode.getChildCount();i++) {
		var parent = treeNode.getParent();
		if (!parent) {
			treeNode.children[i].setLineString(treeNode.getLineString() + 'B');
		}
		else {
			if (parent.children[parent.children.length - 1] == treeNode) {
				treeNode.children[i].setLineString(treeNode.getLineString() + 'B');
			}
			else {
				treeNode.children[i].setLineString(treeNode.getLineString() + 'I');
			}
		}
		str += showNode(treeNode.children[i],i == (treeNode.getChildCount() - 1));
	}
    var actionimage = treeDocument.getElementById('handler' + treeNode.getID());
    if (treeNode.getChildCount() == 0) {
        // TreeNode haven't got any children, make sure the right image is displayed.
        if (actionimage.className && actionimage.className.indexOf('_last') == -1) {
            actionimage.src = href + 'images/t' + imageExtension;
        }
        else {
            actionimage.src = href + 'images/' + imageLastnode + imageExtension;
        }
        actionimage.onclick = null;
        
        // Close the submenu
        if (submenu) {
            submenu.style.display = 'none';
        }
    }
    else {
        // We have children, make sure to display the + and - icon.
        if (actionimage.className && actionimage.className.indexOf('_children') != -1) {
            // The TreeNode has already got children, and displays them.
        }
        else {
            if (actionimage.className && actionimage.className.indexOf('_last') == -1) {
                actionimage.outerHTML = '<img id="handler' + treeNode.getID() + '" src="' + href + 'images/' + imagePlus + imageExtension + '" class="nanotree_children" ' + imageStyle + ' OnClick="parent.handleNode(\'' + treeNode.getID() + '\');">';
            }
            else {
                actionimage.outerHTML = '<img id="handler' + treeNode.getID() + '" src="' + href + 'images/' + imagePlusLast + '" class="nanotree_children_last" ' + imageStyle + ' OnClick="parent.handleNode(\'' + treeNode.getID() + '\');">';
            }
        }
    }
	submenu.innerHTML = str;
}
function handleNode(nodeID) {
	var treeNode = getTreeNode(nodeID);	
	if (!treeNode.hasChildren()) { // No reason to handle a node without children.
		return;
	}
	
	var submenu = treeDocument.getElementById('node' + nodeID + 'sub');
	
	var iconimageholder = treeDocument.getElementById('iconimage' + nodeID);
	var actionimage = treeDocument.getElementById('handler' + nodeID);

	// This will be used if showRootNode is set to false.
	var firstChildOfRoot = false;
	if (actionimage.className && actionimage.className.indexOf('_no_root') != -1) {
		firstChildOfRoot = true;
	}
	
	if (submenu.style.display == 'none') {
		writeStates(nodeID,'open');
		fireOpenEvent(treeNode);
		submenu.style.display = 'block';

		iconimageholder.src = treeNode.getOpenIcon();
	
		if (actionimage.className && actionimage.className.indexOf('_last') == -1) {
			actionimage.src = href + 'images/' + ((firstChildOfRoot) ? imageMinusNoRoot : imageMinus) + imageExtension;
		}
		else {
			actionimage.src = href + 'images/' + ((firstChildOfRoot) ? imageMinusLastNoRoot : imageMinusLast) + imageExtension;
		}
	}
	else {
		writeStates(nodeID,'closed');
		submenu.style.display = 'none';
		
		iconimageholder.src = treeNode.getIcon();
		
		if (actionimage.className && actionimage.className.indexOf('_last') == -1) {
			actionimage.src = href + 'images/' + ((firstChildOfRoot) ? imagePlusNoRoot : imagePlus) + imageExtension;
		}
		else {                                                                   
			actionimage.src = href + 'images/' + ((firstChildOfRoot) ? imagePlusLastNoRoot : imagePlusLast) + imageExtension;
		}
	}
}
function fireOpenEvent(treeNode) {
	if (treeNode.gotOpenEventListeners()) {
		for(var i=0;i<treeNode.openeventlisteners.length;i++) {
			eval(treeNode.openeventlisteners[i] + '(\'' + treeNode.getID() + '\');');
		}
	}
}
function fireEditEvent(treeNode,newVal) {
	if (treeNode.gotEditEventListeners()) {
		for(var i=0;i<treeNode.editeventlisteners.length;i++) {
			eval(treeNode.editeventlisteners[i] + '(\'' + treeNode.getID() + '\',\'' + escape(newVal) + '\');');
		}
	}
}
function fireMoveEvent(treeNode,draggedNodeID,droppedOnNodeID) {
	if (treeNode.gotMoveEventListeners()) {
		for(var i=0;i<treeNode.moveeventlisteners.length;i++) {
			eval(treeNode.moveeventlisteners[i] + '(' + draggedNodeID + ',' + droppedOnNodeID + ');');
		}
	}
}
function blurSelection() {
	if (selectedNode != null) {
		var oldNodeTitle = treeDocument.getElementById('title' + selectedNode);
		if (oldNodeTitle)
            oldNodeTitle.className = 'treetitleselectedblured';
	}
}
function focusSelection() {
	if (selectedNode != null) {
		var oldNodeTitle = treeDocument.getElementById('title' + selectedNode);
        if (oldNodeTitle)
            oldNodeTitle.className = 'treetitleselectedfocused';
	}
}
function getCookieVal (offset) {  
	var endstr = document.cookie.indexOf (";",offset);  
	if (endstr == -1) {
		endstr = document.cookie.length;
	}
	return unescape(document.cookie.substring(offset,endstr));
}
function getCookie (name) {  
	var arg = name + "=";
	var alen = arg.length;
	var clen = document.cookie.length;
	var i = 0;
	while (i < clen) {
		var j = i + alen;
		if (document.cookie.substring(i, j) == arg) {
			return getCookieVal(j);
		}
		i = document.cookie.indexOf(" ", i) + 1;
		if (i == 0) {
			break;
		}
	}
	return null;
}
function setCookie (name, value) {  
	var argv = setCookie.arguments;  
	var argc = setCookie.arguments.length;  
	var expires = (argc > 2) ? argv[2] : null;  
	var path = (argc > 3) ? argv[3] : null;  
	var domain = (argc > 4) ? argv[4] : null;  
	var secure = (argc > 5) ? argv[5] : false;  
	document.cookie = name + "=" + escape (value) + ((expires == null) ? "" : ("; expires=" + expires.toGMTString())) + ((path == null) ? "" : ("; path=" + path)) + ((domain == null) ? "" : ("; domain=" + domain)) + ((secure == true) ? "; secure" : "");
}
function expandNode() {
	var state = getState(selectedNode);
	if (state == 'open') {
		var currentTreeNode = getTreeNode(selectedNode);
		if (currentTreeNode.hasChildren()) {
            if (currentTreeNode.children[0])
                selectNode(currentTreeNode.children[0].getID());
		}
	}
	else {
		handleNode(selectedNode);
	}
}
function subtractNode() {
	var state = getState(selectedNode);
	if (state == 'closed') {
		var currentTreeNode = getTreeNode(selectedNode);
		var parent = currentTreeNode.getParent();
		if (parent != null && parent != rootNode) {
			selectNode(parent.getID());
		}
	}
	else {
		handleNode(selectedNode);
	}
}
function selectPrevNode() {
	var currentTreeNode = getTreeNode(selectedNode);
	if (currentTreeNode.prevSibling != null) {

		var state = getState(currentTreeNode.prevSibling.getID());

		if (state == 'open' && currentTreeNode.prevSibling.hasChildren()) {
			// We have to find the last open child of the previoussiblings children.
			var current = currentTreeNode.prevSibling.children[currentTreeNode.prevSibling.children.length - 1];
			var currentstate = 'open';
			while (current.hasChildren() && (getState(current.getID()) == 'open')) {
				current = current.children[current.children.length - 1];
			}
			selectNode(current.getID());
		}
		else {
			selectNode(currentTreeNode.prevSibling.getID());
		}
	}
	else {
		if (currentTreeNode.getParent() != null && currentTreeNode.getParent() != rootNode) {
			selectNode(currentTreeNode.getParent().getID());
		}
	}
}
function selectNextNode() {
	var currentTreeNode = getTreeNode(selectedNode);

	var state = getState(selectedNode);
	if (state == 'open' && currentTreeNode.hasChildren()) {
		selectNode(currentTreeNode.children[0].getID());
	}	
	else {
		if (currentTreeNode.nextSibling != null) {
			selectNode(currentTreeNode.nextSibling.getID());
		}
		else {
			// Continue up the tree until we either hit null, or a parent which has a child.
			var parent = currentTreeNode;
			while ((parent = parent.getParent()) != rootNode) {
				if (parent.nextSibling != null) {
					selectNode(parent.nextSibling.getID());
					break;
				}
			}
			/*
			if (currentTreeNode.getParent().nextSibling != null) {
				selectNode(currentTreeNode.getParent().nextSibling.getID());
			}
			*/
		}
	}
}
function keyDown(event) {
	if (treeWindow.event) {
		event = treeWindow.event;
	}
	if (event.keyCode == 38) { // Up
		selectPrevNode();
		return false;
	}
	else if (event.keyCode == 40) { // Down
		selectNextNode();
		return false;
	}
	else if (event.keyCode == 37) { // left
		subtractNode();
		return false;
	}
	else if (event.keyCode == 39) { // right
		expandNode();
		return false;
	}
    // 13: enter
    // 32: space
    // 33: PgUp
    // 34: PgDown
}
