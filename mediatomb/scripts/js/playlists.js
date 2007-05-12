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

function addPlaylistItem(location, playlistChain, duration, title)
{
    if (location.match(/^http/))
    {
        var exturl = new Object();
        exturl.mimetype = 'audio/mpeg';
        exturl.objectType = OBJECT_TYPE_ITEM_EXTERNAL_URL;
        exturl.location = location;
        exturl.title = (title ? title : location);
        exturl.protocol = 'http-get';
        exturl.upnpclass = UPNP_CLASS_ITEM_MUSIC_TRACK;
        exturl.description = "Song from " + playlist.title;
        print("Adding " + line);
        addCdsObject(exturl, playlistChain,  UPNP_CLASS_PLAYLIST_CONTAINER);
    }
    else
    {
        var item  = new Object();
        item.location = location;
        item.title = (title ? title : location);
        item.objectType = OBJECT_TYPE_ITEM;
        addCdsObject(item, playlistChain,  UPNP_CLASS_PLAYLIST_CONTAINER);
    }
}

print("Processing playlist: " + playlist.location);

var type = '';
if (playlist.mimetype == 'audio/x-mpegurl')
    type = 'm3u';
if (playlist.mimetype == 'audio/x-scpls')
    type = 'pls';

var playlistChain = createContainerChain(new Array('Audio', 'Playlists', playlist.title));

if (type == '')
{
    print("Unknown playlist mimetype: '" + playlist.mimetype + "' of playlist '" + playlist.location + "'");
}
else if (type == 'm3u')
{
    var line;
    var duration = null;
    var title = null;
    do
    {
        line = readln();
        if (line.match(/^#EXTINF\s*:\s*(\d+)\s*,\s*(\S.+)$/i))
        {
            duration = RegExp.$1;
            title = RegExp.$2;
        }
        else if (! line.match(/^(#|\s*$)/)
        {
            addPlaylistItem(line, playlistChain, duration, title);
            duration = null;
            title = null;
        }
    }
    while (line);
}
else if (type == 'pls')
{
/*
    var line;
    var duration = null;
    var title = null;
    do
    {
    }
    while (line);
*/
}

