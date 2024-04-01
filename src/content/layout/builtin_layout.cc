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
        // We could copy only the enabled containers, but to avoid checking dependencies here, we copy all of them!
        container[bl->getKey()] = std::make_shared<CdsContainer>(bl->getTitle());
    }

    containerAt(BoxKeys::videoRoot)->addMetaData(MetadataFields::M_CONTENT_CLASS, UPNP_CLASS_VIDEO_ITEM);
    chain["/Video/All Video"] = this->content->addContainerTree({ containerAt(BoxKeys::videoRoot), containerAt(BoxKeys::videoAll) }, nullptr);

    /*****************************************************************************/

    containerAt(BoxKeys::imageRoot)->addMetaData(MetadataFields::M_CONTENT_CLASS, UPNP_CLASS_IMAGE_ITEM);
    chain["/Photos/All Photos"] = this->content->addContainerTree({ containerAt(BoxKeys::imageRoot), containerAt(BoxKeys::imageAll) }, nullptr);

    /*****************************************************************************/

    containerAt(BoxKeys::audioRoot)->addMetaData(MetadataFields::M_CONTENT_CLASS, UPNP_CLASS_AUDIO_ITEM);
    chain["/Audio/All Audio"] = this->content->addContainerTree({ containerAt(BoxKeys::audioRoot), containerAt(BoxKeys::audioAll) }, nullptr);
    if (blOption->get(BoxKeys::audioAllTracks)->getEnabled()) {
        chain["/Audio/All - full name"] = this->content->addContainerTree({ containerAt(BoxKeys::audioRoot), containerAt(BoxKeys::audioAllTracks) }, nullptr);
    }

#ifdef ONLINE_SERVICES
    if (config->getBoolOption(CFG_ONLINE_CONTENT_ATRAILERS_ENABLED)) {
#ifdef ATRAILERS
        chain["/Online Services/Apple/All Trailers"] = this->content->addContainerTree({ containerAt(BoxKeys::trailerRoot), containerAt(BoxKeys::trailerApple), containerAt(BoxKeys::trailerAll) }, nullptr);
#endif
    }
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

void BuiltinLayout::getDir(const std::shared_ptr<CdsObject>& obj, const fs::path& rootPath, const std::string_view& c1, const std::string_view& c2, const std::string& upnpClass)
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
        dirVect.push_back(containerAt(c1));
        dirVect.push_back(containerAt(c2));
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
    auto blOption = config->getBoxLayoutListOption(CFG_BOXLAYOUT_BOX);

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

        if (blOption->get(BoxKeys::videoAllYears)->getEnabled()) {
            if ((y > 0) && (m > 0)) {
                std::vector<std::shared_ptr<CdsObject>> ct;
                ct.reserve(4);
                ct.push_back(containerAt(BoxKeys::videoRoot));
                ct.push_back(containerAt(BoxKeys::videoAllYears));
                ct.push_back(std::make_shared<CdsContainer>(year));
                ct.push_back(std::make_shared<CdsContainer>(month));
                id = content->addContainerTree(ct, obj);
                add(obj, id);
            }
        }

        if (blOption->get(BoxKeys::videoAllDates)->getEnabled()) {
            auto t = date.find('T');
            if (t != std::string::npos) {
                date.resize(t);
            }
            std::vector<std::shared_ptr<CdsObject>> ct;
            ct.push_back(containerAt(BoxKeys::videoRoot));
            ct.push_back(containerAt(BoxKeys::videoAllDates));
            ct.push_back(std::make_shared<CdsContainer>(date));
            id = content->addContainerTree(ct, obj);
            add(obj, id);
        }
    }

    if (blOption->get(BoxKeys::videoAllDirectories)->getEnabled()) {
        getDir(obj, rootpath, BoxKeys::videoRoot, BoxKeys::videoAllDirectories, getValueOrDefault(containerMap, AutoscanMediaMode::Video, AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Video)));
    }
}

void BuiltinLayout::addImage(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsContainer>& parent, const fs::path& rootpath, const std::map<AutoscanMediaMode, std::string>& containerMap)
{
    auto f2i = StringConverter::f2i(config);
    auto blOption = config->getBoxLayoutListOption(CFG_BOXLAYOUT_BOX);

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

        if (blOption->get(BoxKeys::imageAllYears)->getEnabled()) {
            if ((y > 0) && (m > 0)) {
                std::vector<std::shared_ptr<CdsObject>> ct;
                ct.push_back(containerAt(BoxKeys::imageRoot));
                ct.push_back(containerAt(BoxKeys::imageAllYears));
                ct.push_back(std::make_shared<CdsContainer>(year));
                ct.push_back(std::make_shared<CdsContainer>(month));
                id = content->addContainerTree(ct, obj);
                add(obj, id);
            }
        }

        if (blOption->get(BoxKeys::imageAllDates)->getEnabled()) {
            auto t = date.find('T');
            if (t != std::string::npos) {
                date.resize(t);
            }
            std::vector<std::shared_ptr<CdsObject>> ct;
            ct.push_back(containerAt(BoxKeys::imageRoot));
            ct.push_back(containerAt(BoxKeys::imageAllYears));
            ct.push_back(std::make_shared<CdsContainer>(date));
            id = content->addContainerTree(ct, obj);
            add(obj, id);
        }
    }

    if (blOption->get(BoxKeys::imageAllDirectories)->getEnabled()) {
        getDir(obj, rootpath, BoxKeys::imageRoot, BoxKeys::imageAllDirectories, getValueOrDefault(containerMap, AutoscanMediaMode::Image, AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Image)));
    }
}

void BuiltinLayout::addAudio(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsContainer>& parent, const fs::path& rootpath, const std::map<AutoscanMediaMode, std::string>& containerMap)
{
    auto f2i = StringConverter::f2i(config);
    auto blOption = config->getBoxLayoutListOption(CFG_BOXLAYOUT_BOX);

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

    obj->setTitle(title);
    auto id = chain["/Audio/All Audio"];

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
    if (blOption->get(BoxKeys::audioAllSongs)->getEnabled() && blOption->get(BoxKeys::audioAllArtists)->getEnabled()) {
        std::vector<std::shared_ptr<CdsObject>> arc;
        arc.push_back(containerAt(BoxKeys::audioRoot));
        arc.push_back(containerAt(BoxKeys::audioAllArtists));
        arc.push_back(artistContainer);
        arc.push_back(containerAt(BoxKeys::audioAllSongs));
        id = content->addContainerTree(arc, obj);
        add(obj, id);
    }

    std::string prefixTitle;
    if (!artistFull.empty())
        prefixTitle = artistFull;

    if (!albumFull.empty())
        prefixTitle = fmt::format("{} - {} - ", prefixTitle, albumFull);
    else
        prefixTitle = fmt::format("{} - ", prefixTitle);

    auto albumContainer = std::make_shared<CdsContainer>(album, UPNP_CLASS_MUSIC_ALBUM);
    albumContainer->setMetaData(obj->getMetaData());
    albumContainer->setResources(obj->getResources());
    albumContainer->setRefID(obj->getID());
    artistContainer->setSearchable(true);

    if (blOption->get(BoxKeys::audioAllArtists)->getEnabled()) {
        std::vector<std::shared_ptr<CdsObject>> alc;
        alc.push_back(containerAt(BoxKeys::audioRoot));
        alc.push_back(containerAt(BoxKeys::audioAllArtists));
        alc.push_back(artistContainer);
        alc.push_back(albumContainer);
        id = content->addContainerTree(alc, obj);
        add(obj, id);
    }

    albumContainer->setSearchable(true);
    if (blOption->get(BoxKeys::audioAllAlbums)->getEnabled()) {
        std::vector<std::shared_ptr<CdsObject>> allc;
        allc.push_back(containerAt(BoxKeys::audioRoot));
        allc.push_back(containerAt(BoxKeys::audioAllAlbums));
        allc.push_back(std::move(albumContainer));
        id = content->addContainerTree(allc, obj);
        add(obj, id);
    }

    if (blOption->get(BoxKeys::audioAllGenres)->getEnabled()) {
        std::vector<std::shared_ptr<CdsObject>> ct;
        ct.push_back(containerAt(BoxKeys::audioRoot));
        ct.push_back(containerAt(BoxKeys::audioAllGenres));
        ct.push_back(std::make_shared<CdsContainer>(genre, UPNP_CLASS_MUSIC_GENRE));
        id = content->addContainerTree(ct, obj);
        add(obj, id);
    }

    if (blOption->get(BoxKeys::audioAllComposers)->getEnabled()) {
        auto composerContainer = std::make_shared<CdsContainer>(composer, UPNP_CLASS_MUSIC_COMPOSER);
        composerContainer->setSearchable(true);
        std::vector<std::shared_ptr<CdsObject>> cc;
        cc.push_back(containerAt(BoxKeys::audioRoot));
        cc.push_back(containerAt(BoxKeys::audioAllComposers));
        cc.push_back(std::move(composerContainer));
        id = content->addContainerTree(cc, obj);
        add(obj, id);
    }

    if (blOption->get(BoxKeys::audioAllYears)->getEnabled()) {
        auto yearContainer = std::make_shared<CdsContainer>(date);
        yearContainer->setSearchable(true);
        std::vector<std::shared_ptr<CdsObject>> yt;
        yt.push_back(containerAt(BoxKeys::audioRoot));
        yt.push_back(containerAt(BoxKeys::audioAllYears));
        yt.push_back(std::move(yearContainer));
        id = content->addContainerTree(yt, obj);
        add(obj, id);
    }

    if (blOption->get(BoxKeys::audioAllDirectories)->getEnabled()) {
        getDir(obj, rootpath, BoxKeys::audioRoot, BoxKeys::audioAllDirectories, getValueOrDefault(containerMap, AutoscanMediaMode::Audio, AutoscanDirectory::ContainerTypesDefaults.at(AutoscanMediaMode::Audio)));
    }

    if (blOption->get(BoxKeys::audioArtistChronology)->getEnabled() && blOption->get(BoxKeys::audioAllArtists)->getEnabled()) {
        artistContainer->setSearchable(false);
        std::vector<std::shared_ptr<CdsObject>> chronology;
        chronology.push_back(containerAt(BoxKeys::audioRoot));
        chronology.push_back(containerAt(BoxKeys::audioAllArtists));
        chronology.push_back(artistContainer);
        chronology.push_back(containerAt(BoxKeys::audioArtistChronology));
        chronology.push_back(std::make_shared<CdsContainer>(date + " - " + album, UPNP_CLASS_MUSIC_ALBUM));
        id = content->addContainerTree(chronology, obj);
        add(obj, id);
    }

    // Keep this last, since it's modifying the object title
    if (blOption->get(BoxKeys::audioAllTracks)->getEnabled()) {
        artistContainer->setSearchable(true);
        obj->setTitle(fmt::format("{}{}", prefixTitle, title));
        add(obj, chain["/Audio/All - full name"]);

        if (blOption->get(BoxKeys::audioAllArtists)->getEnabled()) {
            std::vector<std::shared_ptr<CdsObject>> all;
            all.push_back(containerAt(BoxKeys::audioRoot));
            all.push_back(containerAt(BoxKeys::audioAllArtists));
            all.push_back(std::move(artistContainer));
            all.push_back(containerAt(BoxKeys::audioAllTracks));
            id = content->addContainerTree(all, obj);
            add(obj, id);
        }
    }
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
    auto blOption = config->getBoxLayoutListOption(CFG_BOXLAYOUT_BOX);

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

    if (blOption->get(BoxKeys::trailerAllGenres)->getEnabled() && blOption->get(BoxKeys::trailerApple)->getEnabled()) {
        std::string temp = getValueOrDefault(meta, MetaEnumMapper::getMetaFieldName(MetadataFields::M_GENRE));
        auto genreAr = splitString(temp, ',');
        for (auto&& genre : genreAr) {
            trimStringInPlace(genre);
            genre = mapGenre(genre);
            if (genre.empty())
                continue;

            std::vector<std::shared_ptr<CdsObject>> ct;
            ct.push_back(containerAt(BoxKeys::trailerRoot));
            ct.push_back(containerAt(BoxKeys::trailerApple));
            ct.push_back(containerAt(BoxKeys::trailerAllGenres));
            ct.push_back(std::make_shared<CdsContainer>(genre));
            auto id = content->addContainerTree(ct, obj);
            add(obj, id);
        }
    }

    if (blOption->get(BoxKeys::trailerRelDate)->getEnabled() && blOption->get(BoxKeys::trailerApple)->getEnabled()) {
        std::string temp = getValueOrDefault(meta, MetaEnumMapper::getMetaFieldName(MetadataFields::M_DATE));
        if (temp.length() >= 7) {
            std::vector<std::shared_ptr<CdsObject>> ct;
            ct.push_back(containerAt(BoxKeys::trailerRoot));
            ct.push_back(containerAt(BoxKeys::trailerApple));
            ct.push_back(containerAt(BoxKeys::trailerRelDate));
            ct.push_back(std::make_shared<CdsContainer>(temp.substr(0, 7)));
            auto id = content->addContainerTree(ct, obj);
            add(obj, id);
        }
    }

    if (blOption->get(BoxKeys::trailerPostDate)->getEnabled() && blOption->get(BoxKeys::trailerApple)->getEnabled()) {
        std::string temp = obj->getAuxData(ATRAILERS_AUXDATA_POST_DATE);
        if (temp.length() >= 7) {
            std::vector<std::shared_ptr<CdsObject>> ct;
            ct.push_back(containerAt(BoxKeys::trailerRoot));
            ct.push_back(containerAt(BoxKeys::trailerApple));
            ct.push_back(containerAt(BoxKeys::trailerPostDate));
            ct.push_back(std::make_shared<CdsContainer>(temp.substr(0, 7)));
            auto id = content->addContainerTree(ct, obj);
            add(obj, id);
        }
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
