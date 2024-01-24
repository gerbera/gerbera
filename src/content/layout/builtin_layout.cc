/*MT*

    MediaTomb - http://www.mediatomb.cc/

    builtin_layout.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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
#define LOG_FAC log_facility_t::layout

#include "builtin_layout.h" // API

#include <regex>

#include "cds/cds_container.h"
#include "cds/cds_item.h"
#include "config/config.h"
#include "config/result/autoscan.h"
#include "config/result/box_layout.h"
#include "content/content_manager.h"
#include "metadata/metadata_enums.h"
#include "util/string_converter.h"
#include "util/tools.h"

#ifdef ONLINE_SERVICES

#include "content/onlineservice/online_service.h"

#ifdef ATRAILERS
#include "content/onlineservice/atrailers_content_handler.h"
#endif

#endif // ONLINE_SERVICES

BuiltinLayout::BuiltinLayout(std::shared_ptr<ContentManager> content)
    : Layout(std::move(content))
    , config(this->content->getContext()->getConfig())
    , genreMap(this->config->getDictionaryOption(CFG_IMPORT_SCRIPTING_IMPORT_GENRE_MAP))
{
    auto blOption = config->getBoxLayoutListOption(CFG_BOXLAYOUT_BOX);
    for (auto&& bl : blOption->getArrayCopy()) {
        container[bl->getKey()] = std::make_shared<CdsContainer>(bl->getTitle());
    }

    container["Video"] = container.at("Video/videoRoot");
    container["Video"]->addMetaData(MetadataFields::M_CONTENT_CLASS, UPNP_CLASS_VIDEO_ITEM);
    container["All Video"] = container.at("Video/allVideo");
    chain["/Video/All Video"] = this->content->addContainerTree({ container["Video"], container["All Video"] }, nullptr);
    container["Video/Year"] = container.at("Video/allYears");
    chain["/Video/Year"] = this->content->addContainerTree({ container["Video"], container["Video/Year"] }, nullptr);
    container["Video/Date"] = container.at("Video/allDates");
    chain["/Video/Date"] = this->content->addContainerTree({ container["Video"], container["Video/Date"] }, nullptr);
    container["Video/Directories"] = container.at("Video/allDirectories");
    chain["/Video/Directories"] = this->content->addContainerTree({ container["Video"], container["Video/Directories"] }, nullptr);

    container["Photos"] = container.at("Image/imageRoot");
    container["Photos"]->addMetaData(MetadataFields::M_CONTENT_CLASS, UPNP_CLASS_IMAGE_ITEM);
    container["All Photos"] = container.at("Image/allImages");
    chain["/Photos/All Photos"] = this->content->addContainerTree({ container["Photos"], container["All Photos"] }, nullptr);
    container["Photos/Year"] = container.at("Image/allYears");
    chain["/Photos/Year"] = this->content->addContainerTree({ container["Photos"], container["Photos/Year"] }, nullptr);
    container["Photos/Date"] = container.at("Image/allDates");
    chain["/Photos/Date"] = this->content->addContainerTree({ container["Photos"], container["Photos/Date"] }, nullptr);
    container["Photos/Directories"] = container.at("Image/allDirectories");
    chain["/Photos/Directories"] = this->content->addContainerTree({ container["Photos"], container["Photos/Directories"] }, nullptr);

    container["Audio"] = container.at("Audio/audioRoot");
    container["Audio"]->addMetaData(MetadataFields::M_CONTENT_CLASS, UPNP_CLASS_AUDIO_ITEM);
    container["All Audio"] = container.at("Audio/allAudio");
    container["All Songs"] = container.at("Audio/allSongs");
    chain["/Audio/All Audio"] = this->content->addContainerTree({ container["Audio"], container["All Audio"] }, nullptr);
    container["All - full name"] = container.at("Audio/allTracks");
    chain["/Audio/All - full name"] = this->content->addContainerTree({ container["Audio"], container["All - full name"] }, nullptr);
    container["Albums"] = container.at("Audio/allAlbums");
    chain["/Audio/Albums"] = this->content->addContainerTree({ container["Audio"], container["Albums"] }, nullptr);
    container["Artists"] = container.at("Audio/allArtists");
    chain["/Audio/Artists"] = this->content->addContainerTree({ container["Audio"], container["Artists"] }, nullptr);
    container["Genres"] = container.at("Audio/allGenres");
    chain["/Audio/Genres"] = this->content->addContainerTree({ container["Audio"], container["Genres"] }, nullptr);
    container["Composers"] = container.at("Audio/allComposers");
    chain["/Audio/Composers"] = this->content->addContainerTree({ container["Audio"], container["Composers"] }, nullptr);
    container["Year"] = container.at("Audio/allYears");
    chain["/Audio/Year"] = this->content->addContainerTree({ container["Audio"], container["Year"] }, nullptr);
    container["Audio/Directories"] = container.at("Audio/allDirectories");
    chain["/Audio/Directories"] = this->content->addContainerTree({ container["Audio"], container["Audio/Directories"] }, nullptr);
    container["Artist Chronology"] = container.at("Audio/artistChronology");

#ifdef ONLINE_SERVICES
    if (config->getBoolOption(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED)) {
        container["Online Services"] = container.at("Trailer/trailerRoot");
    }

#ifdef ATRAILERS
    if (config->getBoolOption(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED)) {
        container["Apple"] = container.at("Trailer/appleTrailers");
        container["Apple/All Trailers"] = container.at("Trailer/allTrailers");
        container["Apple/Genres"] = container.at("Trailer/allGenres");
        container["Apple/Release Date"] = container.at("Trailer/relDate");
        container["Apple/Post Date"] = container.at("Trailer/postDate");
        chain["/Online Services/Apple/All Trailers"] = this->content->addContainerTree({ container["Online Services"], container["Apple"], container["Apple/All Trailers"] }, nullptr);
        chain["/Online Services/Apple/Genres"] = this->content->addContainerTree({ container["Online Services"], container["Apple"], container["Apple/Genres"] }, nullptr);
        chain["/Online Services/Apple/Release Date"] = this->content->addContainerTree({ container["Online Services"], container["Apple"], container["Apple/Release Date"] }, nullptr);
        chain["/Online Services/Apple/Post Date"] = this->content->addContainerTree({ container["Online Services"], container["Apple"], container["Apple/Post Date"] }, nullptr);
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

void BuiltinLayout::getDir(const std::shared_ptr<CdsObject>& obj, const fs::path& rootPath, const std::string& c1, const std::string& c2, const std::string& upnpClass)
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
            if (segment != "/" && segment != ".." && !segment.empty())
                dirVect.push_back(std::make_shared<CdsContainer>(segment.string()));
        }
        auto id = content->addContainerTree(dirVect, obj);
        dirVect[dirVect.size() - 1]->setClass(upnpClass);
        add(obj, id);
    }
}

void BuiltinLayout::addVideo(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsContainer>& parent, const fs::path& rootpath, const std::map<AutoscanMediaMode, std::string>& containerMap)
{
    auto f2i = StringConverter::f2i(config);
    auto id = chain["/Video/All Video"];
    log_debug("add video file {}", obj->getLocation().string());

    if (obj->getID() != INVALID_OBJECT_ID) {
        obj->setRefID(obj->getID());
        add(obj, id);
    } else {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    auto meta = obj->getMetaData();

    std::string date = getValueOrDefault(meta, std::string { MetaEnumMapper::getMetaFieldName(MetadataFields::M_CREATION_DATE) });
    if (!date.empty()) {
        std::string year, month;
        auto m = std::numeric_limits<std::size_t>::max();
        auto y = date.find('-');
        if (y != std::string::npos) {
            year = date.substr(0, y);
            month = date.substr(y + 1);
            m = month.find('-');
            if (m != std::string::npos)
                month.resize(m);
        }

        if ((y > 0) && (m > 0)) {
            std::vector<std::shared_ptr<CdsObject>> ct;
            ct.reserve(4);
            ct.push_back(container["Video"]);
            ct.push_back(container["Video/Year"]);
            ct.push_back(std::make_shared<CdsContainer>(year));
            ct.push_back(std::make_shared<CdsContainer>(month));
            id = content->addContainerTree(ct, obj);
            add(obj, id);
        }

        auto t = date.find('T');
        if (t != std::string::npos) {
            date.resize(t);
        }
        std::vector<std::shared_ptr<CdsObject>> ct;
        ct.push_back(container["Video"]);
        ct.push_back(container["Video/Date"]);
        ct.push_back(std::make_shared<CdsContainer>(date));
        id = content->addContainerTree(ct, obj);
        add(obj, id);
    }

    getDir(obj, rootpath, "Video", "Video/Directories", getValueOrDefault(containerMap, AutoscanMediaMode::Video, AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Video)));
}

void BuiltinLayout::addImage(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsContainer>& parent, const fs::path& rootpath, const std::map<AutoscanMediaMode, std::string>& containerMap)
{
    auto f2i = StringConverter::f2i(config);
    log_debug("add image file {}", obj->getLocation().string());

    auto id = chain["/Photos/All Photos"];
    if (obj->getID() != INVALID_OBJECT_ID) {
        obj->setRefID(obj->getID());
        add(obj, id);
    } else {
        add(obj, id);
        obj->setRefID(obj->getID());
    }

    auto meta = obj->getMetaData();

    std::string date = getValueOrDefault(meta, std::string { MetaEnumMapper::getMetaFieldName(MetadataFields::M_DATE) });
    if (!date.empty()) {
        std::string year, month;
        auto m = std::numeric_limits<std::size_t>::max();
        auto y = date.find('-');
        if (y != std::string::npos) {
            year = date.substr(0, y);
            month = date.substr(y + 1);
            m = month.find('-');
            if (m != std::string::npos)
                month.resize(m);
        }

        if ((y > 0) && (m > 0)) {
            std::vector<std::shared_ptr<CdsObject>> ct;
            ct.push_back(container["Photos"]);
            ct.push_back(container["Photos/Year"]);
            ct.push_back(std::make_shared<CdsContainer>(year));
            ct.push_back(std::make_shared<CdsContainer>(month));
            id = content->addContainerTree(ct, obj);
            add(obj, id);
        }

        auto t = date.find('T');
        if (t != std::string::npos) {
            date.resize(t);
        }
        std::vector<std::shared_ptr<CdsObject>> ct;
        ct.push_back(container["Photos"]);
        ct.push_back(container["Photos/Date"]);
        ct.push_back(std::make_shared<CdsContainer>(date));
        id = content->addContainerTree(ct, obj);
        add(obj, id);
    }

    getDir(obj, rootpath, "Photos", "Photos/Directories", getValueOrDefault(containerMap, AutoscanMediaMode::Image, AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Image)));
}

void BuiltinLayout::addAudio(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsContainer>& parent, const fs::path& rootpath, const std::map<AutoscanMediaMode, std::string>& containerMap)
{
    auto f2i = StringConverter::f2i(config);

    std::string desc;
    log_debug("add audio file {}", obj->getLocation().string());

    std::string title = obj->getMetaData(MetadataFields::M_TITLE);
    if (title.empty())
        title = obj->getTitle();

    std::string artist = obj->getMetaData(MetadataFields::M_ARTIST);
    std::string artistFull;
    if (!artist.empty()) {
        artistFull = artist;
        desc = artist;
    } else
        artist = "Unknown";

    std::string album = obj->getMetaData(MetadataFields::M_ALBUM);
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

    std::string date = obj->getMetaData(MetadataFields::M_DATE);
    std::string albumDate;
    if (!date.empty()) {
        auto i = date.find('-');
        if (i != std::string::npos)
            date.resize(i);

        desc = fmt::format("{}, {}", desc, date);
        albumDate = date;
    } else {
        date = "Unknown";
        albumDate = "Unknown";
    }
    obj->removeMetaData(MetadataFields::M_UPNP_DATE);
    obj->addMetaData(MetadataFields::M_UPNP_DATE, albumDate);

    std::string genre = obj->getMetaData(MetadataFields::M_GENRE);
    if (!genre.empty()) {
        genre = mapGenre(genre);
        desc = fmt::format("{}, {}", desc, genre);
    } else {
        genre = "Unknown";
    }

    std::string description = obj->getMetaData(MetadataFields::M_DESCRIPTION);
    if (description.empty()) {
        obj->addMetaData(MetadataFields::M_DESCRIPTION, desc);
    }

    std::string composer = obj->getMetaData(MetadataFields::M_COMPOSER);
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
    id = content->addContainerTree(arc, obj);
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
    alc.push_back(albumContainer);
    id = content->addContainerTree(alc, obj);
    add(obj, id);

    albumContainer->setSearchable(true);
    std::vector<std::shared_ptr<CdsObject>> allc;
    allc.push_back(container["Audio"]);
    allc.push_back(container["Albums"]);
    allc.push_back(std::move(albumContainer));
    id = content->addContainerTree(allc, obj);
    add(obj, id);

    std::vector<std::shared_ptr<CdsObject>> ct;
    ct.push_back(container["Audio"]);
    ct.push_back(container["Genres"]);
    ct.push_back(std::make_shared<CdsContainer>(genre, UPNP_CLASS_MUSIC_GENRE));
    id = content->addContainerTree(ct, obj);
    add(obj, id);

    auto composerContainer = std::make_shared<CdsContainer>(composer, UPNP_CLASS_MUSIC_COMPOSER);
    composerContainer->setSearchable(true);
    std::vector<std::shared_ptr<CdsObject>> cc;
    cc.push_back(container["Audio"]);
    cc.push_back(container["Composers"]);
    cc.push_back(std::move(composerContainer));
    id = content->addContainerTree(cc, obj);
    add(obj, id);

    auto yearContainer = std::make_shared<CdsContainer>(date);
    yearContainer->setSearchable(true);
    std::vector<std::shared_ptr<CdsObject>> yt;
    yt.push_back(container["Audio"]);
    yt.push_back(container["Year"]);
    yt.push_back(std::move(yearContainer));
    id = content->addContainerTree(yt, obj);
    add(obj, id);

    obj->setTitle(fmt::format("{}{}", temp, title));
    add(obj, chain["/Audio/All - full name"]);

    std::vector<std::shared_ptr<CdsObject>> all;
    all.push_back(container["Audio"]);
    all.push_back(container["Artists"]);
    all.push_back(artistContainer);
    all.push_back(container["All - full name"]);
    id = content->addContainerTree(all, obj);
    add(obj, id);

    obj->setTitle(title);
    getDir(obj, rootpath, "Audio", "Audio/Directories", getValueOrDefault(containerMap, AutoscanMediaMode::Audio, AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio)));

    artistContainer->setSearchable(false);
    std::vector<std::shared_ptr<CdsObject>> chronology;
    chronology.push_back(container["Audio"]);
    chronology.push_back(container["Artists"]);
    chronology.push_back(std::move(artistContainer));
    chronology.push_back(container["Artist Chronology"]);
    chronology.push_back(std::make_shared<CdsContainer>(date + " - " + album, UPNP_CLASS_MUSIC_ALBUM));
    id = content->addContainerTree(chronology, obj);
    add(obj, id);
}

#ifdef ONLINE_SERVICES
void BuiltinLayout::addTrailer(const std::shared_ptr<CdsObject>& obj, OnlineServiceType serviceType, const fs::path& rootpath, const std::map<AutoscanMediaMode, std::string>& containerMap)
{
    switch (serviceType) {
#ifdef ATRAILERS
    case OnlineServiceType::OS_ATrailers:
        addATrailers(obj);
        break;
#endif
    case OnlineServiceType::OS_Max:
    default:
        log_warning("No handler for service type");
        break;
    }
}
#endif

#ifdef ATRAILERS
void BuiltinLayout::addATrailers(const std::shared_ptr<CdsObject>& obj)
{
    log_debug("add trailer {}", obj->getLocation().string());
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

    std::string temp = getValueOrDefault(meta, MetaEnumMapper::getMetaFieldName(MetadataFields::M_GENRE));
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
        auto id = content->addContainerTree(ct, obj);
        add(obj, id);
    }

    temp = getValueOrDefault(meta, MetaEnumMapper::getMetaFieldName(MetadataFields::M_DATE));
    if (temp.length() >= 7) {
        std::vector<std::shared_ptr<CdsObject>> ct;
        ct.push_back(container["Online Services"]);
        ct.push_back(container["Apple"]);
        ct.push_back(container["Release Date"]);
        ct.push_back(std::make_shared<CdsContainer>(temp.substr(0, 7)));
        auto id = content->addContainerTree(ct, obj);
        add(obj, id);
    }

    temp = obj->getAuxData(ATRAILERS_AUXDATA_POST_DATE);
    if (temp.length() >= 7) {
        std::vector<std::shared_ptr<CdsObject>> ct;
        ct.push_back(container["Online Services"]);
        ct.push_back(container["Apple"]);
        ct.push_back(container["Post Date"]);
        ct.push_back(std::make_shared<CdsContainer>(temp.substr(0, 7)));
        auto id = content->addContainerTree(ct, obj);
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
