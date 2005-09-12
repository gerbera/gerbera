function link(req_type, param)
{
    var url = "/content/interface?req_type="+ req_type +"&sid="+ SID +
               "&slt="+ Math.random();
    if (param)
    {
        for (var key in param)
        {
            url += "&" + key +"="+ param[key];
        }
    }
    return url;
}

