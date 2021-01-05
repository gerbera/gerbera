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

#ifdef HAVE_EXIV2
#include "exiv2_handler.h" // API

#include <exiv2/exiv2.hpp>
#include <utility>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "iohandler/io_handler.h"
#include "util/string_converter.h"
#include "util/tools.h"

Exiv2Handler::Exiv2Handler(std::shared_ptr<Config> config, std::shared_ptr<Mime> mime)
    : MetadataHandler(std::move(config), std::move(mime))
{
}

void Exiv2Handler::fillMetadata(std::shared_ptr<CdsItem> item)
{
    try {
        std::string value;
        const auto sc = StringConverter::m2i(CFG_IMPORT_LIBOPTS_EXIV2_CHARSET, item->getLocation(), config);

        const auto image = Exiv2::ImageFactory::open(std::string(item->getLocation().c_str()));
        image->readMetadata();
        Exiv2::ExifData& exifData = image->exifData();
        Exiv2::XmpData& xmpData = image->xmpData();

        // first retrieve jpeg comment
        std::string comment = const_cast<char*>(image->comment().c_str());

        if (exifData.empty()) {
            // no exiv2 record found in image
            return;
        }

        // get date/time
        auto md = exifData.findKey(Exiv2::ExifKey("Exif.Photo.DateTimeOriginal"));
        if (md != exifData.end()) {
            value = sc->convert(const_cast<char*>(md->toString().c_str()));

            /// \todo convert date to ISO 8601 as required in the UPnP spec
            // from YYYY:MM:DD to YYYY-MM-DD
            if (value.length() >= 11) {
                value = value.substr(0, 4) + "-" + value.substr(5, 2) + "-" + value.substr(8, 2);
                log_debug("date: {}", value.c_str());
                item->setMetadata(M_DATE, value);
            }
        }

        // if there was no jpeg comment, look if there is an exiv2 comment
        // should we override the normal jpeg comment, if there is an exiv2 one?
        if (comment.empty()) {
            md = exifData.findKey(Exiv2::ExifKey("Exif.Photo.UserComment"));
            if (md != exifData.end())
                comment = const_cast<char*>(md->toString().c_str());
            log_debug("Comment: {}", comment.c_str());
        }

        // if the image has no comment, compose something nice out of the exiv information
        // Not convinced that this is useful - comment it out for now ...
        /*    if (comment.empty())
        {
            std::string cam_model;
            std::string flash;
            std::string focal_length;

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


            if (!cam_model.empty())
                comment = "Taken with " + cam_model;

            if (!flash.empty())
            {
                if (!comment.empty())
                    comment = comment + ", Flash setting:" + flash;
                else
                    comment = "Flash setting: " + flash;
            }

            if (!focal_length.empty())
            {
                if (!comment.empty())
                    comment = comment + ", Focal length: " + focal_length;
                else
                    comment = "Focal length: " + focal_length;
            }
        log_debug("Fabricated Comment: {}", comment.c_str());
        }  */

        if (!comment.empty())
            item->setMetadata(M_DESCRIPTION, sc->convert(comment));

        // if there are any auxilary tags that the user wants - add them
        const auto aux = config->getArrayOption(CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST);
        if (!aux.empty()) {
            std::string value;
            std::string auxtag;

            for (const auto& j : aux) {
                value = "";
                auxtag = j;
                log_debug("auxtag: {} ", auxtag.c_str());
                if (auxtag.substr(0, 4) == "Exif") {
                    const auto md = exifData.findKey(Exiv2::ExifKey(auxtag));
                    if (md != exifData.end())
                        value = const_cast<char*>(md->toString().c_str());
                } else if (auxtag.substr(0, 3) == "Xmp") {
                    const auto md = xmpData.findKey(Exiv2::XmpKey(auxtag));
                    if (md != xmpData.end())
                        value = const_cast<char*>(md->toString().c_str());
                } else {
                    log_debug("Invalid Aux Tag {}", auxtag.c_str());
                    break;
                }
                if (!value.empty()) {
                    value = sc->convert(value);
                    item->setAuxData(auxtag, value);
                    log_debug("Adding aux tag: {} with value {}", auxtag.c_str(), value.c_str());
                }
            }

        } else {
            log_debug("No aux data requested");
        }

    } catch (Exiv2::AnyError& ex) {
        log_warning("Caught Exiv2 exception processing {}: '{}'", item->getLocation().c_str(), ex.what());
    }
}

std::unique_ptr<IOHandler> Exiv2Handler::serveContent(std::shared_ptr<CdsItem> item, int resNum)
{
    return nullptr;
}

#endif // HAVE_EXIV2
