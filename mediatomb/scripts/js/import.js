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

    var aux = obj['aux'];
    if (aux)
    {
        aux_copy = new Object();
        for (key in aux)
        {
            aux_copy[key] = aux[key];
        }
        copy.aux = aux_copy;
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

function escapeSlash(name)
{
    name = name.replace(/\\/g, "\\\\");
    name = name.replace(/\//g, "\\/");
    return name;
}

function createContainerChain(arr)
{
    var path = '';
    for (var i = 0; i < arr.length; i++)
    {
        path = path + '/' + escapeSlash(arr[i]);
    }
    return path;
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
    {
        artist = 'Unknown';
        artist_full = null;
    }
    else
    {
        artist_full = artist;
        desc = artist;
    }

    var album = obj.meta[M_ALBUM];
    if (!album) 
    {
        album = 'Unknown';
        album_full = null;
    }
    else
    {
        desc = desc + ', ' + album;
        album_full = album;
    }

    if (desc)
        desc = desc + ', ';

    desc = desc + title;

    var date = obj.meta[M_DATE];
    if (!date)
    {
        date = 'Unknown';
    }
    else
    {
        date = normalizeDate(date);
        desc = desc + ', ' + date;
    }

    var genre = obj.meta[M_GENRE];
    if (!genre)
    {
        genre = 'Unknown';
    }
    else
    {
        desc = desc + ', ' + genre;
    }

    var description = obj.meta[M_DESCRIPTION];
    if (!description) 
    {
        obj.meta[M_DESCRIPTION] = desc;
    }

    var track = obj.meta[M_TRACKNUMBER];
    if (!track)
        track = '';
    else
    {
        if (track.length == 1)
        {
            track = '0' + track;
        }
        track = track + ' ';
    }
    var chain = new Array('Audio', 'All audio');
    obj.title = title;
    addCdsObject(obj, createContainerChain(chain));

    chain = new Array('Audio', 'Artists', artist, 'All songs');
    addCdsObject(obj, createContainerChain(chain));

    chain = new Array('Audio', 'All - full name');
    temp = '';
    if (artist_full)
        temp = artist_full;

    if (album_full)
        temp = temp + ' - ' + album_full + ' - ';

    obj.title = temp + title;
    addCdsObject(obj, createContainerChain(chain));

    chain = new Array('Audio', 'Artists', artist, 'All - full name');
    addCdsObject(obj, createContainerChain(chain));

    chain = new Array('Audio', 'Artists', artist, album);
    obj.title = track + title;
    addCdsObject(obj, createContainerChain(chain));

    chain = new Array('Audio', 'Albums', album);
    obj.title = track + title; 
    addCdsObject(obj, createContainerChain(chain));

    chain = new Array('Audio', 'Genres', genre);
    addCdsObject(obj, createContainerChain(chain));


    chain = new Array('Audio', 'Date', date);
    addCdsObject(obj, createContainerChain(chain));

    
}

// currently no video metadata supported
function addVideo(obj)
{
    var chain = new Array('Video');
    addCdsObject(obj, createContainerChain(chain));
}

// currently no image metadata supported
function addImage(obj)
{
    var chain = new Array('Photos', 'All Photos');
    addCdsObject(obj, createContainerChain(chain));

    var date = obj.meta[M_DATE];
    if (date)
    {
        chain = new Array('Photos', 'Date', date);
        addCdsObject(obj, createContainerChain(chain));
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

