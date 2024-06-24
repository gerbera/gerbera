/*GRB*
  Gerbera - https://gerbera.io/

  playlists.js - this file is part of Gerbera.

  Copyright (C) 2018-2024 Gerbera Contributors

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

function importPlaylist(obj, cont, rootPath, autoscanId, containerType) {
    if (!obj || obj === undefined) {
        print2("Error", "Playlist undefined");
    }

    print2("Info", "Processing playlist: " + obj.location);

    const objLocation = obj.location.substring(0, obj.location.lastIndexOf('/') + 1);
    const type = getPlaylistType(obj.mimetype);
    var obj_title = obj.title;

    const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];

    const chain = {
        objRoot: {
            id: boxSetup[BK_playlistRoot].id,
            title: boxSetup[BK_playlistRoot].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER,
            metaData: [] },
        allPlaylists: {
            id: boxSetup[BK_playlistAll].id,
            title: boxSetup[BK_playlistAll].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER },
        allDirectories: {
            id: boxSetup[BK_playlistAllDirectories].id,
            title: boxSetup[BK_playlistAllDirectories].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER },

        title: {
            searchable: true,
            title: obj_title,
            refID: obj.id,
            objectType: OBJECT_TYPE_CONTAINER,
            mtime: obj.mtime,
            upnpclass: UPNP_CLASS_PLAYLIST_CONTAINER,
            metaData: [] },
        lastPath: {
            title: '',
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER,
            metaData: [] }
    };
    chain.objRoot.metaData[M_CONTENT_CLASS] = [ UPNP_CLASS_PLAYLIST_ITEM ];
    var last_path = getLastPath2(obj.location, boxSetup[BK_playlistAllDirectories].size);

    var objChain = addContainerTree([chain.objRoot, chain.allPlaylists, chain.title]);

    var result = [];
    var objDirChain;
    if (last_path) {
        chain.title.searchable = false;
        const dirChain = [chain.objRoot, chain.allDirectories];
        for (var di = 0; di < last_path.length; di++) {
            const lastPathChain = {
                title: last_path[di],
                objectType: chain.lastPath.objectType,
                upnpclass: chain.lastPath.upnpclass,
                metaData: chain.lastPath.metaData };
            dirChain.push(lastPathChain);
        };
        dirChain.push(chain.title);
        objDirChain = addContainerTree(dirChain);
    }

    if (type === '') {
        print2("Error", "Unknown playlist mimetype: '" + obj.mimetype + "' of playlist '" + obj.location + "'");
    } else if (type === 'm3u') {
        result = readM3uPlaylist(obj_title, objLocation, objChain, objDirChain);
    } else if (type === 'pls') {
        result = readPlsPlaylist(obj_title, objLocation, objChain, objDirChain);
    } else if (type === 'asx') {
        result = readAsxPlaylist(obj_title, objLocation, objChain, objDirChain);
    }
    return result;
}

var playlist;
var cont;
var object_ref_list;
// compatibility with older configurations
if (!cont || cont === undefined)
    cont = playlist;
if (playlist && playlist !== undefined)
    object_ref_list = importPlaylist(playlist, cont, "", -1, "");
