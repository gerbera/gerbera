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
    
    var token = xmlGetElementText(xml, "token");
    
    var username = document.login_form.username.value;
    var password = document.login_form.password.value;
    
    // create authentication password
    password = '' + token + ':' + hex_md5(token + hex_md5(password));
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
    
    // get session id and redirect to index page
    var rootEl = xmlGetElement(xml, "root");
    SID = xmlGetAttribute(rootEl, "sid");
    setCookie("SID", SID);
    Element.hide("loginDiv");
    Element.show("mainDiv");
    updateTreeAfterLogin();
    if (!SID)
        alert('could not obtain session id');
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
    errorCheck(ajaxRequest.responseXML);
}
