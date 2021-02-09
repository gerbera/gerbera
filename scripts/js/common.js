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
//
// JS code:
// chain = new Array('-Artist-',  '--all--', artist);
// obj.title = title + ' (' + album + ', ' + date + ')';
// addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ARTIST);
// chain = new Array('-Artist-', abcbox(artist, 9, '-'), '-all-', artist);
// obj.title = title + ' (' + album + ', ' + date + ')';
// addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ARTIST);
// chain = new Array('-Artist-', abcbox(artist, 9, '-'), artist.charAt(0).toUpperCase(), artist, '-all-');
// obj.title = title + ' (' + album + ', ' + date + ')';
// addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ARTIST);

function abcbox(stringtobox, boxtype, divchar)
{
    // get ascii value of first character
    var intchar = stringtobox.toUpperCase().charCodeAt(0);

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

    // definition of box types, adjust to your own needs
    // as a start: the number is the same as the number of boxes, evenly spaced ... more or less.
    switch (boxtype)
    {
    case 1:
        var boxwidth = new Array();
        boxwidth[0] = 26;                             // one large box of 26 letters
        break;
    case 2:
        var boxwidth = new Array(13,13);              // two boxes of 13 letters
        break;
    case 3:
        var boxwidth = new Array(8,9,9);              // and so on ...
        break;
    case 4:
        var boxwidth = new Array(7,6,7,6);
        break;
    case 5:
        var boxwidth = new Array(5,5,5,6,5);
        break;
    case 6:
        var boxwidth = new Array(4,5,4,4,5,4);
        break;
    case 7:
        var boxwidth = new Array(4,3,4,4,4,3,4);
        break;
    case 9:
        var boxwidth = new Array(5,5,5,4,1,6);        // When T is a large box...
        break;
    default:
        var boxwidth = new Array(5,5,5,6,5);
        break;
    }

    // check for a total of 26 characters for all boxes
    charttl = 0;
    for (cb=0;cb<boxwidth.length;cb++) { charttl = charttl + boxwidth[cb]; }
    if (charttl != 26) {
        print("Error in box-definition, length is " + charttl + ". Check the file common.js" );
        // maybe an exit call here to stop processing the media ??
        end;
    }

    // declaration of some variables
    boxnum=0;                         // boxnumber start
    sc=65;                            // first ascii character (corresponds to 'A')
    ec=sc + boxwidth[boxnum] - 1;     // last character of first box

    // loop that will define first and last character of the right box
    while (intchar>ec)
    {
        boxnum++;                         // next boxnumber
        sc = ec + 1;                      // next startchar
        ec = sc + boxwidth[boxnum] - 1;   // next endchar
    }

    // construction of output string
    output = divchar;
    for (i=sc;i<=ec;i++) {
        output = output + String.fromCharCode(i);
    }
    output = output + divchar;
    return output;
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

    var conductor = obj.meta[M_CONDUCTOR];
    if (!conductor) {
        conductor = 'None';
    }

    var orchestra = obj.meta[M_ORCHESTRA];
    if (!orchestra) {
        orchestra = 'None';
    }

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
    // nice layout for our audio files. Note how we are constructing
    // the chain, in the line below the array 'chain' will be
    // converted to 'Audio/All audio' by the createContainerChain()
    // function.

    var chain = ['Audio', 'All Audio'];
    obj.title = title;

    // The UPnP class argument to addCdsObject() is optional, if it is
    // not supplied the default UPnP class will be used. However, it
    // is suggested to correctly set UPnP classes of containers and
    // objects - this information may be used by some renderers to
    // identify the type of the container and present the content in a
    // different manner .

    addCdsObject(obj, createContainerChain(chain));

    chain = ['Audio', 'Artists', artist, 'All Songs'];
    addCdsObject(obj, createContainerChain(chain));

    chain = ['Audio', 'All - full name'];
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
    addCdsObject(obj, createContainerChain(chain));

    chain = ['Audio', 'Artists', artist, 'All - full name'];
    addCdsObject(obj, createContainerChain(chain));

    chain = ['Audio', 'Artists', artist, album];
    obj.title = track + title;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);

    chain = ['Audio', 'Albums', album];
    obj.title = track + title;

    // Remember, the server will sort all items by ID3 track if the
    // container class is set to UPNP_CLASS_CONTAINER_MUSIC_ALBUM.

    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);

    chain = ['Audio', 'Genres', genre];
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_GENRE);

    chain = ['Audio', 'Year', date];
    addCdsObject(obj, createContainerChain(chain));
    chain = ['Audio', 'Composers', composer];
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_COMPOSER);
}
// doc-add-audio-end

function addAudioStructured(obj) {

    var desc = '';
    var artist_full;
    var album_full;

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
    if (!description) {
        album_artist = album + ' - ' + artist;
        tracktitle = track + title;
    } else {
        if (description.toUpperCase() === 'VARIOUS') {
            album_artist = album + ' - Various';
            tracktitle = track + title + ' - ' + artist;
        } else {
            album_artist = album + ' - ' + artist;
            tracktitle = track + title;
        }
    }

    chain = ['-Album-', abcbox(album, 6, '-'), album.charAt(0).toUpperCase(), album_artist];
    obj.title = tracktitle;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);

    chain = ['-Album-', abcbox(album, 6, '-'), '-all-', album_artist];
    obj.title = tracktitle;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);

    chain = ['-Album-', '--all--', album_artist];
    obj.title = tracktitle;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);

    // Artist

    chain = ['-Artist-',  '--all--', artist];
    obj.title = title + ' (' + album + ', ' + date + ')';
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ARTIST);

    chain = ['-Artist-', abcbox(artist, 9, '-'), '-all-', artist];
    obj.title = title + ' (' + album + ', ' + date + ')';
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ARTIST);
    chain = ['-Artist-', abcbox(artist, 9, '-'), artist.charAt(0).toUpperCase(), artist, '-all-'];
    obj.title = title + ' (' + album + ', ' + date + ')';
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ARTIST);

    obj.title = tracktitle;
    chain = ['-Artist-', abcbox(artist, 9, '-'), artist.charAt(0).toUpperCase(), artist, album + ' (' + date + ')'];
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);


    // Genre

    chain = ['-Genre-', genre, '--all--'];
    obj.title = title + ' - ' + artist_full;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_GENRE);

    chain = ['-Genre-', genre, abcbox(artist, 6, '-'), artist.charAt(0).toUpperCase(), album_artist];
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);


    // Tracks

    chain = ['-Track-', abcbox(title, 6, '-'), title.charAt(0).toUpperCase()];
    obj.title = title + ' - ' + artist + ' (' + album + ', ' + date + ')';
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ARTIST);

    chain = ['-Track-', '--all--'];
    obj.title = title + ' - ' + artist_full;
    addCdsObject(obj, createContainerChain(chain));


    // Sort years into decades

    chain = ['-Year-', decade, '-all-'];
    addCdsObject(obj, createContainerChain(chain));

    chain = ['-Year-', decade, date, '-all-'];
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ARTIST);

    chain = ['-Year-', decade, date, artist, album];
    obj.title = tracktitle;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);
}

// doc-add-video-begin
function addVideo(obj) {
    var chain = ['Video', 'All Video'];
    addCdsObject(obj, createContainerChain(chain));

    var dir = getRootPath(object_script_path, obj.location);

    if (dir.length > 0) {
        chain = ['Video', 'Directories'];
        chain = chain.concat(dir);
        addCdsObject(obj, createContainerChain(chain));
    }
}
// doc-add-video-end

// doc-add-image-begin
function addImage(obj) {
    var chain = ['Photos', 'All Photos'];
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER);

    var date = obj.meta[M_DATE];
    if (date) {
        var dateParts = date.split('-');
        if (dateParts.length > 1) {
            var year = dateParts[0];
            var month = dateParts[1];

            chain = ['Photos', 'Year', year, month];
            addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER);
        }

        chain = ['Photos', 'Date', date];
        addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER);
    }

    var dir = getRootPath(object_script_path, obj.location);

    if (dir.length > 0) {
        chain = ['Photos', 'Directories'];
        chain = chain.concat(dir);
        addCdsObject(obj, createContainerChain(chain));
    }
}
// doc-add-image-end

// doc-add-trailer-begin
function addTrailer(obj) {
    var chain;

    // First we will add the item to the 'All Trailers' container, so
    // that we get a nice long playlist:

    chain = ['Online Services', 'Apple Trailers', 'All Trailers'];
    addCdsObject(obj, createContainerChain(chain));

    // We also want to sort the trailers by genre, however we need to
    // take some extra care here: the genre property here is a comma
    // separated value list, so one trailer can have several matching
    // genres that will be returned as one string. We will split that
    // string and create individual genre containers.

    var genre = obj.meta[M_GENRE];
    if (genre) {

        // A genre string "Science Fiction, Thriller" will be split to
        // "Science Fiction" and "Thriller" respectively.

        genres = genre.split(', ');
        for (var i = 0; i < genres.length; i++) {
            chain = ['Online Services', 'Apple Trailers', 'Genres', genres[i]];
            addCdsObject(obj, createContainerChain(chain));
        }
    }

    // The release date is offered in a YYYY-MM-DD format, we won't do
    // too much extra checking regading validity, however we only want
    // to group the trailers by year and month:

    var reldate = obj.meta[M_DATE];
    if ((reldate) && (reldate.length >= 7)) {
        chain = ['Online Services', 'Apple Trailers', 'Release Date', reldate.slice(0, 7)];
        addCdsObject(obj, createContainerChain(chain));
    }

    // We also want to group the trailers by the date when they were
    // originally posted, the post date is available via the aux
    // array. Similar to the release date, we will cut off the day and
    // create our containres in the YYYY-MM format.

    var postdate = obj.aux[APPLE_TRAILERS_AUXDATA_POST_DATE];
    if ((postdate) && (postdate.length >= 7)) {
        chain = ['Online Services', 'Apple Trailers', 'Post Date', postdate.slice(0, 7)];
        addCdsObject(obj, createContainerChain(chain));
    }
}
// doc-add-trailer-end
