/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    fallback_layout.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

using namespace zmm;

static void add(zmm::Ref<CdsObject> obj, int parentID)
{
    obj->setParentID(parentID);
    obj->setFlag(OBJECT_FLAG_USE_RESOURCE_REF);
    obj->setID(INVALID_OBJECT_ID);

    ContentManager::getInstance()->addObject(obj);
}

static zmm::String esc(zmm::String str)
{
    return escape(str, VIRTUAL_CONTAINER_ESCAPE, VIRTUAL_CONTAINER_SEPARATOR);
}

static void addVideo(zmm::Ref<CdsObject> obj)
{
    int id = ContentManager::getInstance()->addContainerChain(_("/Video"));
    add(obj, id);
}

static void addImage(zmm::Ref<CdsObject> obj)
{
    int id;
    
    id = ContentManager::getInstance()->addContainerChain(_("/Photos/All Photos"));
    add(obj, id);

    Ref<Dictionary> meta = obj->getMetadata();

    String date = meta->get(MetadataHandler::getMetaFieldName(M_DATE));
    if (string_ok(date))
    {
        String chain = _("/Photos/Date/") + esc(date);

        id = ContentManager::getInstance()->addContainerChain(chain);
        add(obj, id);
    }
}

static void addAudio(zmm::Ref<CdsObject> obj)
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

    id = ContentManager::getInstance()->addContainerChain(_("/Audio/All Audio"), _(UPNP_DEFAULT_CLASS_MUSIC_CONT));
    obj->setTitle(title);
    add(obj, id);

    artist = esc(artist);

    chain = _("/Audio/Artists/") + artist + "/All songs";

    id = ContentManager::getInstance()->addContainerChain(chain, _(UPNP_DEFAULT_CLASS_MUSIC_CONT));
    add(obj, id);

    String temp;
    if (string_ok(artist_full))
        temp = artist_full;

    if (string_ok(album_full))
        temp = temp + " - " + album_full + " - ";

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
    id = ContentManager::getInstance()->addContainerChain(chain, _(UPNP_DEFAULT_CLASS_MUSIC_CONT));
    add(obj, id);

    obj->setTitle(temp + title);

    id = ContentManager::getInstance()->addContainerChain(_("/Audio/All - full name"), _(UPNP_DEFAULT_CLASS_MUSIC_CONT));
    add(obj, id);

    chain = _("/Audio/Artists/") + artist + "/All - full name";
    id = ContentManager::getInstance()->addContainerChain(chain, _(UPNP_DEFAULT_CLASS_MUSIC_CONT));
    add(obj, id);


}

FallbackLayout::FallbackLayout() : Layout()
{
}

void FallbackLayout::processCdsObject(zmm::Ref<CdsObject> obj)
{
    log_debug("Process CDS Object: %s\n", obj->getTitle().c_str());
    Ref<CdsObject> clone = CdsObject::createObject(OBJECT_TYPE_ITEM);
    obj->copyTo(clone);
    clone->setVirtual(1);

    String mimetype = RefCast(obj, CdsItem)->getMimeType();
    if (mimetype.startsWith(_("video")))
        addVideo(clone);
    else if (mimetype.startsWith(_("image")))
        addImage(clone);
    else if (mimetype.startsWith(_("audio")) || (mimetype == "application/ogg"))
        addAudio(clone);
}
