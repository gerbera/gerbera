/*GRB*
  Gerbera - https://gerbera.io/

  playlists.js - this file is part of Gerbera.

  Copyright (C) 2018 Gerbera Contributors

  Gerbera is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2
  as published by the Free Software Foundation.

  Gerbera is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

  $Id$
*/

var title, line, matches, file, lastId, id,
    type, playlistLocation, playlistOrder = 1,
    playlist_title, dot_index, playlistChain, playlistDirChain,
    last_path;

function addPlaylistItem(location, title, playlistChain, order) {
    if (location.match(/^.*:\/\//)) {
        var exturl = {};
        exturl.mimetype = 'audio/mpeg';
        exturl.objectType = OBJECT_TYPE_ITEM_EXTERNAL_URL;
        exturl.location = location;
        exturl.title = (title ? title : location);
        exturl.protocol = 'http-get';
        exturl.upnpclass = UPNP_CLASS_ITEM_MUSIC_TRACK;
        exturl.description = "Song from " + playlist.title;
        exturl.playlistOrder = (order ? order : playlistOrder++);
        addCdsObject(exturl, playlistChain,  UPNP_CLASS_PLAYLIST_CONTAINER);
    } else {
        if (location.substr(0,1) !== '/') {
            location = playlistLocation + location;
        }

        var cds = getCdsObject(location);
        var item = copyObject(cds);

        item.playlistOrder = (order ? order : playlistOrder++);
        item.title = item.meta[M_TITLE];
        addCdsObject(item, playlistChain,  UPNP_CLASS_PLAYLIST_CONTAINER);
    }
}

print("Processing playlist: " + playlist.location);

playlistLocation = playlist.location.substring(0, playlist.location.lastIndexOf('/') + 1);

// the function getPlaylistType is defined in common.js
type = getPlaylistType(playlist.mimetype);

// the function createContainerChain is defined in common.js
playlist_title = playlist.title;
dot_index = playlist_title.lastIndexOf('.');
if (dot_index > 1) {
    playlist_title = playlist_title.substring(0, dot_index);
}

playlistChain = createContainerChain(['Playlists', 'All Playlists', playlist_title]);
last_path = getLastPath(playlist.location);

if (last_path) {
    playlistDirChain = createContainerChain(['Playlists', 'Directories', last_path, playlist_title]);
}

if (type === '') {
    print("Unknown playlist mimetype: '" + playlist.mimetype + "' of playlist '" + playlist.location + "'");
} else if (type === 'm3u') {
    title = null;
    line = readln();
    do {
        if (line.match(/^#EXTINF:(-?\d+),(\S.+)$/i)) {
            // duration = RegExp.$1; // currently unused
            matches = line.match(/^#EXTINF:(-?\d+),(\S.+)$/i);
            title = matches[2];
        }
        else if (! line.match(/^(#|\s*$)/)) {
            addPlaylistItem(line, title, playlistChain);
            if (playlistDirChain)
                addPlaylistItem(line, title, playlistDirChain);

            title = null;
        }
        
        line = readln();
    } while (line);
} else if (type === 'pls') {
    title = null;
    file = null;
    lastId = -1;
    line = readln();
    do {
        if (line.match(/^\[playlist\]$/i)) {
            // It seems to be a correct playlist, but we will try to parse it
            // anyway even if this header is missing, so do nothing.
        } else if (line.match(/^NumberOfEntries=(\d+)$/i)) {
            // var numEntries = RegExp.$1;
        } else if (line.match(/^Version=(\d+)$/i)) {
            // var plsVersion =  RegExp.$1;
        } else if (line.match(/^File\s*(\d+)\s*=\s*(\S.+)$/i)) {
            matches = line.match(/^File\s*(\d+)\s*=\s*(\S.+)$/i);
            var thisFile = matches[2];
            id = parseInt(matches[1], 10);
            if (lastId === -1)
                lastId = id;
            if (lastId !== id)
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
        } else if (line.match(/^Title\s*(\d+)\s*=\s*(\S.+)$/i)) {
            matches = line.match(/^Title\s*(\d+)\s*=\s*(\S.+)$/i);
            var thisTitle = matches[2];
            id = parseInt(matches[1], 10);
            if (lastId === -1)
                lastId = id;
            if (lastId !== id)
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
        } else if (line.match(/^Length\s*(\d+)\s*=\s*(\S.+)$/i)) {
            // currently unused
        }
        
        line = readln();
    } while (line);
    
    if (file) {
        addPlaylistItem(file, title, playlistChain, lastId);
        if (playlistDirChain)
            addPlaylistItem(file, title, playlistDirChain, lastId);
    }
}
