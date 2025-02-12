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

// doc-add-audio-begin
function addAudio(obj, cont, rootPath, containerType) {
  const audio = getAudioDetails(obj);
  const scriptOptions = config['/import/scripting/virtual-layout/script-options/script-option'];

  // We finally gathered all data that we need, so let's create a
  // nice layout for our audio files.

  obj.title = audio.title;

  // set option in config if you want to have track numbers in front of the title
  if (!scriptOptions || scriptOptions['trackNumbers'] !== 'show')
    audio.track = '';
  // set option in config if you want to have channel numbers in front of the title
  if (!scriptOptions || scriptOptions['channelCount'] !== 'show')
    audio.channels = '';
  audio.genre = mapGenre(audio.genre).value;

  // The UPnP class argument to addCdsObject() is optional, if it is
  // not supplied the default UPnP class will be used. However, it
  // is suggested to correctly set UPnP classes of containers and
  // objects - this information may be used by some renderers to
  // identify the type of the container and present the content in a
  // different manner.

  // Remember, the server will sort all items by ID3 track if the
  // container class is set to UPNP_CLASS_CONTAINER_MUSIC_ALBUM.

  const parentCount = intFromConfig('/import/resources/container/attribute::parentCount', 1);
  const containerResource = parentCount > 1 ? cont.res : undefined;
  const containerRefID = cont.res.count > 0 ? cont.id : obj.id;
  const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];

  const chain = {
    audio: {
      id: boxSetup[BK_audioRoot].id,
      title: boxSetup[BK_audioRoot].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioRoot].class,
      upnpShortcut: boxSetup[BK_audioRoot].upnpShortcut,
      metaData: []
    },
    allAudio: {
      id: boxSetup[BK_audioAll].id,
      title: boxSetup[BK_audioAll].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioAll].class,
      upnpShortcut: boxSetup[BK_audioAll].upnpShortcut,
      metaData: []
    },
    allArtists: {
      id: boxSetup[BK_audioAllArtists].id,
      title: boxSetup[BK_audioAllArtists].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioAllArtists].class,
      upnpShortcut: boxSetup[BK_audioAllArtists].upnpShortcut,
      metaData: []
    },
    allGenres: {
      id: boxSetup[BK_audioAllGenres].id,
      title: boxSetup[BK_audioAllGenres].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioAllGenres].class,
      upnpShortcut: boxSetup[BK_audioAllGenres].upnpShortcut,
      metaData: []
    },
    allAlbums: {
      id: boxSetup[BK_audioAllAlbums].id,
      title: boxSetup[BK_audioAllAlbums].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioAllAlbums].class,
      upnpShortcut: boxSetup[BK_audioAllAlbums].upnpShortcut,
      metaData: []
    },
    allYears: {
      id: boxSetup[BK_audioAllYears].id,
      title: boxSetup[BK_audioAllYears].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioAllYears].class,
      upnpShortcut: boxSetup[BK_audioAllYears].upnpShortcut,
      metaData: []
    },
    allComposers: {
      id: boxSetup[BK_audioAllComposers].id,
      title: boxSetup[BK_audioAllComposers].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioAllComposers].class,
      upnpShortcut: boxSetup[BK_audioAllComposers].upnpShortcut,
      metaData: []
    },
    allSongs: {
      id: boxSetup[BK_audioAllSongs].id,
      title: boxSetup[BK_audioAllSongs].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioAllSongs].class,
      upnpShortcut: boxSetup[BK_audioAllSongs].upnpShortcut,
      metaData: []
    },
    allFull: {
      id: boxSetup[BK_audioAllTracks].id,
      title: boxSetup[BK_audioAllTracks].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioAllTracks].class,
      upnpShortcut: boxSetup[BK_audioAllTracks].upnpShortcut,
      metaData: []
    },
    allFullArtist: {
      id: boxSetup[BK_audioAllTracks].id,
      title: boxSetup[BK_audioAllTracks].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioAllTracks].class,
      metaData: []
    },
    artistChronology: {
      id: boxSetup[BK_audioArtistChronology].id,
      title: boxSetup[BK_audioArtistChronology].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioArtistChronology].class,
      upnpShortcut: boxSetup[BK_audioArtistChronology].upnpShortcut,
      metaData: []
    },

    artist: {
      searchable: false,
      title: audio.artists[0],
      location: audio.artists[0],
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ARTIST,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    album: {
      searchable: false,
      title: audio.album,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ALBUM,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    genre: {
      title: audio.genre,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_GENRE,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    year: {
      title: audio.date,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    composer: {
      title: audio.composer,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_COMPOSER,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
  };

  chain.audio.metaData[M_CONTENT_CLASS] = [UPNP_CLASS_AUDIO_ITEM];
  chain.album.metaData[M_ARTIST] = audio.artists;
  chain.album.metaData[M_ALBUMARTIST] = [audio.albumArtist];
  chain.album.metaData[M_GENRE] = [audio.genre];
  chain.album.metaData[M_DATE] = obj.metaData[M_DATE];
  chain.album.metaData[M_UPNP_DATE] = obj.metaData[M_UPNP_DATE];
  chain.album.metaData[M_ALBUM] = [audio.album];
  chain.artist.metaData[M_ARTIST] = audio.artists;
  chain.artist.metaData[M_ALBUMARTIST] = [audio.albumArtist];
  chain.genre.metaData[M_GENRE] = [audio.genre];
  chain.year.metaData[M_DATE] = [audio.date];
  chain.year.metaData[M_UPNP_DATE] = [audio.date];
  chain.composer.metaData[M_COMPOSER] = [audio.composer];
  const result = [];

  var container;
  if (boxSetup[BK_audioAll].enabled) {
    container = addContainerTree([chain.audio, chain.allAudio]);
    result.push(addCdsObject(obj, container));
  }

  if (boxSetup[BK_audioAllSongs].enabled && boxSetup[BK_audioAllArtists].enabled) {
    container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.allSongs]);
    result.push(addCdsObject(obj, container));
  }

  if (boxSetup[BK_audioAllTracks].enabled) {
    var temp = obj.title; // Backup the object title
    var prefix = '';
    if (audio.artistFull) {
      prefix = audio.artistFull;
    }

    if (audio.albumFull) {
      prefix = prefix + ' - ' + audio.albumFull + ' - ';
    } else {
      prefix = prefix + ' - ';
    }

    obj.title = prefix + audio.title;
    container = addContainerTree([chain.audio, chain.allFull]);
    result.push(addCdsObject(obj, container));

    if (boxSetup[BK_audioAllArtists].enabled) {
      const artCnt = audio.artists.length;
      for (var i = 0; i < artCnt; i++) {
        chain.artist.title = audio.artists[i];
        container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.allFullArtist]);
        result.push(addCdsObject(obj, container));
      }
    }

    obj.title = temp; // Restore the title
  }

  if (boxSetup[BK_audioAllArtists].enabled) {
    var temp = obj.title; // Backup the object title
    obj.title = audio.track + audio.title;

    const artCnt = audio.artists.length;
    for (var i = 0; i < artCnt; i++) {
      chain.artist.title = audio.artists[i];
      chain.artist.searchable = true;
      container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.album]);
      result.push(addCdsObject(obj, container));
    }

    obj.title = temp; // Restore the title
  }

  if (boxSetup[BK_audioAllAlbums].enabled) {
    chain.album.searchable = true;
    var temp = obj.title; // Backup the object title
    obj.title = audio.track + audio.title;

    chain.album.location = getRootPath(rootPath, obj.location).join('_');
    container = addContainerTree([chain.audio, chain.allAlbums, chain.album]);
    chain.album.location = '';
    result.push(addCdsObject(obj, container));

    obj.title = temp; // Restore the title
  }

  if (boxSetup[BK_audioAllGenres].enabled) {
    chain.genre.searchable = true;
    const genCnt = audio.genres.length;
    for (var j = 0; j < genCnt; j++) {
      chain.genre.title = audio.genres[j];
      chain.genre.metaData[M_GENRE] = [audio.genres[j]];
      container = addContainerTree([chain.audio, chain.allGenres, chain.genre]);
      result.push(addCdsObject(obj, container));
    }
  }

  if (boxSetup[BK_audioAllYears].enabled) {
    container = addContainerTree([chain.audio, chain.allYears, chain.year]);
    result.push(addCdsObject(obj, container));
  }

  if (boxSetup[BK_audioAllComposers].enabled) {
    container = addContainerTree([chain.audio, chain.allComposers, chain.composer]);
    result.push(addCdsObject(obj, container));
  }

  if (boxSetup[BK_audioArtistChronology].enabled && boxSetup[BK_audioAllArtists].enabled) {
    chain.album.searchable = false;
    chain.artist.searchable = false;
    chain.album.title = audio.date + " - " + audio.album;
    container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.artistChronology, chain.album]);
    result.push(addCdsObject(obj, container));

    chain.album.title = audio.album; // Restore the title;
  }
  return result;
}
// doc-add-audio-end

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

function addAudioStructured(obj, cont, rootPath, containerType) {
  // first gather data
  const audio = getAudioDetails(obj);
  const scriptOptions = config['/import/scripting/virtual-layout/script-options/script-option'];

  // Extra code for correct display of albums with various artists (usually collections)
  var tracktitle = audio.track + audio.title;
  audio.albumArtist = audio.album + ' - ' + audio.artists.join(' / ');
  if (audio.description) {
    if (audio.description.toUpperCase() === 'VARIOUS') {
      audio.albumArtist = audio.album + ' - Various';
      tracktitle = tracktitle + ' - ' + audio.artists.join(' / ');
    }
  }

  // set option in config if you want to have track numbers in front of the title
  if (!scriptOptions || scriptOptions['trackNumbers'] !== 'show')
    audio.track = '';
  // set option in config if you want to have channel numbers in front of the title
  if (!scriptOptions || scriptOptions['channelCount'] !== 'show')
    audio.channels = '';
  audio.genre = mapGenre(audio.genre).value;

  const boxConfig = {
    divChar: stringFromConfig('/import/scripting/virtual-layout/structured-layout/attribute::div-char', '-'),
  };
  boxConfig.singleLetterBoxSize = 2 * boxConfig.divChar.length + 1;
  const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];
  const parentCount = intFromConfig('/import/resources/container/attribute::parentCount', 1);
  const containerResource = parentCount > 1 ? cont.res : undefined;
  const containerRefID = cont.res.count > 0 ? cont.id : obj.id;

  const chain = {
    allArtists: {
      id: boxSetup[BK_audioStructuredAllArtists].id,
      title: boxSetup[BK_audioStructuredAllArtists].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioStructuredAllArtists].class,
      upnpShortcut: boxSetup[BK_audioStructuredAllArtists].upnpShortcut,
      metaData: []
    },
    allGenres: {
      id: boxSetup[BK_audioStructuredAllGenres].id,
      title: boxSetup[BK_audioStructuredAllGenres].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioStructuredAllGenres].class,
      upnpShortcut: boxSetup[BK_audioStructuredAllGenres].upnpShortcut,
      metaData: []
    },
    allAlbums: {
      id: boxSetup[BK_audioStructuredAllAlbums].id,
      title: boxSetup[BK_audioStructuredAllAlbums].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioStructuredAllAlbums].class,
      upnpShortcut: boxSetup[BK_audioStructuredAllAlbums].upnpShortcut,
      metaData: []
    },
    allYears: {
      id: boxSetup[BK_audioStructuredAllYears].id,
      title: boxSetup[BK_audioStructuredAllYears].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioStructuredAllYears].class,
      upnpShortcut: boxSetup[BK_audioStructuredAllYears].upnpShortcut,
      metaData: []
    },
    allTracks: {
      id: boxSetup[BK_audioStructuredAllTracks].id,
      title: boxSetup[BK_audioStructuredAllTracks].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioStructuredAllTracks].class,
      upnpShortcut: boxSetup[BK_audioStructuredAllTracks].upnpShortcut,
      metaData: []
    },
    entryAllLevel1: {
      title: allbox(boxSetup[BK_audioStructuredAllArtistTracks].title, 6, boxConfig.divChar),
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER
    },
    entryAllLevel2: {
      title: allbox(boxSetup[BK_audioStructuredAllArtistTracks].title, 9, boxConfig.divChar),
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER
    },
    entryAllLevel3: {
      title: allbox(boxSetup[BK_audioStructuredAllArtistTracks].title, 9, boxConfig.divChar),
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ARTIST
    },

    abc: {
      title: abcbox(audio.album, boxSetup[BK_audioStructuredAllAlbums].size, boxConfig.divChar),
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER
    },
    init: {
      title: mapInitial(audio.album.charAt(0)),
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER
    },
    artist: {
      title: audio.artists[0],
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ARTIST,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    album_artist: {
      title: audio.albumArtist,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: containerType,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    album: {
      title: audio.album,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: containerType,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    genre: {
      title: audio.genre,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_GENRE,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    decade: {
      title: audio.decade,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER
    },
    date: {
      title: audio.date,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER
    },
  };

  chain.allArtists.metaData[M_CONTENT_CLASS] = [UPNP_CLASS_AUDIO_ITEM];
  chain.album.metaData[M_ARTIST] = [audio.albumArtist];
  chain.album.metaData[M_ALBUMARTIST] = [audio.albumArtist];
  chain.album.metaData[M_GENRE] = [audio.genre];
  chain.album.metaData[M_DATE] = obj.metaData[M_DATE];
  chain.album.metaData[M_UPNP_DATE] = obj.metaData[M_UPNP_DATE];
  chain.album.metaData[M_ALBUM] = [audio.album];
  chain.artist.metaData[M_ARTIST] = audio.artists;
  chain.artist.metaData[M_ALBUMARTIST] = audio.artists;
  chain.album_artist.metaData[M_ARTIST] = [audio.albumArtist];
  chain.album_artist.metaData[M_ALBUMARTIST] = [audio.albumArtist];
  var isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;
  const result = [];

  obj.title = tracktitle;
  var container = addContainerTree(isSingleCharBox ? [chain.allAlbums, chain.abc, chain.album_artist] : [chain.allAlbums, chain.abc, chain.init, chain.album_artist]);
  result.push(addCdsObject(obj, container));

  container = addContainerTree([chain.allAlbums, chain.abc, chain.entryAllLevel2, chain.album_artist]);
  result.push(addCdsObject(obj, container));

  container = addContainerTree([chain.allAlbums, chain.entryAllLevel1, chain.album_artist]);
  result.push(addCdsObject(obj, container));

  // Artist
  obj.title = audio.title + ' (' + audio.album + ', ' + audio.date + ')';
  const artCnt = audio.artists.length;
  var i;
  for (i = 0; i < artCnt; i++) {
    chain.artist.title = audio.artists[i];
    chain.artist.searchable = true;
    container = addContainerTree([chain.allArtists, chain.entryAllLevel1, chain.artist]);
    result.push(addCdsObject(obj, container));
  }
  chain.artist.searchable = false;

  for (i = 0; i < artCnt; i++) {
    chain.abc.title = abcbox(audio.artists[i], boxSetup[BK_audioStructuredAllArtists].size, boxConfig.divChar);
    isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;
    container = addContainerTree([chain.allArtists, chain.abc, chain.entryAllLevel2, chain.artist]);
    result.push(addCdsObject(obj, container));
  }

  obj.title = audio.title + ' (' + audio.album + ', ' + audio.date + ')';
  for (i = 0; i < artCnt; i++) {
    chain.init.title = mapInitial(audio.artists[i].charAt(0));
    container = addContainerTree(isSingleCharBox ? [chain.allArtists, chain.abc, chain.artist, chain.entryAllLevel3] : [chain.allArtists, chain.abc, chain.init, chain.artist, chain.entryAllLevel3]);
    result.push(addCdsObject(obj, container));
  }

  obj.title = tracktitle;
  chain.album.title = audio.album + ' (' + audio.date + ')';
  chain.album.searchable = true;
  container = addContainerTree(isSingleCharBox ? [chain.allArtists, chain.abc, chain.artist, chain.album] : [chain.allArtists, chain.abc, chain.init, chain.artist, chain.album]);
  result.push(addCdsObject(obj, container));
  chain.album.searchable = false;

  // Genre
  if (boxSetup[BK_audioStructuredAllGenres].enabled) {
    obj.title = audio.title + ' - ' + audio.artistFull;
    chain.entryAllLevel2.upnpclass = UPNP_CLASS_CONTAINER_MUSIC_GENRE;
    const genCnt = audio.genres.length;
    for (var j = 0; j < genCnt; j++) {
      chain.genre.title = audio.genres[j];
      chain.genre.metaData[M_GENRE] = [audio.genres[j]];
      container = addContainerTree([chain.allGenres, chain.genre, chain.entryAllLevel1]);
      result.push(addCdsObject(obj, container));
    }

    for (i = 0; i < artCnt; i++) {
      chain.abc.title = abcbox(audio.artists[i], boxSetup[BK_audioStructuredAllGenres].size, boxConfig.divChar);
      isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;
      chain.album_artist.searchable = true;
      for (var j = 0; j < genCnt; j++) {
        chain.genre.title = audio.genres[j];
        chain.genre.metaData[M_GENRE] = [audio.genres[j]];
        container = addContainerTree(isSingleCharBox ? [chain.allGenres, chain.genre, chain.abc, chain.album_artist] : [chain.allGenres, chain.genre, chain.abc, chain.init, chain.album_artist]);
        result.push(addCdsObject(obj, container));
      }
    }
  }
  chain.album_artist.searchable = false;

  // Tracks
  if (boxSetup[BK_audioStructuredAllTracks].enabled) {
    obj.title = audio.title + ' - ' + audio.artists.join(' / ') + ' (' + audio.album + ', ' + audio.date + ')';
    chain.abc.title = abcbox(audio.title, boxSetup[BK_audioStructuredAllTracks].size, boxConfig.divChar);
    isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;
    chain.init.title = mapInitial(audio.title.charAt(0));
    chain.init.upnpclass = UPNP_CLASS_CONTAINER_MUSIC_ARTIST;
    container = addContainerTree(isSingleCharBox ? [chain.allTracks, chain.abc] : [chain.allTracks, chain.abc, chain.init]);
    result.push(addCdsObject(obj, container));

    obj.title = audio.title + ' - ' + audio.artistFull;
    container = addContainerTree([chain.allTracks, chain.entryAllLevel1]);
    result.push(addCdsObject(obj, container));
  }

  // Sort years into decades
  if (boxSetup[BK_audioStructuredAllYears].enabled) {
    obj.title = audio.title + ' - ' + audio.artistFull;

    container = addContainerTree([chain.allYears, chain.decade, chain.entryAllLevel3]);
    result.push(addCdsObject(obj, container));

    container = addContainerTree([chain.allYears, chain.decade, chain.date, chain.entryAllLevel3]);
    result.push(addCdsObject(obj, container));

    obj.title = tracktitle;
    chain.album.title = audio.album;
    chain.album.searchable = true;
    container = addContainerTree([chain.allYears, chain.decade, chain.date, chain.artist, chain.album]);
    result.push(addCdsObject(obj, container));
  }
  return result;
}

// doc-add-video-begin
function addVideo(obj, cont, rootPath, containerType) {
  const dir = getRootPath(rootPath, obj.location);
  const parentCount = intFromConfig('/import/resources/container/attribute::parentCount', 1);
  const containerResource = parentCount > 1 ? cont.res : undefined;
  const containerRefID = cont.res.count > 0 ? cont.id : obj.id;
  const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];
  const chain = {
    video: {
      id: boxSetup[BK_videoRoot].id,
      title: boxSetup[BK_videoRoot].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_videoRoot].class,
      upnpShortcut: boxSetup[BK_videoRoot].upnpShortcut,
      metaData: []
    },
    allVideo: {
      id: boxSetup[BK_videoAll].id,
      title: boxSetup[BK_videoAll].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_videoAll].class,
      upnpShortcut: boxSetup[BK_videoAll].upnpShortcut,
      metaData: []
    },
    allDirectories: {
      id: boxSetup[BK_videoAllDirectories].id,
      title: boxSetup[BK_videoAllDirectories].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_videoAllDirectories].class,
      upnpShortcut: boxSetup[BK_videoAllDirectories].upnpShortcut,
      metaData: []
    },
    allYears: {
      id: boxSetup[BK_videoAllYears].id,
      title: boxSetup[BK_videoAllYears].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_videoAllYears].class,
      upnpShortcut: boxSetup[BK_videoAllYears].upnpShortcut,
      metaData: []
    },
    allDates: {
      id: boxSetup[BK_videoAllDates].id,
      title: boxSetup[BK_videoAllDates].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_videoAllDates].class,
      upnpShortcut: boxSetup[BK_videoAllDates].upnpShortcut,
      metaData: []
    },

    year: {
      title: boxSetup[BK_videoUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER
    },
    month: {
      title: boxSetup[BK_videoUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    date: {
      title: boxSetup[BK_videoUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
  };
  chain.video.metaData[M_CONTENT_CLASS] = [UPNP_CLASS_VIDEO_ITEM];
  var container = addContainerTree([chain.video, chain.allVideo]);
  const result = [];
  result.push(addCdsObject(obj, container));

  // Year
  if (boxSetup[BK_videoAll].enabled && obj.metaData[M_CREATION_DATE] && obj.metaData[M_CREATION_DATE][0]) {
    var date = obj.metaData[M_CREATION_DATE][0];
    var dateParts = date.split('-');
    if (dateParts.length > 1) {
      chain.year.title = dateParts[0];
      chain.month.title = dateParts[1];
      result.push(addCdsObject(obj, addContainerTree([chain.video, chain.allYears, chain.year, chain.month])));
    }
  }

  // Dates
  if (boxSetup[BK_videoAllDates].enabled && obj.metaData[M_CREATION_DATE] && obj.metaData[M_CREATION_DATE][0]) {
    var date = obj.metaData[M_CREATION_DATE][0];
    var dateParts = date.split('T');
    if (dateParts.length > 1) {
      date = dateParts[0];
    }
    chain.date.title = date;
    result.push(addCdsObject(obj, addContainerTree([chain.video, chain.allDates, chain.date])));
  }

  // Directories
  if (boxSetup[BK_videoAllDirectories].enabled && dir.length > 0) {
    var tree = [chain.video, chain.allDirectories];
    for (var i = 0; i < dir.length; i++) {
      tree = tree.concat({ title: dir[i], objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER });
    }
    tree[tree.length - 1].upnpclass = containerType;
    tree[tree.length - 1].metaData = [];
    tree[tree.length - 1].res = containerResource;
    tree[tree.length - 1].aux = obj.aux;
    tree[tree.length - 1].refID = containerRefID;
    result.push(addCdsObject(obj, addContainerTree(tree)));
  }

  return result;
}
// doc-add-video-end

// doc-add-image-begin
function addImage(obj, cont, rootPath, containerType) {
  const dir = getRootPath(rootPath, obj.location);
  const parentCount = intFromConfig('/import/resources/container/attribute::parentCount', 1);
  const containerResource = parentCount > 1 ? cont.res : undefined;
  const containerRefID = cont.res.count > 0 ? cont.id : obj.id;
  const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];

  const chain = {
    imageRoot: {
      id: boxSetup[BK_imageRoot].id,
      title: boxSetup[BK_imageRoot].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_imageRoot].class,
      upnpShortcut: boxSetup[BK_imageRoot].upnpShortcut,
      metaData: []
    },
    allImages: {
      id: boxSetup[BK_imageAll].id,
      title: boxSetup[BK_imageAll].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_imageAll].class,
      upnpShortcut: boxSetup[BK_imageAll].upnpShortcut,
      metaData: []
    },
    allDirectories: {
      id: boxSetup[BK_imageAllDirectories].id,
      title: boxSetup[BK_imageAllDirectories].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_imageAllDirectories].class,
      upnpShortcut: boxSetup[BK_imageAllDirectories].upnpShortcut,
      metaData: []
    },
    allYears: {
      id: boxSetup[BK_imageAllYears].id,
      title: boxSetup[BK_imageAllYears].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_imageAllYears].class,
      upnpShortcut: boxSetup[BK_imageAllYears].upnpShortcut,
      metaData: []
    },
    allDates: {
      id: boxSetup[BK_imageAllDates].id,
      title: boxSetup[BK_imageAllDates].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_imageAllDates].class,
      upnpShortcut: boxSetup[BK_imageAllDates].upnpShortcut,
      metaData: []
    },

    year: {
      title: boxSetup[BK_imageUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER
    },
    month: {
      title: boxSetup[BK_imageUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      efID: containerRefID
    },
    date: {
      title: boxSetup[BK_imageUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
  };
  chain.imageRoot.metaData[M_CONTENT_CLASS] = [UPNP_CLASS_IMAGE_ITEM];
  const result = [];
  result.push(addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allImages])));

  // Years
  if (boxSetup[BK_imageAllYears].enabled && obj.metaData[M_DATE] && obj.metaData[M_DATE][0]) {
    var date = obj.metaData[M_DATE][0];
    var dateParts = date.split('-');
    if (dateParts.length > 1) {
      chain.year.title = dateParts[0];
      chain.month.title = dateParts[1];
      result.push(addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allYears, chain.year, chain.month])));
    }
  }

  // Dates
  if (boxSetup[BK_imageAllDates].enabled && obj.metaData[M_DATE] && obj.metaData[M_DATE][0]) {
    var date = obj.metaData[M_DATE][0];
    var dateParts = date.split('T');
    if (dateParts.length > 1) {
      date = dateParts[0];
    }
    chain.date.title = date;
    result.push(addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allDates, chain.date])));
  }

  // Directories
  if (boxSetup[BK_imageAllDirectories].enabled && dir.length > 0) {
    var tree = [chain.imageRoot, chain.allDirectories];
    for (var i = 0; i < dir.length; i++) {
      tree = tree.concat([{ title: dir[i], objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER }]);
    }
    tree[tree.length - 1].upnpclass = containerType;
    tree[tree.length - 1].metaData = [];
    tree[tree.length - 1].res = containerResource;
    tree[tree.length - 1].aux = obj.aux;
    tree[tree.length - 1].refID = containerRefID;
    result.push(addCdsObject(obj, addContainerTree(tree)));
  }
  return result;
}
// doc-add-image-end

// doc-add-trailer-begin
function addTrailer(obj) {
  const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];
  const chain = {
    trailerRoot: {
      id: boxSetup[BK_trailerRoot].id,
      title: boxSetup[BK_trailerRoot].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_trailerRoot].class,
      upnpShortcut: boxSetup[BK_trailerRoot].upnpShortcut,
      metaData: []
    },
    appleTrailers: {
      id: boxSetup[BK_trailerApple].id,
      title: boxSetup[BK_trailerApple].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_trailerApple].class,
      upnpShortcut: boxSetup[BK_trailerApple].upnpShortcut,
      metaData: []
    },
    allTrailers: {
      id: boxSetup[BK_trailerAll].id,
      title: boxSetup[BK_trailerAll].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_trailerAll].class,
      upnpShortcut: boxSetup[BK_trailerAll].upnpShortcut,
      metaData: []
    },
    allGenres: {
      id: boxSetup[BK_trailerAllGenres].id,
      title: boxSetup[BK_trailerAllGenres].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_trailerAllGenres].class,
      upnpShortcut: boxSetup[BK_trailerAllGenres].upnpShortcut,
      metaData: []
    },
    relDate: {
      id: boxSetup[BK_trailerRelDate].id,
      title: boxSetup[BK_trailerRelDate].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_trailerRelDate].class,
      upnpShortcut: boxSetup[BK_trailerRelDate].upnpShortcut,
      metaData: []
    },
    postDate: {
      id: boxSetup[BK_trailerPostDate].id,
      title: boxSetup[BK_trailerPostDate].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_trailerPostDate].class,
      upnpShortcut: boxSetup[BK_trailerPostDate].upnpShortcut,
      metaData: []
    },

    genre: {
      title: boxSetup[BK_trailerUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_GENRE,
      metaData: []
    },
    date: {
      title: boxSetup[BK_trailerUnknown].title,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: []
    }
  };
  // First we will add the item to the 'All Trailers' container, so
  // that we get a nice long playlist:

  const result = [];
  result.push(addCdsObject(obj, addContainerTree([chain.trailerRoot, chain.appleTrailers, chain.allTrailers])));

  // We also want to sort the trailers by genre, however we need to
  // take some extra care here: the genre property here is a comma
  // separated value list, so one trailer can have several matching
  // genres that will be returned as one string. We will split that
  // string and create individual genre containers.

  if (boxSetup[BK_trailerAllGenres].enabled && obj.metaData[M_GENRE] && obj.metaData[M_GENRE][0]) {
    var genre = obj.metaData[M_GENRE][0];

    // A genre string "Science Fiction, Thriller" will be split to
    // "Science Fiction" and "Thriller" respectively.

    const genres = genre.split(', ');
    for (var i = 0; i < genres.length; i++) {
      chain.genre.title = genres[i];
      chain.genre.metaData[M_GENRE] = [genres[i]];
      result.push(addCdsObject(obj, addContainerTree([chain.trailerRoot, chain.appleTrailers, chain.allGenres, chain.genre])));
    }
  }

  // The release date is offered in a YYYY-MM-DD format, we won't do
  // too much extra checking regading validity, however we only want
  // to group the trailers by year and month:

  if (boxSetup[BK_trailerRelDate].enabled && obj.metaData[M_DATE] && obj.metaData[M_DATE][0]) {
    var reldate = obj.metaData[M_DATE][0];
    if (reldate.length >= 7) {
      chain.date.title = reldate.slice(0, 7);
      chain.date.metaData[M_DATE] = [reldate.slice(0, 7)];
      chain.date.metaData[M_UPNP_DATE] = [reldate.slice(0, 7)];
      result.push(addCdsObject(obj, addContainerTree([chain.trailerRoot, chain.appleTrailers, chain.relDate, chain.date])));
    }
  }

  // We also want to group the trailers by the date when they were
  // originally posted, the post date is available via the aux
  // array. Similar to the release date, we will cut off the day and
  // create our containres in the YYYY-MM format.
  if (boxSetup[BK_trailerPostDate].enabled) {
    var postdate = obj.aux[APPLE_TRAILERS_AUXDATA_POST_DATE];
    if (postdate && postdate.length >= 7) {
      chain.date.title = postdate.slice(0, 7);
      chain.date.metaData[M_DATE] = [postdate.slice(0, 7)];
      chain.date.metaData[M_UPNP_DATE] = [postdate.slice(0, 7)];
      result.push(addCdsObject(obj, addContainerTree([chain.trailerRoot, chain.appleTrailers, chain.postDate, chain.date])));
    }
  }
  return result;
}
// doc-add-trailer-end

// doc-add-playlist-item-begin
function addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result) {
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
    result.push(addCdsObject(exturl, playlistChain));
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
    print2("Debug", "Playlist '" + item.title + "' Adding entry: " + item.playlistOrder + " " + item.location);

    result.push(addCdsObject(item, playlistChain));
  }
  return true;
}
// doc-add-playlist-item-end

// doc-playlist-m3u-parse-begin
function readM3uPlaylist(playlist_title, playlistLocation, playlistChain, playlistDirChain) {
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
      var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result);

      // Also add to "Directories"
      if (playlistDirChain && state)
        state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result);

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

function addMeta(obj, key, value) {
  if (obj.metaData[key])
    obj.metaData[key].push(value);
  else
    obj.metaData[key] = [value];
}

function readAsxPlaylist(playlist_title, playlistLocation, playlistChain, playlistDirChain) {
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
        var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result);
        if (playlistDirChain && state) {
          entry.writeThrough = 0;
          state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result);
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
    var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result);
    if (playlistDirChain && state) {
      entry.writeThrough = 0;
      addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result);
    }
  }
  return result;
}

function readPlsPlaylist(playlist_title, playlistLocation, playlistChain, playlistDirChain) {
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
          var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result);
          if (playlistDirChain && state)
            state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result);
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
          var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result);
          if (playlistDirChain && state)
            state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result);
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
          var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result);
          if (playlistDirChain && state)
            state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result);
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
    var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder, result);
    if (playlistDirChain && state)
      addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder, result);
  }
  return result;
}

function parseNfo(obj, nfo_file_name) {
  var node = readXml(-2);
  var level = 0;
  var isActor = false;

  while (node || level > 0) {
    if (!node && level > 0) {
      node = readXml(-1); // read parent
      node = readXml(0); // read next
      isActor = false;
      level--;
    } else if (node.NAME === "movie") {
      node = readXml(1); // read children
      obj.upnpclass = UPNP_CLASS_VIDEO_MOVIE;
      level++;
    } else if (node.NAME === "musicvideo") {
      node = readXml(1); // read children
      obj.upnpclass = UPNP_CLASS_VIDEO_MUSICVIDEOCLIP;
      level++;
    } else if (node.NAME === "tvshow") {
      node = readXml(1); // read children
      obj.upnpclass = UPNP_CLASS_VIDEO_BROADCAST;
      level++;
    } else if (node.NAME === "episodedetails") {
      node = readXml(1); // read children
      obj.upnpclass = UPNP_CLASS_VIDEO_BROADCAST;
      level++;
    } else if (node.NAME === "actor") {
      node = readXml(1); // read children
      isActor = true;
      level++;
    } else if (node.NAME == "title") {
      obj.title = node.VALUE;
      node = readXml(0); // read next
    } else if (node.NAME == "plot") {
      obj.description = node.VALUE;
      node = readXml(0); // read next
    } else if (node.NAME == "name" && isActor) {
      addMeta(obj, M_ACTOR, node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "genre") {
      addMeta(obj, M_GENRE, node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "premiered") {
      addMeta(obj, M_DATE, node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "season") {
      addMeta(obj, M_PARTNUMBER, node.VALUE);
      obj.partNumber = node.VALUE;
      node = readXml(0); // read next
    } else if (node.NAME == "episode") {
      addMeta(obj, "upnp:episodeNumber", node.VALUE);
      obj.trackNumber = node.VALUE;
      node = readXml(0); // read next
    } else if (node.NAME == "mpaa" && node.VALUE) {
      addMeta(obj, "upnp:rating", node.VALUE);
      addMeta(obj, "upnp:rating@type", "MPAA.ORG");
      addMeta(obj, "upnp:rating@equivalentAge", node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "country") {
      addMeta(obj, M_REGION, node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "studio") {
      addMeta(obj, M_PUBLISHER, node.VALUE);
      node = readXml(0); // read next
    } else if (node.NAME == "director") {
      addMeta(obj, M_DIRECTOR, node.VALUE);
      node = readXml(0); // read next
    } else {
      node = readXml(0); // read next
    }
  }
}
