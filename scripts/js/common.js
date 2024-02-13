/*GRB*
  Gerbera - https://gerbera.io/

  common.js - this file is part of Gerbera.

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

function escapeSlash(name) {
    name = name.replace(/\\/g, "\\\\");
    name = name.replace(/\//g, "\\/");
    return name;
}

function createContainerChain(arr) {
    var path = '';
    for (var i = 0; i < arr.length; i++)
    {
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

function getLastPath(location) {
    var path = location.split('/');
    if ((path.length > 1) && (path[path.length - 2]))
        return path[path.length - 2];
    else
        return '';
}

function getRootPath(rootpath, location) {
    var path = new Array();

    if (rootpath && rootpath.length !== 0)
    {
        rootpath = rootpath.substring(0, rootpath.lastIndexOf('/'));

        var dir = location.substring(rootpath.length, location.lastIndexOf('/'));

        if (dir.charAt(0) == '/')
            dir = dir.substring(1);

        path = dir.split('/');
    }
    else
    {
        dir = getLastPath(location);
        if (dir != '')
        {
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
    switch (boxtype)
    {
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
        boxwidth = Math.round( (boxwidth - allstring.length +1) / 2);
    } else {
        boxwidth = Math.round( (boxwidth +1) / 2 );
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
    if ( (intchar >= 48) && (intchar <= 57) )
    {
        return divchar + '0-9' + divchar;
    }
    // check for other characters
    if ( !((intchar >= 65) && (intchar <= 90)) )
    {
        return divchar + '^\&#\'' + divchar;
    }
    // all other characters are letters
    var boxwidth;
    // definition of box types, adjust to your own needs
    // as a start: the number is the same as the number of boxes, evenly spaced ... more or less.
    switch (boxtype)
    {
    case 1:
        boxwidth = new Array();
        boxwidth[0] = 26;                             // one large box of 26 letters
        break;
    case 2:
        boxwidth = new Array(13,13);              // two boxes of 13 letters
        break;
    case 3:
        boxwidth = new Array(8,9,9);              // and so on ...
        break;
    case 4:
        boxwidth = new Array(7,6,7,6);
        break;
    case 5:
        boxwidth = new Array(5,5,5,6,5);
        break;
    case 6:
        boxwidth = new Array(4,5,4,4,5,4);
        break;
    case 7:
        boxwidth = new Array(4,3,4,4,4,3,4);
        break;
    case 9:
        boxwidth = new Array(5,5,5,4,1,6);        // When T is a large box...
        break;
    case 13:
        boxwidth = new Array(2,2,2,2,2,2,2,2,2,2,2,2,2);
        break;
    case 26:
        boxwidth = new Array(1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1);
        break;
    default:
        boxwidth = new Array(5,5,5,6,5);
        break;
    }

    // check for a total of 26 characters for all boxes
    var charttl = 0;
    for (var cb = 0; cb < boxwidth.length; cb++) {
        charttl = charttl + boxwidth[cb];
    }
    if (charttl != 26) {
        print2("Error", "In box-definition, total length is " + charttl + ". Check the file common.js" );
        // maybe an exit call here to stop processing the media ??
        return "???";
    }

    // declaration of some variables
    var boxnum = 0;                         // boxnumber start
    var sc = 65;                            // first ascii character (corresponds to 'A')
    var ec = sc + boxwidth[boxnum] - 1;     // last character of first box

    // loop that will define first and last character of the right box
    while (intchar > ec)
    {
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
    if (genreConfig) {
        const genreNames = Object.getOwnPropertyNames(genreConfig);
        for (var idx = 0; idx < genreNames.length; idx++) {
            var re = new RegExp('(' + genreNames[idx] + ')', 'i');
            var match = re.exec(genre);
            if (match) {
                genre = genreConfig[genreNames[idx]];
                break;
            }
        }
    }
    return genre;
}

// doc-add-audio-begin
function addAudio(obj, cont, rootPath, containerType) {
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

    var genre = 'Unknown';
    if (obj.metaData[M_GENRE] && obj.metaData[M_GENRE][0]) {
        genre = mapGenre(obj.metaData[M_GENRE].join(' '));
        desc = desc + ', ' + genre;
    }

    if (!obj.metaData[M_DESCRIPTION] || !obj.metaData[M_DESCRIPTION][0]) {
        obj.description = desc;
    }

    var composer = 'None';
    if (obj.metaData[M_COMPOSER] && obj.metaData[M_COMPOSER][0]) {
        composer = obj.metaData[M_COMPOSER].join(' / ');
    }

/*
    var conductor = 'None';
    if (obj.metaData[M_CONDUCTOR] && obj.metaData[M_CONDUCTOR][0]) {
        conductor = obj.metaData[M_CONDUCTOR].join(' / ');
    }

    var orchestra = 'None';
    if (obj.metaData[M_ORCHESTRA] && obj.metaData[M_ORCHESTRA][0]) {
        orchestra = obj.metaData[M_ORCHESTRA].join(' / ');
    }
*/
    // uncomment this if you want to have track numbers in front of the title
    // in album view
/*
    var track = '';
    if (obj.trackNumber > 0) {
        track = trackNumber;
        if (track.length === 1) {
            track = '0' + track;
        }
        track = track + ' ';
    }
*/
    // comment the following line out if you uncomment the stuff above  :)
    var track = '';

// uncomment this if you want to have channel numbers in front of the title
/*
    var channels = obj.res[R_NRAUDIOCHANNELS];
    if (channels) {
        if (channels === "1") {
            track = track + '[MONO]';
        } else if (channels === "2") {
            track = track + '[STEREO]';
        } else {
            track = track + '[MULTI]';
        }
    }
*/

    // We finally gathered all data that we need, so let's create a
    // nice layout for our audio files.

    obj.title = title;

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
            id: boxSetup['Audio/audioRoot'].id,
            title: boxSetup['Audio/audioRoot'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Audio/audioRoot'].class,
            metaData: [] },
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
        artistChronology: {
            id: boxSetup['Audio/artistChronology'].id,
            title: boxSetup['Audio/artistChronology'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Audio/artistChronology'].class },

        artist: {
            searchable: false,
            title: artist[0],
            location: artist[0],
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ARTIST,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: containerRefID },
        album: {
            searchable: false,
            title: album,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ALBUM,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: containerRefID },
        genre: {
            title: genre,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER_MUSIC_GENRE,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: containerRefID },
        year: {
            title: date,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: containerRefID },
        composer: {
            title: composer,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER_MUSIC_COMPOSER,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: containerRefID },
    };

    chain.audio.metaData[M_CONTENT_CLASS] = [ UPNP_CLASS_AUDIO_ITEM ];
    chain.album.metaData[M_ARTIST] = artist;
    chain.album.metaData[M_ALBUMARTIST] = artist;
    chain.album.metaData[M_GENRE] = [ genre ];
    chain.album.metaData[M_DATE] = obj.metaData[M_DATE];
    chain.album.metaData[M_UPNP_DATE] = obj.metaData[M_UPNP_DATE];
    chain.album.metaData[M_ALBUM] = [ album ];
    chain.artist.metaData[M_ARTIST] = artist;
    chain.artist.metaData[M_ALBUMARTIST] = artist;
    chain.genre.metaData[M_GENRE] = [ genre ];
    chain.year.metaData[M_DATE] = [ date ];
    chain.year.metaData[M_UPNP_DATE] = [ date ];
    chain.composer.metaData[M_COMPOSER] = [ composer ];

    var container = addContainerTree([chain.audio, chain.allAudio]);
    addCdsObject(obj, container);

    container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.allSongs]);
    addCdsObject(obj, container);

    var temp = '';
    if (artist_full) {
        temp = artist_full;
    }

    if (album_full) {
        temp = temp + ' - ' + album_full + ' - ';
    } else {
        temp = temp + ' - ';
    }

    obj.title = temp + title;
    if (boxSetup['Audio/allTracks'].enabled) {
        container = addContainerTree([chain.audio, chain.allFull]);
        addCdsObject(obj, container);
    }

    const artCnt = artist.length;
    var i;
    for (i = 0; i < artCnt; i++) {
        chain.artist.title = artist[i];
        container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.allFull]);
        addCdsObject(obj, container);
    }

    obj.title = track + title;
    for (i = 0; i < artCnt; i++) {
        chain.artist.title = artist[i];
        chain.artist.searchable = true;
        container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.album]);
        addCdsObject(obj, container);
    }

    obj.title = track + title;
    chain.album.searchable = true;
    if (boxSetup['Audio/allAlbums'].enabled) {
        chain.album.location = getRootPath(rootPath, obj.location).join('_');
        container = addContainerTree([chain.audio, chain.allAlbums, chain.album]);
        chain.album.location = '';
        addCdsObject(obj, container);
    }

    if (boxSetup['Audio/allGenres'].enabled) {
        chain.genre.searchable = true;
        if (obj.metaData[M_GENRE]) {
            for (var oneGenre in obj.metaData[M_GENRE]) {
                chain.genre.title = obj.metaData[M_GENRE][oneGenre];
                chain.genre.metaData[M_GENRE] = [ oneGenre ];
                container = addContainerTree([chain.audio, chain.allGenres, chain.genre]);
                addCdsObject(obj, container);
            }
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

    if (boxSetup['Audio/artistChronology'].enabled) {
        chain.album.searchable = false;
        chain.artist.searchable = false;
        chain.album.title = date + " - " + album;
        container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.artistChronology, chain.album]);
        addCdsObject(obj, container);
    }
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
    switch(entry){
        case null:
        case undefined:
          return false;
    }
    switch(entry.toString().trim().toLowerCase()){
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
    var title = obj.title;
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

    var album = 'Unknown';
    if (obj.metaData[M_ALBUM] && obj.metaData[M_ALBUM][0]) {
        album = obj.metaData[M_ALBUM][0];
        desc = desc + ', ' + album;
    }

    if (desc) {
        desc = desc + ', ';
    }

    desc = desc + title;

    var date = '-Unknown-';
    var decade = '-Unknown-';
    if (obj.metaData[M_DATE] && obj.metaData[M_DATE][0]) {
        date = getYear(obj.metaData[M_DATE][0]);
        obj.metaData[M_UPNP_DATE] = [ date ];
        desc = desc + ', ' + date;
        decade = date.substring(0,3) + '0 - ' + String(10 * (parseInt(date.substring(0,3))) + 9) ;
    }

    var genre = 'Unknown';
    if (obj.metaData[M_GENRE] && obj.metaData[M_GENRE][0]) {
        genre = mapGenre(obj.metaData[M_GENRE][0]);
        desc = desc + ', ' + genre;
    }

    var description = '';
    if (!obj.metaData[M_DESCRIPTION] || !obj.metaData[M_DESCRIPTION][0]) {
        obj.metaData[M_DESCRIPTION] = [ desc ];
    } else {
        description = obj.metaData[M_DESCRIPTION][0];
    }

// uncomment this if you want to have track numbers in front of the title
// in album view

/*
    var track = '';
    if (obj.trackNumber > 0) {
        track = '' + obj.trackNumber;
        if (trackNumber < 10)
        {
            track = '0' + track;
        }
        track = track + ' ';
    }
*/
    // comment the following line out if you uncomment the stuff above  :)
    var track = '';


    // Album

    // Extra code for correct display of albums with various artists (usually collections)
    var tracktitle = track + title;
    var album_artist = album + ' - ' + artist.join(' / ');
    if (description) {
        if (description.toUpperCase() === 'VARIOUS') {
            album_artist = album + ' - Various';
            tracktitle = tracktitle + ' - ' + artist.join(' / ');
        }
    }

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
            id: boxSetup['AudioStructured/allArtists'].id,
            title: boxSetup['AudioStructured/allArtists'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['AudioStructured/allArtists'].class,
            metaData: [] },
        allGenres: {
            id: boxSetup['AudioStructured/allGenres'].id,
            title: boxSetup['AudioStructured/allGenres'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['AudioStructured/allGenres'].class },
        allAlbums: {
            id: boxSetup['AudioStructured/allAlbums'].id,
            title: boxSetup['AudioStructured/allAlbums'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['AudioStructured/allAlbums'].class },
        allYears: {
            id: boxSetup['AudioStructured/allYears'].id,
            title: boxSetup['AudioStructured/allYears'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['AudioStructured/allYears'].class },
        allTracks: {
            id: boxSetup['AudioStructured/allTracks'].id,
            title: boxSetup['AudioStructured/allTracks'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['AudioStructured/allTracks'].class },
        entryAllLevel1: {
            title: allbox(boxSetup['AudioStructured/allArtistTracks'].title, 6, boxConfig.divChar),
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER },
        entryAllLevel2: {
            title: allbox(boxSetup['AudioStructured/allArtistTracks'].title, 9, boxConfig.divChar),
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER },
        entryAllLevel3: {
            title: allbox(boxSetup['AudioStructured/allArtistTracks'].title, 9, boxConfig.divChar),
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ARTIST },

        abc: {
            title: abcbox(album, boxSetup['AudioStructured/allAlbums'].size, boxConfig.divChar),
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER },
        init: {
            title: mapInitial(album.charAt(0)),
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER },
        artist: {
            title: artist[0],
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ARTIST,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: containerRefID },
        album_artist: {
            title: album_artist,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: containerType,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: containerRefID },
        album: {
            title: album,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: containerType,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: containerRefID },
        genre: {
            title: genre,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER_MUSIC_GENRE,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: containerRefID },
        decade: {
            title: decade,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER },
        date: {
            title: date,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: UPNP_CLASS_CONTAINER },
    };

    chain.allArtists.metaData[M_CONTENT_CLASS] = [ UPNP_CLASS_AUDIO_ITEM ];
    chain.album.metaData[M_ARTIST] = [ album_artist ];
    chain.album.metaData[M_ALBUMARTIST] = [ album_artist ];
    chain.album.metaData[M_GENRE] = [ genre ];
    chain.album.metaData[M_DATE] = obj.metaData[M_DATE];
    chain.album.metaData[M_UPNP_DATE] = obj.metaData[M_UPNP_DATE];
    chain.album.metaData[M_ALBUM] = [ album ];
    chain.artist.metaData[M_ARTIST] = artist;
    chain.artist.metaData[M_ALBUMARTIST] = artist;
    chain.album_artist.metaData[M_ARTIST] = [ album_artist ];
    chain.album_artist.metaData[M_ALBUMARTIST] = [ album_artist ];
    var isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;

    obj.title = tracktitle;
    var container = addContainerTree(isSingleCharBox ? [chain.allAlbums, chain.abc, chain.album_artist] : [chain.allAlbums, chain.abc, chain.init, chain.album_artist]);
    addCdsObject(obj, container);

    container = addContainerTree([chain.allAlbums, chain.abc, chain.entryAllLevel2, chain.album_artist]);
    addCdsObject(obj, container);

    container = addContainerTree([chain.allAlbums, chain.entryAllLevel1, chain.album_artist]);
    addCdsObject(obj, container);

    // Artist
    obj.title = title + ' (' + album + ', ' + date + ')';
    const artCnt = artist.length;
    var i;
    for (i = 0; i < artCnt; i++) {
        chain.artist.title = artist[i];
        chain.artist.searchable = true;
        container = addContainerTree([chain.allArtists, chain.entryAllLevel1, chain.artist]);
        addCdsObject(obj, container);
    }
    chain.artist.searchable = false;

    for (i = 0; i < artCnt; i++) {
        chain.abc.title = abcbox(artist[i], boxSetup['AudioStructured/allArtists'].size, boxConfig.divChar);
        isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;
        container = addContainerTree([chain.allArtists, chain.abc, chain.entryAllLevel2, chain.artist]);
        addCdsObject(obj, container);
    }

    obj.title = title + ' (' + album + ', ' + date + ')';
    for (i = 0; i < artCnt; i++) {
        chain.init.title = mapInitial(artist[i].charAt(0));
        container = addContainerTree(isSingleCharBox ? [chain.allArtists, chain.abc, chain.artist, chain.entryAllLevel3] : [chain.allArtists, chain.abc, chain.init, chain.artist, chain.entryAllLevel3]);
        addCdsObject(obj, container);
    }

    obj.title = tracktitle;
    chain.album.title = album + ' (' + date + ')';
    chain.album.searchable = true;
    container = addContainerTree(isSingleCharBox ? [chain.allArtists, chain.abc, chain.artist, chain.album] : [chain.allArtists, chain.abc, chain.init, chain.artist, chain.album]);
    addCdsObject(obj, container);
    chain.album.searchable = false;

    // Genre
    if (boxSetup['AudioStructured/allGenres'].enabled) {
        obj.title = title + ' - ' + artist_full;
        chain.entryAllLevel2.upnpclass = UPNP_CLASS_CONTAINER_MUSIC_GENRE;
        if (obj.metaData[M_GENRE]) {
            for (var oneGenre in obj.metaData[M_GENRE]) {
                chain.genre.title = obj.metaData[M_GENRE][oneGenre];
                chain.genre.metaData[M_GENRE] = [ oneGenre ];
                container = addContainerTree([chain.allGenres, chain.genre, chain.entryAllLevel1]);
                addCdsObject(obj, container);
            }
        }

        for (i = 0; i < artCnt; i++) {
            chain.abc.title = abcbox(artist[i], boxSetup['AudioStructured/allGenres'].size, boxConfig.divChar);
            isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;
            chain.album_artist.searchable = true;
            if (obj.metaData[M_GENRE]) {
                for (var oneGenre in obj.metaData[M_GENRE]) {
                    chain.genre.title = obj.metaData[M_GENRE][oneGenre];
                    chain.genre.metaData[M_GENRE] = [ oneGenre ];
                    container = addContainerTree(isSingleCharBox ? [chain.allGenres, chain.genre, chain.abc, chain.album_artist] : [chain.allGenres, chain.genre, chain.abc, chain.init, chain.album_artist]);
                    addCdsObject(obj, container);
                }
            }
        }
    }
    chain.album_artist.searchable = false;

    // Tracks
    if (boxSetup['AudioStructured/allTracks'].enabled) {
        obj.title = title + ' - ' + artist.join(' / ') + ' (' + album + ', ' + date + ')';
        chain.abc.title = abcbox(title, boxSetup['AudioStructured/allTracks'].size, boxConfig.divChar);
        isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;
        chain.init.title = mapInitial(title.charAt(0));
        chain.init.upnpclass = UPNP_CLASS_CONTAINER_MUSIC_ARTIST;
        container = addContainerTree(isSingleCharBox ? [chain.allTracks, chain.abc] : [chain.allTracks, chain.abc, chain.init]);
        addCdsObject(obj, container);

        obj.title = title + ' - ' + artist_full;
        container = addContainerTree([chain.allTracks, chain.entryAllLevel1]);
        addCdsObject(obj, container);
    }

    // Sort years into decades
    if (boxSetup['AudioStructured/allYears'].enabled) {
        obj.title = title + ' - ' + artist_full;

        container = addContainerTree([chain.allYears, chain.decade, chain.entryAllLevel3]);
        addCdsObject(obj, container);

        container = addContainerTree([chain.allYears, chain.decade, chain.date, chain.entryAllLevel3]);
        addCdsObject(obj, container);

        obj.title = tracktitle;
        chain.album.title = album;
        chain.album.searchable = true;
        container = addContainerTree([chain.allYears, chain.decade, chain.date, chain.artist, chain.album]);
        addCdsObject(obj, container);
    }
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
            id: boxSetup['Video/videoRoot'].id,
            title: boxSetup['Video/videoRoot'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Video/videoRoot'].class, metaData: [] },
        allVideo: {
            id: boxSetup['Video/allVideo'].id,
            title: boxSetup['Video/allVideo'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Video/allVideo'].class, metaData: [] },
        allDirectories: {
            id: boxSetup['Video/allDirectories'].id,
            title: boxSetup['Video/allDirectories'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Video/allDirectories'].class, metaData: [] },
        allYears: {
            id: boxSetup['Video/allYears'].id,
            title: boxSetup['Video/allYears'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Video/allYears'].class, metaData: [] },
        allDates: {
            id: boxSetup['Video/allDates'].id,
            title: boxSetup['Video/allDates'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Video/allDates'].class, metaData: [] },

        year: {
            title: boxSetup['Video/unknown'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            searchable: true,
            upnpclass: UPNP_CLASS_CONTAINER },
        month: {
            title: boxSetup['Video/unknown'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            searchable: true,
            upnpclass: UPNP_CLASS_CONTAINER,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: containerRefID },
        date: {
            title: boxSetup['Video/unknown'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            searchable: true,
            upnpclass: UPNP_CLASS_CONTAINER,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: containerRefID },
    };
    chain.video.metaData[M_CONTENT_CLASS] = [ UPNP_CLASS_VIDEO_ITEM ];
    var container = addContainerTree([chain.video, chain.allVideo]);
    addCdsObject(obj, container);

    // Year
    if (boxSetup['Video/allYears'].enabled && obj.metaData[M_CREATION_DATE] && obj.metaData[M_CREATION_DATE][0]) {
        var date = obj.metaData[M_CREATION_DATE][0];
        var dateParts = date.split('-');
        if (dateParts.length > 1) {
            chain.year.title = dateParts[0];
            chain.month.title = dateParts[1];
            addCdsObject(obj, addContainerTree([chain.video, chain.allYears, chain.year, chain.month]));
        }
    }

    // Dates
    if (boxSetup['Video/allDates'].enabled && obj.metaData[M_CREATION_DATE] && obj.metaData[M_CREATION_DATE][0]) {
        var date = obj.metaData[M_CREATION_DATE][0];
        var dateParts = date.split('T');
        if (dateParts.length > 1) {
            date = dateParts[0];
        }
        chain.date.title = date;
        addCdsObject(obj, addContainerTree([chain.video, chain.allDates, chain.date]));
    }

    // Directories
    if (boxSetup['Video/allDirectories'].enabled && dir.length > 0) {
        var tree = [chain.video, chain.allDirectories];
        for (var i = 0; i < dir.length; i++) {
            tree = tree.concat({ title: dir[i], objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER });
        }
        tree[tree.length-1].upnpclass = containerType;
        tree[tree.length-1].metaData = [];
        tree[tree.length-1].res = containerResource;
        tree[tree.length-1].aux = obj.aux;
        tree[tree.length-1].refID = containerRefID;
        addCdsObject(obj, addContainerTree(tree));
    }
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
            id: boxSetup['Image/imageRoot'].id,
            title: boxSetup['Image/imageRoot'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Image/imageRoot'].class, metaData: [] },
        allImages: {
            id: boxSetup['Image/allImages'].id,
            title: boxSetup['Image/allImages'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Image/allImages'].class, metaData: [] },
        allDirectories: {
            id: boxSetup['Image/allDirectories'].id,
            title: boxSetup['Image/allDirectories'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Image/allDirectories'].class, metaData: [] },
        allYears: {
            id: boxSetup['Image/allYears'].id,
            title: boxSetup['Image/allYears'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Image/allYears'].class, metaData: [] },
        allDates: {
            id: boxSetup['Image/allDates'].id,
            title: boxSetup['Image/allDates'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Image/allDates'].class, metaData: [] },

        year: {
            title: boxSetup['Image/unknown'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            searchable: true,
            upnpclass: UPNP_CLASS_CONTAINER },
        month: {
            title: boxSetup['Image/unknown'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            searchable: true,
            upnpclass: UPNP_CLASS_CONTAINER,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            efID: containerRefID },
        date: {
            title: boxSetup['Image/unknown'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            searchable: true,
            upnpclass: UPNP_CLASS_CONTAINER,
            metaData: [],
            res: containerResource,
            aux: obj.aux,
            refID: containerRefID },
    };
    chain.imageRoot.metaData[M_CONTENT_CLASS] = [ UPNP_CLASS_IMAGE_ITEM ];
    addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allImages]));

    // Years
    if (boxSetup['Image/allYears'].enabled && obj.metaData[M_DATE] && obj.metaData[M_DATE][0]) {
        var date = obj.metaData[M_DATE][0];
        var dateParts = date.split('-');
        if (dateParts.length > 1) {
            chain.year.title = dateParts[0];
            chain.month.title = dateParts[1];
            addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allYears, chain.year, chain.month]));
        }
    }

    // Dates
    if (boxSetup['Image/allDates'].enabled && obj.metaData[M_DATE] && obj.metaData[M_DATE][0]) {
        var date = obj.metaData[M_DATE][0];
        var dateParts = date.split('T');
        if (dateParts.length > 1) {
            date = dateParts[0];
        }
        chain.date.title = date;
        addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allDates, chain.date]));
    }

    // Directories
    if (boxSetup['Image/allDirectories'].enabled && dir.length > 0) {
        var tree = [chain.imageRoot, chain.allDirectories];
        for (var i = 0; i < dir.length; i++) {
            tree = tree.concat([{ title: dir[i], objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER }]);
        }
        tree[tree.length-1].upnpclass = containerType;
        tree[tree.length-1].metaData = [];
        tree[tree.length-1].res = containerResource;
        tree[tree.length-1].aux = obj.aux;
        tree[tree.length-1].refID = containerRefID;
        addCdsObject(obj, addContainerTree(tree));
    }
}
// doc-add-image-end

// doc-add-trailer-begin
function addTrailer(obj) {
    const boxSetup = config['/import/scripting/virtual-layout/boxlayout/box'];
    const chain = {
        trailerRoot: {
            id: boxSetup['Trailer/trailerRoot'].id,
            title: boxSetup['Trailer/trailerRoot'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Trailer/trailerRoot'].class, metaData: [] },
        appleTrailers: {
            id: boxSetup['Trailer/appleTrailers'].id,
            title: boxSetup['Trailer/appleTrailers'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Trailer/appleTrailers'].class, metaData: [] },
        allTrailers: {
            id: boxSetup['Trailer/allTrailers'].id,
            title: boxSetup['Trailer/allTrailers'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Trailer/allTrailers'].class, metaData: [] },
        allGenres: {
            id: boxSetup['Trailer/allGenres'].id,
            title: boxSetup['Trailer/allGenres'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Trailer/allGenres'].class, metaData: [] },
        relDate: {
            id: boxSetup['Trailer/relDate'].id,
            title: boxSetup['Trailer/relDate'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Trailer/relDate'].class, metaData: [] },
        postDate: {
            id: boxSetup['Trailer/postDate'].id,
            title: boxSetup['Trailer/postDate'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            upnpclass: boxSetup['Trailer/postDate'].class, metaData: [] },

        genre: {
            title: boxSetup['Trailer/unknown'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            searchable: true,
            upnpclass: UPNP_CLASS_CONTAINER_MUSIC_GENRE,
            metaData: [] },
        date: {
            title: boxSetup['Trailer/unknown'].title,
            objectType: OBJECT_TYPE_CONTAINER,
            searchable: true,
            upnpclass: UPNP_CLASS_CONTAINER,
            metaData: [] }
    };
    // First we will add the item to the 'All Trailers' container, so
    // that we get a nice long playlist:

    addCdsObject(obj, addContainerTree([chain.trailerRoot , chain.appleTrailers, chain.allTrailers]));

    // We also want to sort the trailers by genre, however we need to
    // take some extra care here: the genre property here is a comma
    // separated value list, so one trailer can have several matching
    // genres that will be returned as one string. We will split that
    // string and create individual genre containers.

    if (boxSetup['Trailer/allGenres'].enabled && obj.metaData[M_GENRE] && obj.metaData[M_GENRE][0]) {
        var genre = obj.metaData[M_GENRE][0];

        // A genre string "Science Fiction, Thriller" will be split to
        // "Science Fiction" and "Thriller" respectively.

        const genres = genre.split(', ');
        for (var i = 0; i < genres.length; i++) {
            chain.genre.title = genres[i];
            chain.genre.metaData[M_GENRE] = [ genres[i] ];
            addCdsObject(obj, addContainerTree([chain.trailerRoot , chain.appleTrailers, chain.allGenres, chain.genre]));
        }
    }

    // The release date is offered in a YYYY-MM-DD format, we won't do
    // too much extra checking regading validity, however we only want
    // to group the trailers by year and month:

    if (boxSetup['Trailer/relDate'].enabled && obj.metaData[M_DATE] && obj.metaData[M_DATE][0]) {
        var reldate = obj.metaData[M_DATE][0];
        if (reldate.length >= 7) {
            chain.date.title = reldate.slice(0, 7);
            chain.date.metaData[M_DATE] = [ reldate.slice(0, 7) ];
            chain.date.metaData[M_UPNP_DATE] = [ reldate.slice(0, 7) ];
            addCdsObject(obj, addContainerTree([chain.trailerRoot , chain.appleTrailers, chain.relDate, chain.date]));
        }
    }

    // We also want to group the trailers by the date when they were
    // originally posted, the post date is available via the aux
    // array. Similar to the release date, we will cut off the day and
    // create our containres in the YYYY-MM format.
    if (boxSetup['Trailer/postDate'].enabled) {
        var postdate = obj.aux[APPLE_TRAILERS_AUXDATA_POST_DATE];
        if (postdate && postdate.length >= 7) {
            chain.date.title = postdate.slice(0, 7);
            chain.date.metaData[M_DATE] = [ postdate.slice(0, 7) ];
            chain.date.metaData[M_UPNP_DATE] = [ postdate.slice(0, 7) ];
            addCdsObject(obj, addContainerTree([chain.trailerRoot , chain.appleTrailers, chain.postDate, chain.date]));
        }
    }
}
// doc-add-trailer-end

// doc-add-playlist-item-begin
function addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder) {
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
        addCdsObject(exturl, playlistChain);
    } else {
        if (entry.location.substr(0,1) !== '/') {
            entry.location = playlistLocation + entry.location;
        }

        var cds = getCdsObject(entry.location);
        if (!cds) {
            print2("Warning", "Playlist '" + playlist_title + "' Skipping unknown entry: " + entry.location);
            return false;
        }

        var item = copyObject(cds);

        item.playlistOrder = (entry.order ? entry.order : playlistOrder);
        item.title = entry.writeThrough <= 0 ? (item.metaData[M_TITLE] ? item.metaData[M_TITLE][0] : cds.title) : entry.title;
        item.metaData[M_CONTENT_CLASS] = [ UPNP_CLASS_PLAYLIST_ITEM ];
        item.description = entry.description ? entry.description : null;

        item.extra = entry.extra;
        item.writeThrough = entry.writeThrough;
        print2("Debug", "Playlist '" + item.title + "' Adding entry: " + item.playlistOrder + " " + item.location);

        addCdsObject(item, playlistChain);
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
            var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder);

            // Also add to "Directories"
            if (playlistDirChain && state)
                state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder);

            entry.title = null;
            entry.mimetype = null;
            if (state)
                playlistOrder++;
        }

        line = readln();
    } while (line);
}
// doc-playlist-m3u-parse-end

function addMeta(obj, key, value) {
    if (obj.metaData[key])
        obj.metaData[key].push(value);
    else
        obj.metaData[key] = [ value ];
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
    
    while (node || level > 0) {
        if (!node && level > 0) {
            node = readXml(-1); // read parent
            node = readXml(0); // read next
            level--;
            if (entry.location) {
                if (base)
                    entry.location = base + '/' + entry.location;
                var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder);
                if (playlistDirChain && state) {
                    entry.writeThrough = 0;
                    state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder);
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
        var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder);
        if (playlistDirChain && state) {
            entry.writeThrough = 0;
            addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder);
        }
    }
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
                    var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder);
                    if (playlistDirChain && state)
                        state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder);
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
                    var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder);
                    if (playlistDirChain && state)
                        state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder);
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
                    var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder);
                    if (playlistDirChain && state)
                        state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder);
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
        var state = addPlaylistItem(playlist_title, playlistLocation, entry, playlistChain, playlistOrder);
        if (playlistDirChain && state)
            addPlaylistItem(playlist_title, playlistLocation, entry, playlistDirChain, playlistOrder);
    }
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
