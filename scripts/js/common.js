/*GRB*
  Gerbera - https://gerbera.io/

  common.js - this file is part of Gerbera.

  Copyright (C) 2018-2021 Gerbera Contributors

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

function escapeSlash(name)
{
    name = name.replace(/\\/g, "\\\\");
    name = name.replace(/\//g, "\\/");
    return name;
}

function createContainerChain(arr)
{
    var path = '';
    for (var i = 0; i < arr.length; i++)
    {
        path = path + '/' + escapeSlash(arr[i]);
    }
    return path;
}

function getYear(date)
{
    var matches = date.match(/^([0-9]{4})-/);
    if (matches)
        return matches[1];
    else
        return date;
}

function getPlaylistType(mimetype)
{
    if (mimetype == 'audio/x-mpegurl')
        return 'm3u';
    if (mimetype == 'audio/x-scpls')
        return 'pls';
    return '';
}

function getLastPath(location)
{
    var path = location.split('/');
    if ((path.length > 1) && (path[path.length - 2]))
        return path[path.length - 2];
    else
        return '';
}

function getRootPath(rootpath, location)
{
    var path = new Array();

    if (rootpath && rootpath.length !== 0)
    {
        rootpath = rootpath.substring(0, rootpath.lastIndexOf('/'));

        var dir = location.substring(rootpath.length,location.lastIndexOf('/'));

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
function abcbox(stringtobox, boxtype, divchar)
{
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
        print("Error in box-definition, length is " + charttl + ". Check the file common.js" );
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
function addAudio(obj) {
    var desc = '';
    var artist_full;
    var album_full;

    // First we will gather all the metadata that is provided by our
    // object, of course it is possible that some fields are empty -
    // we will have to check that to make sure that we handle this
    // case correctly.
    var title = obj.meta[M_TITLE];

    // Note the difference between obj.title and obj.meta[M_TITLE] -
    // while object.title will originally be set to the file name,
    // obj.meta[M_TITLE] will contain the parsed title - in this
    // particular example the ID3 title of an MP3.
    if (!title) {
        title = obj.title;
    }

    var artist = obj.meta[M_ARTIST];
    if (!artist) {
        artist = 'Unknown';
        artist_full = null;
    } else {
        artist_full = artist;
        desc = artist;
    }

    var album = obj.meta[M_ALBUM];
    if (!album) {
        album = 'Unknown';
        album_full = null;
    } else {
        desc = desc + ', ' + album;
        album_full = album;
    }

    if (desc) {
        desc = desc + ', ';
    }
    desc = desc + title;

    var date = obj.meta[M_DATE];
    if (!date) {
        date = 'Unknown';
    } else {
        date = getYear(date);
        obj.meta[M_UPNP_DATE] = date;
        desc = desc + ', ' + date;
    }

    var genre = obj.meta[M_GENRE];
    if (!genre) {
        genre = 'Unknown';
    } else {
        genre = mapGenre(genre);
        desc = desc + ', ' + genre;
    }

    var description = obj.meta[M_DESCRIPTION];
    if (!description) {
        obj.description = desc;
    }

    var composer = obj.meta[M_COMPOSER];
    if (!composer) {
        composer = 'None';
    }

/*
    var conductor = obj.meta[M_CONDUCTOR];
    if (!conductor) {
        conductor = 'None';
    }

    var orchestra = obj.meta[M_ORCHESTRA];
    if (!orchestra) {
        orchestra = 'None';
    }
*/
    // uncomment this if you want to have track numbers in front of the title
    // in album view
/*
    var track = obj.meta[M_TRACKNUMBER];
    if (!track) {
        track = '';
    } else {
        if (track.length == 1) {
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

    const chain = {
        audio: { title: 'Audio', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allAudio: { title: 'All Audio', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allArtists: { title: 'Artists', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allGenres: { title: 'Genres', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allAlbums: { title: 'Albums', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allYears: { title: 'Year', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allComposers: { title: 'Composers', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allSongs: { title: 'All Songs', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allFull: { title: 'All - full name', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        artist: { title: artist, objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ARTIST, meta: {}, res: obj.res, aux: obj.aux, refID: obj.id },
        album: { title: album, objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ALBUM, meta: {}, res: obj.res, aux: obj.aux, refID: obj.id },
        genre: { title: genre, objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER_MUSIC_GENRE, meta: {}, res: obj.res, aux: obj.aux, refID: obj.id },
        year: { title: date, objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER, meta: {}, res: obj.res, aux: obj.aux, refID: obj.id },
        composer: { title: composer, objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER_MUSIC_COMPOSER, meta: {}, res: obj.res, aux: obj.aux, refID: obj.id },
    };

    chain.album.meta[M_ARTIST] = artist;
    chain.album.meta[M_ALBUMARTIST] = artist;
    chain.album.meta[M_GENRE] = genre;
    chain.album.meta[M_DATE] = obj.meta[M_DATE];
    chain.album.meta[M_ALBUM] = album;
    chain.artist.meta[M_ARTIST] = artist;
    chain.artist.meta[M_ALBUMARTIST] = artist;
    chain.genre.meta[M_GENRE] = genre;
    chain.year.meta[M_DATE] = date;
    chain.composer.meta[M_COMPOSER] = composer;

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
    container = addContainerTree([chain.audio, chain.allFull]);
    addCdsObject(obj, container);

    container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.allFull]);
    addCdsObject(obj, container);

    obj.title = track + title;
    container = addContainerTree([chain.audio, chain.allArtists, chain.artist, chain.album]);
    addCdsObject(obj, container);

    container = addContainerTree([chain.audio, chain.allAlbums, chain.album]);
    obj.title = track + title;
    addCdsObject(obj, container);

    container = addContainerTree([chain.audio, chain.allGenres, chain.genre]);
    addCdsObject(obj, container);

    container = addContainerTree([chain.audio, chain.allYears, chain.year]);
    addCdsObject(obj, container);

    container = addContainerTree([chain.audio, chain.allComposers, chain.composer]);
    addCdsObject(obj, container);
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

function addAudioStructured(obj) {
    var desc = '';
    var artist_full;

    var decade = null;

    // first gather data
    var title = obj.meta[M_TITLE];
    if (!title) title = obj.title;

    var artist = obj.meta[M_ARTIST];
    if (!artist) {
        artist = 'Unknown';
        artist_full = null;
    } else {
        artist_full = artist;
        desc = artist;
    }

    var album = obj.meta[M_ALBUM];
    if (!album) {
        album = 'Unknown';
    } else {
        desc = desc + ', ' + album;
    }

    if (desc) {
        desc = desc + ', ';
    }

    desc = desc + title;

    var date = obj.meta[M_DATE];
    if (!date) {
        date = '-Unknown-';
        decade = '-Unknown-';
    }
    else {
        date = getYear(date);
        obj.meta[M_UPNP_DATE] = date;
        desc = desc + ', ' + date;
        decade = date.substring(0,3) + '0 - ' + String(10 * (parseInt(date.substring(0,3))) + 9) ;
    }

    var genre = obj.meta[M_GENRE];
    if (!genre) {
        genre = 'Unknown';
    } else {
        genre = mapGenre(genre);
        desc = desc + ', ' + genre;
    }

    var description = obj.meta[M_DESCRIPTION];
    if (!description) {
        obj.meta[M_DESCRIPTION] = desc;
    }

// uncomment this if you want to have track numbers in front of the title
// in album view

/*
    var track = obj.meta[M_TRACKNUMBER];
    if (!track)
        track = '';
    else
    {
        if (track.length == 1)
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
    var album_artist = album + ' - ' + artist;
    if (description) {
        if (description.toUpperCase() === 'VARIOUS') {
            album_artist = album + ' - Various';
            tracktitle = tracktitle + ' - ' + artist;
        }
    }

    const boxConfig = {
        album: intFromConfig('/import/scripting/virtual-layout/structured-layout/attribute::album-box', 6),
        artist: intFromConfig('/import/scripting/virtual-layout/structured-layout/attribute::artist-box', 9),
        genre: intFromConfig('/import/scripting/virtual-layout/structured-layout/attribute::genre-box', 6),
        track: intFromConfig('/import/scripting/virtual-layout/structured-layout/attribute::track-box', 6),
        divChar: stringFromConfig('/import/scripting/virtual-layout/structured-layout/attribute::div-char', '-'),
    };
    boxConfig.singleLetterBoxSize = 2 * boxConfig.divChar.length + 1;

    const chain = {
        allArtists: { title: '-Artist-', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allGenres: { title: '-Genre-', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allAlbums: { title: '-Album-', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allTracks: { title: '-Track-', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allYears: { title: '-Year-', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allFull: { title: 'All - full name', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        abc: { title: abcbox(album, boxConfig.album, boxConfig.divChar), objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        init: { title: mapInitial(album.charAt(0)), objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        artist: { title: artist, objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ARTIST, meta: {}, res: obj.res, aux: obj.aux, refID: obj.id },
        album_artist: { title: album_artist, objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ALBUM, meta: {}, res: obj.res, aux: obj.aux, refID: obj.id },
        album: { title: album, objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER_MUSIC_ALBUM, meta: {}, res: obj.res, aux: obj.aux, refID: obj.id },
        entryAll: { title: '-all-', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        genre: { title: genre, objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER_MUSIC_GENRE, meta: {}, res: obj.res, aux: obj.aux, refID: obj.id },
        decade: { title: decade, objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        date: { title: date, objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER }
    };

    chain.album.meta[M_ARTIST] = album_artist;
    chain.album.meta[M_ALBUMARTIST] = album_artist;
    chain.album.meta[M_GENRE] = genre;
    chain.album.meta[M_DATE] = obj.meta[M_DATE];
    chain.album.meta[M_ALBUM] = album;
    chain.artist.meta[M_ARTIST] = artist;
    chain.artist.meta[M_ALBUMARTIST] = artist;
    chain.album_artist.meta[M_ARTIST] = album_artist;
    chain.album_artist.meta[M_ALBUMARTIST] = album_artist;
    var isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;

    obj.title = tracktitle;
    var container = addContainerTree(isSingleCharBox ? [chain.allAlbums, chain.abc, chain.album_artist] : [chain.allAlbums, chain.abc, chain.init, chain.album_artist]);
    addCdsObject(obj, container);

    container = addContainerTree([chain.allAlbums, chain.abc, chain.entryAll, chain.album_artist]);
    addCdsObject(obj, container);

    chain.entryAll.title = '--all--';
    container = addContainerTree([chain.allAlbums, chain.entryAll, chain.album_artist]);
    addCdsObject(obj, container);

    // Artist
    obj.title = title + ' (' + album + ', ' + date + ')';
    container = addContainerTree([chain.allArtists, chain.entryAll, chain.artist]);
    addCdsObject(obj, container);

    chain.abc.title = abcbox(artist, boxConfig.artist, boxConfig.divChar);
    isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;
    chain.entryAll.title = '-all-';
    container = addContainerTree([chain.allArtists, chain.abc, chain.entryAll, chain.artist]);
    addCdsObject(obj, container);

    obj.title = title + ' (' + album + ', ' + date + ')';
    chain.init.title = mapInitial(artist.charAt(0));
    chain.entryAll.upnpclass = UPNP_CLASS_CONTAINER_MUSIC_ARTIST;
    container = addContainerTree(isSingleCharBox ? [chain.allArtists, chain.abc, chain.artist, chain.entryAll] : [chain.allArtists, chain.abc, chain.init, chain.artist, chain.entryAll]);
    addCdsObject(obj, container);

    obj.title = tracktitle;
    chain.album.title = album + ' (' + date + ')';
    container = addContainerTree(isSingleCharBox ? [chain.allArtists, chain.abc, chain.artist, chain.album] : [chain.allArtists, chain.abc, chain.init, chain.artist, chain.album]);
    addCdsObject(obj, container);

    // Genre

    obj.title = title + ' - ' + artist_full;
    chain.entryAll.title = '--all--';
    chain.entryAll.upnpclass = UPNP_CLASS_CONTAINER_MUSIC_GENRE;
    container = addContainerTree([chain.allGenres, chain.genre, chain.entryAll]);
    addCdsObject(obj, container);

    chain.abc.title = abcbox(artist, boxConfig.genre, boxConfig.divChar);
    isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;
    container = addContainerTree(isSingleCharBox ? [chain.allGenres, chain.genre, chain.abc, chain.album_artist] : [chain.allGenres, chain.genre, chain.abc, chain.init, chain.album_artist]);
    addCdsObject(obj, container);

    // Tracks

    obj.title = title + ' - ' + artist + ' (' + album + ', ' + date + ')';
    chain.abc.title = abcbox(title, boxConfig.track, boxConfig.divChar);
    isSingleCharBox = boxConfig.singleLetterBoxSize >= chain.abc.title.length;
    chain.init.title = mapInitial(title.charAt(0));
    chain.init.upnpclass = UPNP_CLASS_CONTAINER_MUSIC_ARTIST;
    container = addContainerTree(isSingleCharBox ? [chain.allTracks, chain.abc] : [chain.allTracks, chain.abc, chain.init]);
    addCdsObject(obj, container);

    obj.title = title + ' - ' + artist_full;
    chain.entryAll.upnpclass = UPNP_CLASS_CONTAINER;
    container = addContainerTree([chain.allTracks, chain.entryAll]);
    addCdsObject(obj, container);

    // Sort years into decades

    chain.entryAll.title = '-all-';
    container = addContainerTree([chain.allYears, chain.decade, chain.entryAll]);
    addCdsObject(obj, container);

    chain.entryAll.upnpclass = UPNP_CLASS_CONTAINER_MUSIC_ARTIST;
    container = addContainerTree([chain.allYears, chain.decade, chain.date, chain.entryAll]);
    addCdsObject(obj, container);

    obj.title = tracktitle;
    chain.album.title = album;
    container = addContainerTree([chain.allYears, chain.decade, chain.date, chain.artist, chain.album]);
    addCdsObject(obj, container);
}

// doc-add-video-begin
function addVideo(obj) {
    const dir = getRootPath(object_script_path, obj.location);
    const chain = {
        video: { title: 'Video', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allVideo: { title: 'All Video', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allDirectories: { title: 'Directories', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allYears: { title: 'Year', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allDates: { title: 'Date', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },

        year: { title: 'Unbekannt', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        month: { title: 'Unbekannt', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER, meta: {}, res: obj.res, aux: obj.aux, refID: obj.id },
        date: { title: 'Unbekannt', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER, meta: {}, res: obj.res, aux: obj.aux, refID: obj.id }
    };
    var container = addContainerTree([chain.video, chain.allVideo]);
    addCdsObject(obj, container);
    var date = obj.meta[M_CREATION_DATE];
    if (date) {
        var dateParts = date.split('-');
        if (dateParts.length > 1) {
            chain.year.title = dateParts[0];
            chain.month.title = dateParts[1];
            addCdsObject(obj, addContainerTree([chain.video, chain.allYears, chain.year, chain.month]));
        }

        chain.date.title = date;
        addCdsObject(obj, addContainerTree([chain.video, chain.allDates, chain.date]));
    }
    if (dir.length > 0) {
        var tree = [chain.video, chain.allDirectories];
        for (var i = 0; i < dir.length; i++) {
            tree = tree.concat({ title: dir[i], objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER });
        }
        tree[tree.length-1].meta = {};
        tree[tree.length-1].res = obj.res;
        tree[tree.length-1].aux = obj.aux;
        tree[tree.length-1].refID = obj.id;
        addCdsObject(obj, addContainerTree(tree));
    }
}
// doc-add-video-end

// doc-add-image-begin
function addImage(obj) {
    const dir = getRootPath(object_script_path, obj.location);

    const chain = {
        imageRoot: { title: 'Photos', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allImages: { title: 'All Photos', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },

        allDirectories: { title: 'Directories', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allYears: { title: 'Year', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allDates: { title: 'Date', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },

        year: { title: 'Unbekannt', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        month: { title: 'Unbekannt', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER, meta: {}, res: obj.res, aux: obj.aux, refID: obj.id },
        date: { title: 'Unbekannt', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER, meta: {}, res: obj.res, aux: obj.aux, refID: obj.id }
    };
    addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allImages]));
    var date = obj.meta[M_DATE];
    if (date) {
        var dateParts = date.split('-');
        if (dateParts.length > 1) {
            chain.year.title = dateParts[0];
            chain.month.title = dateParts[1];
            addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allYears, chain.year, chain.month]));
        }

        chain.date.title = date;
        addCdsObject(obj, addContainerTree([chain.imageRoot, chain.allDates, chain.date]));
    }
    if (dir.length > 0) {
        var tree = [chain.imageRoot, chain.allDirectories];
        for (var i = 0; i < dir.length; i++) {
            tree = tree.concat([{ title: dir[i], objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER }]);
        }
        tree[tree.length-1].meta = {};
        tree[tree.length-1].res = obj.res;
        tree[tree.length-1].aux = obj.aux;
        tree[tree.length-1].refID = obj.id;
        addCdsObject(obj, addContainerTree(tree));
    }
}
// doc-add-image-end

// doc-add-trailer-begin
function addTrailer(obj) {
    const chain = {
        trailerRoot: { title: 'Online Services', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        appleTrailers: { title: 'Apple Trailers', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        allTrailers: { title: 'All Trailers', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },

        allGenres: { title: 'Genres', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        genre: { title: 'Unknown', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER_MUSIC_GENRE, meta: {} },

        relDate: { title: 'Release Date', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER },
        postDate: { title: 'Post Date', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER, meta: {}},
        date: { title: 'Unbekannt', objectType: OBJECT_TYPE_CONTAINER, upnpclass: UPNP_CLASS_CONTAINER, meta: {}}
    };
    // First we will add the item to the 'All Trailers' container, so
    // that we get a nice long playlist:

    addCdsObject(obj, addContainerTree([chain.trailerRoot , chain.appleTrailers, chain.allTrailers]));

    // We also want to sort the trailers by genre, however we need to
    // take some extra care here: the genre property here is a comma
    // separated value list, so one trailer can have several matching
    // genres that will be returned as one string. We will split that
    // string and create individual genre containers.

    var genre = obj.meta[M_GENRE];
    if (genre) {

        // A genre string "Science Fiction, Thriller" will be split to
        // "Science Fiction" and "Thriller" respectively.

        const genres = genre.split(', ');
        for (var i = 0; i < genres.length; i++) {
            chain.genre.title = genres[i];
            chain.genre.meta[M_GENRE] = genres[i];
            addCdsObject(obj, addContainerTree([chain.trailerRoot , chain.appleTrailers, chain.allGenres, chain.genre]));
        }
    }

    // The release date is offered in a YYYY-MM-DD format, we won't do
    // too much extra checking regading validity, however we only want
    // to group the trailers by year and month:

    var reldate = obj.meta[M_DATE];
    if ((reldate) && (reldate.length >= 7)) {
        chain.date.title = reldate.slice(0, 7);
        chain.date.meta[M_DATE] = reldate.slice(0, 7);
        addCdsObject(obj, addContainerTree([chain.trailerRoot , chain.appleTrailers, chain.relDate, chain.date]));
    }

    // We also want to group the trailers by the date when they were
    // originally posted, the post date is available via the aux
    // array. Similar to the release date, we will cut off the day and
    // create our containres in the YYYY-MM format.

    var postdate = obj.aux[APPLE_TRAILERS_AUXDATA_POST_DATE];
    if ((postdate) && (postdate.length >= 7)) {
        chain.date.title = postdate.slice(0, 7);
        chain.date.meta[M_DATE] = postdate.slice(0, 7);
        addCdsObject(obj, addContainerTree([chain.trailerRoot , chain.appleTrailers, chain.postDate, chain.date]));
    }
}
// doc-add-trailer-end

// doc-add-playlist-item-begin
function addPlaylistItem(playlist_title, location, title, playlistChain, order, playlistOrder) {
    // Determine if the item that we got is an URL or a local file.

    if (location.match(/^.*:\/\//)) {
        var exturl = {};

        // Setting the mimetype is crucial and tricky... if you get it
        // wrong your renderer may show the item as unsupported and refuse
        // to play it. Unfortunately most playlist formats do not provide
        // any mimetype information.
        exturl.mimetype = 'audio/mpeg';

        // Make sure to correctly set the object type, then populate the
        // remaining fields.
        exturl.objectType = OBJECT_TYPE_ITEM_EXTERNAL_URL;

        exturl.location = location;
        exturl.title = (title ? title : location);
        exturl.protocol = 'http-get';
        exturl.upnpclass = UPNP_CLASS_ITEM_MUSIC_TRACK;
        exturl.description = "Song from " + playlist_title;

        // This is a special field which ensures that your playlist files
        // will be displayed in the correct order inside a playlist
        // container. It is similar to the id3 track number that is used
        // to sort the media in album containers.
        exturl.playlistOrder = (order ? order : playlistOrder++);

        // Your item will be added to the container named by the playlist
        // that you are currently parsing.
        addCdsObject(exturl, playlistChain);
    } else {
        if (location.substr(0,1) !== '/') {
            location = playlistLocation + location;
        }
        var cds = getCdsObject(location);
        if (!cds) {
            print("Skipping item: " + location);
            return playlistOrder;
        }

        var item = copyObject(cds);

        item.playlistOrder = (order ? order : playlistOrder++);
        item.title = item.meta[M_TITLE];

        addCdsObject(item, playlistChain);
    }
    return playlistOrder;
}
// doc-add-playlist-item-end

// doc-playlist-m3u-parse-begin
function readM3uPlaylist(playlist_title, playlistChain, playlistDirChain) {
    var title = null;
    var line = readln();
    var playlistOrder = 1;

    // Here is the do - while loop which will read the playlist line by line.
    do {
        var matches = line.match(/^#EXTINF:(-?\d+),\s?(\S.+)$/i);
        if (matches) {
            // duration = matches[1]; // currently unused
            title = matches[2];
        }
        else if (!line.match(/^(#|\s*$)/)) {
            // Call the helper function to add the item once you gathered the data:
            playlistOrder = addPlaylistItem(playlist_title, line, title, playlistChain, 0, playlistOrder);

            // Also add to "Directories"
            if (playlistDirChain)
                playlistOrder = addPlaylistItem(playlist_title, line, title, playlistDirChain, 0, playlistOrder);

            title = null;
        }
        
        line = readln();
    } while (line);
}
// doc-playlist-m3u-parse-end

function readPlsPlaylist(playlist_title, playlistChain, playlistDirChain) {
    var title = null;
    var file = null;
    var lastId = -1;
    var id = -1;
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
            var thisFile = matches[2];
            id = parseInt(matches[1], 10);
            if (lastId === -1)
                lastId = id;
            if (lastId !== id) {
                if (file) {
                    playlistOrder = addPlaylistItem(playlist_title, file, title, playlistChain, lastId, playlistOrder);
                    if (playlistDirChain)
                        playlistOrder = addPlaylistItem(playlist_title, file, title, playlistDirChain, lastId, playlistOrder);
                }

                title = null;
                lastId = id;
            }
            file = thisFile;
        } else if (line.match(/^Title\s*(\d+)\s*=\s*(\S.+)$/i)) {
            const matches = line.match(/^Title\s*(\d+)\s*=\s*(\S.+)$/i);
            var thisTitle = matches[2];
            id = parseInt(matches[1], 10);
            if (lastId === -1)
                lastId = id;
            if (lastId !== id) {
                if (file) {
                    playlistOrder = addPlaylistItem(playlist_title, file, title, playlistChain, lastId, playlistOrder);
                    if (playlistDirChain)
                        playlistOrder = addPlaylistItem(playlist_title, file, title, playlistDirChain, lastId, playlistOrder);
                }

                file = null;
                lastId = id;
            }
            title = thisTitle;
        } else if (line.match(/^Length\s*(\d+)\s*=\s*(\S.+)$/i)) {
            // currently unused
        }

        line = readln();
    } while (line);

    if (file) {
        playlistOrder = addPlaylistItem(playlist_title, file, title, playlistChain, lastId, playlistOrder);
        if (playlistDirChain)
            playlistOrder = addPlaylistItem(playlist_title, file, title, playlistDirChain, lastId, playlistOrder);
    }
}
