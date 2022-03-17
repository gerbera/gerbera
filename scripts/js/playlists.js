/*GRB*
  Gerbera - https://gerbera.io/

  playlists.js - this file is part of Gerbera.

  Copyright (C) 2018-2022 Gerbera Contributors

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

print("Processing playlist: " + playlist.location);

const playlistLocation = playlist.location.substring(0, playlist.location.lastIndexOf('/') + 1);

// the function getPlaylistType is defined in common.js
const type = getPlaylistType(playlist.mimetype);
var playlist_title = playlist.title;
var last_path = getLastPath(playlist.location);

const chain = {
    playlistRoot: { title: 'Playlists', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER, metaData: [] },
    allPlaylists: { title: 'All Playlists', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
    allDirectories: { title: 'Directories', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },

    title: { searchable: true, title: playlist_title, refID: playlist.id, objectType: OBJECT_TYPE_CONTAINER, mtime: playlist.mtime, upnpclass: UPNP_CLASS_PLAYLIST_CONTAINER, metaData: [] },
    lastPath: { title: last_path, objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER, metaData: [] }
};
chain.playlistRoot.metaData[M_CONTENT_CLASS] = [ UPNP_CLASS_PLAYLIST_ITEM ];

var playlistChain = addContainerTree([chain.playlistRoot, chain.allPlaylists, chain.title]);

var playlistDirChain;
if (last_path) {
    chain.title.searchable = false;
    playlistDirChain = addContainerTree([chain.playlistRoot, chain.allDirectories, chain.lastPath, chain.title]);
}

if (type === '') {
    print("Unknown playlist mimetype: '" + playlist.mimetype + "' of playlist '" + playlist.location + "'");
} else if (type === 'm3u') {
    readM3uPlaylist(playlist_title, playlistChain, playlistDirChain);
} else if (type === 'pls') {
    readPlsPlaylist(playlist_title, playlistChain, playlistDirChain);
} else if (type === 'asx') {
    readAsxPlaylist(playlist_title, playlistChain, playlistDirChain);
}
