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

// Create layout that has a slot for each initial of the album artist
var object_ref_list;
function importAudioInitial(obj, cont, rootPath, autoscanId, containerType) {
  object_autoscan_id = autoscanId;
  object_ref_list = addAudioInitial(obj, cont, rootPath, containerType);
  return object_ref_list;
}

function addAudioInitial(obj, cont, rootPath, containerType) {

  const audio = getAudioDetails(obj);
  const scriptOptions = config['/import/scripting/virtual-layout/script-options/script-option'];

  obj.title = audio.title;
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
    all000: {
      id: boxSetup[BK_audioInitialAllArtistTracks].id,
      title: boxSetup[BK_audioInitialAllArtistTracks].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioInitialAllArtistTracks].class,
      upnpShortcut: boxSetup[BK_audioInitialAllArtistTracks].upnpShortcut,
      metaData: [],
      res: parentCount > 0 ? cont.res : undefined,
      aux: obj.aux,
      refID: containerRefID
    },
    abc: {
      id: boxSetup[BK_audioInitialAbc].id,
      title: boxSetup[BK_audioInitialAbc].title,
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: boxSetup[BK_audioInitialAbc].class
    },

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
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    album: {
      title: audio.album,
      location: getRootPath(rootPath, obj.location).join('_'),
      objectType: OBJECT_TYPE_CONTAINER,
      upnpclass: containerType,
      metaData: [],
      res: parentCount > 0 ? cont.res : undefined,
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
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
    composer: {
      title: audio.composer,
      objectType: OBJECT_TYPE_CONTAINER,
      searchable: true,
      upnpclass: UPNP_CLASS_CONTAINER_MUSIC_COMPOSER,
      metaData: [],
      res: containerResource,
      aux: obj.aux,
      refID: containerRefID
    },
  };

  const isAudioBook = obj.upnpclass === UPNP_CLASS_AUDIO_BOOK;
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
    result.push(addCdsObject(obj, container));
    container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.allSongs]);
    result.push(addCdsObject(obj, container));
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
      result.push(addCdsObject(obj, container));
      container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.allFullArtist]);
      result.push(addCdsObject(obj, container));
    }
  }
  obj.title = audio.track + audio.title;
  container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.album]);
  result.push(addCdsObject(obj, container));

  if (boxSetup[BK_audioAllAlbums].enabled) {
    if (!isAudioBook) {
      obj.title = audio.album + " - " + audio.track + audio.title;
      container = addContainerTree([chain.audio, chain.allAlbums, chain.artist, chain.all000]);
      result.push(addCdsObject(obj, container));
    }
    obj.title = audio.track + audio.title;
    container = addContainerTree([chain.audio, chain.allAlbums, chain.artist, chain.album]);
    result.push(addCdsObject(obj, container));

    obj.title = audio.track + audio.title;
    // Remember, the server will sort all items by ID3 track if the
    // container class is set to UPNP_CLASS_CONTAINER_MUSIC_ALBUM.
    chain.all000.metaData[M_ARTIST] = [];
    chain.all000.metaData[M_ALBUMARTIST] = [];

    if (!isAudioBook) {
      chain.album.location = getRootPath(rootPath, obj.location).join('_');
      container = addContainerTree([chain.audio, chain.allAlbums, chain.all000, chain.album]);
      chain.album.location = '';
      result.push(addCdsObject(obj, container));
    }
  }

  var init = mapInitial(audio.albumArtist.charAt(0));
  if (audio.disknr != '') {
    if (specialGenre.exec(audio.genre)) {
      if (disknr.length == 1) {
        chain.album.title = '0' + audio.disknr + ' - ' + audio.album;
      } else {
        chain.album.title = audio.disknr + ' - ' + audio.album;
      }
    }
  }
  if (init && init !== '') {
    chain.init.title = init;
  }

  var spokenMatch = spokenGenre.exec(audio.genre) ? true : false;
  obj.title = audio.track + audio.title;
  chain.artist.searchable = true;
  chain.album.searchable = true;

  if (!isAudioBook) {
    container = addContainerTree([chain.audio, chain.abc, chain.init, chain.artist, chain.album]);
    result.push(addCdsObject(obj, container));
  }
  chain.artist.searchable = false;
  chain.album.searchable = false;

  if (spokenMatch === false && !isAudioBook) {
    var part = '';
    if (obj.metaData[M_UPNP_DATE] && obj.metaData[M_UPNP_DATE][0])
      part = obj.metaData[M_UPNP_DATE][0] + ' - ';
    obj.title = part + chain.album.title + " - " + audio.track + audio.title;
    container = addContainerTree([chain.audio, chain.abc, chain.init, chain.artist, chain.all000]);
    result.push(addCdsObject(obj, container));
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
        chain.all000.metaData[M_GENRE] = [ mapResult.value ];
        chain.genre.metaData[M_GENRE] = [ mapResult.value ];

        if (spokenMatch === false && !isAudioBook) {
          container = addContainerTree([chain.audio, chain.allGenres, chain.genre, chain.all000]);
          result.push(addCdsObject(obj, container));
        }
        if (specialGenre.exec(chain.genre.title)) {
          if (disknr.length === 1) {
            chain.album.title = '0' + audio.disknr + ' - ' + audio.album;
          } else {
            chain.album.title = audio.disknr + ' - ' + audio.album;
          }
        }
        container = addContainerTree([chain.audio, chain.allGenres, chain.genre, chain.artist, chain.album]);
        result.push(addCdsObject(obj, container));
      }
    }
    obj.title = temp + audio.title;
    if (gMatch === 0) {
      chain.genre.title = 'Other';
      chain.genre.metaData[M_GENRE] = ['Other'];
      container = addContainerTree([chain.audio, chain.allGenres, chain.genre]);
      result.push(addCdsObject(obj, container));
    }

    chain.genre.title = audio.genre;
    chain.genre.metaData[M_GENRE] = [ audio.genre ];
    chain.all000.upnpclass = UPNP_CLASS_CONTAINER;
    chain.all000.metaData[M_GENRE] = [];
    chain.genre.searchable = true;

    if (!isAudioBook) {
      container = addContainerTree([chain.audio, chain.allGenres, chain.all000, chain.genre]);
      result.push(addCdsObject(obj, container));
    }
  }

  if (boxSetup[BK_audioAllYears].enabled) {
    container = addContainerTree([chain.audio, chain.allYears, chain.year]);
    result.push(addCdsObject(obj, container));
  }

  if (boxSetup[BK_audioAllComposers].enabled && audio.composer !== 'None') {
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

// Global Variables
var orig;
var cont;
var object_ref_list;
// compatibility with older configurations
if (!cont || cont === undefined)
  cont = orig;
if (orig && orig !== undefined)
  object_ref_list = importAudioInitial(orig, cont);
