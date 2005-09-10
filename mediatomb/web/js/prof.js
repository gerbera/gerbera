var _prof_text;
var _prof_last_millis;

function getMillis()
{
    var date = new Date();
    return date.getTime();
}
function profStart(tag)
{
    _prof_text = tag + '\n';
    _prof_last_millis = getMillis();
}
function profNext(tag)
{
    var millis = getMillis();
    var elapsed = millis - _prof_last_millis;
    _prof_text += ''+ elapsed +' : '+ tag +'\n';
    _prof_last_millis = millis;
}
function profEnd(tag)
{
    profNext(tag);
    alert(_prof_text);
    _prof_text = '';
}

