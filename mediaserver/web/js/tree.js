var DIV_POOL_SIZE = 100; // maximum nuber of expanded containers
var NODE_HEIGHT = 16;

var div_pool_inst;
var node_hash;
var tree_inst;


/********** DivPool class, used to render html **********/

function DivPool(maxDivs, parentDiv)
{
    this.maxDivs = maxDivs;
    this.parent = parentDiv;
    
    this.freeDivs = maxDivs;
    this.freeList = new Array(maxDivs);
    
    this.init = DivPoolInit;
    this.alloc = DivPoolAlloc;
    this.free = DivPoolFree;
}

function DivPoolInit()
{
    var i;
    var html = '';
    for (i = 0; i < this.maxDivs; i++)
    {
        html += '<div></div>';
    }
    this.parent.innerHTML = html;
    var subdivs = this.parent.childNodes;

    var len = subdivs.length;
    var curEl = null;
    for (i = 0; i < len; i++)
    {
        var subdiv = subdivs[i];
        var el = {div: subdiv, next: curEl};
        curEl = el;
    }
    this.freeList = curEl;
}
function DivPoolAlloc()
{
    if (this.freeDivs <= 0)
    {
        alert("DivPool.alloc(): Maximal number of divs reached: "+
              this.maxDivs);
        return null;
    }
    this.freeDivs--;

    var ret = this.freeList.div;
    this.freeList = this.freeList.next;
    return ret;
}
function DivPoolFree(theDiv)
{
    div.innerHTML = "";
    var el = {div: theDiv, next: this.freeList};
    this.freeList = el;

    this.freeDivs++;
}


/********** special functions used by tree **********/
function imgUrl(name)
{
    return '/tree_icons/'+ name +'.gif';
}
function renderImg(name, extra)
{
    var attrs;
    if (extra)
        attrs = ' '+extra;
    else
        attrs = '';
    return '<img src="'+ imgUrl(name) +'" width="18" height="16" align="absmiddle" '+ attrs +'>';
}

// toggler
function ntgl(id)
{
    var node = node_hash[id];
    node.toggle();
}
function nopen(id)
{
    var url = link('items', {parent_id: id});
    parent.items.location.href = url;
}

var _tree_y;
function setNextNodeY(y)
{
    _tree_y = y;
}
function nextNodeY()
{
    var ret = _tree_y;
    _tree_y += NODE_HEIGHT;
    return ret;
}

// will be directly used from NodeRenderOutline()
var _outline_html;
function setOutline(html)
{
    _outline_html = html;
}

/********** Node class **********/

function Node(parent, id, title, expandable)
{
    node_hash[id] = this;

    this.parent = parent;
    this.id = id;
    this.title = title;
    this.expandable = expandable;

    this.parentIndex = 0;
    this.depth = 0;
    
    this.expanded = false;

    if (expandable)
        this.dirty = true;

    this.render = NodeRender;
    this.renderOutline = NodeRenderOutline;
    this.renderOutlineRec = NodeRenderOutlineRec;
    this.toggle = NodeToggle;

    /* private */
    this.reloadChildren = NodeReloadChildren;
    this.getY = NodeGetY;
    this.setY = NodeSetY;
    this.setDiv = NodeSetDiv;

    this.getExpanderName = NodeGetExpanderName;

    this.expand = NodeExpand;
    this.collapse = NodeCollapse;
    this.show = NodeShow;
    this.hide = NodeHide;
    this.adjustUpwards = NodeAdjustUpwards;
    this.adjustDownwards = NodeAdjustDownwards;
}

function NodeRender()
{

    var r = '';
    r += '<div id="node_'+ this.id +'" class="node" style="top:'+ nextNodeY() +'px"><nobr>';
    r += this.renderOutline();
    r += '<a href="javascript:nopen('+ this.id +')">'+ this.title +'</a>';
    r += '</nobr></div>';
    return r;
}

function NodeGetExpanderName()
{
    var type;
    if (! this.expandable)
        type = 'join';
    else
    {
        if (this.expanded)
            type = 'minus';
        else
            type = 'plus';
    }

    var dir;
    if (! this.parent)
        dir = 'none';
    else
    {
        if (this.parentIndex < this.parent.children.length - 1)
            dir = 'both';
        else
            dir = 'bottom';
    }

    return type +'_'+ dir;
}

function NodeRenderOutline()
{
    var extra = 'onclick="ntgl('+ this.id +')"';
    var outlineImg = renderImg(this.getExpanderName(), extra);
    var folderImg = renderImg('folder_closed');
    var ret = outlineImg + folderImg;
   
    return _outline_html + ret;
}

function NodeRenderOutlineRec()
{
    var line;
    if (! this.parent)
        line = 'blank';
    else
    {
        if (this.parentIndex < this.parent.children.length - 1)
            line = 'join';
        else
            line = 'blank';
    }

    var ret = renderImg(line);
    
    if (this.parent)
        ret = this.parent.renderOutlineRec() + ret;
    return ret;
}

function NodeGetY()
{
    return parseInt(this.div.style.top);
}
function NodeSetY(y)
{
    this.div.style.top = '' + y + 'px';
}

function NodeSetDiv(div)
{
    this.div = div;
    // finding expander icon
    var imgs = div.getElementsByTagName('img');
    this.expImg = imgs[imgs.length - 2];
}

function NodeReloadChildren()
{
    // fetching xml
    var url = link('containers', {parent_id: this.id});
    var xml = fetchXML(url);
    if (! xml)
    {
        alert("could not fetch XML");
        return;
    }

    // parsing xml
    var containers = xmlGetElement(xml, "containers");
    if (! containers)
    {
        alert("no containers tag found");
        return;
    }
    var cts = containers.getElementsByTagName("container");
    if (! cts)
    {
        alert("no containers found");
        return;
    }

    // creating nodes
    var i;
    var len = cts.length;

    var htmls = new Array(len);
    var children = new Array(len);
    this.children = children;

    // set the y of the divs to be adjusted
    setNextNodeY(this.getY() + NODE_HEIGHT);

    setOutline(this.renderOutlineRec());
    for (var i = 0; i < len; i++)
    {
        var c = cts[i];

        var id = xmlGetAttribute(c, "id");
        var childCount = xmlGetAttribute(c, "childCount");
        if (childCount)
            childCount = parseInt(childCount);
        var title = xmlGetText(c);
        
        var expandable = childCount ? true : false;
        var child = new Node(this, id, title, expandable);
        child.depth = this.depth + 1;
        child.parentIndex = i;
        children[i] = child;
        htmls[i] = child.render();
    }

    if (! this.cdiv)
        this.cdiv = div_pool_inst.alloc();
    var html = htmls.join('');
    this.cdiv.innerHTML = html;

    // finding div elements and passing them to the children;
    var cur = this.cdiv.firstChild;
    for (i = 0; i < len; i++)
    {
        children[i].setDiv(cur);
        cur = cur.nextSibling;
    }

    this.dirty = false;
}

function NodeAdjustUpwards(childIndex)
{
    var children = this.children;
    var len = children.length;
    for (var i = childIndex + 1; i < len; i++)
        children[i].adjustDownwards();
    if (this.parent)
        this.parent.adjustUpwards(this.parentIndex);
}
function NodeAdjustDownwards()
{
    this.setY(nextNodeY());
    var children = this.children;
    if (! children)
        return;
    if (! this.expanded)
        return;
    var len = children.length;
    for (var i = 0; i < len; i++)
    {
        var child = children[i];
        child.adjustDownwards();
    }
}

function NodeToggle()
{
    // set the y of the divs to be adjusted
    setNextNodeY(this.getY() + NODE_HEIGHT);
    
    if (! this.expanded && this.dirty)
    {
        this.reloadChildren();
    }
    else
    {
        if (this.expanded)
            this.collapse();
        else
            this.expand();
    }
    this.expanded = !this.expanded;

    // update expander icon
    var expanderName = this.getExpanderName();
    this.expImg.src = imgUrl(expanderName);

    // adjusting positions
    // next y position is now where it should be
    if (this.parent)
        this.parent.adjustUpwards(this.parentIndex);
}

function NodeExpand()
{
    var children = this.children;
    var len = children.length;
    for (var i = 0; i < len; i++)
        children[i].show();
}
function NodeCollapse()
{
    var children = this.children;
    var len = children.length;
    for (var i = 0; i < len; i++)
        children[i].hide();
}

function NodeShow()
{
    this.setY(nextNodeY());
    this.div.style.visibility = 'visible';
    if (! this.expanded)
        return;
    var children = this.children;
    if (! children)
        return;
    var len = children.length;
    for (var i = 0; i < len; i++)
        children[i].show();
}
function NodeHide()
{
    this.div.style.visibility = 'hidden';
    this.setY(0);
    if (! this.expanded)
        return;
    var children = this.children;
    if (! children)
        return;
    var len = children.length;
    for (var i = 0; i < len; i++)
        children[i].hide();
}

/********** Tree class **********/

function Tree()
{
    node_hash = new Object();
    setNextNodeY(0);

    this.root = new Node(null, 0, "Root", true);
    this.cdiv = div_pool_inst.alloc();

    setOutline('');
    this.cdiv.innerHTML = this.root.render();
    this.root.setDiv(this.cdiv.firstChild);
}

/* *********************************** */

function initializeTree()
{
    var pool_div = document.getElementById('pool_div');
    div_pool_inst = new DivPool(DIV_POOL_SIZE, pool_div);
    div_pool_inst.init();
    
    tree_inst = new Tree();
    
}

