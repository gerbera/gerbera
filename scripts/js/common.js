/*GRB*
  Gerbera - https://gerbera.io/

  common.js - this file is part of Gerbera.

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

function escapeSlash(name) {
  name = name.replace(/\\/g, "\\\\");
  name = name.replace(/\//g, "\\/");
  return name;
}

function createContainerChain(arr) {
  var path = '';
  for (var i = 0; i < arr.length; i++) {
    path = path + '/' + escapeSlash(arr[i]);
  }
  return path;
}

function getYear(date) {
  var matches = date.match(/^([0-9]{4})-/);
  if (matches)
    return matches[1];
  else
    return date;
}

function getPlaylistType(mimetype) {
  if (mimetype == 'audio/x-mpegurl')
    return 'm3u';
  if (mimetype == 'audio/x-scpls')
    return 'pls';
  if (mimetype == 'video/x-ms-asf' || mimetype === 'video/x-ms-asx')
    return 'asx';
  return '';
}

function getLastPath(fileLocation) {
  const path = fileLocation.split('/');
  if (path.length > 1 && path[path.length - 2])
    return path[path.length - 2];
  else
    return '';
}

function getLastPath2(fileLocation, length) {
  var avoidDouble = false;
  if (!length) length = 1;
  if (length < 0) { length = -length + 1; avoidDouble = true; }
  var path = fileLocation ? fileLocation.split('/') : [];
  if (path.length > length) {
    path.splice(-1); // remove fileName
    path.splice(0, path.length - length);
  } else if (path.length <= 1) {
    return [];
  }
  if (avoidDouble && path.length > 0)
    path.splice(-1); // remove folder
  return path;
}

function getRootPath(rootpath, fileLocation) {
  var path = new Array();

  if (rootpath && rootpath.length !== 0) {
    rootpath = rootpath.substring(0, rootpath.lastIndexOf('/'));

    var dir = fileLocation.substring(rootpath.length, fileLocation.lastIndexOf('/'));

    if (dir.charAt(0) === '/')
      dir = dir.substring(1);

    path = dir.split('/');
  } else {
    dir = getLastPath2(fileLocation, 1).join('/');
    if (dir !== '') {
      dir = escapeSlash(dir);
      path.push(dir);
    }
  }

  return path;
}

function allbox(allstring, boxtype, divchar) {
  var boxwidth = 4;
  // definition of box types, adjust to your own needs
  // as a start: the number is the same as the size of the box, evenly spaced ... more or less.
  switch (boxtype) {
    case 1:
      boxwidth = 26;       // one large box of 26 letters
      break;
    case 2:
      boxwidth = 13;       // two boxes of 13 letters
      break;
    case 3:
      boxwidth = 8;        // and so on ...
      break;
    case 4:
      boxwidth = 6;
      break;
    case 5:
      boxwidth = 5;
      break;
    case 6:
      boxwidth = 3;
      break;
    case 7:
      boxwidth = 3;
      break;
    case 9:
      boxwidth = 1;        // When T is a large box...
      break;
    case 13:
      boxwidth = 1;
      break;
    case 26:
      boxwidth = 0;
      break;
  }
  if (allstring.length < boxwidth) {
    boxwidth = Math.round((boxwidth - allstring.length + 1) / 2);
  } else {
    boxwidth = Math.round((boxwidth + 1) / 2);
  }
  return divchar.repeat(boxwidth) + allstring + divchar.repeat(boxwidth);
}

// Virtual folders which split collections into alphabetic groups.
// Example for box type 9 to create a list of artist folders:
//
// --all--
// -0-9-
// -ABCDE-
//    A
//    B
//    C
//    ...
// -FGHIJ-
// -KLMNO-
// -PQRS-
// -T-           <-- tends to be big
// -UVWXYZ-
// -^&#'!-
function abcbox(stringtobox, boxtype, divchar) {
  var boxReplace = stringFromConfig('/import/scripting/virtual-layout/structured-layout/attribute::skip-chars', '');
  if (boxReplace !== '') {
    stringtobox = stringtobox.replace(RegExp('^[' + boxReplace + ']', 'i'), "");
  }
  // get ascii value of first character
  var firstChar = mapInitial(stringtobox.charAt(0));
  var intchar = firstChar.charCodeAt(0);
  // check for numbers
  if ((intchar >= 48) && (intchar <= 57)) {
    return divchar + '0-9' + divchar;
  }
  // check for other characters
  if (!((intchar >= 65) && (intchar <= 90))) {
    return divchar + '^\&#\'' + divchar;
  }
  // all other characters are letters
  var boxwidth;
  // definition of box types, adjust to your own needs
  // as a start: the number is the same as the number of boxes, evenly spaced ... more or less.
  switch (boxtype) {
    case 1:
      boxwidth = new Array();
      boxwidth[0] = 26;                             // one large box of 26 letters
      break;
    case 2:
      boxwidth = new Array(13, 13);              // two boxes of 13 letters
      break;
    case 3:
      boxwidth = new Array(8, 9, 9);              // and so on ...
      break;
    case 4:
      boxwidth = new Array(7, 6, 7, 6);
      break;
    case 5:
      boxwidth = new Array(5, 5, 5, 6, 5);
      break;
    case 6:
      boxwidth = new Array(4, 5, 4, 4, 5, 4);
      break;
    case 7:
      boxwidth = new Array(4, 3, 4, 4, 4, 3, 4);
      break;
    case 9:
      boxwidth = new Array(5, 5, 5, 4, 1, 6);        // When T is a large box...
      break;
    case 13:
      boxwidth = new Array(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2);
      break;
    case 26:
      boxwidth = new Array(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
      break;
    default:
      boxwidth = new Array(5, 5, 5, 6, 5);
      break;
  }

  // check for a total of 26 characters for all boxes
  var charttl = 0;
  for (var cb = 0; cb < boxwidth.length; cb++) {
    charttl = charttl + boxwidth[cb];
  }
  if (charttl != 26) {
    print2("Error", "In box-definition, total length is " + charttl + ". Check the file common.js");
    // maybe an exit call here to stop processing the media ??
    return "???";
  }

  // declaration of some variables
  var boxnum = 0;                         // boxnumber start
  var sc = 65;                            // first ascii character (corresponds to 'A')
  var ec = sc + boxwidth[boxnum] - 1;     // last character of first box

  // loop that will define first and last character of the right box
  while (intchar > ec) {
    boxnum++;                         // next boxnumber
    sc = ec + 1;                      // next startchar
    ec = sc + boxwidth[boxnum] - 1;   // next endchar
  }

  // construction of output string
  var output = divchar;
  for (var i = sc; i <= ec; i++) {
    output = output + String.fromCharCode(i);
  }
  output = output + divchar;
  return output;
}

// doc-map-initial-begin
function mapInitial(firstChar) {
  firstChar = firstChar.toUpperCase();
  // map character to latin version
  const charTable = [
    ["ÄÁÀÂÆààáâãäåæ", "A"],
    ["Çç", "C"],
    ["Ðð", "D"],
    ["ÉÈÊÈÉÊËèéêë", "E"],
    ["ÍÌÎÏìíîï", "I"],
    ["Ññ", "N"],
    ["ÕÖÓÒÔØòóôõöøœ", "O"],
    ["Š", "S"],
    ["ÜÚÙÛùúûü", "U"],
    ["Ýýÿ", "Y"],
    ["Žž", "Z"],
  ];
  for (var idx = 0; idx < charTable.length; idx++) {
    var re = new RegExp('[' + charTable[idx][0] + ']', 'i');
    var match = re.exec(firstChar);
    if (match) {
      firstChar = charTable[idx][1];
      break;
    }
  }
  return firstChar;
}
// doc-map-initial-end

function mapGenre(genre) {
  const genreConfig = config['/import/scripting/virtual-layout/genre-map/genre'];
  var mapped = false;
  if (genreConfig) {
    const genreNames = Object.getOwnPropertyNames(genreConfig);
    for (var idx = 0; idx < genreNames.length; idx++) {
      var re = new RegExp('(' + genreNames[idx] + ')', 'i');
      var match = re.exec(genre);
      if (match) {
        genre = genreConfig[genreNames[idx]];
        mapped = true;
        break;
      }
    }
  }
  return { value: genre, mapped: mapped };
}

// doc-map-audio-details-begin
function getAudioDetails(obj) {
  // Note the difference between obj.title and obj.metaData[M_TITLE] -
  // while object.title will originally be set to the file name,
  // obj.metaData[M_TITLE] will contain the parsed title - in this
  // particular example the ID3 title of an MP3.
  var title = obj.title;

  // First we will gather all the metadata that is provided by our
  // object, of course it is possible that some fields are empty -
  // we will have to check that to make sure that we handle this
  // case correctly.
  if (obj.metaData[M_TITLE] && obj.metaData[M_TITLE][0]) {
    title = obj.metaData[M_TITLE][0];
  }

  var desc = '';
  var artist = ['Unknown'];
  var artist_full = null;
  if (obj.metaData[M_ARTIST] && obj.metaData[M_ARTIST][0]) {
    artist = obj.metaData[M_ARTIST];
    artist_full = artist.join(' / ');
    desc = artist_full;
  }


  var aartist = obj.aux && obj.aux['TPE2'] ? obj.aux['TPE2'] : artist[0];
  if (obj.metaData[M_ALBUMARTIST] && obj.metaData[M_ALBUMARTIST][0]) {
    aartist = obj.metaData[M_ALBUMARTIST][0];
  }

  var disknr = '';
  if (obj.aux) {
    disknr = obj.aux['DISCNUMBER'];
    if (!disknr || disknr.length == 0) {
      disknr = obj.aux['TPOS'];
    }
  }
  if (!disknr || disknr.length == 0) {
    disknr = '';
  } else if (disknr == '1/1') {
    disknr = '';
  } else {
    var re = new RegExp("[^0-9].*", "i");
    disknr = new String(disknr).replace(re, "");
  }

  var album = 'Unknown';
  var album_full = null;
  if (obj.metaData[M_ALBUM] && obj.metaData[M_ALBUM][0]) {
    album = obj.metaData[M_ALBUM][0];
    desc = desc + ', ' + album;
    album_full = album;
  }

  if (desc) {
    desc = desc + ', ';
  }
  desc = desc + title;

  var date = 'Unknown';
  if (obj.metaData[M_DATE] && obj.metaData[M_DATE][0]) {
    date = getYear(obj.metaData[M_DATE][0]);
    obj.metaData[M_UPNP_DATE] = [date];
    desc = desc + ', ' + date;
    decade = date.substring(0, 3) + '0 - ' + String(10 * (parseInt(date.substring(0, 3))) + 9);
  }

  var genre = 'Unknown';
  var genres = [];
  if (obj.metaData[M_GENRE]) {
    genres = obj.metaData[M_GENRE];
    if (obj.metaData[M_GENRE][0]) {
      genre = obj.metaData[M_GENRE][0];
      desc = desc + ', ' + genre;
    }
  }

  var description = '';
  if (obj.metaData[M_DESCRIPTION] && obj.metaData[M_DESCRIPTION][0]) {
    // do not overwrite existing description
    desc = obj.metaData[M_DESCRIPTION][0];
    description = obj.metaData[M_DESCRIPTION][0];
  }

  var composer = 'None';
  if (obj.metaData[M_COMPOSER] && obj.metaData[M_COMPOSER][0]) {
    composer = obj.metaData[M_COMPOSER].join(' / ');
  }

  var conductor = 'None';
  if (obj.metaData[M_CONDUCTOR] && obj.metaData[M_CONDUCTOR][0]) {
    conductor = obj.metaData[M_CONDUCTOR].join(' / ');
  }

  var orchestra = 'None';
  if (obj.metaData[M_ORCHESTRA] && obj.metaData[M_ORCHESTRA][0]) {
    orchestra = obj.metaData[M_ORCHESTRA].join(' / ');
  }

  var track = obj.metaData[M_TRACKNUMBER] ? obj.metaData[M_TRACKNUMBER][0] : '';
  if (!track) {
    track = '';
  } else {
    if (track.length == 1) {
      track = '0' + track;
    }
    if ((disknr.length > 0) && (track.length == 2)) {
      track = '0' + track;
    }
    if (disknr.length == 1) {
      obj.metaData[M_TRACKNUMBER] = [disknr + '' + track];
      track = '0' + disknr + ' ' + track;
    }
    else {
      obj.metaData[M_TRACKNUMBER] = [disknr + '' + track];
      if (disknr.length > 1) {
        track = disknr + ' ' + track;
      }
    }
    track = track + ' ';
  }

  if (artist.join(' - ') != aartist) {
    title = artist.join(' - ') + ' - ' + title;
  }

  var channels = obj.res[R_NRAUDIOCHANNELS];
  if (channels) {
    if (channels === "1") {
      channels = '[MONO]';
    } else if (channels === "2") {
      channels = '[STEREO]';
    } else {
      channels = '[MULTI]';
    }
  } else {
    channels = '';
  }

  return {
    title: title,
    disknr: disknr,
    albumArtist: aartist,
    artists: artist,
    artistFull: artist_full,
    album: album,
    albumFull: album_full,
    genre: genre,
    genres: genres,
    date: date,
    decade: decade,
    track: track,
    channels: channels,
    composer: composer,
    conductor: conductor,
    orchestra: orchestra,
    desc: desc,
    description: description,
  };
}
// doc-map-audio-details-end

function prepareChains(boxes, boxSetup, chainSetup) {
  var _Chain = {};
  if (chainSetup) {
    for (var j = 0; j < chainSetup.size; j++) {
      const uchain = chainSetup[j];
      for (var k = 0; k < uchain.size; k++) {
        const ulink = uchain[k];
        if (boxes.indexOf(ulink.key) === -1) {
          boxes.push(ulink.key)
        }
      }
    }
  }
  for (var j = 0; j < boxes.length; j++) {
    if (boxes[j] in boxSetup) {
      const bxSetup = boxSetup[boxes[j]];
      _Chain[boxes[j]] = {
        id: bxSetup.id,
        title: bxSetup.title,
        searchable: false,
        objectType: OBJECT_TYPE_CONTAINER,
        upnpclass: bxSetup.class,
        upnpShortcut: bxSetup.upnpShortcut,
        sortKey: bxSetup.sortKey,
        metaData: {},
      };
    }
  }
  return _Chain;
}

function createUserChain(obj, media, chain, boxSetup, chainSetup, result, rootPath) {
  if (chainSetup) {
    for (var j = 0; j < chainSetup.size; j++) {
      const uchain = chainSetup[j];
      var dynChain = [];
      for (var k = 0; k < uchain.size; k++) {
        const ulink = uchain[k];
        if (!(ulink.key in boxSetup)) {
          chain[ulink.key] = {
            id: (ulink.id ? eval(ulink.id) : undefined),
            title: (ulink.title ? eval(ulink.title) : ulink.key),
            searchable: false,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: (ulink.class ? eval(ulink.class) : UPNP_CLASS_CONTAINER),
            upnpShortcut: (ulink.upnpShortcut ? eval(ulink.upnpShortcut) : ''),
            sortKey: (ulink.sortKey ? eval(ulink.sortKey) : ''),
            res: (ulink.res ? eval(ulink.res) : []),
            aux: (ulink.aux ? eval(ulink.aux) : []),
            metaData: {},
            refID: (ulink.refID ? eval(ulink.refID) : undefined),
          };
          if (ulink.metaData) {
            const mdArray = ulink.metaData.split(',');
            for (var mdi = 0; mdi < mdArray.length; mdi++) {
              const md = mdArray[mdi].split(':');
              if (md.length > 1) {
                chain[ulink.key].metaData[eval(md[0].trim())] = eval(md[1].trim());
              } else {
                chain[ulink.key].metaData = eval(mdArray[mdi].trim());
              }
            }
          }
        }
        dynChain.push(chain[ulink.key]);
      }
      if (dynChain.length > 0) {
        const container = addContainerTree(dynChain);
        result.push(addCdsObject(obj, container, rootPath));
      }
    }
  }
}

// doc-map-int-config-begin
function intFromConfig(entry, defValue) {
  if (entry in config) {
    var value = config[entry];
    return parseInt(value.toString());
  }
  return defValue;
}
// doc-map-int-config-end

// doc-map-string-config-begin
function stringFromConfig(entry, defValue) {
  if (entry in config) {
    var value = config[entry];
    return value.toString();
  }
  return defValue;
}
// doc-map-string-config-end

function parseBool(entry) {
  switch (entry) {
    case null:
    case undefined:
      return false;
  }
  switch (entry.toString().trim().toLowerCase()) {
    case "true":
    case "yes":
    case "1":
      return true;

    case "false":
    case "no":
    case "0":
      return false;

    default:
      return JSON.parse(entry);
  }
}

// doc-map-bool-config-begin
function boolFromConfig(entry, defValue) {
  if (entry in config) {
    var value = config[entry];
    return parseBool(value);
  }
  return defValue;
}
// doc-map-bool-config-end

function getVideoDetails(obj, rootPath) {
  const date = obj.metaData[M_CREATION_DATE] && obj.metaData[M_CREATION_DATE][0] ? obj.metaData[M_CREATION_DATE][0] : '';
  const dateParts = date.split('-');
  const dateParts2 = date.split('T');
  const dir = getRootPath(rootPath, obj.location);
  return {
    year: (dateParts.length > 1) ? dateParts[0] : '',
    month: (dateParts.length > 1) ? dateParts[1] : '',
    date: (dateParts2.length > 1) ? dateParts2[0] : date,
    dir: dir,
  }
}

function getImageDetails(obj, rootPath) {
  const date = obj.metaData[M_DATE] && obj.metaData[M_DATE][0] ? obj.metaData[M_DATE][0] : '';
  const dateParts = date.split('-');
  const dateParts2 = date.split('T');
  const dir = getRootPath(rootPath, obj.location);
  return {
    year: (dateParts.length > 1) ? dateParts[0] : '',
    month: (dateParts.length > 1) ? dateParts[1] : '',
    date: (dateParts2.length > 1) ? dateParts2[0] : date,
    dir: dir,
  }
}

// doc-add-playlist-item-begin
function addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result, rootPath) {
  // Determine if the item that we got is an URL or a local file.

  if (entry.location.match(/^.*:\/\//)) {
    var exturl = {};

    // Setting the mimetype is crucial and tricky... if you get it
    // wrong your renderer may show the item as unsupported and refuse
    // to play it. Unfortunately most playlist formats do not provide
    // any mimetype information.
    exturl.mimetype = entry.mimetype ? entry.mimetype : 'audio/mpeg';

    // Make sure to correctly set the object type, then populate the
    // remaining fields.
    exturl.objectType = OBJECT_TYPE_ITEM_EXTERNAL_URL;

    exturl.location = entry.location;
    exturl.title = (entry.title ? entry.title : entry.location);
    exturl.protocol = entry.protocol ? entry.protocol : 'http-get';
    exturl.upnpclass = exturl.mimetype.startsWith('video') ? UPNP_CLASS_VIDEO_ITEM : UPNP_CLASS_ITEM_MUSIC_TRACK;
    exturl.description = entry.description ? entry.description : ("Entry from " + playlist_title);

    exturl.extra = entry.extra;
    exturl.size = entry.size;
    exturl.writeThrough = entry.writeThrough;

    // This is a special field which ensures that your playlist files
    // will be displayed in the correct order inside a playlist
    // container. It is similar to the id3 track number that is used
    // to sort the media in album containers.
    exturl.playlistOrder = (entry.order ? entry.order : playlistOrder);

    // Your item will be added to the container named by the playlist
    // that you are currently parsing.
    result.push(addCdsObject(exturl, playlistChain, rootPath));
  } else {
    if (entry.location.substr(0, 1) !== '/') {
      entry.location = playlistLocation + entry.location;
    }

    var cds = getCdsObject(entry.location);
    if (!cds) {
      print2("Warning", "Playlist '" + playlist_title + "' Skipping unknown entry: " + entry.location);
      return false;
    }

    var item = copyObject(cds);

    item.playlistOrder = (entry.order ? entry.order : playlistOrder);

    // Select the title for the playlist item.
    if (entry.writeThrough > 0 && entry.title) {
      // For ASX files, use the title in the playlist if present.
      item.title = entry.title;
    } else if (item.metaData[M_TITLE]) {
      // Otherwise, prefer the metadata if set.
      item.title = item.metaData[M_TITLE][0];
    } else if (entry.title) {
      // Otherwise, use the title in the playlist if set.
      item.title = entry.title;
    } else {
      // Use title of the referenced item.
      item.title = cds.title;
    }

    item.metaData[M_CONTENT_CLASS] = [UPNP_CLASS_PLAYLIST_ITEM];
    item.description = entry.description ? entry.description : null;

    item.extra = entry.extra;
    item.writeThrough = entry.writeThrough;

    result.push(addCdsObject(item, playlistChain, rootPath));
  }
  return true;
}
// doc-add-playlist-item-end

function addMeta(obj, key, value) {
  if (obj.metaData[key])
    obj.metaData[key].push(value);
  else
    obj.metaData[key] = [value];
}
