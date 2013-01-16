// Default MediaTomb dvd import script.
// see MediaTomb scripting documentation for more information

/*MT_F*
    
    MediaTomb - http://www.mediatomb.cc/
    
    import-dvd.js - this file is part of MediaTomb.
    
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

var title = dvd.title;
var index = title.lastIndexOf('.');
if (index > 1)
    title = title.substring(0, index);

// we got the ISO here but our virtual items should have the video class
dvd.upnpclass = UPNP_CLASS_ITEM_VIDEO;

var title_count = dvd.aux[DVD].titles.length;
for (var t = 0; t < title_count; t++)
{
    var title_name = 'Title';

    if (t < 9)
        title_name = title_name + ' 0' + (t + 1);
    else
        title_name = title_name + ' ' + (t + 1);

    var chapter_count = dvd.aux[DVD].titles[t].chapters.length;
    var audio_track_count = dvd.aux[DVD].titles[t].audio_tracks.length;
    for (var a = 0; a < audio_track_count; a++)
    {
        var chain;
        var audio_name = ' - Audio Track ' + (a + 1);
        var audio_language = dvd.aux[DVD].titles[t].audio_tracks[a].language;
        var audio_format = dvd.aux[DVD].titles[t].audio_tracks[a].format;
        if (audio_format != '')
        {
            if (audio_language != '')
                audio_name = audio_name + ' - ' + audio_language;

            chain = new Array('Video', 'DVD', title, 'Audio Formats', 
                              audio_format, title_name + audio_name);

            for (var c = 0; c < chapter_count; c++) 
            {
                if (c < 9)
                    dvd.title = "Chapter 0" + (c + 1);
                else
                    dvd.title = "Chapter " + (c + 1);

                addDVDObject(dvd, t, c, a, createContainerChain(chain));
            } 
        }

        if (audio_language != '')
        {
            chain = new Array('Video', 'DVD', title, 'Languages', 
                              audio_language);
            if (audio_format != '')
                chain.push(title_name + audio_name + ' - ' + audio_format);
            else 
                chain.push(title_name + audio_name);

            for (var c = 0; c < chapter_count; c++)
            {
                if (c < 9)
                    dvd.title = "Chapter 0" + (c + 1);
                else
                    dvd.title = "Chapter " + (c + 1);

                addDVDObject(dvd, t, c, a, createContainerChain(chain));
            } 
        }

        chain = new Array('Video', 'DVD', title, 'Titles');

        var titles = title_name + ' - Audio Track ' + (a + 1);

        if (audio_format != '')
            titles = titles + ' - ' + audio_format;

        if (audio_language != '')
            titles = titles + ' - ' + audio_language;

        chain.push(titles);
        
        for (var c = 0; c < chapter_count; c++)
        {
            if (c < 9)
                dvd.title = "Chapter 0" + (c + 1);
            else
                dvd.title = "Chapter " + (c + 1);

            addDVDObject(dvd, t, c, a, createContainerChain(chain));
        } 
    }
}
