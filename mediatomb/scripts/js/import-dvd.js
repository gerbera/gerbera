print("Processing object: " + dvd.title + " at " + dvd.location);

var title = dvd.title;
var index = title.lastIndexOf('.');

// we got the ISO here but our virtual items should have the video class
dvd.upnpclass = UPNP_CLASS_ITEM_VIDEO;

if (index > 1)
    title = title.substring(0, index);

var title_count = dvd.aux[DVD].titles.length;

for (var t = 0; t < title_count; t++)
{
    var title_name = 'Title';

    if (t < 9)
        title_name = title_name + ' 0' + (t + 1);
    else
        title_name = title_name + ' ' + (t + 1);

    var audio_track_count = dvd.aux[DVD].titles[t].audio_tracks.length

    var chapter_count = dvd.aux[DVD].titles[t].chapters.length

    for (var a = 0; a < audio_track_count; a++)
    {
        var chain;

        var audio_name = ' - Audio Track ' + (a + 1);

        var audio_language = dvd.aux[DVD].titles[t].audio_tracks[a].language
       
        var audio_format = dvd.aux[DVD].titles[t].audio_tracks[a].format;
        if (audio_format != '')
        {
            if (audio_language != '')
                audio_name = audio_name + ' - ' + audio_language;

            chain = new Array('Video', 'DVD', 'Audio Formats', audio_format,
                    title_name + audio_name);

            print("Chain used: " + createContainerChain(chain));
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
            chain = new Array('Video', 'DVD', 'Languages', audio_language);
            if (audio_format != '')
                chain.push(title_name + audio_name + ' - ' + audio_format);
            else 
                chain.push(title_name + audio_name);

            print("Chain used: " + createContainerChain(chain));
            for (var c = 0; c < chapter_count; c++)
            {
                if (c < 9)
                    dvd.title = "Chapter 0" + (c + 1);
                else
                    dvd.title = "Chapter " + (c + 1);

                addDVDObject(dvd, t, c, a, createContainerChain(chain));
            } 
        }

        chain = new Array('Video', 'DVD', 'Titles');

        var titles = title_name + ' - Audio Track ' + (a + 1);

        if (audio_format != '')
            titles = titles + ' - ' + audio_format;

        if (audio_language != '')
            titles = titles + ' - ' + audio_language;

        chain.push(titles);
        
        print("Chain used: " + createContainerChain(chain));
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

