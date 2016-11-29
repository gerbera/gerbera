/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    fallback_layout.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/// \file fallback_layout.cc

#include "fallback_layout.h"
#include "content_manager.h"
#include "config_manager.h"
#include "metadata_handler.h"
#include "string_converter.h"
#include "tools.h"

#ifdef HAVE_LIBDVDNAV
    #include "metadata/dvd_handler.h"
#endif

#ifdef ONLINE_SERVICES

#include "online_service.h"

#ifdef YOUTUBE
    #include "youtube_content_handler.h"
    #include "youtube_service.h"
#endif

#ifdef WEBORAMA
    #include "weborama_content_handler.h"
#endif

#ifdef SOPCAST
    #include "sopcast_content_handler.h"
#endif

#ifdef ATRAILERS
    #include "atrailers_content_handler.h"
#endif

#endif//ONLINE_SERVICES

using namespace zmm;

void FallbackLayout::add(Ref<CdsObject> obj, int parentID, bool use_ref)
{
    obj->setParentID(parentID);
    if (use_ref)
        obj->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    obj->setID(INVALID_OBJECT_ID);

    ContentManager::getInstance()->addObject(obj);
}

zmm::String FallbackLayout::esc(zmm::String str)
{
    return escape(str, VIRTUAL_CONTAINER_ESCAPE, VIRTUAL_CONTAINER_SEPARATOR);
}

void FallbackLayout::addVideo(zmm::Ref<CdsObject> obj, String rootpath)
{
    Ref<StringConverter> f2i = StringConverter::f2i();
    int id = ContentManager::getInstance()->addContainerChain(_("/Video/All Video"));

    if (obj->getID() != INVALID_OBJECT_ID)
    {
        obj->setRefID(obj->getID());
        add(obj, id);
    }
    else
    {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    String dir;

    if (string_ok(rootpath))
    {
        rootpath = rootpath.substring(0, rootpath.rindex(DIR_SEPARATOR));

        dir = obj->getLocation().substring(rootpath.length(), obj->getLocation().rindex(DIR_SEPARATOR)-rootpath.length());

        if (dir.startsWith(_DIR_SEPARATOR))
            dir = dir.substring(1);

        dir = f2i->convert(dir);
    }
    else
        dir = esc(f2i->convert(get_last_path(obj->getLocation())));

    if (string_ok(dir))
    {
        id = ContentManager::getInstance()->addContainerChain(_("/Video/Directories/") + dir);
        add(obj, id);
    }
}

#ifdef HAVE_LIBDVDNAV

Ref<CdsObject> FallbackLayout::prepareChapter(Ref<CdsObject> obj, int title_idx,
                                              int chapter_idx)
{
    String chapter_name = _("Chapter ");
            
    obj->getResource(0)->addParameter(DVDHandler::renderKey(DVD_Chapter), 
            String::from(chapter_idx));

    if (chapter_idx < 9)
        chapter_name = chapter_name + _("0") + (chapter_idx + 1); // remap
    else
        chapter_name = chapter_name + (chapter_idx + 1);

    obj->setTitle(chapter_name);
//    String tmp = obj->getAuxData(DVDHandler::renderKey(DVD_ChapterRestDuration,
//                                 title_idx, chapter_idx));
//    if (string_ok(tmp))
//        obj->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_DURATION), tmp);

    return obj;
}

void FallbackLayout::addDVD(Ref<CdsObject> obj)
{
    #define DVD_VPATH "/Video/DVD/"

    int dot = obj->getTitle().rindex('.');
    int pcd_id = obj->getID();
    String dvd_name;

    if (dot > -1)
        dvd_name = obj->getTitle().substring(0, dot);
    else
        dvd_name = obj->getTitle();

    String dvd_container = _(DVD_VPATH) + esc(dvd_name);
    Ref<ContentManager> cm = ContentManager::getInstance();

    int id = cm->addContainerChain(dvd_container, nil, pcd_id);

    // we get the main object here, so the object that we will add below
    // will be a reference of the main object, that's why we set the ref
    // id to the object id - the add function will clear out the object
    // id
    if (obj->getID() != INVALID_OBJECT_ID)
    {
        obj->setRefID(obj->getID());
        add(obj, id);
    }
    // the object is not yet in the database (probably we got it from a
    // playlist script, so we set the ref id after adding - it will be used
    // for all consequent virtual objects
    else
    {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    dvd_container = dvd_container + _("/");

    int title_count = obj->getAuxData(DVDHandler::renderKey(DVD_TitleCount)).toInt();
    // set common item attributes 
    RefCast(obj, CdsItem)->setMimeType(mpeg_mimetype);
    obj->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_PROTOCOLINFO), renderProtocolInfo(RefCast(obj, CdsItem)->getMimeType()));
    obj->setClass(_(UPNP_DEFAULT_CLASS_VIDEO_ITEM));
    /// \todo this has to be changed once we add seeking
    obj->getResource(0)->removeAttribute(MetadataHandler::getResAttrName(R_SIZE));

    for (int t = 0; t < title_count; t++)
    {
        int audio_track_count = obj->getAuxData(DVDHandler::renderKey(DVD_AudioTrackCount, t)).toInt();
        obj->getResource(0)->addParameter(DVDHandler::renderKey(DVD_Title), 
                                          String::from(t));
        for (int a = 0; a < audio_track_count; a++)
        {
            // set common audio track resource attributes
            obj->getResource(0)->addParameter(DVDHandler::renderKey(DVD_AudioStreamID), obj->getAuxData(DVDHandler::renderKey(DVD_AudioTrackStreamID, t, 0, a)));

            String tmp = obj->getAuxData(DVDHandler::renderKey(DVD_AudioTrackChannels, t, 0, a));
            if (string_ok(tmp))
            {
                obj->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_NRAUDIOCHANNELS), tmp);
                log_debug("Setting Audio Channels, object %s -  num: %s\n",
                          obj->getLocation().c_str(), tmp.c_str());
            }

            tmp = obj->getAuxData(DVDHandler::renderKey(DVD_AudioTrackSampleFreq, t, 0, a));
            if (string_ok(tmp))
                obj->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_SAMPLEFREQUENCY), tmp);

            String title_container = _("Titles/");
            String title_name;

            if (t < 9) // 9 because of later remapping
                title_name = _("Title 0") + (t + 1); // remap
            else
                title_name = _("Title ") + (t + 1); // remap
            
            String format = obj->getAuxData(DVDHandler::renderKey(DVD_AudioTrackFormat, t, 0, a));
            String language = obj->getAuxData(DVDHandler::renderKey(DVD_AudioTrackLanguage, t, 0, a));

            title_container = title_container + title_name;

            title_container = title_container + _(" - Audio Track ") + (a + 1);
            if (string_ok(format))
                title_container = title_container + _(" - ") + esc(format);

            if (string_ok(language))
                title_container = title_container + _(" - ") + esc(language);

            title_container = dvd_container + title_container;
            id = cm->addContainerChain(title_container, nil, pcd_id);
            int chapter_count = obj->getAuxData(DVDHandler::renderKey(DVD_ChapterCount, t)).toInt();

            for (int c = 0; c < chapter_count; c++)
            {
                prepareChapter(obj, t, c);
                add(obj, id, false);
            }

            if (string_ok(language))
            {
                String language_container = _("Languages/");
                String chain = dvd_container + language_container +
                               esc(language) + _("/") + title_name;

                chain = chain + _(" - Audio Track ") + (a + 1); 

                if (string_ok(format))
                        chain = chain + _(" - ") + esc(format);

                id = cm->addContainerChain(chain, nil, pcd_id);
                
                for (int c = 0; c < chapter_count; c++)
                {
                    prepareChapter(obj, t, c);
                    add(obj, id, false);
                }
            }

            if (string_ok(format))
            {
                String format_container = _("Audio Formats/");
                String chain = dvd_container + format_container + esc(format) + 
                               _("/") + title_name;

                chain = chain + _(" - Audio Track ") + (a + 1); 
                
                if (string_ok(language))
                    chain = chain + _(" - ") + esc(language);

                id = cm->addContainerChain(chain, nil, pcd_id);
                for (int c = 0; c < chapter_count; c++)
                {
                    prepareChapter(obj, t, c);
                    add(obj, id, false);
                }
            } // format
        } // audio track count
    } // title count
}
#endif

void FallbackLayout::addImage(Ref<CdsObject> obj, String rootpath)
{
    int id;
    Ref<StringConverter> f2i = StringConverter::f2i();

    
    id = ContentManager::getInstance()->addContainerChain(_("/Photos/All Photos"));
    if (obj->getID() != INVALID_OBJECT_ID)
    {
        obj->setRefID(obj->getID());
        add(obj, id);
    }
    else
    {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    Ref<Dictionary> meta = obj->getMetadata();

    String date = meta->get(MetadataHandler::getMetaFieldName(M_DATE));
    if (string_ok(date))
    {
        String year, month;
        int m = -1;
        int y = date.index('-');
        if (y > 0)
        {
            year = date.substring(0, y);
            month = date.substring(y + 1);
            m = month.index('-');
            if (m > 0)
              month = month.substring(0, m);
        }

        String chain;
        if ((y > 0) && (m > 0))
        {
            chain = _("/Photos/Year/") + esc(year) + "/" + esc(month);
            id = ContentManager::getInstance()->addContainerChain(chain);
            add(obj, id);
        }

        chain = _("/Photos/Date/") + esc(date);
        id = ContentManager::getInstance()->addContainerChain(chain);
        add(obj, id);
    }

    String dir;

    if (string_ok(rootpath))
    {
        rootpath = rootpath.substring(0, rootpath.rindex(DIR_SEPARATOR));

        dir = obj->getLocation().substring(rootpath.length(), obj->getLocation().rindex(DIR_SEPARATOR)-rootpath.length());

        if (dir.startsWith(_DIR_SEPARATOR))
            dir = dir.substring(1);

        dir = f2i->convert(dir);
    }
    else
        dir = esc(f2i->convert(get_last_path(obj->getLocation())));

    if (string_ok(dir))
    {
        id = ContentManager::getInstance()->addContainerChain(_("/Photos/Directories/") + dir);
        add(obj, id);
    }
}

void FallbackLayout::addAudio(zmm::Ref<CdsObject> obj)
{
    String desc;
    String chain;
    String artist_full;
    String album_full;

    int id;

    Ref<Dictionary> meta = obj->getMetadata();

    String title = meta->get(MetadataHandler::getMetaFieldName(M_TITLE));
    if (!string_ok(title))
        title = obj->getTitle();

    String artist = meta->get(MetadataHandler::getMetaFieldName(M_ARTIST));
    if (string_ok(artist))
    {
        artist_full = artist;
        desc = artist;
    }
    else
        artist = _("Unknown");

    String album = meta->get(MetadataHandler::getMetaFieldName(M_ALBUM));
    if (string_ok(album))
    {
        desc = desc + _(", ") + album;
        album_full = album;
    }
    else
    {
        album = _("Unknown");
    }

    if (string_ok(desc))
        desc = desc + _(", ");

    desc = desc + title;

    String date = meta->get(MetadataHandler::getMetaFieldName(M_DATE));
    if (string_ok(date))
    {
        int i = date.index('-');
        if (i > 0)
            date = date.substring(0, i);

        desc = desc + _(", ") + date;
    }
    else
        date = _("Unknown");

    String genre = meta->get(MetadataHandler::getMetaFieldName(M_GENRE));
    if (string_ok(genre))
        desc = desc + ", " + genre;
    else
        genre = _("Unknown");


    String description = meta->get(MetadataHandler::getMetaFieldName(M_DESCRIPTION));
    if (!string_ok(description))
    {
        meta->put(MetadataHandler::getMetaFieldName(M_DESCRIPTION), desc);
        obj->setMetadata(meta);
    }

    id = ContentManager::getInstance()->addContainerChain(_("/Audio/All Audio"));
    obj->setTitle(title);

    // we get the main object here, so the object that we will add below
    // will be a reference of the main object, that's why we set the ref
    // id to the object id - the add function will clear out the object
    // id
    if (obj->getID() != INVALID_OBJECT_ID)
    {
        obj->setRefID(obj->getID());
        add(obj, id);
    }
    // the object is not yet in the database (probably we got it from a
    // playlist script, so we set the ref id after adding - it will be used
    // for all consequent virtual objects
    else
    {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    artist = esc(artist);

    chain = _("/Audio/Artists/") + artist + "/All Songs";

    id = ContentManager::getInstance()->addContainerChain(chain);
    add(obj, id);

    String temp;
    if (string_ok(artist_full))
        temp = artist_full;

    if (string_ok(album_full))
        temp = temp + " - " + album_full + " - ";
    else
        temp = temp + " - ";

    album = esc(album);
    chain = _("/Audio/Artists/") +  artist + _("/") + album;
    id = ContentManager::getInstance()->addContainerChain(chain, _(UPNP_DEFAULT_CLASS_MUSIC_ALBUM));
    add(obj, id);

    chain = _("/Audio/Albums/") + album;
    id = ContentManager::getInstance()->addContainerChain(chain, _(UPNP_DEFAULT_CLASS_MUSIC_ALBUM));
    add(obj, id);

    chain = _("/Audio/Genres/") + esc(genre);
    id = ContentManager::getInstance()->addContainerChain(chain, _(UPNP_DEFAULT_CLASS_MUSIC_GENRE));
    add(obj, id);

    chain = _("/Audio/Year/") + esc(date);
    id = ContentManager::getInstance()->addContainerChain(chain);
    add(obj, id);

    obj->setTitle(temp + title);

    id = ContentManager::getInstance()->addContainerChain(_("/Audio/All - full name"));
    add(obj, id);

    chain = _("/Audio/Artists/") + artist + "/All - full name";
    id = ContentManager::getInstance()->addContainerChain(chain);
    add(obj, id);


}
#ifdef YOUTUBE
void FallbackLayout::addYouTube(zmm::Ref<CdsObject> obj)
{
    #define YT_VPATH "/Online Services/YouTube"
    String chain;
    String temp;
    int id;
    bool ref_set = false;

    if (obj->getID() != INVALID_OBJECT_ID)
    {
        obj->setRefID(obj->getID());
        ref_set = true;
    }

    temp = obj->getAuxData(_(YOUTUBE_AUXDATA_AVG_RATING));
    if (string_ok(temp))
    {
        int rating = (int)temp.toDouble();
        if (rating > 3)
        {
            chain = _(YT_VPATH "/Rating/") + esc(String::from(rating));
            id = ContentManager::getInstance()->addContainerChain(chain);
            add(obj, id, ref_set);
            if (!ref_set)
            {
                obj->setRefID(obj->getID());
                ref_set = true;
            }
        }
    }

    temp = obj->getAuxData(_(YOUTUBE_AUXDATA_REQUEST));
    if (string_ok(temp))
    {
        yt_requests_t req = (yt_requests_t)temp.toInt();
        temp = YouTubeService::getRequestName(req);

        String subName = obj->getAuxData(_(YOUTUBE_AUXDATA_SUBREQUEST_NAME));
        String feedName = obj->getAuxData(_(YOUTUBE_AUXDATA_FEED));
        String region = obj->getAuxData(_(YOUTUBE_AUXDATA_REGION));

        chain = _(YT_VPATH) + '/' + esc(temp);

        if (string_ok(subName))
            chain = chain + '/' + esc(subName);

        if (string_ok(feedName))
            chain = chain + '/' + esc(feedName);

        if (string_ok(region))
        {   yt_regions_t reg = (yt_regions_t)region.toInt();
            if (reg != YT_region_none)
            {
                region = YouTubeService::getRegionName(reg);
                if (string_ok(region));
                    chain = chain + '/' + esc(region);
            }
        }

        id = ContentManager::getInstance()->addContainerChain(chain);
        add(obj, id, ref_set);
    }
}
#endif

#ifdef SOPCAST
void FallbackLayout::addSopCast(zmm::Ref<CdsObject> obj)
{
    #define SP_VPATH "/Online Services/SopCast"
    String chain;
    String temp;
    int id;
    bool ref_set = false;

    if (obj->getID() != INVALID_OBJECT_ID)
    {
        obj->setRefID(obj->getID());
        ref_set = true;
    }

    chain = _(SP_VPATH "/" "All Channels");
    id =  ContentManager::getInstance()->addContainerChain(chain);
    add(obj, id, ref_set);
    if (!ref_set)
    {
        obj->setRefID(obj->getID());
        ref_set = true;
    }

    temp = obj->getAuxData(_(SOPCAST_AUXDATA_GROUP));
    if (string_ok(temp))
    {
        chain = _(SP_VPATH "/" "Groups" "/") + esc(temp);
        id =  ContentManager::getInstance()->addContainerChain(chain);
        add(obj, id, ref_set);
    }
}
#endif

#ifdef WEBORAMA
void FallbackLayout::addWeborama(zmm::Ref<CdsObject> obj)
{
    #define WB_VPATH "/Online Services/Weborama"
    String chain;
    String temp;
    int id;
    bool ref_set = false;

    if (obj->getID() != INVALID_OBJECT_ID)
    {
        obj->setRefID(obj->getID());
        ref_set = true;
    }

    temp = obj->getAuxData(_(WEBORAMA_AUXDATA_REQUEST_NAME));
    if (!string_ok(temp))
    {
        log_warning("Skipping Weborama item %s: missing playlist name\n", 
                    obj->getTitle().c_str());
        return;
    }

    chain = _(WB_VPATH "/") + esc(temp);
    id = ContentManager::getInstance()->addContainerChain(chain, 
                                      _(UPNP_DEFAULT_CLASS_PLAYLIST_CONTAINER));
    add(obj, id, ref_set);
}
#endif

#ifdef ATRAILERS
void FallbackLayout::addATrailers(zmm::Ref<CdsObject> obj)
{
    #define AT_VPATH "/Online Services/Apple Trailers"
    String chain;
    String temp;

    int id = ContentManager::getInstance()->addContainerChain(_(AT_VPATH 
                                                              "/All Trailers"));

    if (obj->getID() != INVALID_OBJECT_ID)
    {
        obj->setRefID(obj->getID());
        add(obj, id);
    }
    else
    {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    Ref<Dictionary> meta = obj->getMetadata();

    temp = meta->get(MetadataHandler::getMetaFieldName(M_GENRE));
    if (string_ok(temp))
    {
        Ref<StringTokenizer> st(new StringTokenizer(temp));
        String genre;
        String next;
        do
        {
            if (!string_ok(genre))
                genre = st->nextToken(_(","));
            next = st->nextToken(_(","));

            genre = trim_string(genre);

            if (!string_ok(genre))
                break;

            id = ContentManager::getInstance()->addContainerChain(_(AT_VPATH
                        "/Genres/") + esc(genre));
            add(obj, id);

            if (string_ok(next))
                genre = next;
            else
                genre = nil;
                    
        } while (genre != nil);
    }

    temp = meta->get(MetadataHandler::getMetaFieldName(M_DATE));
    if (string_ok(temp) && temp.length() >= 7)
    {
        id = ContentManager::getInstance()->addContainerChain(_(AT_VPATH
                    "/Release Date/") + esc(temp.substring(0, 7)));
        add(obj, id);
    }

    temp = obj->getAuxData(_(ATRAILERS_AUXDATA_POST_DATE));
    if (string_ok(temp) && temp.length() >= 7)
    {
        id = ContentManager::getInstance()->addContainerChain(_(AT_VPATH
                    "/Post Date/") + esc(temp.substring(0, 7)));
        add(obj, id);
    }
}
#endif

FallbackLayout::FallbackLayout() : Layout()
{
#ifdef ENABLE_PROFILING
    PROF_INIT_GLOBAL(layout_profiling, "fallback layout");
#endif

#ifdef HAVE_LIBDVDNAV
    Ref<Dictionary> mappings = ConfigManager::getInstance()->getDictionaryOption(CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
    mpeg_mimetype = mappings->get(_(CONTENT_TYPE_MPEG));
    if (!string_ok(mpeg_mimetype))
        mpeg_mimetype = _("video/mpeg");
#endif
}

void FallbackLayout::processCdsObject(zmm::Ref<CdsObject> obj, String rootpath)
{
    log_debug("Process CDS Object: %s\n", obj->getTitle().c_str());
#ifdef ENABLE_PROFILING
    PROF_START(&layout_profiling);
#endif
    Ref<CdsObject> clone = CdsObject::createObject(obj->getObjectType());
    obj->copyTo(clone);
    clone->setVirtual(1);

#ifdef ONLINE_SERVICES
    if (clone->getFlag(OBJECT_FLAG_ONLINE_SERVICE))
    {
        service_type_t service = (service_type_t)(clone->getAuxData(_(ONLINE_SERVICE_AUX_ID)).toInt());

        switch (service)
        {
#ifdef YOUTUBE
            case OS_YouTube:
                addYouTube(clone);
                break;
#endif
#ifdef SOPCAST
            case OS_SopCast:
                addSopCast(clone);
                break;
#endif
#ifdef WEBORAMA
            case OS_Weborama:
                addWeborama(clone);
                break;
#endif
#ifdef ATRAILERS
            case OS_ATrailers:
                addATrailers(clone);
                break;
#endif
            case OS_Max:
            default:
                log_warning("No handler for service type\n");
                break;
        }
    }
    else
    {
#endif

        String mimetype = RefCast(obj, CdsItem)->getMimeType();
        Ref<Dictionary> mappings = 
            ConfigManager::getInstance()->getDictionaryOption(
                    CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        String content_type = mappings->get(mimetype);

        if (mimetype.startsWith(_("video")))
            addVideo(clone, rootpath);
        else if (mimetype.startsWith(_("image")))
            addImage(clone, rootpath);
        else if ((mimetype.startsWith(_("audio")) && 
                    (content_type != CONTENT_TYPE_PLAYLIST)))
            addAudio(clone);
        else if (content_type == CONTENT_TYPE_OGG)
        {
            if (obj->getFlag(OBJECT_FLAG_OGG_THEORA))
                addVideo(clone, rootpath);
            else
                addAudio(clone);
        }
#ifdef HAVE_LIBDVDNAV
        else if (content_type == CONTENT_TYPE_DVD)
            addDVD(clone);
#endif

#ifdef ONLINE_SERVICES
    }
#endif
#ifdef ENABLE_PROFILING
    PROF_END(&layout_profiling);
#endif
}

#ifdef ENABLE_PROFILING
FallbackLayout::~FallbackLayout()
{
    PROF_PRINT(&layout_profiling);
}
#endif
