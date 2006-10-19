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
    loggedIn = true;
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
    errorCheck(ajaxRequest.responseXML, true);
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
