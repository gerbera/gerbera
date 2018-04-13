// Extended MediaTomb import script.
// see MediaTomb scripting documentation for more information

/*MT_F*
    
    MediaTomb - http://www.mediatomb.cc/
    
    import.js - this file is part of MediaTomb.
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    This file is free software; the copyright owners give unlimited permission
    to copy and/or redistribute it; with or without modifications, as long as
    this notice is preserved.
    
    This file is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    
    $Id$
*/

function addAudio(obj)
{
 
    var desc = '';
    var artist_full;
    var album_full;

    var decade = null;
    
    // first gather data
    var title = obj.meta[M_TITLE];
    if (!title) title = obj.title;
    
    var artist = obj.meta[M_ARTIST];
    if (!artist) 
    {
        artist = 'Unknown';
        artist_full = null;
    }
    else
    {
        artist_full = artist;
        desc = artist;
    }
    
    var album = obj.meta[M_ALBUM];
    if (!album) 
    {
        album = 'Unknown';
        album_full = null;
    }
    else
    {
        desc = desc + ', ' + album;
        album_full = album;
    }
    
    if (desc)
        desc = desc + ', ';
    
    desc = desc + title;
    
    var date = obj.meta[M_DATE];
    if (!date)
    {
        date = '-Unknown-';
	decade = '-Unknown-';
    }
    else
    {
        date = getYear(date);
        desc = desc + ', ' + date;
	decade = date.substring(0,3) + '0 - ' + String(10 * (parseInt(date.substring(0,3))) + 9) ;
    }
    
    var genre = obj.meta[M_GENRE];
    if (!genre)
    {
        genre = 'Unknown';
    }
    else
    {
        desc = desc + ', ' + genre;
    }
    
    var description = obj.meta[M_DESCRIPTION];
    if (!description) 
    {
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
    if (!description)
    {
	album_artist = album + ' - ' + artist;
	tracktitle = track + title;
    }
    else
    {
	if (description.toUpperCase() == 'VARIOUS')
	{
	    album_artist = album + ' - Various';
	    tracktitle = track + title + ' - ' + artist;
	}
	else
	{
	    album_artist = album + ' - ' + artist;
	    tracktitle = track + title;
	}
    }
      
    chain = new Array('-Album-', abcbox(album, 6, '-'), album.charAt(0).toUpperCase(), album_artist);
    obj.title = tracktitle;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);
    chain = new Array('-Album-', abcbox(album, 6, '-'), '-all-', album_artist);
    obj.title = tracktitle;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);
    chain = new Array('-Album-', '--all--', album_artist);
    obj.title = tracktitle;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);
    
    // // Artist

    chain = new Array('-Artist-',  '--all--', artist);
    obj.title = title + ' (' + album + ', ' + date + ')'; 
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ARTIST);
    chain = new Array('-Artist-', abcbox(artist, 9, '-'), '-all-', artist);
    obj.title = title + ' (' + album + ', ' + date + ')'; 
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ARTIST);
    chain = new Array('-Artist-', abcbox(artist, 9, '-'), artist.charAt(0).toUpperCase(), artist, '-all-');
    obj.title = title + ' (' + album + ', ' + date + ')'; 
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ARTIST);
    
    obj.title = tracktitle;    
    chain = new Array('-Artist-', abcbox(artist, 9, '-'), artist.charAt(0).toUpperCase(), artist, album + ' (' + date + ')');
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);

    
    // Genre

    chain = new Array('-Genre-', genre, '--all--');
    obj.title = title + ' - ' + artist_full;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_GENRE);
    
    chain = new Array('-Genre-', genre, abcbox(artist, 6, '-'), artist.charAt(0).toUpperCase(), album_artist);
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);


    // Tracks

    chain = new Array('-Track-', abcbox(title, 6, '-'), title.charAt(0).toUpperCase());
    obj.title = title + ' - ' + artist + ' (' + album + ', ' + date + ')';
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ARTIST);
      
    chain = new Array('-Track-', '--all--');
    obj.title = title + ' - ' + artist_full;
    addCdsObject(obj, createContainerChain(chain));

    
    // Sort years into decades
    
    chain = new Array('-Year-', decade, '-all-');
    addCdsObject(obj, createContainerChain(chain));
    
    chain = new Array('-Year-', decade, date, '-all-');
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ARTIST);

    chain = new Array('-Year-', decade, date, artist, album);
    obj.title = tracktitle;
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER_MUSIC_ALBUM);
}

function addVideo(obj)
{
    var chain = new Array('Video', 'All Video');
    addCdsObject(obj, createContainerChain(chain));

    var dir = getRootPath(object_script_path, obj.location);

    if (dir.length > 0)
    {
        chain = new Array('Video', 'Directories');
        chain = chain.concat(dir);

        addCdsObject(obj, createContainerChain(chain));
    }
}

function addImage(obj)
{
    var chain = new Array('Photos', 'All Photos');
    addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER);

    var date = obj.meta[M_DATE];
    if (date)
    {
        var dateParts = date.split('-');
        if (dateParts.length > 1)
        {
            var year = dateParts[0];
            var month = dateParts[1];

            chain = new Array('Photos', 'Year', year, month);
            addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER);
        }

        chain = new Array('Photos', 'Date', date);
        addCdsObject(obj, createContainerChain(chain), UPNP_CLASS_CONTAINER);
    }

    var dir = getRootPath(object_script_path, obj.location);

    if (dir.length > 0)
    {
        chain = new Array('Photos', 'Directories');
        chain = chain.concat(dir);

        addCdsObject(obj, createContainerChain(chain));
    }
}


function addYouTube(obj)
{
    var chain;

    var temp = parseInt(obj.aux[YOUTUBE_AUXDATA_AVG_RATING], 10);
    if (temp != Number.NaN)
    {
        temp = Math.round(temp);
        if (temp > 3)
        {
            chain = new Array('Online Services', 'YouTube', 'Rating', 
                                  temp.toString());
            addCdsObject(obj, createContainerChain(chain));
        }
    }

    temp = obj.aux[YOUTUBE_AUXDATA_REQUEST];
    if (temp)
    {
        var subName = (obj.aux[YOUTUBE_AUXDATA_SUBREQUEST_NAME]);
        var feedName = (obj.aux[YOUTUBE_AUXDATA_FEED]);
        var region = (obj.aux[YOUTUBE_AUXDATA_REGION]);

            
        chain = new Array('Online Services', 'YouTube', temp);

        if (subName)
            chain.push(subName);

        if (feedName)
            chain.push(feedName);

        if (region)
            chain.push(region);

        addCdsObject(obj, createContainerChain(chain));
    }
}

function addTrailer(obj)
{
    var chain;

    chain = new Array('Online Services', 'Apple Trailers', 'All Trailers');
    addCdsObject(obj, createContainerChain(chain));

    var genre = obj.meta[M_GENRE];
    if (genre)
    {
        genres = genre.split(', ');
        for (var i = 0; i < genres.length; i++)
        {
            chain = new Array('Online Services', 'Apple Trailers', 'Genres',
                              genres[i]);
            addCdsObject(obj, createContainerChain(chain));
        }
    }

    var reldate = obj.meta[M_DATE];
    if ((reldate) && (reldate.length >= 7))
    {
        chain = new Array('Online Services', 'Apple Trailers', 'Release Date',
                          reldate.slice(0, 7));
        addCdsObject(obj, createContainerChain(chain));
    }

    var postdate = obj.aux[APPLE_TRAILERS_AUXDATA_POST_DATE];
    if ((postdate) && (postdate.length >= 7))
    {
        chain = new Array('Online Services', 'Apple Trailers', 'Post Date',
                          postdate.slice(0, 7));
        addCdsObject(obj, createContainerChain(chain));
    }
}

// main script part

if (getPlaylistType(orig.mimetype) == '')
{
    var arr = orig.mimetype.split('/');
    var mime = arr[0];
    
    // var obj = copyObject(orig);
    
    var obj = orig; 
    obj.refID = orig.id;
    
    if (mime == 'audio')
    {
            addAudio(obj);
    }
    
    if (mime == 'video')
    {
        if (obj.onlineservice == ONLINE_SERVICE_YOUTUBE)
            addYouTube(obj);
        else if (obj.onlineservice == ONLINE_SERVICE_APPLE_TRAILERS)
            addTrailer(obj);
        else
            addVideo(obj);
    }
    
    if (mime == 'image')
    {
        addImage(obj);
    }

    if (orig.mimetype == 'application/ogg')
    {
        if (orig.theora == 1)
            addVideo(obj);
        else
            addAudio(obj);
    }
}
