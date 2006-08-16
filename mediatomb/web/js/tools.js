function link(req_type, param)
{
    var url = "/content/interface?req_type="+ req_type +"&sid="+ SID;
    if (param)
        for (var key in param)
        {
            url += "&" + key +"="+ param[key];
        }
    return url;
}

function isTypeDb()
{
    return (TYPE == "db");
}

function xmlGetElement(parent, name)
{
    /*var returnVal = Try.these
    (
        function()
        {
            var ns = null;
            var tmpname = name;
            if (tmpname.substring(0, 3) == "dc:")
            {
                ns = "http://purl.org/dc/elements/1.1/";
                tmpname = tmpname.substring(3);
            }
            else if (tmpname.substring(0, 5) == "upnp:")
            {
                ns = "urn:schemas-upnp-org:metadata-1-0/upnp/";
                tmpname = tmpname.substring(5);
            }
            if (ns)
                return parent.getElementsByTagNameNS(ns, tmpname);
            else
                return parent.getElementsByTagName(tmpname);
        },
        function()
        {
            return parent.getElementsByTagName(name);
        }
    );*/
    
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

function ensureElementVisibility(win, el, bottomAdd)
{
    var scrollpos = win.pageYOffset;
    var xoffset = win.pageXOffset;
    var viewheight = win.innerHeight;
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
