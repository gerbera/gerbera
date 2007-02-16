// global defines

// UPnP container classes
UPNP_MUSIC_CONTAINER = 'object.container.musicContainer';
UPNP_MUSIC_ALBUM     = 'object.container.album.musicAlbum';
//UPNP_MUSIC_ARTIST    = 'object.container.person.musicArtist';
UPNP_MUSIC_GENRE     = 'object.container.genre.musicGenre';

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
    addCdsObject(obj, createContainerChain(chain), UPNP_MUSIC_CONTAINER);
    
    chain = new Array('Audio', 'Artists', artist, 'All songs');
    addCdsObject(obj, createContainerChain(chain), UPNP_MUSIC_CONTAINER);
    
    chain = new Array('Audio', 'All - full name');
    temp = '';
    if (artist_full)
        temp = artist_full;
    
    if (album_full)
        temp = temp + ' - ' + album_full + ' - ';
    
    obj.title = temp + title;
    addCdsObject(obj, createContainerChain(chain), UPNP_MUSIC_CONTAINER);
    
    chain = new Array('Audio', 'Artists', artist, 'All - full name');
    addCdsObject(obj, createContainerChain(chain), UPNP_MUSIC_CONTAINER);
    
    chain = new Array('Audio', 'Artists', artist, album);
    obj.title = track + title;
    addCdsObject(obj, createContainerChain(chain), UPNP_MUSIC_ALBUM);
    
    chain = new Array('Audio', 'Albums', album);
    obj.title = track + title; 
    addCdsObject(obj, createContainerChain(chain), UPNP_MUSIC_ALBUM);
    
    chain = new Array('Audio', 'Genres', genre);
    addCdsObject(obj, createContainerChain(chain), UPNP_MUSIC_GENRE);
    
    
    chain = new Array('Audio', 'Year', date);
    addCdsObject(obj, createContainerChain(chain), UPNP_MUSIC_CONTAINER);
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

