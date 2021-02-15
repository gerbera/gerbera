/*GRB*
  Gerbera - https://gerbera.io/

  playlists.js - this file is part of Gerbera.

  Copyright (C) 2018-2021 Gerbera Contributors

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
const dot_index = playlist_title.lastIndexOf('.');
if (dot_index > 1) {
    playlist_title = playlist_title.substring(0, dot_index);
}

// the function createContainerChain is defined in common.js
var playlistChain = createContainerChain(['Playlists', 'All Playlists', playlist_title]);
var last_path = getLastPath(playlist.location);

var playlistDirChain;
if (last_path) {
    playlistDirChain = createContainerChain(['Playlists', 'Directories', last_path, playlist_title]);
}

if (type === '') {
    print("Unknown playlist mimetype: '" + playlist.mimetype + "' of playlist '" + playlist.location + "'");
} else if (type === 'm3u') {
    readM3uPlaylist(playlist_title, playlistChain, playlistDirChain);
} else if (type === 'pls') {
    readPlsPlaylist(playlist_title, playlistChain, playlistDirChain);
}
