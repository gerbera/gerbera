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

/// \file builtin_layout.cc

#include "builtin_layout.h" // API

#include <regex>

#include "config/config_manager.h"
#include "content/content_manager.h"
#include "metadata/metadata_handler.h"
#include "util/string_converter.h"

#ifdef ONLINE_SERVICES

#include "content/onlineservice/online_service.h"

#ifdef SOPCAST
#include "content/onlineservice/sopcast_content_handler.h"
#endif

#ifdef ATRAILERS
#include "content/onlineservice/atrailers_content_handler.h"
#endif

#endif //ONLINE_SERVICES

void BuiltinLayout::add(const std::shared_ptr<CdsObject>& obj, const std::pair<int, bool>& parentID, bool use_ref)
{
    obj->setParentID(parentID.first);
    if (use_ref)
        obj->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    obj->setID(INVALID_OBJECT_ID);

    content->addObject(obj, parentID.second);
}

std::string BuiltinLayout::esc(std::string str)
{
    return escape(std::move(str), VIRTUAL_CONTAINER_ESCAPE, VIRTUAL_CONTAINER_SEPARATOR);
}

void BuiltinLayout::addVideo(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath)
{
    auto f2i = StringConverter::f2i(config);
    auto id = content->addContainerChain("/Video/All Video");

    if (obj->getID() != INVALID_OBJECT_ID) {
        obj->setRefID(obj->getID());
        add(obj, id);
    } else {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    auto meta = obj->getMetadata();

    std::string date = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_CREATION_DATE));
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
            chain = fmt::format("/Video/Year/{}/{}", esc(year), esc(month));
            id = content->addContainerChain(chain);
            add(obj, id);
        }

        chain = fmt::format("/Video/Date/{}", esc(date));
        id = content->addContainerChain(chain);
        add(obj, id);
    }

    fs::path dir;
    if (!rootpath.empty()) {
        // make location relative to rootpath: "/home/.../Videos/Action/a.mkv" with rootpath "/home/.../Videos" -> "Action"
        dir = fs::relative(obj->getLocation().parent_path(), config->getBoolOption(CFG_IMPORT_LAYOUT_PARENT_PATH) ? rootpath.parent_path() : rootpath);
        dir = f2i->convert(dir);
    } else
        dir = esc(f2i->convert(getLastPath(obj->getLocation())));

    if (!dir.empty()) {
        id = content->addContainerChain(fmt::format("/Video/Directories/{}", dir.string().c_str()));
        add(obj, id);
    }
}

void BuiltinLayout::addImage(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath)
{
    auto f2i = StringConverter::f2i(config);

    auto id = content->addContainerChain("/Photos/All Photos");
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
            chain = fmt::format("/Photos/Year/{}/{}", esc(year), esc(month));
            id = content->addContainerChain(chain);
            add(obj, id);
        }

        chain = fmt::format("/Photos/Date/{}", esc(date));
        id = content->addContainerChain(chain);
        add(obj, id);
    }

    fs::path dir;
    if (!rootpath.empty()) {
        // make location relative to rootpath: "/home/.../Photos/Action/a.mkv" with rootpath "/home/.../Photos" -> "Action"
        dir = fs::relative(obj->getLocation().parent_path(), config->getBoolOption(CFG_IMPORT_LAYOUT_PARENT_PATH) ? rootpath.parent_path() : rootpath);
        dir = f2i->convert(dir);
    } else
        dir = esc(f2i->convert(getLastPath(obj->getLocation())));

    if (!dir.empty()) {
        id = content->addContainerChain(fmt::format("/Photos/Directories/{}", dir.string().c_str()));
        add(obj, id);
    }
}

void BuiltinLayout::addAudio(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath)
{
    auto f2i = StringConverter::f2i(config);

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
        desc = fmt::format("{}, {}", desc, album);
        album_full = album;
    } else {
        album = "Unknown";
    }

    if (!desc.empty())
        desc = fmt::format("{}, ", desc);
    desc = fmt::format("{}{}", desc, title);

    std::string date = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_DATE));
    std::string albumDate;
    if (!date.empty()) {
        size_t i = date.find('-');
        if (i != std::string::npos)
            date = date.substr(0, i);

        desc = fmt::format("{}, {}", desc, date);
        albumDate = esc(date);
    } else {
        date = "Unknown";
        albumDate = "Unknown";
    }
    meta[MetadataHandler::getMetaFieldName(M_UPNP_DATE)] = albumDate;

    std::string genre = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_GENRE));
    if (!genre.empty()) {
        genre = mapGenre(genre);
        desc = fmt::format("{}, {}", desc, genre);
    } else {
        genre = "Unknown";
    }

    std::string description = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_DESCRIPTION));
    if (description.empty()) {
        meta[MetadataHandler::getMetaFieldName(M_DESCRIPTION)] = desc;
        obj->setMetadata(meta);
    }

    std::string composer = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_COMPOSER), "None");
    std::string conductor = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_CONDUCTOR), "None");
    std::string orchestra = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_ORCHESTRA), "None");

    auto id = content->addContainerChain("/Audio/All Audio");
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

    std::string chain = fmt::format("/Audio/Artists/{}/All Songs", artist);

    id = content->addContainerChain(chain);
    add(obj, id);

    std::string temp;
    if (!artist_full.empty())
        temp = artist_full;

    if (!album_full.empty())
        temp = fmt::format("{} - {} - ", temp, album_full);
    else
        temp = fmt::format("{} - ", temp);

    album = esc(album);
    chain = fmt::format("/Audio/Artists/{}/{}", artist, album);
    id = content->addContainerChain(chain, UPNP_CLASS_MUSIC_ALBUM, obj->getID(), obj);
    add(obj, id);

    chain = fmt::format("/Audio/Albums/{}", album);
    id = content->addContainerChain(chain, UPNP_CLASS_MUSIC_ALBUM, obj->getID(), obj);
    add(obj, id);

    chain = fmt::format("/Audio/Genres/{}", esc(genre));
    id = content->addContainerChain(chain, UPNP_CLASS_MUSIC_GENRE);
    add(obj, id);

    chain = fmt::format("/Audio/Composers/{}", esc(composer));
    id = content->addContainerChain(chain, UPNP_CLASS_MUSIC_COMPOSER);
    add(obj, id);

    chain = fmt::format("/Audio/Year/{}", esc(date));
    id = content->addContainerChain(chain);
    add(obj, id);

    obj->setTitle(fmt::format("{}{}", temp, title));

    id = content->addContainerChain("/Audio/All - full name");
    add(obj, id);

    chain = fmt::format("/Audio/Artists/{}/All - full name", artist);
    id = content->addContainerChain(chain);
    add(obj, id);

    obj->setTitle(title);
    fs::path dir;
    if (!rootpath.empty()) {
        // make location relative to rootpath: "/home/.../Audio/Action/a.mp3" with rootpath "/home/.../Audio" -> "Action"
        dir = fs::relative(obj->getLocation().parent_path(), config->getBoolOption(CFG_IMPORT_LAYOUT_PARENT_PATH) ? rootpath.parent_path() : rootpath);
        dir = f2i->convert(dir);
    } else
        dir = esc(f2i->convert(getLastPath(obj->getLocation())));

    if (!dir.empty()) {
        id = content->addContainerChain(fmt::format("/Audio/Directories/{}", dir.string().c_str()));
        add(obj, id);
    }
}

#ifdef SOPCAST
void BuiltinLayout::addSopCast(const std::shared_ptr<CdsObject>& obj)
{
#define SP_VPATH "/Online Services/SopCast"
    bool ref_set = false;

    if (obj->getID() != INVALID_OBJECT_ID) {
        obj->setRefID(obj->getID());
        ref_set = true;
    }

    auto id = content->addContainerChain(SP_VPATH "/All Channels");
    add(obj, id, ref_set);
    if (!ref_set) {
        obj->setRefID(obj->getID());
        ref_set = true;
    }

    auto temp = obj->getAuxData(SOPCAST_AUXDATA_GROUP);
    if (!temp.empty()) {
        id = content->addContainerChain(fmt::format(SP_VPATH "/Groups/{}", esc(temp)));
        add(obj, id, ref_set);
    }
}
#endif

#ifdef ATRAILERS
void BuiltinLayout::addATrailers(const std::shared_ptr<CdsObject>& obj)
{
#define AT_VPATH "/Online Services/Apple Trailers"
    auto id = content->addContainerChain(AT_VPATH "/All Trailers");

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
    for (auto&& genre : genreAr) {
        genre = trimString(genre);
        genre = mapGenre(genre);
        if (genre.empty())
            continue;

        id = content->addContainerChain(fmt::format(AT_VPATH "/Genres/{}", esc(genre)));
        add(obj, id);
    }

    temp = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_DATE));
    if (temp.length() >= 7) {
        id = content->addContainerChain(fmt::format(AT_VPATH "/Release Date/{}", esc(temp.substr(0, 7))));
        add(obj, id);
    }

    temp = obj->getAuxData(ATRAILERS_AUXDATA_POST_DATE);
    if (temp.length() >= 7) {
        id = content->addContainerChain(fmt::format(AT_VPATH "/Post Date/{}", esc(temp.substr(0, 7))));
        add(obj, id);
    }
}
#endif

BuiltinLayout::BuiltinLayout(std::shared_ptr<ContentManager> content)
    : Layout(std::move(content))
{
    genreMap = config->getDictionaryOption(CFG_IMPORT_SCRIPTING_IMPORT_GENRE_MAP);
#ifdef ENABLE_PROFILING
    PROF_INIT_GLOBAL(layout_profiling, "builtin layout");
#endif
}

std::string BuiltinLayout::mapGenre(const std::string& genre)
{
    for (auto&& [from, to] : genreMap) {
        if (std::regex_match(genre, std::regex(from, std::regex::ECMAScript | std::regex::icase))) {
            return std::regex_replace(genre, std::regex(from, std::regex::ECMAScript | std::regex::icase), to);
        }
    }
    return genre;
}

void BuiltinLayout::processCdsObject(std::shared_ptr<CdsObject> obj, fs::path rootpath)
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
        auto service = service_type_t(std::stoi(clone->getAuxData(ONLINE_SERVICE_AUX_ID)));

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
            addAudio(clone, rootpath);
        else if (content_type == CONTENT_TYPE_OGG) {
            if (obj->getFlag(OBJECT_FLAG_OGG_THEORA))
                addVideo(clone, rootpath);
            else
                addAudio(clone, rootpath);
        }

#ifdef ONLINE_SERVICES
    }
#endif
#ifdef ENABLE_PROFILING
    PROF_END(&layout_profiling);
#endif
}

#ifdef ENABLE_PROFILING
BuiltinLayout::~BuiltinLayout()
{
    PROF_PRINT(&layout_profiling);
}
#endif
