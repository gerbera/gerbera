// Default MediaTomb playlist parsing script.
//
// Note: This script currently only handles .m3u and .pls playlists, but it can
// easily be extended. It doesn't use the "correct" way to parse the playlist, 
// but a more fault-tolerant way, so it will try to do it's best and won't
// complain even if the playlist is completely messed up.

/*MT_F*
    
    MediaTomb - http://www.mediatomb.cc/
    
    playlists.js - this file is part of MediaTomb.
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    This file is free software; the copyright owners give unlimited permission
    to copy and/or redistribute it; with or without modifications, as long as
    this notice is preserved.
    
    This file is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    
    $Id$
*/

var playlistOrder = 1;

function addPlaylistItem(location, title, playlistChain, order)
{
    if (location.match(/^.*:\/\//))
    {
        var exturl = new Object();
        exturl.mimetype = 'audio/mpeg';
        exturl.objectType = OBJECT_TYPE_ITEM_EXTERNAL_URL;
        exturl.location = location;
        exturl.title = (title ? title : location);
        exturl.protocol = 'http-get';
        exturl.upnpclass = UPNP_CLASS_ITEM_MUSIC_TRACK;
        exturl.description = "Song from " + playlist.title;
        exturl.playlistOrder = (order ? order : playlistOrder++);
        addCdsObject(exturl, playlistChain,  UPNP_CLASS_PLAYLIST_CONTAINER);
    }
    else
    {
        if (location.substr(0,1) != '/')
            location = playlistLocation + location;
        var item  = new Object();
        item.location = location;
        if (title)
            item.title = title;
        else
        {
            var locationParts = location.split('/');
            item.title = locationParts[locationParts.length - 1];
            if (! item.title)
                item.title = location;
        }
        item.objectType = OBJECT_TYPE_ITEM;
        item.playlistOrder = (order ? order : playlistOrder++);
        addCdsObject(item, playlistChain,  UPNP_CLASS_PLAYLIST_CONTAINER);
    }
}

print("Processing playlist: " + playlist.location);

var playlistLocation = playlist.location.substring(0, playlist.location.lastIndexOf('/') + 1);

// the function getPlaylistType is defined in common.js
var type = getPlaylistType(playlist.mimetype);

// the function createContainerChain is defined in common.js
var playlist_title = playlist.title
var dot_index = playlist_title.lastIndexOf('.');
if (dot_index > 1)
        playlist_title = playlist_title.substring(0, dot_index);

var playlistChain = createContainerChain(new Array('Playlists', 'All Playlists', playlist_title));

var playlistDirChain;

var last_path = getLastPath(playlist.location);
if (last_path)
    playlistDirChain = createContainerChain(new Array('Playlists', 'Directories', last_path, playlist_title));

if (type == '')
{
    print("Unknown playlist mimetype: '" + playlist.mimetype + "' of playlist '" + playlist.location + "'");
}
else if (type == 'm3u')
{
    var title = null;
    var line = readln();
    do
    {
        if (line.match(/^#EXTINF:(-?\d+),(\S.+)$/i))
        {
            // duration = RegExp.$1; // currently unused
            title = RegExp.$2;
        }
        else if (! line.match(/^(#|\s*$)/))
        {
            addPlaylistItem(line, title, playlistChain);
            if (playlistDirChain)
                addPlaylistItem(line, title, playlistDirChain);

            title = null;
        }
        
        line = readln();
    }
    while (line);
}
else if (type == 'pls')
{
    var title = null;
    var file = null;
    var lastId = -1;
    var line = readln();
    do
    {
        if (line.match(/^\[playlist\]$/i))
        {
            // It seems to be a correct playlist, but we will try to parse it
            // anyway even if this header is missing, so do nothing.
        }
        else if (line.match(/^NumberOfEntries=(\d+)$/i))
        {
            // var numEntries = RegExp.$1;
        }
        else if (line.match(/^Version=(\d+)$/i))
        {
            // var plsVersion =  RegExp.$1;
        }
        else if (line.match(/^File\s*(\d+)\s*=\s*(\S.+)$/i))
        {
            var thisFile = RegExp.$2;
            var id = parseInt(RegExp.$1, 10);
            if (lastId == -1)
                lastId = id;
            if (lastId != id)
            {
                if (file)
                {
                    addPlaylistItem(file, title, playlistChain, lastId);
                    if (playlistDirChain)
                        addPlaylistItem(file, title, playlistDirChain, lastId);
                }

                title = null;
                lastId = id;
            }
            file = thisFile
        }
        else if (line.match(/^Title\s*(\d+)\s*=\s*(\S.+)$/i))
        {
            var thisTitle = RegExp.$2;
            var id = parseInt(RegExp.$1, 10);
            if (lastId == -1)
                lastId = id;
            if (lastId != id)
            {
                if (file)
                {
                    addPlaylistItem(file, title, playlistChain, lastId);
                    if (playlistDirChain)
                        addPlaylistItem(file, title, playlistDirChain, lastId);
                }

                file = null;
                lastId = id;
            }
            title = thisTitle;
        }
        else if (line.match(/^Length\s*(\d+)\s*=\s*(\S.+)$/i))
        {
            // currently unused
        }
        
        line = readln();
    }
    while (line);
    
    if (file)
    {
        addPlaylistItem(file, title, playlistChain, lastId);
        if (playlistDirChain)
            addPlaylistItem(file, title, playlistDirChain, lastId);
    }
}
