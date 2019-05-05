/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    exiv2_handler.cc - this file is part of MediaTomb.
    
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

/// \file exiv2_handler.cc
/// \brief Implementeation of the Exiv2Handler class.

#ifdef HAVE_EXIV2
#include <exiv2/exiv2.hpp>

#include "config_manager.h"
#include "exiv2_handler.h"
#include "string_converter.h"
#include "tools.h"

using namespace zmm;

Exiv2Handler::Exiv2Handler()
    : MetadataHandler()
{
}

void Exiv2Handler::fillMetadata(Ref<CdsItem> item)
{
    try {
        String value;
        Ref<StringConverter> sc = StringConverter::m2i();

        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(std::string(item->getLocation().c_str()));
        image->readMetadata();
        Exiv2::ExifData& exifData = image->exifData();
        Exiv2::XmpData& xmpData = image->xmpData();

        // first retrieve jpeg comment
        String comment = (char*)image->comment().c_str();

        if (exifData.empty()) {
            // no exiv2 record found in image
            return;
        }

        // get date/time
        Exiv2::ExifData::const_iterator md = exifData.findKey(Exiv2::ExifKey("Exif.Photo.DateTimeOriginal"));
        if (md != exifData.end()) {
            value = sc->convert((char*)md->toString().c_str());

            /// \todo convert date to ISO 8601 as required in the UPnP spec
            // from YYYY:MM:DD to YYYY-MM-DD
            if (value.length() >= 11) {
                value = value.substring(0, 4) + "-" + value.substring(5, 2) + "-" + value.substring(8, 2);
                log_debug("date: %s\n", value.c_str());
                item->setMetadata(MetadataHandler::getMetaFieldName(M_DATE), value);
            }
        }

        // if there was no jpeg comment, look if there is an exiv2 comment
        // should we override the normal jpeg comment, if there is an exiv2 one?
        if (!string_ok(comment)) {
            md = exifData.findKey(Exiv2::ExifKey("Exif.Photo.UserComment"));
            if (md != exifData.end())
                comment = (char*)md->toString().c_str();
            log_debug("Comment: %s\n", comment.c_str());
        }

        // if the image has no comment, compose something nice out of the exiv information
        // Not convinced that this is useful - comment it out for now ...
        /*    if (!string_ok(comment))
        {
            String cam_model;
            String flash;
            String focal_length;

            md = exifData.findKey(Exiv2::ExifKey("Exif.Image.Model"));
            if (md !=  exifData.end())
                cam_model = (char *)md->toString().c_str();

            md = exifData.findKey(Exiv2::ExifKey("Exif.Photo.Flash"));
            if (md !=  exifData.end())
                flash = (char *)md->toString().c_str();

            md = exifData.findKey(Exiv2::ExifKey("Exif.Photo.FocalLength"));
            if (md !=  exifData.end())
            {
                focal_length = (char *)md->toString().c_str();
                md = exifData.findKey(Exiv2::ExifKey("Exif.Photo.FocalLengthIn35mmFilm"));
                if (md !=  exifData.end())
                {
                    focal_length = focal_length + " (35 mm equivalent: " + (char *)md->toString().c_str() + ")";
                }
            }


            if (string_ok(cam_model))
                comment = _("Taken with ") + cam_model;

            if (string_ok(flash))
            {
                if (string_ok(comment))
                    comment = comment + ", Flash setting:" + flash;
                else
                    comment = _("Flash setting: ") + flash;
            }

            if (string_ok(focal_length))
            {
                if (string_ok(comment))
                    comment = comment + ", Focal length: " + focal_length;
                else
                    comment = _("Focal length: ") + focal_length;
            }
        log_debug("Fabricated Comment: %s\n", comment.c_str());
        }  */

        if (string_ok(comment))
            item->setMetadata(MT_KEYS[M_DESCRIPTION].upnp, sc->convert(comment));

        // if there are any auxilary tags that the user wants - add them
        Ref<Array<StringBase>> aux;
        Ref<ConfigManager> cm = ConfigManager::getInstance();

        aux = cm->getStringArrayOption(CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST);
        if (aux != nullptr) {
            String value;
            String auxtag;

            for (int j = 0; j < aux->size(); j++) {
                value = "";
                auxtag = aux->get(j);
                log_debug("auxtag: %s \n", auxtag.c_str());
                if (auxtag.substring(0, 4) == "Exif") {
                    Exiv2::ExifData::const_iterator md = exifData.findKey(Exiv2::ExifKey(auxtag.c_str()));
                    if (md != exifData.end())
                        value = (char*)md->toString().c_str();
                } else if (auxtag.substring(0, 3) == "Xmp") {
                    Exiv2::XmpData::const_iterator md = xmpData.findKey(Exiv2::XmpKey(auxtag.c_str()));
                    if (md != xmpData.end())
                        value = (char*)md->toString().c_str();
                } else {
                    log_debug("Invalid Aux Tag %s\n", auxtag.c_str());
                    break;
                }
                if (string_ok(value)) {
                    value = sc->convert(value);
                    item->setAuxData(auxtag, value);
                    log_debug("Adding aux tag: %s with value %s\n", auxtag.c_str(), value.c_str());
                }
            }

        } else {
            log_debug("No aux data requested\n");
        }

    } catch (Exiv2::AnyError& ex) {
        log_warning("Caught Exiv2 exception processing %s: '%s'\n", item->getLocation().c_str(), ex.what());
    }
}

Ref<IOHandler> Exiv2Handler::serveContent(Ref<CdsItem> item, int resNum, off_t* data_size)
{
    return nullptr;
}

#endif // HAVE_EXIV2
