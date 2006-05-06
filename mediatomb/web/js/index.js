function setValue(key, value)
{
    var el = document.getElementById(key);
    if (el)
    {
        if (value)
            el.innerHTML = value;
        else
            el.innerHTML = '&nbsp;';
    }
}

function updateInfo()
{
    var url = link('acct');
    var xml = fetchXML(url);
    if (! xml)
        return;
    var totalFiles = xmlGetElementText(xml, 'total-files');
    setValue('total-files', totalFiles);
    
    var currentTask = xmlGetElementText(xml, 'current-task');
    setValue('current-task', currentTask);

    if (currentTask)
        setTimeout('updateInfo()', 5000);
}

function refreshView()
{
    top.main.location.href = top.main.location.href;
}

function addTask(name, params)
{
    params.name = name;
    var url = link('task', params);
    delete params.name;
    var xml = fetchXML(url);
    if (! xml)
        return;
    updateInfo();
}

