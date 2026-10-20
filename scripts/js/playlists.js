/*GRB*
  Gerbera - https://gerbera.io/

  playlists.js - this file is part of Gerbera.

  Copyright (C) 2018-2026 Gerbera Contributors

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
      entryType: ET_PlaylistContainer,
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
  } else if (type === 'wpl' || type === 'xspf') {
    result = readXmlPlaylist(obj_title, objLocation, objChain, objDirChain, rootPath);
  }
  return result;
}

// XML parsing, namespace and URI resolution are performed before the script runs.
function readXmlPlaylist(playlist_title, playlistLocation, playlistChain, playlistDirChain, rootPath) {
  var result = [];
  var playlistOrder = 1;
  var source = readPlaylistEntry();
  while (source) {
    for (var i = 0; i < source.locations.length; i++) {
      var entry = { location: source.locations[i], title: source.title };
      if (addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result, rootPath)) {
        if (playlistDirChain)
          addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result, rootPath);
        playlistOrder++;
        break; // Alternative XSPF locations describe the same position.
      }
    }
    source = readPlaylistEntry();
  }
  return result;
}

// doc-playlist-m3u-parse-begin
const reM3uLine = new RegExp('^#EXTINF:(-?\\d+),\\s*(\\S.*)$', 'i');
const reM3uTitle = new RegExp('^#EXTINF:(-?\\d+),\\s*(\\S.*),\\s*(\\S.*)$', 'i');
const reM3uComment = new RegExp('^(#|\\s*$)');
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
    var matches = line ? reM3uLine.exec(line) : undefined;
    if (matches && matches.length > 1) {
      // duration = matches[1]; // currently unused
      entry.title = matches[2];
      matches = reM3uTitle.exec(line);
      if (matches && matches.length > 1) {
        entry.title = matches[2];
        entry.mimetype = matches[3];
      }
    } else if (line && !reM3uComment.test(line)) {
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

// The native parser validates and expands the complete structure before any
// containers or metadata updates are created. Entries retain child contexts.
function readAsxPlaylist(playlist_title, playlistLocation, playlistChain, playlistDirChain, rootPath) {
  const result = [];
  var playlistOrder = 1;
  var source = readAsxPlaylistEntry();
  while (source) {
    for (var i = 0; i < source.references.length; i++) {
      const reference = source.references[i];
      var location = reference.href;
      // BASE is a Gerbera compatibility extension for relative media paths.
      if (source.base && location.charAt(0) !== '/' && !/^[a-zA-Z][a-zA-Z0-9+.-]*:/.test(location))
        location = source.base.replace(/\/+$/, '') + '/' + location;
      location = resolvePlaylistReference(location, source.sourcePath);
      if (!location)
        continue;
      const entry = {
        location: location,
        title: source.title,
        description: source.description,
        writeThrough: reference.writeThrough,
        extra: {},
        defaults: {}
      };
      if (source.author)
        entry.defaults['upnp:author'] = source.author;
      if (source.copyright)
        entry.defaults['dc:rights'] = source.copyright;
      for (var parameter in source.parameters) {
        if (parameter === 'size' || parameter === 'mimetype' || parameter === 'protocol')
          entry[parameter] = source.parameters[parameter];
        else
          entry.extra[parameter] = source.parameters[parameter];
      }
      if (addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result, rootPath)) {
        if (playlistDirChain) {
          entry.writeThrough = 0; // Apply source updates once, not once per view.
          addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result, rootPath);
        }
        playlistOrder++;
        break; // REF children are alternatives for one logical position.
      }
    }
    source = readAsxPlaylistEntry();
  }
  return result;
}

const rePlsPlaylist = new RegExp("^\\[playlist\\]$", 'i');
const rePlsNumberOfEntries = new RegExp("^NumberOfEntries=(\\d+)$", 'i');
const rePlsVersion = new RegExp("^Version=(\\d+)$", 'i');
const rePlsFile = new RegExp("^File\\s*(\\d+)\\s*=\\s*(\\S.+)$", 'i');
const rePlsTitle = new RegExp("^Title\\s*(\\d+)\\s*=\\s*(\\S.+)$", 'i');
const rePlsMime = new RegExp("^MimeType\\s*(\\d+)\\s*=\\s*(\\S.+)$", 'i');
const rePlsLength = new RegExp("^Length\\s*(\\d+)\\s*=\\s*(\\S.+)$", 'i');
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
    if (rePlsPlaylist.test(line)) {
      // It seems to be a correct playlist, but we will try to parse it
      // anyway even if this header is missing, so do nothing.
    } else if (rePlsNumberOfEntries.test(line)) {
      // var numEntries = RegExp.$1;
    } else if (rePlsVersion.test(line)) {
      // var plsVersion =  RegExp.$1;
    } else if (rePlsFile.test(line)) {
      const matches = rePlsFile.exec(line);
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
    } else if (rePlsTitle.test(line)) {
      const matches = rePlsTitle.exec(line);
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
    } else if (rePlsMime.test(line)) {
      const matches = rePlsMime.exec(line);
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
    } else if (rePlsLength.test(line)) {
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
