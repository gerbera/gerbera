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

#include "fallback_layout.h" // API

#include <utility>

#include "config/config_manager.h"
#include "content_manager.h"
#include "metadata/metadata_handler.h"
#include "util/string_converter.h"

#ifdef ONLINE_SERVICES

#include "onlineservice/online_service.h"

#ifdef SOPCAST
#include "onlineservice/sopcast_content_handler.h"
#endif

#ifdef ATRAILERS
#include "onlineservice/atrailers_content_handler.h"
#endif

#endif //ONLINE_SERVICES

void FallbackLayout::add(const std::shared_ptr<CdsObject>& obj, int parentID, bool use_ref)
{
    obj->setParentID(parentID);
    if (use_ref)
        obj->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    obj->setID(INVALID_OBJECT_ID);

    content->addObject(obj);
}

std::string FallbackLayout::esc(std::string str)
{
    return escape(std::move(str), VIRTUAL_CONTAINER_ESCAPE, VIRTUAL_CONTAINER_SEPARATOR);
}

void FallbackLayout::addVideo(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath)
{
    auto f2i = StringConverter::f2i(config);
    int id = content->addContainerChain("/Video/All Video");

    if (obj->getID() != INVALID_OBJECT_ID) {
        obj->setRefID(obj->getID());
        add(obj, id);
    } else {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    std::string dir;
    if (!rootpath.empty()) {
        // make location relative to rootpath: "/home/.../Videos/Action/a.mkv" with rootpath "/home/.../Videos" -> "Action"
        dir = fs::relative(obj->getLocation().parent_path(), config->getBoolOption(CFG_IMPORT_LAYOUT_PARENT_PATH) ? rootpath.parent_path() : rootpath);
        dir = f2i->convert(dir);
    } else
        dir = esc(f2i->convert(getLastPath(obj->getLocation())));

    if (!dir.empty()) {
        id = content->addContainerChain("/Video/Directories/" + dir);
        add(obj, id);
    }
}

void FallbackLayout::addImage(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath)
{
    int id;
    auto f2i = StringConverter::f2i(config);

    id = content->addContainerChain("/Photos/All Photos");
    if (obj->getID() != INVALID_OBJECT_ID) {
        obj->setRefID(obj->getID());
        add(obj, id);
    } else {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    auto meta = obj->getMetadata();

    std::string date = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_DATE));
    if (!date.empty()) {
        std::string year, month;
        size_t m = -1;
        size_t y = date.find('-');
        if (y != std::string::npos) {
            year = date.substr(0, y);
            month = date.substr(y + 1);
            m = month.find('-');
            if (m != std::string::npos)
                month = month.substr(0, m);
        }

        std::string chain;
        if ((y > 0) && (m > 0)) {
            chain = "/Photos/Year/" + esc(year) + "/" + esc(month);
            id = content->addContainerChain(chain);
            add(obj, id);
        }

        chain = "/Photos/Date/" + esc(date);
        id = content->addContainerChain(chain);
        add(obj, id);
    }

    std::string dir;
    if (!rootpath.empty()) {
        // make location relative to rootpath: "/home/.../Photos/Action/a.mkv" with rootpath "/home/.../Photos" -> "Action"
        dir = fs::relative(obj->getLocation().parent_path(), config->getBoolOption(CFG_IMPORT_LAYOUT_PARENT_PATH) ? rootpath.parent_path() : rootpath);
        dir = f2i->convert(dir);
    } else
        dir = esc(f2i->convert(getLastPath(obj->getLocation())));

    if (!dir.empty()) {
        id = content->addContainerChain("/Photos/Directories/" + dir);
        add(obj, id);
    }
}

void FallbackLayout::addAudio(const std::shared_ptr<CdsObject>& obj)
{
    std::string desc;

    auto meta = obj->getMetadata();

    std::string title = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_TITLE), obj->getTitle());

    std::string artist = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_ARTIST));
    std::string artist_full;
    if (!artist.empty()) {
        artist_full = artist;
        desc = artist;
    } else
        artist = "Unknown";

    std::string album = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_ALBUM));
    std::string album_full;
    if (!album.empty()) {
        desc = desc + ", " + album;
        album_full = album;
    } else {
        album = "Unknown";
    }

    if (!desc.empty())
        desc = desc + ", ";
    desc = desc + title;

    std::string date = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_DATE));
    std::string albumDate;
    if (!date.empty()) {
        size_t i = date.find('-');
        if (i != std::string::npos)
            date = date.substr(0, i);

        desc = desc + ", " + date;
        albumDate = esc(date);
    } else {
        date = "Unknown";
        albumDate = "Unknown";
    }
    meta[MetadataHandler::getMetaFieldName(M_UPNP_DATE)] = albumDate;

    std::string genre = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_GENRE));
    if (!genre.empty())
        desc = desc + ", " + genre;
    else
        genre = "Unknown";

    std::string description = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_DESCRIPTION));
    if (description.empty()) {
        meta[MetadataHandler::getMetaFieldName(M_DESCRIPTION)] = desc;
        obj->setMetadata(meta);
    }

    std::string composer = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_COMPOSER), "None");
    std::string conductor = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_CONDUCTOR), "None");
    std::string orchestra = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_ORCHESTRA), "None");

    int id = content->addContainerChain("/Audio/All Audio");
    obj->setTitle(title);

    // we get the main object here, so the object that we will add below
    // will be a reference of the main object, that's why we set the ref
    // id to the object id - the add function will clear out the object
    // id
    if (obj->getID() != INVALID_OBJECT_ID) {
        obj->setRefID(obj->getID());
        add(obj, id);
    }
    // the object is not yet in the database (probably we got it from a
    // playlist script, so we set the ref id after adding - it will be used
    // for all consequent virtual objects
    else {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    artist = esc(artist);

    std::string chain = "/Audio/Artists/" + artist + "/All Songs";

    id = content->addContainerChain(chain);
    add(obj, id);

    std::string temp;
    if (!artist_full.empty())
        temp = artist_full;

    if (!album_full.empty())
        temp = temp + " - " + album_full + " - ";
    else
        temp = temp + " - ";

    album = esc(album);
    chain = "/Audio/Artists/" + artist + "/" + album;
    id = content->addContainerChain(chain, UPNP_CLASS_MUSIC_ALBUM, obj->getID(), obj->getMetadata());
    add(obj, id);

    chain = "/Audio/Albums/" + album;
    id = content->addContainerChain(chain, UPNP_CLASS_MUSIC_ALBUM, obj->getID(), obj->getMetadata());
    add(obj, id);

    chain = "/Audio/Genres/" + esc(genre);
    id = content->addContainerChain(chain, UPNP_CLASS_MUSIC_GENRE);
    add(obj, id);

    chain = "/Audio/Composers/" + esc(composer);
    id = content->addContainerChain(chain, UPNP_CLASS_MUSIC_COMPOSER);
    add(obj, id);

    chain = "/Audio/Year/" + esc(date);
    id = content->addContainerChain(chain);
    add(obj, id);

    obj->setTitle(temp + title);

    id = content->addContainerChain("/Audio/All - full name");
    add(obj, id);

    chain = "/Audio/Artists/" + artist + "/All - full name";
    id = content->addContainerChain(chain);
    add(obj, id);
}

#ifdef SOPCAST
void FallbackLayout::addSopCast(const std::shared_ptr<CdsObject>& obj)
{
#define SP_VPATH "/Online Services/SopCast"
    std::string chain;
    std::string temp;
    int id;
    bool ref_set = false;

    if (obj->getID() != INVALID_OBJECT_ID) {
        obj->setRefID(obj->getID());
        ref_set = true;
    }

    chain = SP_VPATH "/"
                     "All Channels";
    id = content->addContainerChain(chain);
    add(obj, id, ref_set);
    if (!ref_set) {
        obj->setRefID(obj->getID());
        ref_set = true;
    }

    temp = obj->getAuxData(SOPCAST_AUXDATA_GROUP);
    if (!temp.empty()) {
        chain = SP_VPATH "/"
                         "Groups"
                         "/"
            + esc(temp);
        id = content->addContainerChain(chain);
        add(obj, id, ref_set);
    }
}
#endif

#ifdef ATRAILERS
void FallbackLayout::addATrailers(const std::shared_ptr<CdsObject>& obj)
{
#define AT_VPATH "/Online Services/Apple Trailers"
    int id = content->addContainerChain(AT_VPATH "/All Trailers");

    if (obj->getID() != INVALID_OBJECT_ID) {
        obj->setRefID(obj->getID());
        add(obj, id);
    } else {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    auto meta = obj->getMetadata();

    std::string temp = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_GENRE));
    auto genreAr = splitString(temp, ',');
    for (auto& genre : genreAr) {
        genre = trimString(genre);
        if (genre.empty())
            continue;

        id = content->addContainerChain(AT_VPATH "/Genres/" + esc(genre));
        add(obj, id);
    }

    temp = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_DATE));
    if (temp.length() >= 7) {
        id = content->addContainerChain(AT_VPATH "/Release Date/" + esc(temp.substr(0, 7)));
        add(obj, id);
    }

    temp = obj->getAuxData(ATRAILERS_AUXDATA_POST_DATE);
    if (temp.length() >= 7) {
        id = content->addContainerChain(AT_VPATH "/Post Date/" + esc(temp.substr(0, 7)));
        add(obj, id);
    }
}
#endif

FallbackLayout::FallbackLayout(std::shared_ptr<Config> config,
    std::shared_ptr<Database> database,
    std::shared_ptr<ContentManager> content)
    : config(std::move(config))
    , database(std::move(database))
    , content(std::move(content))
{
#ifdef ENABLE_PROFILING
    PROF_INIT_GLOBAL(layout_profiling, "fallback layout");
#endif
}

void FallbackLayout::processCdsObject(std::shared_ptr<CdsObject> obj, fs::path rootpath)
{
    log_debug("Process CDS Object: {}", obj->getTitle().c_str());
#ifdef ENABLE_PROFILING
    PROF_START(&layout_profiling);
#endif
    auto clone = CdsObject::createObject(obj->getObjectType());
    obj->copyTo(clone);
    clone->setVirtual(true);

#ifdef ONLINE_SERVICES
    if (clone->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
        auto service = static_cast<service_type_t>(std::stoi(clone->getAuxData(ONLINE_SERVICE_AUX_ID)));

        switch (service) {
#ifdef SOPCAST
        case OS_SopCast:
            addSopCast(clone);
            break;
#endif
#ifdef ATRAILERS
        case OS_ATrailers:
            addATrailers(clone);
            break;
#endif
        case OS_Max:
        default:
            log_warning("No handler for service type");
            break;
        }
    } else {
#endif

        std::string mimetype = std::static_pointer_cast<CdsItem>(obj)->getMimeType();
        auto mappings = config->getDictionaryOption(
            CFG_IMPORT_MAPPINGS_MIMETYPE_TO_CONTENTTYPE_LIST);
        std::string content_type = getValueOrDefault(mappings, mimetype);

        if (startswith(mimetype, "video"))
            addVideo(clone, rootpath);
        else if (startswith(mimetype, "image"))
            addImage(clone, rootpath);
        else if ((startswith(mimetype, "audio") && (content_type != CONTENT_TYPE_PLAYLIST)))
            addAudio(clone);
        else if (content_type == CONTENT_TYPE_OGG) {
            if (obj->getFlag(OBJECT_FLAG_OGG_THEORA))
                addVideo(clone, rootpath);
            else
                addAudio(clone);
        }

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
