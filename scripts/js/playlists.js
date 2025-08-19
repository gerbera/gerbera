/*GRB*
  Gerbera - https://gerbera.io/

  playlists.js - this file is part of Gerbera.

  Copyright (C) 2018-2025 Gerbera Contributors

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
  const chainSetup = config['/import/scripting/virtual-layout/boxlayout/chain/audio'];
  const boxes = [
    BK_playlistRoot,
    BK_playlistAll,
    BK_playlistAllDirectories,
  ];
  const _Chain = prepareChains(boxes, boxSetup, chainSetup);
  const chain = {
    objRoot: _Chain[BK_playlistRoot],
    allPlaylists: _Chain[BK_playlistAll],
    allDirectories: _Chain[BK_playlistAllDirectories],
    title: {
      searchable: true,
      title: obj_title,
      refID: obj.id,
      objectType: OBJECT_TYPE_CONTAINER,
      mtime: obj.mtime,
      upnpclass: UPNP_CLASS_PLAYLIST_CONTAINER,
      metaData: []
    },
    lastPath: {
      title: '',
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: []
    }
  };
  chain.objRoot.metaData[M_CONTENT_CLASS] = [UPNP_CLASS_PLAYLIST_ITEM];
  var last_path = getLastPath2(obj.location, boxSetup[BK_playlistAllDirectories].size);

  const objChain = addContainerTree([chain.objRoot, chain.allPlaylists, chain.title]);

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
        metaData: chain.lastPath.metaData
      };
      dirChain.push(lastPathChain);
    };
    dirChain.push(chain.title);
    objDirChain = addContainerTree(dirChain);
  }

  if (type === '') {
    print2("Error", "Unknown playlist mimetype: '" + obj.mimetype + "' of playlist '" + obj.location + "'");
  } else if (type === 'm3u') {
    result = readM3uPlaylist(obj_title, objLocation, objChain, objDirChain, rootPath);
  } else if (type === 'pls') {
    result = readPlsPlaylist(obj_title, objLocation, objChain, objDirChain, rootPath);
  } else if (type === 'asx') {
    result = readAsxPlaylist(obj_title, objLocation, objChain, objDirChain, rootPath);
  }
  return result;
}

// doc-playlist-m3u-parse-begin
function readM3uPlaylist(playlist_title, playlistLocation, playlistChain, playlistDirChain, rootPath) {
  var entry = {
    title: null,
    location: null,
    order: 0,
    size: -1,
    writeThrough: -1,
    mimetype: null,
    description: null,
    protocol: null,
    extra: [],
  };
  var line = readln();
  var playlistOrder = 1;

  // Remove a BOM if there is one
  if (line && line.charCodeAt(0) === 0xFEFF) {
    line = line.substr(1);
  }
  const result = [];

  // Here is the do - while loop which will read the playlist line by line.
  do {
    var matches = line ? line.match(/^#EXTINF:(-?\d+),\s*(\S.*)$/i) : undefined;

    if (matches && matches.length > 1) {
      // duration = matches[1]; // currently unused
      entry.title = matches[2];
      matches = line.match(/^#EXTINF:(-?\d+),\s*(\S.*),\s*(\S.*)$/i);
      if (matches && matches.length > 1) {
        entry.title = matches[2];
        entry.mimetype = matches[3];
      }
    } else if (line && !line.match(/^(#|\s*$)/)) {
      entry.location = line;
      // Call the helper function to add the item once you gathered the data:
      var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result, rootPath);

      // Also add to "Directories"
      if (playlistDirChain && state)
        state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result, rootPath);

      entry.title = null;
      entry.mimetype = null;
      if (state)
        playlistOrder++;
    }

    line = readln();
  } while (line);

  return result;
}
// doc-playlist-m3u-parse-end

function readAsxPlaylist(playlist_title, playlistLocation, playlistChain, playlistDirChain, rootPath) {
  var entry = {
    title: null,
    location: null,
    order: 0,
    size: -1,
    writeThrough: -1,
    mimetype: null,
    description: null,
    protocol: null,
    extra: [],
  };
  var base = null;
  var playlistOrder = 1;
  var node = readXml(-2);
  var level = 0;
  const result = [];

  while (node || level > 0) {
    if (!node && level > 0) {
      node = readXml(-1); // read parent
      node = readXml(0); // read next
      level--;
      if (entry.location) {
        if (base)
          entry.location = base + '/' + entry.location;
        var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result, rootPath);
        if (playlistDirChain && state) {
          entry.writeThrough = 0;
          state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result, rootPath);
        }
        if (state)
          playlistOrder++;
      }
      entry.title = null;
      entry.location = null;
      entry.size = -1;
      entry.writeThrough = -1;
      entry.mimetype = null;
      entry.protocol = null;
      entry.extra = [];
      entry.description = null;
    } else if (node.NAME === "asx") {
      node = readXml(1); // read children
      level++;
    } else if (node.NAME === "entry") {
      node = readXml(1); // read children
      level++;
    } else if (node.NAME === "ref" && node.href) {
      entry.location = node.href;
      entry.writeThrough = node.writethrough;
      node = readXml(0); // read next
    } else if (node.NAME === "base" && node.href && level < 2) {
      base = node.href;
      node = readXml(0); // read next
    } else if (node.NAME == "title") {
      entry.title = node.VALUE;
      node = readXml(0); // read next
    } else if (node.NAME == "abstract") {
      entry.description = node.VALUE;
      node = readXml(0); // read next
      // currently unused
    } else if (node.NAME == "param" && node.name === "size") {
      entry.size = node.value;
      node = readXml(0); // read next
    } else if (node.NAME == "param" && node.name === "mimetype") {
      entry.mimetype = node.value;
      node = readXml(0); // read next
    } else if (node.NAME == "param" && node.name === "protocol") {
      entry.protocol = node.value;
      node = readXml(0); // read next
    } else if (node.NAME == "param") {
      entry.extra[node.name] = node.value;
      node = readXml(0); // read next
    } else {
      node = readXml(0); // read next
    }
  }

  if (entry.location) {
    var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result, rootPath);
    if (playlistDirChain && state) {
      entry.writeThrough = 0;
      addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result, rootPath);
    }
  }
  return result;
}

function readPlsPlaylist(playlist_title, playlistLocation, playlistChain, playlistDirChain, rootPath) {
  var entry = {
    title: null,
    location: null,
    order: -1,
    size: -1,
    writeThrough: -1,
    mimetype: null,
    description: null,
    protocol: null,
    extra: [],
  };
  var line = readln();
  var playlistOrder = 1;
  const result = [];

  do {
    if (line.match(/^\[playlist\]$/i)) {
      // It seems to be a correct playlist, but we will try to parse it
      // anyway even if this header is missing, so do nothing.
    } else if (line.match(/^NumberOfEntries=(\d+)$/i)) {
      // var numEntries = RegExp.$1;
    } else if (line.match(/^Version=(\d+)$/i)) {
      // var plsVersion =  RegExp.$1;
    } else if (line.match(/^File\s*(\d+)\s*=\s*(\S.+)$/i)) {
      const matches = line.match(/^File\s*(\d+)\s*=\s*(\S.+)$/i);
      const thisFile = matches[2];
      const id = parseInt(matches[1], 10);
      if (entry.order === -1)
        entry.order = id;
      if (entry.order !== id) {
        if (entry.location) {
          var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result, rootPath);
          if (playlistDirChain && state)
            state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result, rootPath);
          if (state)
            playlistOrder++;
        }

        entry.title = null;
        entry.mimetype = null;
        entry.order = id;
      }
      entry.location = thisFile;
    } else if (line.match(/^Title\s*(\d+)\s*=\s*(\S.+)$/i)) {
      const matches = line.match(/^Title\s*(\d+)\s*=\s*(\S.+)$/i);
      const thisTitle = matches[2];
      const id = parseInt(matches[1], 10);
      if (entry.order === -1)
        entry.order = id;
      if (entry.order !== id) {
        if (entry.location) {
          var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result, rootPath);
          if (playlistDirChain && state)
            state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result, rootPath);
          if (state)
            playlistOrder++;
        }

        entry.location = null;
        entry.mimetype = null;
        entry.order = id;
      }
      entry.title = thisTitle;
    } else if (line.match(/^MimeType\s*(\d+)\s*=\s*(\S.+)$/i)) {
      const matches = line.match(/^MimeType\s*(\d+)\s*=\s*(\S.+)$/i);
      const thisMime = matches[2];
      const id = parseInt(matches[1], 10);
      if (entry.order === -1)
        entry.order = id;
      if (entry.order !== id) {
        if (entry.location) {
          var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result, rootPath);
          if (playlistDirChain && state)
            state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result, rootPath);
          if (state)
            playlistOrder++;
        }

        entry.location = null;
        entry.title = null;
        entry.order = id;
      }
      entry.mimetype = thisMime;
    } else if (line.match(/^Length\s*(\d+)\s*=\s*(\S.+)$/i)) {
      // currently unused
    }

    line = readln();
  } while (line !== undefined);

  if (entry.location) {
    var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result, rootPath);
    if (playlistDirChain && state)
      addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result, rootPath);
  }
  return result;
}
