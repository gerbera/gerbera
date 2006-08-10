/*
function fetchXML(url)
{
    var req = false;
    try
    {
        req = new ActiveXObject("Msxml2.XMLHTTP");
    }
    catch (e)
    {
        try
        {
            req = new ActiveXObject("Microsoft.XMLHTTP");
        }
        catch (E)
        {
            req = false;
        }
     }
    if (!req && typeof XMLHttpRequest!='undefined')
    {
        req = new XMLHttpRequest();
    }
    if (!req)
    {
        alert("XMLHttpRequest unavailable");
        return null;
    }
    req.open("GET", url, false);
    req.send(null);
    return req.responseXML;
}
*/

function xmlGetElement(parent, name)
{
    var ns = null;
    
    /*
    if (name.substring(0, 3) == "dc:")
    {
        ns = "http://purl.org/dc/elements/1.1/";
        name = name.substring(3);
    }
    else if (name.substring(0, 5) == "upnp:")
    {
        ns = "urn:schemas-upnp-org:metadata-1-0/upnp/";
        name = name.substring(5);
    }
    */
    
    var els;
    if (ns)
        els = parent.getElementsByTagNameNS(ns, name);
    else
        els = parent.getElementsByTagName(name);
    if (!els || !els.length)
        return null;
    else
        return els[0];
}

function xmlGetElementText(parent, name)
{
    var el = xmlGetElement(parent, name);
    return xmlGetText(el);
}
function xmlGetText(el)
{
    if (el == null)
        return null;
    if (el.childNodes.length > 1)
        return el.childNodes[1].nodeValue;
    else
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


    /*
    xmlhttp.onreadystatechange = function() {
        if (xmlhttp.readyState == 4)
        {
            if (.status == 200)
            {
                alert(req.responseXML);
            }
            else
            {
                alert(xmlhttp.responseText);
            }
        }
    }
    */
