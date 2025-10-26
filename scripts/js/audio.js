/*GRB*
  Gerbera - https://gerbera.io/

  audio.js - this file is part of Gerbera.

  Copyright (C) 2023-2025 Gerbera Contributors

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
// doc-add-audio-begin
function importAudio(obj, cont, rootPath, autoscanId, containerType) {
  const audio = getAudioDetails(obj);
  obj.sortKey = '';
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
  const chainSetup = config['/import/scripting/virtual-layout/boxlayout/chain/audio'];
  const boxes = [
    BK_audioRoot,
    BK_audioAll,
    BK_audioAllArtists,
    BK_audioAllGenres,
    BK_audioAllAlbums,
    BK_audioAllYears,
    BK_audioAllComposers,
    BK_audioAllSongs,
    BK_audioAllTracks,
    BK_audioArtistChronology
  ];
  const _Chain = prepareChains(boxes, boxSetup, chainSetup);
  const chain = {
    audio: _Chain[BK_audioRoot],
    allAudio: _Chain[BK_audioAll],
    allArtists: _Chain[BK_audioAllArtists],
    allGenres: _Chain[BK_audioAllGenres],
    allAlbums: _Chain[BK_audioAllAlbums],
    allYears: _Chain[BK_audioAllYears],
    allComposers: _Chain[BK_audioAllComposers],
    allSongs: _Chain[BK_audioAllSongs],
    allFull: _Chain[BK_audioAllTracks],
    allFullArtist: {
      id: boxSetup[BK_audioAllTracks].id,
      title: boxSetup[BK_audioAllTracks].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioAllTracks].class,
      metaData: {},
    },
    artistChronology: _Chain[BK_audioArtistChronology],

    artist: {
      searchable: true,
      title: audio.artists[0],
      location: audio.artists[0],
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ARTIST,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    album: {
      searchable: false,
      title: audio.album,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ALBUM,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    genre: {
      title: audio.genre,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_GENRE,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    year: {
      title: audio.date,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    composer: {
      title: audio.composer,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_COMPOSER,
      metaData: {},
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

  createUserChain(obj, audio, _Chain, boxSetup, chainSetup, result, rootPath);

  var container;
  if (boxSetup[BK_audioAll].enabled) {
    container = addContainerTree([chain.audio, chain.allAudio]);
    result.push(addCdsObject(obj, container, rootPath));
  }

  const artCnt = audio.artists.length;
  if (boxSetup[BK_audioAllSongs].enabled && boxSetup[BK_audioAllArtists].enabled) {

    for (var i = 0; i < artCnt; i++) {
      chain.artist.title = audio.artists[i];

      container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.allSongs]);
      result.push(addCdsObject(obj, container, rootPath));
    }
  }

  if (boxSetup[BK_audioAllTracks].enabled) {
    const titleBackup = obj.title;
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
    result.push(addCdsObject(obj, container, rootPath));

    if (boxSetup[BK_audioAllArtists].enabled) {
      for (var i = 0; i < artCnt; i++) {
        chain.artist.title = audio.artists[i];

        container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.allFullArtist]);
        result.push(addCdsObject(obj, container, rootPath));
      }
    }

    obj.title = titleBackup; // Restore the title
  }


  // 'chain.album' is NOT searchable by default.
  // it shall be enabled at least in one of the following trees:
  //  - chain.audio/chain.allArtists/chain.artist/chain.album (boxSetup[BK_audioAllArtists].enabled)
  //  - chain.audio/chain.allAlbums/chain.album (boxSetup[BK_audioAllAlbums].enabled)
  //
  // If you change the sequence of if's below, you might get into issues!

  if (boxSetup[BK_audioAllArtists].enabled) {
    const titleBackup = obj.title;

    if (!boxSetup[BK_audioAllAlbums].enabled) {
      chain.album.searchable = true;
    }

    obj.title = audio.track + audio.title;
    for (var i = 0; i < artCnt; i++) {
      chain.artist.title = audio.artists[i];

      container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.album]);
      result.push(addCdsObject(obj, container, rootPath));
    }

    if (!boxSetup[BK_audioAllAlbums].enabled) {
      chain.album.searchable = false;
    }

    obj.title = titleBackup; // Restore the title
  }

  if (boxSetup[BK_audioAllAlbums].enabled) {
    const titleBackup = obj.title;
    
    chain.album.searchable = true;
    obj.title = audio.track + audio.title;

    chain.album.location = getRootPath(rootPath, obj.location).join('_');
    container = addContainerTree([chain.audio, chain.allAlbums, chain.album]);
    chain.album.location = '';
    result.push(addCdsObject(obj, container, rootPath));

    chain.album.searchable = false;
    obj.title = titleBackup; // Restore the title
  }

  if (boxSetup[BK_audioAllGenres].enabled) {
    chain.genre.searchable = true;

    const genCnt = audio.genres.length;
    for (var j = 0; j < genCnt; j++) {
      chain.genre.title = audio.genres[j];
      chain.genre.metaData[M_GENRE] = [audio.genres[j]];
      container = addContainerTree([chain.audio, chain.allGenres, chain.genre]);
      result.push(addCdsObject(obj, container, rootPath));
    }

    chain.genre.searchable = false;
  }

  if (boxSetup[BK_audioAllYears].enabled) {
    container = addContainerTree([chain.audio, chain.allYears, chain.year]);
    result.push(addCdsObject(obj, container, rootPath));
  }

  if (boxSetup[BK_audioAllComposers].enabled) {
    container = addContainerTree([chain.audio, chain.allComposers, chain.composer]);
    result.push(addCdsObject(obj, container, rootPath));
  }

  if (boxSetup[BK_audioArtistChronology].enabled && boxSetup[BK_audioAllArtists].enabled) {
    // NOT searchable here, because it shall be already if boxSetup[BK_audioAllAlbums].enabled or worstcase if boxSetup[BK_audioAllArtists].enabled
    chain.album.searchable = false;

    chain.album.title = audio.date + " - " + audio.album;

    for (var i = 0; i < artCnt; i++) {
      chain.artist.title = audio.artists[i];
      container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.artistChronology, chain.album]);
      result.push(addCdsObject(obj, container, rootPath));
    }

    chain.album.title = audio.album; // Restore the title;
  }
  return result;
}
// doc-add-audio-end

function importAudioStructured(obj, cont, rootPath, autoscanId, containerType) {
  // first gather data
  const audio = getAudioDetails(obj);
  obj.sortKey = '';
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
  const chainSetup = config['/import/scripting/virtual-layout/boxlayout/chain/audio'];
  const parentCount = intFromConfig('/import/resources/container/attribute::parentCount', 1);
  const containerResource = parentCount > 1 ? cont.res : undefined;
  const containerRefID = cont.res.count > 0 ? cont.id : obj.id;
  const boxes = [
    BK_audioStructuredAllArtists,
    BK_audioStructuredAllGenres,
    BK_audioStructuredAllAlbums,
    BK_audioStructuredAllYears,
    BK_audioStructuredAllTracks
  ];
  const _Chain = prepareChains(boxes, boxSetup, chainSetup);
  const chain = {
    allArtists: _Chain[BK_audioStructuredAllArtists],
    allGenres: _Chain[BK_audioStructuredAllGenres],
    allAlbums: _Chain[BK_audioStructuredAllAlbums],
    allYears: _Chain[BK_audioStructuredAllYears],
    allTracks: _Chain[BK_audioStructuredAllTracks],
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
      searchable: false,
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

  createUserChain(obj, audio, _Chain, boxSetup, chainSetup, result, rootPath);

  obj.title = tracktitle;
  var container = addContainerTree(isSingleCharBox ? [chain.allAlbums, chain.abc, chain.album_artist] : [chain.allAlbums, chain.abc, chain.init, chain.album_artist]);
  result.push(addCdsObject(obj, container, rootPath));

  container = addContainerTree([chain.allAlbums, chain.abc, chain.entryAllLevel2, chain.album_artist]);
  result.push(addCdsObject(obj, container, rootPath));

  container = addContainerTree([chain.allAlbums, chain.entryAllLevel1, chain.album_artist]);
  result.push(addCdsObject(obj, container, rootPath));

  // Artist
  const artCnt = audio.artists.length;
  var i;

  // A track may be interpreted by more than one artist 
  for (i = 0; i < artCnt; i++) {
    obj.title = audio.title + ' (' + audio.album + ', ' + audio.date + ')';

    chain.artist.title = audio.artists[i];
    
    // Ensure the ARTIST entries in entryAllLevel1 are searchable and nowhere else
    chain.artist.searchable = true;
    container = addContainerTree([chain.allArtists, chain.entryAllLevel1, chain.artist]);
    result.push(addCdsObject(obj, container, rootPath));
    chain.artist.searchable = false;

    chain.abc.title = abcbox(audio.artists[i], boxSetup[BK_audioStructuredAllArtists].size, boxConfig.divChar);
    isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;
    container = addContainerTree([chain.allArtists, chain.abc, chain.entryAllLevel2, chain.artist]);
    result.push(addCdsObject(obj, container, rootPath));

    chain.init.title = mapInitial(audio.artists[i].charAt(0));
    container = addContainerTree(isSingleCharBox ? [chain.allArtists, chain.abc, chain.artist, chain.entryAllLevel3] : [chain.allArtists, chain.abc, chain.init, chain.artist, chain.entryAllLevel3]);
    result.push(addCdsObject(obj, container, rootPath));

    obj.title = tracktitle;
    chain.album.title = audio.album + ' (' + audio.date + ')';
    container = addContainerTree(isSingleCharBox ? [chain.allArtists, chain.abc, chain.artist, chain.album] : [chain.allArtists, chain.abc, chain.init, chain.artist, chain.album]);
    result.push(addCdsObject(obj, container, rootPath));
  }

  // Genre
  if (boxSetup[BK_audioStructuredAllGenres].enabled) {
    obj.title = audio.title + ' - ' + audio.artistFull;
    chain.entryAllLevel2.upnpclass = UPNP_CLASS_CONTAINER_MUSIC_GENRE;
    const genCnt = audio.genres.length;
    for (var j = 0; j < genCnt; j++) {
      chain.genre.title = audio.genres[j];
      chain.genre.metaData[M_GENRE] = [audio.genres[j]];
      container = addContainerTree([chain.allGenres, chain.genre, chain.entryAllLevel1]);
      result.push(addCdsObject(obj, container, rootPath));
    }

    for (i = 0; i < artCnt; i++) {
      chain.abc.title = abcbox(audio.artists[i], boxSetup[BK_audioStructuredAllGenres].size, boxConfig.divChar);
      isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;
      chain.album_artist.searchable = true;
      for (var j = 0; j < genCnt; j++) {
        chain.genre.title = audio.genres[j];
        chain.genre.metaData[M_GENRE] = [audio.genres[j]];
        container = addContainerTree(isSingleCharBox ? [chain.allGenres, chain.genre, chain.abc, chain.album_artist] : [chain.allGenres, chain.genre, chain.abc, chain.init, chain.album_artist]);
        result.push(addCdsObject(obj, container, rootPath));
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
    result.push(addCdsObject(obj, container, rootPath));

    obj.title = audio.title + ' - ' + audio.artistFull;
    container = addContainerTree([chain.allTracks, chain.entryAllLevel1]);
    result.push(addCdsObject(obj, container, rootPath));
  }

  // Sort years into decades
  if (boxSetup[BK_audioStructuredAllYears].enabled) {
    obj.title = audio.title + ' - ' + audio.artistFull;

    container = addContainerTree([chain.allYears, chain.decade, chain.entryAllLevel3]);
    result.push(addCdsObject(obj, container, rootPath));

    container = addContainerTree([chain.allYears, chain.decade, chain.date, chain.entryAllLevel3]);
    result.push(addCdsObject(obj, container, rootPath));

    obj.title = tracktitle;
    chain.album.title = audio.album;
    container = addContainerTree([chain.allYears, chain.decade, chain.date, chain.artist, chain.album]);
    result.push(addCdsObject(obj, container, rootPath));
  }
  return result;
}

// Create layout that has a slot for each initial of the album artist
function importAudioInitial(obj, cont, rootPath, autoscanId, containerType) {
  const audio = getAudioDetails(obj);
  const scriptOptions = config['/import/scripting/virtual-layout/script-options/script-option'];

  obj.title = audio.title;
  obj.sortKey = '';
  // set option in config if you want to remove track numbers in front of the title
  if (scriptOptions && scriptOptions['trackNumbers'] === 'hide')
    audio.track = '';

  const specialGenre = new RegExp(scriptOptions && scriptOptions['specialGenre'] ? scriptOptions['specialGenre'] : 'NotDefinedGenre');
  const spokenGenre = new RegExp(scriptOptions && scriptOptions['spokenGenre'] ? scriptOptions['spokenGenre'] : 'Audio Book|H.*rspiel|Stories|Gesprochen|Comedy|Spoken');

  if (audio.desc) {
    obj.description = audio.desc;
  }

  // We finally gathered all data that we need, so let's create a
  // nice layout for our audio files.

  // The UPnP class argument to addCdsObject() is optional, if it is
  // not supplied the default UPnP class will be used. However, it
  // is suggested to correctly set UPnP classes of containers and
  // objects - this information may be used by some renderers to
  // identify the type of the container and present the content in a
  // different manner.
  const parentCount = intFromConfig('/import/resources/container/attribute::parentCount', 1);
  const containerResource = parentCount > 1 ? cont.res : undefined;
  const containerRefID = cont.res.count > 0 ? cont.id : obj.id;
  const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];
  const chainSetup = config['/import/scripting/virtual-layout/boxlayout/chain/audio'];
  const boxes = [
    BK_audioRoot,
    BK_audioAll,
    BK_audioAllArtists,
    BK_audioAllGenres,
    BK_audioAllAlbums,
    BK_audioAllYears,
    BK_audioAllComposers,
    BK_audioAllSongs,
    BK_audioAllTracks,
    BK_audioArtistChronology,
    BK_audioInitialAllArtistTracks,
    BK_audioInitialAbc,
    BK_audioInitialAudioBookRoot,
    BK_audioInitialAllBooks];
  const _Chain = prepareChains(boxes, boxSetup, chainSetup);
  const chain = {
    audio: _Chain[BK_audioRoot],
    allAudio: _Chain[BK_audioAll],
    allArtists: _Chain[BK_audioAllArtists],
    allGenres: _Chain[BK_audioAllGenres],
    allAlbums: _Chain[BK_audioAllAlbums],
    allYears: _Chain[BK_audioAllYears],
    allComposers: _Chain[BK_audioAllComposers],
    allSongs: _Chain[BK_audioAllSongs],
    allFull: _Chain[BK_audioAllTracks],
    allFullArtist: {
      id: boxSetup[BK_audioAllTracks].id,
      title: boxSetup[BK_audioAllTracks].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioAllTracks].class,
      metaData: {},
    },
    artistChronology: _Chain[BK_audioArtistChronology],
    all000: {
      id: boxSetup[BK_audioInitialAllArtistTracks].id,
      title: boxSetup[BK_audioInitialAllArtistTracks].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioInitialAllArtistTracks].class,
      upnpShortcut: boxSetup[BK_audioInitialAllArtistTracks].upnpShortcut,
      metaData: {},
      res: parentCount > 0 ? cont.res : undefined,
      aux: obj.aux,
      refID: containerRefID
    },
    abc: _Chain[BK_audioInitialAbc],

    init: {
      title: '_',
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER
    },
    artist: {
      title: audio.albumArtist,
      location: audio.albumArtist,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ARTIST,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    album: {
      title: audio.album,
      location: getRootPath(rootPath, obj.location).join('_'),
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: containerType,
      metaData: {},
      res: parentCount > 0 ? cont.res : undefined,
      aux: obj.aux,
      refID: containerRefID
    },
    genre: {
      title: audio.genre,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_GENRE,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    year: {
      title: audio.date,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    composer: {
      title: audio.composer,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_COMPOSER,
      metaData: {},
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
  };

  const isAudioBook = obj.upnpclass === UPNP_CLASS_AUDIO_BOOK;
  chain.audio.metaData[M_CONTENT_CLASS] = [UPNP_CLASS_AUDIO_ITEM];
  chain.album.metaData[M_ARTIST] = [audio.albumArtist];
  chain.album.metaData[M_ALBUMARTIST] = [audio.albumArtist];
  chain.album.metaData[M_GENRE] = [audio.genre];
  chain.album.metaData[M_DATE] = obj.metaData[M_DATE];
  chain.album.metaData[M_UPNP_DATE] = obj.metaData[M_UPNP_DATE];
  chain.album.metaData[M_ALBUM] = [audio.album];
  chain.artist.metaData[M_ARTIST] = [audio.albumArtist];
  chain.artist.metaData[M_ALBUMARTIST] = [audio.albumArtist];
  chain.all000.metaData[M_ARTIST] = [audio.albumArtist];
  chain.all000.metaData[M_ALBUMARTIST] = [audio.albumArtist];
  chain.year.metaData[M_DATE] = [audio.date];
  chain.year.metaData[M_UPNP_DATE] = [audio.date];
  chain.composer.metaData[M_COMPOSER] = [audio.composer];

  const result = [];

  createUserChain(obj, audio, _Chain, boxSetup, chainSetup, result, rootPath);

  var container;
  if (isAudioBook) {
    chain.audio = {
      id: boxSetup[BK_audioInitialAudioBookRoot].id,
      title: boxSetup[BK_audioInitialAudioBookRoot].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioInitialAudioBookRoot].class
    };
    chain.allAlbums = {
      id: boxSetup[BK_audioInitialAllBooks].id,
      title: boxSetup[BK_audioInitialAllBooks].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioInitialAllBooks].class
    };
  } else {
    container = addContainerTree([chain.audio, chain.allAudio]);
    result.push(addCdsObject(obj, container, rootPath));
    container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.allSongs]);
    result.push(addCdsObject(obj, container, rootPath));
  }

  var temp = '';
  if (audio.artistFull) {
    temp = audio.artistFull;
  }

  if (audio.albumFull) {
    temp = temp + ' - ' + audio.albumFull + ' - ';
  } else {
    temp = temp + ' - ';
  }

  if (!isAudioBook) {
    obj.title = temp + audio.title;
    if (boxSetup[BK_audioAllTracks].enabled) {
      container = addContainerTree([chain.audio, chain.allFull]);
      result.push(addCdsObject(obj, container, rootPath));
      container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.allFullArtist]);
      result.push(addCdsObject(obj, container, rootPath));
    }
  }
  obj.title = audio.track + audio.title;
  container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.album]);
  result.push(addCdsObject(obj, container, rootPath));

  if (boxSetup[BK_audioAllAlbums].enabled) {
    if (!isAudioBook) {
      obj.title = audio.album + " - " + audio.track + audio.title;
      container = addContainerTree([chain.audio, chain.allAlbums, chain.artist, chain.all000]);
      result.push(addCdsObject(obj, container, rootPath));
    }
    obj.title = audio.track + audio.title;
    container = addContainerTree([chain.audio, chain.allAlbums, chain.artist, chain.album]);
    result.push(addCdsObject(obj, container, rootPath));

    obj.title = audio.track + audio.title;
    // Remember, the server will sort all items by ID3 track if the
    // container class is set to UPNP_CLASS_CONTAINER_MUSIC_ALBUM.
    chain.all000.metaData[M_ARTIST] = [];
    chain.all000.metaData[M_ALBUMARTIST] = [];

    if (!isAudioBook) {
      chain.album.location = getRootPath(rootPath, obj.location).join('_');
      container = addContainerTree([chain.audio, chain.allAlbums, chain.all000, chain.album]);
      chain.album.location = '';
      result.push(addCdsObject(obj, container, rootPath));
    }
  }

  if (audio.disknr != '') {
    if (specialGenre.exec(audio.genre)) {
      if (audio.disknr.length == 1) {
        chain.album.title = '0' + audio.disknr + ' - ' + audio.album;
      } else {
        chain.album.title = audio.disknr + ' - ' + audio.album;
      }
    }
  }
  var init = mapInitial(audio.albumArtist.charAt(0));
  if (init && init !== '') {
    chain.init.title = init;
  }

  var spokenMatch = spokenGenre.exec(audio.genre) ? true : false;
  obj.title = audio.track + audio.title;
  chain.artist.searchable = true;
  chain.album.searchable = true;

  if (!isAudioBook) {
    container = addContainerTree([chain.audio, chain.abc, chain.init, chain.artist, chain.album]);
    result.push(addCdsObject(obj, container, rootPath));
  }
  chain.artist.searchable = false;
  chain.album.searchable = false;

  if (spokenMatch === false && !isAudioBook) {
    var part = '';
    if (obj.metaData[M_UPNP_DATE] && obj.metaData[M_UPNP_DATE][0])
      part = obj.metaData[M_UPNP_DATE][0] + ' - ';
    obj.title = part + chain.album.title + " - " + audio.track + audio.title;
    container = addContainerTree([chain.audio, chain.abc, chain.init, chain.artist, chain.all000]);
    result.push(addCdsObject(obj, container, rootPath));
  }
  chain.album.title = audio.album;

  if (boxSetup[BK_audioAllGenres].enabled) {
    const genCnt = audio.genres.length;
    for (var j = 0; j < genCnt; j++) {
      var mapResult = mapGenre(audio.genres[j]);
      var gMatch = 0;
      if (mapResult.mapped) {
        obj.title = temp + audio.track + audio.title;
        gMatch = 1;
        chain.genre.title = mapResult.value;
        chain.all000.upnpclass = UPNP_CLASS_CONTAINER_MUSIC_GENRE;
        chain.all000.metaData[M_GENRE] = [mapResult.value];
        chain.genre.metaData[M_GENRE] = [mapResult.value];

        if (spokenMatch === false && !isAudioBook) {
          container = addContainerTree([chain.audio, chain.allGenres, chain.genre, chain.all000]);
          result.push(addCdsObject(obj, container, rootPath));
        }
        if (specialGenre.exec(chain.genre.title)) {
          if (audio.disknr.length === 1) {
            chain.album.title = '0' + audio.disknr + ' - ' + audio.album;
          } else {
            chain.album.title = audio.disknr + ' - ' + audio.album;
          }
        }
        container = addContainerTree([chain.audio, chain.allGenres, chain.genre, chain.artist, chain.album]);
        result.push(addCdsObject(obj, container, rootPath));
      }
    }
    obj.title = temp + audio.title;
    if (gMatch === 0) {
      chain.genre.title = 'Other';
      chain.genre.metaData[M_GENRE] = ['Other'];
      container = addContainerTree([chain.audio, chain.allGenres, chain.genre]);
      result.push(addCdsObject(obj, container, rootPath));
    }

    chain.genre.title = audio.genre;
    chain.genre.metaData[M_GENRE] = [audio.genre];
    chain.all000.upnpclass = UPNP_CLASS_CONTAINER;
    chain.all000.metaData[M_GENRE] = [];
    chain.genre.searchable = true;

    if (!isAudioBook) {
      container = addContainerTree([chain.audio, chain.allGenres, chain.all000, chain.genre]);
      result.push(addCdsObject(obj, container, rootPath));
    }
  }

  if (boxSetup[BK_audioAllYears].enabled) {
    container = addContainerTree([chain.audio, chain.allYears, chain.year]);
    result.push(addCdsObject(obj, container, rootPath));
  }

  if (boxSetup[BK_audioAllComposers].enabled && audio.composer !== 'None') {
    container = addContainerTree([chain.audio, chain.allComposers, chain.composer]);
    result.push(addCdsObject(obj, container, rootPath));
  }

  if (boxSetup[BK_audioArtistChronology].enabled && boxSetup[BK_audioAllArtists].enabled) {
    chain.album.searchable = false;
    chain.artist.searchable = false;
    chain.album.title = audio.date + " - " + audio.album;
    container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.artistChronology, chain.album]);
    result.push(addCdsObject(obj, container, rootPath));

    chain.album.title = audio.album; // Restore the title;
  }
  return result;
}
