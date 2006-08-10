function copyObject(obj)
{
    var key;
    var copy = new Object();
    for (key in obj)
    {
        copy[key] = obj[key];
    }
    var meta = obj['meta'];
    if (meta)
    {
        meta_copy = new Object();
        for (key in meta)
        {
            meta_copy[key] = meta[key];
        }
        copy.meta = meta_copy;
    }
    return obj;
}
function splitPath(path)
{
    var start = 0;
    var end = path.length;
    if (path[0] == '/')
        start++;
    if (path[end - 1] == '/')
        end--;
    var str = path.substring(start, end);
    return str.split('/');
}
function getFileName(path)
{
    if (path[path.length - 1] == '/')
        return null;
    var i = path.lastIndexOf('/');
    return path.substring(i+1);
}
function getFilePath(path)
{
    if (path[path.length - 1] == '/')
        return path.substring(0, path.length - 1);
    var i = path.lastIndexOf('/');
    return path.substring(0, i);
}

function concatArrays(arr1, arr2)
{
    var arr = new Array();
    var i;
    for(i = 0; i < arr1.length; i++)
        arr[arr.length] = arr1[i];
    for(i = 0; i < arr2.length; i++)
        arr[arr.length] = arr2[i];
    return arr;
}

function addContainerChain(parentID, arr)
{
    for (var i = 0; i < arr.length; i++)
    {
        var title = arr[i];
        var cont = new Object();
        cont.objectType = OBJECT_TYPE_CONTAINER;
        cont.parentID = parentID;
        cont.title = title;
        cont.upnp_class = 'object.container';
        parentID = addCdsObject(cont);
    }
    return parentID;
}

function normalizeDate(date)
{
    var matches = date.match(/([0-9]+)-00-00/);
    if (matches)
        return matches[1];
    else
        return date;
}

// ---------------------------------------------------------------------------

function addAudio(obj)
{
 
    var desc = '';
    
    // first gather data
    var title = obj.meta[M_TITLE];
    if (!title) title = obj.title;

    var artist = obj.meta[M_ARTIST];
    if (!artist) 
        artist = 'Unknown';
    else
        desc = artist;

    var album = obj.meta[M_ALBUM];
    if (!album) 
        album = 'Unknown';
    else
        desc = desc + ', ' + album;

    if (desc)
        desc = desc + ', ';

    desc = desc + title;

    var date = obj.meta[M_DATE];
    if (!date)
        date = 'Unknown';
    else
    {
        date = normalizeDate(date);
        desc = desc + ', ' + date;
    }

    var genre = obj.meta[M_GENRE];
    if (!genre)
        genre = 'Unknown';
    else
        desc = desc + ', ' + genre;

    var description = obj.meta[M_DESCRIPTION];
    if (!description) 
    {
        obj.meta[M_DESCRIPTION] = desc;
    }

    var chain = new Array('Audio', 'All audio');
    obj.title = title;
    obj.parentID = addContainerChain("0", chain);
    addCdsObject(obj);

    chain = new Array('Audio', 'Artists', artist, 'All songs');
    obj.parentID = addContainerChain("0", chain);
    addCdsObject(obj);

    chain = new Array('Audio', 'All - full name');
    obj.title = artist + ' - ' + album + ' - ' + title;
    obj.parentID = addContainerChain("0", chain);
    addCdsObject(obj);

    chain = new Array('Audio', 'Artists', artist, 'All - full name');
    obj.parentID = addContainerChain("0", chain);
    addCdsObject(obj);

    chain = new Array('Audio', 'Artists', artist, album);
    obj.title = title;
    obj.parentID = addContainerChain("0", chain);
    addCdsObject(obj);

    chain = new Array('Audio', 'Albums', album);
    obj.parentID = addContainerChain("0", chain);
    addCdsObject(obj);

    chain = new Array('Audio', 'Genres', genre);
    obj.parentID = addContainerChain("0", chain);
    addCdsObject(obj);


    chain = new Array('Audio', 'Date', date);
    obj.parentID = addContainerChain("0", chain);
    addCdsObject(obj);

    
}

// currently no video metadata supported
function addVideo(obj)
{
    var chain = new Array('Video');
    obj.parentID = addContainerChain("0", chain);
    addCdsObject(obj);
}

// currently no image metadata supported
function addImage(obj)
{
    var chain = new Array('Photos', 'All Photos');
    obj.parentID = addContainerChain("0", chain);
    addCdsObject(obj);

    var date = obj.meta[M_DATE];
    if (date)
    {
        chain = new Array('Photos', 'Date', date);
        obj.parentID = addContainerChain("0", chain);
        addCdsObject(obj);
    }
}

var arr = orig.mimetype.split("/");
var mime = arr[0];

var obj = copyObject(orig);
obj.refID = orig.id;

if (mime == 'audio')
{
    addAudio(obj);
}

if (mime == 'video')
{
    addVideo(obj);
}

if (mime == 'image')
{
    addImage(obj);
}

