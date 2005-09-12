var Audio;
var Audio_All;
var Audio_Artists;
var Audio_Fullname;
var Audio_Albums;
var Audio_Genres;
var Audio_Date;

var Video;

var Photos;
var Photos_All;
var Photos_Date;

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

function addContainer(parentID, title)
{
    var cont = new Object();
    cont.objectType = OBJECT_TYPE_CONTAINER;
    cont.parentID = parentID;
    cont.title = title;
    cont.upnp_class = 'object.container';
    return addCdsObject(cont);
}

function addContainers(parentID, arr)
{
    for (var i = 0; i < arr.length; i++)
        parentID = addContainer(parentID, arr[i]);
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

    var parentID;
    if (! Audio)
        Audio = addContainer(0, 'Audio');
    
    if (! Audio_All)
        Audio_All = addContainer(Audio, 'All audio');
    obj.title = title;
    obj.parentID = Audio_All;
    addCdsObject(obj);

    if (! Audio_Artists)
        Audio_Artists = addContainer(Audio, 'Artists');
    
    obj.parentID = addContainers(Audio_Artists, [artist, 'All songs']);
    addCdsObject(obj);

    if (! Audio_Fullname)
        Audio_Fullname = addContainer(Audio, 'All - full name');
    obj.title = artist + ' - ' + album + ' - ' + title;
    obj.parentID = Audio_Fullname;
    addCdsObject(obj);

    obj.parentID = addContainers(Audio_Artists, [artist, 'All - full name']);
    addCdsObject(obj);

    obj.parentID = addContainers(Audio_Artists, [artist, album]);
    obj.title = title;
    addCdsObject(obj);

    if (! Audio_Albums)
        Audio_Albums = addContainer(Audio, 'Albums');
    obj.parentID = addContainer(Audio_Albums, album);
    addCdsObject(obj);

    if (! Audio_Genres)
        Audio_Genres = addContainer(Audio, 'Genres');
    obj.parentID = addContainer(Audio_Genres, genre);
    addCdsObject(obj);

    if (! Audio_Date)
        Audio_Date = addContainer(Audio, 'Date');
    obj.parentID = addContainer(Audio_Date, date);
    addCdsObject(obj);
}

// currently no video metadata supported
function addVideo(obj)
{
    if (! Video)
        Video = addContainer(0, 'Video');
    obj.parentID = Video;
    addCdsObject(obj);
}

// currently no image metadata supported
function addImage(obj)
{
    if (! Photos)
        Photos = addContainer(0, 'Photos');

    if (! Photos_All)
        Photos_All = addContainer(Photos, 'All Photos');
    obj.parentID = Photos_All;
    addCdsObject(obj);

    var date = obj.meta[M_DATE];
    if (date)
    {
        if (! Photos_Date)
            Photos_Date = addContainer(Photos, 'Date');
        obj.parentID = addContainer(Photos_Date, date);
        addCdsObject(obj);
    }
}

function PROCESS_OBJECT(orig)
{
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
}

