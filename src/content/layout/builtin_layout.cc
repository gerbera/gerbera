/*MT*

    MediaTomb - http://www.mediatomb.cc/

    builtin_layout.cc - this file is part of MediaTomb.

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

#include "content/content_manager.h"
#include "metadata/metadata_handler.h"
#include "util/string_converter.h"

#ifdef ONLINE_SERVICES

#include "content/onlineservice/online_service.h"

#ifdef ATRAILERS
#include "content/onlineservice/atrailers_content_handler.h"
#endif

#endif // ONLINE_SERVICES

BuiltinLayout::BuiltinLayout(const std::shared_ptr<ContentManager>& content)
    : Layout(content)
    , config(this->content->getContext()->getConfig())
    , genreMap(this->config->getDictionaryOption(CFG_IMPORT_SCRIPTING_IMPORT_GENRE_MAP))
{
    container["Video"] = std::make_shared<CdsContainer>("Video");
    container["Video"]->addMetaData(M_CONTENT_CLASS, UPNP_CLASS_VIDEO_ITEM);
    container["All Video"] = std::make_shared<CdsContainer>("All Video");
    chain["/Video/All Video"] = this->content->addContainerTree({ container["Video"], container["All Video"] });
    container["Video/Year"] = std::make_shared<CdsContainer>("Year");
    chain["/Video/Year"] = this->content->addContainerTree({ container["Video"], container["Video/Year"] });
    container["Video/Date"] = std::make_shared<CdsContainer>("Date");
    chain["/Video/Date"] = this->content->addContainerTree({ container["Video"], container["Video/Date"] });
    container["Video/Directories"] = std::make_shared<CdsContainer>("Directories");
    chain["/Video/Directories"] = this->content->addContainerTree({ container["Video"], container["Video/Directories"] });

    container["Photos"] = std::make_shared<CdsContainer>("Photos");
    container["Photos"]->addMetaData(M_CONTENT_CLASS, UPNP_CLASS_IMAGE_ITEM);
    container["All Photos"] = std::make_shared<CdsContainer>("All Photos");
    chain["/Photos/All Photos"] = this->content->addContainerTree({ container["Photos"], container["All Photos"] });
    container["Photos/Year"] = std::make_shared<CdsContainer>("Year");
    chain["/Photos/Year"] = this->content->addContainerTree({ container["Photos"], container["Photos/Year"] });
    container["Photos/Date"] = std::make_shared<CdsContainer>("Date");
    chain["/Photos/Date"] = this->content->addContainerTree({ container["Photos"], container["Photos/Date"] });
    container["Photos/Directories"] = std::make_shared<CdsContainer>("Directories");
    chain["/Photos/Directories"] = this->content->addContainerTree({ container["Photos"], container["Photos/Directories"] });

    container["Audio"] = std::make_shared<CdsContainer>("Audio");
    container["Audio"]->addMetaData(M_CONTENT_CLASS, UPNP_CLASS_AUDIO_ITEM);
    container["All Audio"] = std::make_shared<CdsContainer>("All Audio");
    container["All Songs"] = std::make_shared<CdsContainer>("All Songs");
    chain["/Audio/All Audio"] = this->content->addContainerTree({ container["Audio"], container["All Audio"] });
    container["All - full name"] = std::make_shared<CdsContainer>("All - full name");
    chain["/Audio/All - full name"] = this->content->addContainerTree({ container["Audio"], container["All - full name"] });
    container["Albums"] = std::make_shared<CdsContainer>("Albums");
    chain["/Audio/Albums"] = this->content->addContainerTree({ container["Audio"], container["Albums"] });
    container["Artists"] = std::make_shared<CdsContainer>("Artists");
    chain["/Audio/Artists"] = this->content->addContainerTree({ container["Audio"], container["Artists"] });
    container["Genres"] = std::make_shared<CdsContainer>("Genres");
    chain["/Audio/Genres"] = this->content->addContainerTree({ container["Audio"], container["Genres"] });
    container["Composers"] = std::make_shared<CdsContainer>("Composers");
    chain["/Audio/Composers"] = this->content->addContainerTree({ container["Audio"], container["Composers"] });
    container["Year"] = std::make_shared<CdsContainer>("Year");
    chain["/Audio/Year"] = this->content->addContainerTree({ container["Audio"], container["Year"] });
    container["Audio/Directories"] = std::make_shared<CdsContainer>("Directories");
    chain["/Audio/Directories"] = this->content->addContainerTree({ container["Audio"], container["Audio/Directories"] });

#ifdef ONLINE_SERVICES
    if (config->getBoolOption(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED)) {
        container["Online Services"] = std::make_shared<CdsContainer>("Online Services");
    }

#ifdef ATRAILERS
    if (config->getBoolOption(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED)) {
        container["Apple"] = std::make_shared<CdsContainer>("Apple Trailers");
        container["Apple/All Trailers"] = std::make_shared<CdsContainer>("All Trailers");
        container["Apple/Genres"] = std::make_shared<CdsContainer>("Genres");
        container["Apple/Release Date"] = std::make_shared<CdsContainer>("Release Date");
        container["Apple/Post Date"] = std::make_shared<CdsContainer>("Post Date");
        chain["/Online Services/Apple/All Trailers"] = this->content->addContainerTree({ container["Online Services"], container["Apple"], container["Apple/All Trailers"] });
        chain["/Online Services/Apple/Genres"] = this->content->addContainerTree({ container["Online Services"], container["Apple"], container["Apple/Genres"] });
        chain["/Online Services/Apple/Release Date"] = this->content->addContainerTree({ container["Online Services"], container["Apple"], container["Apple/Release Date"] });
        chain["/Online Services/Apple/Post Date"] = this->content->addContainerTree({ container["Online Services"], container["Apple"], container["Apple/Post Date"] });
    }
#endif
#endif
}

void BuiltinLayout::add(const std::shared_ptr<CdsObject>& obj, const std::pair<int, bool>& parentID, bool useRef)
{
    obj->setParentID(parentID.first);
    if (useRef)
        obj->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    obj->setID(INVALID_OBJECT_ID);

    content->addObject(obj, parentID.second);
}

void BuiltinLayout::getDir(const std::shared_ptr<CdsObject>& obj, const fs::path& rootPath, const std::string& c1, const std::string& c2)
{
    fs::path dir;
    auto&& objPath = obj->getLocation();
    if (!rootPath.empty()) {
        // make location relative to rootpath: "/home/.../Videos/Action/a.mkv" with rootpath "/home/.../Videos" -> "Action"
        dir = fs::relative(objPath.parent_path(), config->getBoolOption(CFG_IMPORT_LAYOUT_PARENT_PATH) ? rootPath.parent_path() : rootPath);
        if (dir == ".")
            dir = getLastPath(objPath);
    } else
        dir = getLastPath(objPath);

    auto f2i = StringConverter::f2i(config);
    dir = f2i->convert(dir);

    if (!dir.empty()) {
        std::vector<std::shared_ptr<CdsObject>> dirVect;
        dirVect.push_back(container[c1]);
        dirVect.push_back(container[c2]);
        for (auto&& segment : dir) {
            if (segment != "/" && !segment.empty())
                dirVect.push_back(std::make_shared<CdsContainer>(segment.string()));
        }
        auto id = content->addContainerTree(dirVect);
        add(obj, id);
    }
}

void BuiltinLayout::addVideo(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath)
{
    auto f2i = StringConverter::f2i(config);
    auto id = chain["/Video/All Video"];

    if (obj->getID() != INVALID_OBJECT_ID) {
        obj->setRefID(obj->getID());
        add(obj, id);
    } else {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    auto meta = obj->getMetaData();

    std::string date = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_CREATION_DATE));
    if (!date.empty()) {
        std::string year, month;
        auto m = std::numeric_limits<std::size_t>::max();
        auto y = date.find('-');
        if (y != std::string::npos) {
            year = date.substr(0, y);
            month = date.substr(y + 1);
            m = month.find('-');
            if (m != std::string::npos)
                month = month.substr(0, m);
        }

        if ((y > 0) && (m > 0)) {
            std::vector<std::shared_ptr<CdsObject>> ct;
            ct.reserve(4);
            ct.push_back(container["Video"]);
            ct.push_back(container["Video/Year"]);
            ct.push_back(std::make_shared<CdsContainer>(year));
            ct.push_back(std::make_shared<CdsContainer>(month));
            id = content->addContainerTree(ct);
            add(obj, id);
        }

        std::vector<std::shared_ptr<CdsObject>> ct;
        ct.push_back(container["Video"]);
        ct.push_back(container["Video/Date"]);
        ct.push_back(std::make_shared<CdsContainer>(date));
        id = content->addContainerTree(ct);
        add(obj, id);
    }

    getDir(obj, rootpath, "Video", "Video/Directories");
}

void BuiltinLayout::addImage(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath)
{
    auto f2i = StringConverter::f2i(config);

    auto id = chain["/Photos/All Photos"];
    if (obj->getID() != INVALID_OBJECT_ID) {
        obj->setRefID(obj->getID());
        add(obj, id);
    } else {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    auto meta = obj->getMetaData();

    std::string date = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_DATE));
    if (!date.empty()) {
        std::string year, month;
        auto m = std::numeric_limits<std::size_t>::max();
        auto y = date.find('-');
        if (y != std::string::npos) {
            year = date.substr(0, y);
            month = date.substr(y + 1);
            m = month.find('-');
            if (m != std::string::npos)
                month = month.substr(0, m);
        }

        if ((y > 0) && (m > 0)) {
            std::vector<std::shared_ptr<CdsObject>> ct;
            ct.push_back(container["Photos"]);
            ct.push_back(container["Photos/Year"]);
            ct.push_back(std::make_shared<CdsContainer>(year));
            ct.push_back(std::make_shared<CdsContainer>(month));
            id = content->addContainerTree(ct);
            add(obj, id);
        }

        std::vector<std::shared_ptr<CdsObject>> ct;
        ct.push_back(container["Photos"]);
        ct.push_back(container["Photos/Date"]);
        ct.push_back(std::make_shared<CdsContainer>(date));
        id = content->addContainerTree(ct);
        add(obj, id);
    }

    getDir(obj, rootpath, "Photos", "Photos/Directories");
}

void BuiltinLayout::addAudio(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath)
{
    auto f2i = StringConverter::f2i(config);

    std::string desc;

    std::string title = obj->getMetaData(M_TITLE);
    if (title.empty())
        title = obj->getTitle();

    std::string artist = obj->getMetaData(M_ARTIST);
    std::string artistFull;
    if (!artist.empty()) {
        artistFull = artist;
        desc = artist;
    } else
        artist = "Unknown";

    std::string album = obj->getMetaData(M_ALBUM);
    std::string albumFull;
    if (!album.empty()) {
        desc = fmt::format("{}, {}", desc, album);
        albumFull = album;
    } else {
        album = "Unknown";
    }

    if (!desc.empty())
        desc = fmt::format("{}, ", desc);
    desc = fmt::format("{}{}", desc, title);

    std::string date = obj->getMetaData(M_DATE);
    std::string albumDate;
    if (!date.empty()) {
        auto i = date.find('-');
        if (i != std::string::npos)
            date = date.substr(0, i);

        desc = fmt::format("{}, {}", desc, date);
        albumDate = date;
    } else {
        date = "Unknown";
        albumDate = "Unknown";
    }
    obj->removeMetaData(M_UPNP_DATE);
    obj->addMetaData(M_UPNP_DATE, albumDate);

    std::string genre = obj->getMetaData(M_GENRE);
    if (!genre.empty()) {
        genre = mapGenre(genre);
        desc = fmt::format("{}, {}", desc, genre);
    } else {
        genre = "Unknown";
    }

    std::string description = obj->getMetaData(M_DESCRIPTION);
    if (description.empty()) {
        obj->addMetaData(M_DESCRIPTION, desc);
    }

    std::string composer = obj->getMetaData(M_COMPOSER);
    if (composer.empty())
        composer = "None";

    auto id = chain["/Audio/All Audio"];
    obj->setTitle(title);

    // we get the main object here, so the object that we will add below
    // will be a reference of the main object, that's why we set the ref
    // id to the object id - the add function will clear out the object
    // id
    if (obj->getID() != INVALID_OBJECT_ID) {
        obj->setRefID(obj->getID());
        add(obj, id);
    } else {
        // the object is not yet in the database (probably we got it from a
        // playlist script, so we set the ref id after adding - it will be used
        // for all consequent virtual objects
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    auto artistContainer = std::make_shared<CdsContainer>(artist, UPNP_CLASS_MUSIC_ARTIST);
    std::vector<std::shared_ptr<CdsObject>> arc;
    arc.push_back(container["Audio"]);
    arc.push_back(container["Artists"]);
    arc.push_back(artistContainer);
    arc.push_back(container["All Songs"]);
    id = content->addContainerTree(arc);
    add(obj, id);

    std::string temp;
    if (!artistFull.empty())
        temp = artistFull;

    if (!albumFull.empty())
        temp = fmt::format("{} - {} - ", temp, albumFull);
    else
        temp = fmt::format("{} - ", temp);

    auto albumContainer = std::make_shared<CdsContainer>(album, UPNP_CLASS_MUSIC_ALBUM);
    albumContainer->setMetaData(obj->getMetaData());
    albumContainer->setResources(obj->getResources());
    albumContainer->setRefID(obj->getID());
    artistContainer->setSearchable(true);

    std::vector<std::shared_ptr<CdsObject>> alc;
    alc.push_back(container["Audio"]);
    alc.push_back(container["Artists"]);
    alc.push_back(artistContainer);
    arc.push_back(albumContainer);
    id = content->addContainerTree(alc);
    add(obj, id);

    albumContainer->setSearchable(true);
    std::vector<std::shared_ptr<CdsObject>> allc;
    allc.push_back(container["Audio"]);
    allc.push_back(container["Albums"]);
    allc.push_back(std::move(albumContainer));
    id = content->addContainerTree(allc);
    add(obj, id);

    std::vector<std::shared_ptr<CdsObject>> ct;
    ct.push_back(container["Audio"]);
    ct.push_back(container["Genres"]);
    ct.push_back(std::make_shared<CdsContainer>(genre, UPNP_CLASS_MUSIC_GENRE));
    id = content->addContainerTree(ct);
    add(obj, id);

    auto composerContainer = std::make_shared<CdsContainer>(composer, UPNP_CLASS_MUSIC_COMPOSER);
    composerContainer->setSearchable(true);
    std::vector<std::shared_ptr<CdsObject>> cc;
    cc.push_back(container["Audio"]);
    cc.push_back(container["Composers"]);
    cc.push_back(std::move(composerContainer));
    id = content->addContainerTree(cc);
    add(obj, id);

    auto yearContainer = std::make_shared<CdsContainer>(date);
    yearContainer->setSearchable(true);
    std::vector<std::shared_ptr<CdsObject>> yt;
    yt.push_back(container["Audio"]);
    yt.push_back(container["Year"]);
    yt.push_back(std::move(yearContainer));
    add(obj, id);

    obj->setTitle(fmt::format("{}{}", temp, title));
    add(obj, chain["/Audio/All - full name"]);

    std::vector<std::shared_ptr<CdsObject>> all;
    all.push_back(container["Audio"]);
    all.push_back(container["Artists"]);
    all.push_back(std::move(artistContainer));
    all.push_back(container["All - full name"]);
    id = content->addContainerTree(all);
    add(obj, id);

    obj->setTitle(title);
    getDir(obj, rootpath, "Audio", "Audio/Directories");
}

#ifdef ATRAILERS
void BuiltinLayout::addATrailers(const std::shared_ptr<CdsObject>& obj)
{
    {
        auto id = chain["/Online Services/Apple/All Trailers"];
        if (obj->getID() != INVALID_OBJECT_ID) {
            obj->setRefID(obj->getID());
            add(obj, id);
        } else {
            add(obj, id);
            obj->setRefID(obj->getID());
        }
    }

    auto meta = obj->getMetaData();

    std::string temp = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_GENRE));
    auto genreAr = splitString(temp, ',');
    for (auto&& genre : genreAr) {
        trimStringInPlace(genre);
        genre = mapGenre(genre);
        if (genre.empty())
            continue;

        std::vector<std::shared_ptr<CdsObject>> ct;
        ct.push_back(container["Online Services"]);
        ct.push_back(container["Apple"]);
        ct.push_back(container["Genres"]);
        ct.push_back(std::make_shared<CdsContainer>(genre));
        auto id = content->addContainerTree(ct);
        add(obj, id);
    }

    temp = getValueOrDefault(meta, MetadataHandler::getMetaFieldName(M_DATE));
    if (temp.length() >= 7) {
        std::vector<std::shared_ptr<CdsObject>> ct;
        ct.push_back(container["Online Services"]);
        ct.push_back(container["Apple"]);
        ct.push_back(container["Release Date"]);
        ct.push_back(std::make_shared<CdsContainer>(temp.substr(0, 7)));
        auto id = content->addContainerTree(ct);
        add(obj, id);
    }

    temp = obj->getAuxData(ATRAILERS_AUXDATA_POST_DATE);
    if (temp.length() >= 7) {
        std::vector<std::shared_ptr<CdsObject>> ct;
        ct.push_back(container["Online Services"]);
        ct.push_back(container["Apple"]);
        ct.push_back(container["Post Date"]);
        ct.push_back(std::make_shared<CdsContainer>(temp.substr(0, 7)));
        auto id = content->addContainerTree(ct);
        add(obj, id);
    }
}
#endif

std::string BuiltinLayout::mapGenre(const std::string& genre)
{
    for (auto&& [from, to] : genreMap) {
        if (std::regex_match(genre, std::regex(from, std::regex::ECMAScript | std::regex::icase))) {
            return std::regex_replace(genre, std::regex(from, std::regex::ECMAScript | std::regex::icase), to);
        }
    }
    return genre;
}

void BuiltinLayout::processCdsObject(const std::shared_ptr<CdsObject>& obj, const fs::path& rootpath, const std::string& contentType)
{
    log_debug("Process CDS Object: {}", obj->getTitle());
    auto clone = CdsObject::createObject(obj->getObjectType());
    obj->copyTo(clone);
    clone->setVirtual(true);

#ifdef ONLINE_SERVICES
    if (clone->getFlag(OBJECT_FLAG_ONLINE_SERVICE)) {
        auto service = service_type_t(std::stoi(clone->getAuxData(ONLINE_SERVICE_AUX_ID)));

        switch (service) {
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

        auto objCls = std::static_pointer_cast<CdsItem>(obj)->getClass();
        if (contentType == CONTENT_TYPE_OGG) {
            if (obj->getFlag(OBJECT_FLAG_OGG_THEORA))
                addVideo(clone, rootpath);
            else
                addAudio(clone, rootpath);
        } else if (objCls == UPNP_CLASS_VIDEO_ITEM) {
            addVideo(clone, rootpath);
        } else if (objCls == UPNP_CLASS_IMAGE_ITEM) {
            addImage(clone, rootpath);
        } else if (objCls == UPNP_CLASS_MUSIC_TRACK && contentType != CONTENT_TYPE_PLAYLIST) {
            addAudio(clone, rootpath);
        }

#ifdef ONLINE_SERVICES
    }
#endif
}
