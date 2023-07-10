/*GRB*
  Gerbera - https://gerbera.io/

  audio.js - this file is part of Gerbera.

  Copyright (C) 2023 Gerbera Contributors

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
function importAudioInitial(obj, cont, rootPath, autoscanId, containerType) {
    object_autoscan_id = autoscanId;
    addAudioInitial(obj, cont, rootPath, containerType);
}

function addAudioInitial(obj, cont, rootPath, containerType) {

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
    var artist = [ 'Unknown' ];
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
        if (!disknr || disknr.length==0) {
           disknr = obj.aux['TPOS'];
        }
    }
    if (!disknr || disknr.length==0) {
        disknr = '';
    } else if (disknr == '1/1') {
        disknr = '';
    } else {
        var re = new RegExp("[^0-9].*","i");
        disknr = new String(disknr).replace(re,"");
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
        obj.metaData[M_UPNP_DATE] = [ date ];
        desc = desc + ', ' + date;
    }

    var genre= 'Unknown';
    if (obj.metaData[M_GENRE] && obj.metaData[M_GENRE][0]) {
        //genre = mapGenre(obj.metaData[M_GENRE][0]);
        genre = obj.metaData[M_GENRE][0];
        desc = desc + ', ' + genre;
    }

    if (!obj.metaData[M_DESCRIPTION] || !obj.metaData[M_DESCRIPTION][0]) {
        obj.description = desc;
    } else {
        obj.description = obj.metaData[M_DESCRIPTION][0];
    }

    var composer = 'None';
    if (obj.metaData[M_COMPOSER] && obj.metaData[M_COMPOSER][0]) {
        composer = obj.metaData[M_COMPOSER][0];
    }

    if (desc) {
         obj.description = desc;
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
            obj.metaData[M_TRACKNUMBER] = [ disknr + '' + track ];
            track = '0' + disknr + ' ' + track;
        }
        else {
            obj.metaData[M_TRACKNUMBER] = [ disknr + '' + track ];
            if (disknr.length > 1) {
                track = disknr + ' ' + track;
            }
        }
        track = track + ' ';
    }

    if (artist.join(' - ') != aartist) {
        title = artist.join(' - ') + ' - ' + title;
    }

    obj.title = title;

    // We finally gathered all data that we need, so let's create a
    // nice layout for our audio files.

    // The UPnP class argument to addCdsObject() is optional, if it is
    // not supplied the default UPnP class will be used. However, it
    // is suggested to correctly set UPnP classes of containers and
    // objects - this information may be used by some renderers to
    // identify the type of the container and present the content in a
    // different manner.
    const containerResource = cont.res;
    const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];
    const chain = {
        audio: {
            id: boxSetup['Audio/audioRoot'].id,
            title: boxSetup['Audio/audioRoot'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Audio/audioRoot'].class },
        allAudio: {
            id: boxSetup['Audio/allAudio'].id,
            title: boxSetup['Audio/allAudio'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Audio/allAudio'].class },
        allArtists: {
            id: boxSetup['Audio/allArtists'].id,
            title: boxSetup['Audio/allArtists'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Audio/allArtists'].class },
        allGenres: {
            id: boxSetup['Audio/allGenres'].id,
            title: boxSetup['Audio/allGenres'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Audio/allGenres'].class },
        allAlbums: {
            id: boxSetup['Audio/allAlbums'].id,
            title: boxSetup['Audio/allAlbums'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Audio/allAlbums'].class },
        allYears: {
            id: boxSetup['Audio/allYears'].id,
            title: boxSetup['Audio/allYears'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Audio/allYears'].class },
        allComposers: {
            id: boxSetup['Audio/allComposers'].id,
            title: boxSetup['Audio/allComposers'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Audio/allComposers'].class },
        allSongs: {
            id: boxSetup['Audio/allSongs'].id,
            title: boxSetup['Audio/allSongs'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Audio/allSongs'].class },
        allFull: {
            id: boxSetup['Audio/allTracks'].id,
            title: boxSetup['Audio/allTracks'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Audio/allTracks'].class },
        all000: {
            id: boxSetup['AudioInitial/allArtistTracks'].id,
            title: boxSetup['AudioInitial/allArtistTracks'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['AudioInitial/allArtistTracks'].class,
            metaData: [], res: containerResource, aux: obj.aux, refID: cont.id },
        abc: {
            id: boxSetup['AudioInitial/abc'].id,
            title: boxSetup['AudioInitial/abc'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['AudioInitial/abc'].class },

        init: {
            title: '_',
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER },
        artist: {
            title: aartist,
            location: aartist,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ARTIST,
            metaData: [],
            aux: obj.aux,
            refID: cont.id },
        album: {
            title: album,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: containerType,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: cont.id },
        genre: {
            title: genre,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER_MUSIC_GENRE,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: cont.id },
        year: {
            title: date,
            objectType: OBJECT_TYPE_CONTAINER,
            searchable: true,
            upnpclass: UPNP_CLASS_CONTAINER,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: cont.id },
        composer: {
            title: composer,
            objectType: OBJECT_TYPE_CONTAINER,
            searchable: true,
            upnpclass: UPNP_CLASS_CONTAINER_MUSIC_COMPOSER,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: cont.id },
    };
    const isAudioBook = obj.upnpclass === UPNP_CLASS_AUDIO_BOOK;
    chain.album.metaData[M_ARTIST] = [ aartist ];
    chain.album.metaData[M_ALBUMARTIST] = [ aartist ];
    chain.album.metaData[M_GENRE] = [ genre ];
    chain.album.metaData[M_DATE] = obj.metaData[M_DATE];
    chain.album.metaData[M_UPNP_DATE] = obj.metaData[M_UPNP_DATE];
    chain.album.metaData[M_ALBUM] = [ album ];
    chain.artist.metaData[M_ARTIST] = [ aartist ];
    chain.artist.metaData[M_ALBUMARTIST] = [ aartist ];
    chain.all000.metaData[M_ARTIST] = [ aartist ];
    chain.all000.metaData[M_ALBUMARTIST] = [ aartist ];
    chain.year.metaData[M_DATE] = [ date ];
    chain.year.metaData[M_UPNP_DATE] = [ date ];
    chain.composer.metaData[M_COMPOSER] = [ composer ];

    var container;
    if (isAudioBook) {
        chain.audio = {
            id: boxSetup['AudioInitial/audioBookRoot'].id,
            title: boxSetup['AudioInitial/audioBookRoot'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['AudioInitial/audioBookRoot'].class };
        chain.allAlbums = {
            id: boxSetup['AudioInitial/allBooks'].id,
            title: boxSetup['AudioInitial/allBooks'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['AudioInitial/allBooks'].class };
    } else {
        container = addContainerTree([chain.audio, chain.allAudio]);
        addCdsObject(obj, container);
        container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.allSongs]);
        addCdsObject(obj, container);
    }

    var temp = '';
    if (artist_full) {
        temp = artist_full;
    }

    if (album_full) {
        temp = temp + ' - ' + album_full + ' - ';
    } else {
        temp = temp + ' - ';
    }

    if (!isAudioBook) {
        obj.title = temp + title;
        if (boxSetup['Audio/allTracks'].enabled) {
            container = addContainerTree([chain.audio, chain.allFull]);
            addCdsObject(obj, container);
            container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.allFull]);
            addCdsObject(obj, container);
        }
    }
    obj.title = track + title;
    container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.album]);
    addCdsObject(obj, container);

    if (boxSetup['Audio/allAlbums'].enabled) {
        if (!isAudioBook) {
            obj.title = album + " - " + track + title;
            container = addContainerTree([chain.audio, chain.allAlbums, chain.artist, chain.all000]);
            addCdsObject(obj, container);
        }
        obj.title = track + title;
        container = addContainerTree([chain.audio, chain.allAlbums, chain.artist, chain.album]);
        addCdsObject(obj, container);

        obj.title = track + title;
        // Remember, the server will sort all items by ID3 track if the
        // container class is set to UPNP_CLASS_CONTAINER_MUSIC_ALBUM.
        chain.all000.metaData[M_ARTIST] = [];
        chain.all000.metaData[M_ALBUMARTIST] = [];

        if (!isAudioBook) {
            container = addContainerTree([chain.audio, chain.allAlbums, chain.all000, chain.album]);
            addCdsObject(obj, container);
        }
    }

    var init = mapInitial(aartist.charAt(0));
    if (disknr != '') {
        var re = new RegExp('Children', 'i');
        var match = re.exec(genre);
        if (match) {
            if (disknr.length == 1) {
                chain.album.title = '0' + disknr + ' - ' + album;
            } else {
                chain.album.title = disknr + ' - ' + album;
            }
        }
    }
    if (init && init !== '') {
        chain.init.title = init;
    }

    var noAll = ['Audio Book', 'H.*rspiel', 'Stories', 'Gesprochen', 'Comedy'];
    var spokenMatch = false;
    for (var k=0; k < noAll.length; k++) {
        var re = new RegExp(noAll[k], 'i');
        var match = re.exec(genre);
        if (match) {
            spokenMatch = true;
        }
    }
    obj.title = track + title;
    chain.artist.searchable = true;
    chain.album.searchable = true;

    if (!isAudioBook) {
        container = addContainerTree([chain.audio, chain.abc, chain.init, chain.artist, chain.album]);
        addCdsObject(obj, container);
    }
    chain.artist.searchable = false;
    chain.album.searchable = false;

    if (spokenMatch === false && !isAudioBook) {
        var part = '';
        if (obj.metaData[M_UPNP_DATE] && obj.metaData[M_UPNP_DATE][0])
            part = obj.metaData[M_UPNP_DATE][0] + ' - ';
        obj.title = part + chain.album.title + " - " + track + title;
        container = addContainerTree([chain.audio, chain.abc, chain.init, chain.artist, chain.all000]);
        addCdsObject(obj, container);
    }
    chain.album.title = album;

    if (boxSetup['Audio/allGenres'].enabled) {
        const genreConfig = config['/import/scripting/virtual-layout/genre-map/genre'];
        const genreNames = (genreConfig) ? Object.getOwnPropertyNames(genreConfig) : [];

        var gMatch = 0;
        for (var idx = 0; idx < genreNames.length; idx++) {
            var re = new RegExp('(' + genreNames[idx] + ')', 'i');
            var match = re.exec(genre);
            if (match) {
                obj.title = temp + track + title;
                gMatch = 1;
                chain.genre.title = genreConfig[genreNames[idx]];
                chain.all000.upnpclass = UPNP_CLASS_CONTAINER_MUSIC_GENRE;
                chain.all000.metaData[M_GENRE] = [ genreConfig[genreNames[idx]] ];
                chain.genre.metaData[M_GENRE] = [ genreConfig[genreNames[idx]] ];

                if (!isAudioBook) {
                    container = addContainerTree([chain.audio, chain.allGenres, chain.genre, chain.all000]);
                    addCdsObject(obj, container);
                }
                if (chain.genre.title === 'Children\'s') {
                    if (disknr.length === 1) {
                        chain.album.title = '0' + disknr + ' - ' + album;
                    } else {
                        chain.album.title = disknr + ' - ' + album;
                    }
                }
                container = addContainerTree([chain.audio, chain.allGenres, chain.genre, chain.artist, chain.album]);
                addCdsObject(obj, container);
            }
        }
        obj.title = temp + title;
        if (gMatch === 0) {
            chain.genre.title = 'Other';
            chain.genre.metaData[M_GENRE] = [ 'Other' ];
            container = addContainerTree([chain.audio, chain.allGenres, chain.genre]);
            addCdsObject(obj, container);
        }

        chain.genre.title = genre;
        chain.genre.metaData[M_GENRE] = [ genre ];
        chain.all000.upnpclass = UPNP_CLASS_CONTAINER;
        chain.all000.metaData[M_GENRE] = [];
        chain.genre.searchable = true;

        if (!isAudioBook) {
            container = addContainerTree([chain.audio, chain.allGenres, chain.all000, chain.genre]);
            addCdsObject(obj, container);
        }
    }

    if (boxSetup['Audio/allYears'].enabled) {
        container = addContainerTree([chain.audio, chain.allYears, chain.year]);
        addCdsObject(obj, container);
    }

    if (boxSetup['Audio/allComposers'].enabled) {
        container = addContainerTree([chain.audio, chain.allComposers, chain.composer]);
        addCdsObject(obj, container);
    }
}
