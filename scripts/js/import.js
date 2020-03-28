/*GRB*
  Gerbera - https://gerbera.io/

  import.js - this file is part of Gerbera.

  Copyright (C) 2018 Gerbera Contributors

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

// main script part

if (getPlaylistType(orig.mimetype) === '') {
    var arr = orig.mimetype.split('/');
    var mime = arr[0];
    
    // All virtual objects are references to objects in the
    // PC-Directory, so make sure to correctly set the reference ID!
    
    var obj = orig; 
    obj.refID = orig.id;
    
    if (mime === 'audio') {
        addAudio(obj);
    }
    
    if (mime === 'video') {
        if (obj.onlineservice === ONLINE_SERVICE_APPLE_TRAILERS) {
            addTrailer(obj);
        } else {
            addVideo(obj);
        }
    }
    
    if (mime === 'image') {
        addImage(obj);
    }

    // We now also have OGG Theora recognition, so we can ensure that
    // Vorbis
    if (orig.mimetype === 'application/ogg') {
        if (orig.theora === 1) {
            addVideo(obj);
        } else {
            addAudio(obj);
        }
    }
}
