/*GRB*
  Gerbera - https://gerbera.io/

  playlists.js - this file is part of Gerbera.

  Copyright (C) 2018-2023 Gerbera Contributors

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

var playlist;

function importPlaylist(obj, rootPath, autoscanId, containerType) {
    playlist = obj;
    if (!obj || obj === undefined) {
        print("Playlist undefined");
    }

    print("Processing obj: " + obj.location);

    const objLocation = obj.location.substring(0, obj.location.lastIndexOf('/') + 1);
    const type = getPlaylistType(obj.mimetype);
    var obj_title = obj.title;
    var last_path = getLastPath(obj.location);

    const chain = {
        objRoot: { title: 'Playlists', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER, metaData: [] },
        allPlaylists: { title: 'All Playlists', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allDirectories: { title: 'Directories', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },

        title: { searchable: true, title: obj_title, refID: obj.id, objectType: OBJECT_TYPE_CONTAINER, mtime: obj.mtime, upnpclass: UPNP_CLASS_PLAYLIST_CONTAINER, metaData: [] },
        lastPath: { title: last_path, objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER, metaData: [] }
    };
    chain.objRoot.metaData[M_CONTENT_CLASS] = [ UPNP_CLASS_PLAYLIST_ITEM ];

    var objChain = addContainerTree([chain.objRoot, chain.allPlaylists, chain.title]);

    var objDirChain;
    if (last_path) {
        chain.title.searchable = false;
        objDirChain = addContainerTree([chain.objRoot, chain.allDirectories, chain.lastPath, chain.title]);
    }

    if (type === '') {
        print("Unknown obj mimetype: '" + obj.mimetype + "' of obj '" + obj.location + "'");
    } else if (type === 'm3u') {
        readM3uPlaylist(obj_title, objLocation, objChain, objDirChain);
    } else if (type === 'pls') {
        readPlsPlaylist(obj_title, objLocation, objChain, objDirChain);
    } else if (type === 'asx') {
        readAsxPlaylist(obj_title, objLocation, objChain, objDirChain);
    }
}

// compatibility with older configurations
if (playlist && playlist !== undefined)
    importPlaylist(playlist, "", -1, "");
