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
    var matches = date.match(/([0-9]+)-01-01/);
    if (matches)
        return matches[1];
    else
        return date;
}

// ---------------------------------------------------------------------------

function addAudio(obj)
{
 
    var desc = '';
    var artist_full;
    var album_full;
    
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

// uncomment this if you want to have track numbers in front of the title
// in album view
    
/*    
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
*/
    // comment the following line out if you uncomment the stuff above  :)
    var track = '';

    var chain = new Array('Audio', 'All audio');
    obj.title = title;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC);
    
    chain = new Array('Audio', 'Artists', artist, 'All songs');
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC);
    
    chain = new Array('Audio', 'All - full name');
    var temp = '';
    if (artist_full)
        temp = artist_full;
    
    if (album_full)
        temp = temp + ' - ' + album_full + ' - ';
    
    obj.title = temp + title;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC);
    
    chain = new Array('Audio', 'Artists', artist, 'All - full name');
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC);
    
    chain = new Array('Audio', 'Artists', artist, album);
    obj.title = track + title;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);
    
    chain = new Array('Audio', 'Albums', album);
    obj.title = track + title; 
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);
    
    chain = new Array('Audio', 'Genres', genre);
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_GENRE);
    
    chain = new Array('Audio', 'Year', date);
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC);
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
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER);

    var date = obj.meta[M_DATE];
    if (date)
    {
        chain = new Array('Photos', 'Date', date);
        addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER);
    }
}

var arr = orig.mimetype.split('/');
var mime = arr[0];

// var obj = copyObject(orig);

var obj = orig; 
obj.refID = orig.id;

if ((mime == 'audio') || (orig.mimetype == 'application/ogg'))
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

