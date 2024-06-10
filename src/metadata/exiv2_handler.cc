/*MT*

    MediaTomb - http://www.mediatomb.cc/

    exiv2_handler.cc - this file is part of MediaTomb.

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

/// \file exiv2_handler.cc

#ifdef HAVE_EXIV2
#define GRB_LOG_FAC GrbLogFacility::exiv2
#include "exiv2_handler.h" // API

#include <exiv2/exiv2.hpp>

#include "cds/cds_item.h"
#include "iohandler/io_handler.h"
#include "util/string_converter.h"
#include "util/tools.h"

#if EXIV2_TEST_VERSION(0, 28, 0)
#define AnyError Error
#endif

Exiv2Handler::Exiv2Handler(const std::shared_ptr<Context>& context)
    : MediaMetadataHandler(context, CFG_IMPORT_LIBOPTS_EXIV2_ENABLED, CFG_IMPORT_LIBOPTS_EXIV2_METADATA_TAGS_LIST, CFG_IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST)
{
    // silence exiv2 messages without debug
    Exiv2::LogMsg::setHandler([](auto, auto s) { log_debug("Exiv2: {}", s); });
}

void Exiv2Handler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item || !isEnabled)
        return;

    try {
        std::string value;
        const auto sc = StringConverter::m2i(CFG_IMPORT_LIBOPTS_EXIV2_CHARSET, item->getLocation(), config);

        const auto image = Exiv2::ImageFactory::open(item->getLocation());
        image->readMetadata();
        Exiv2::ExifData& exifData = image->exifData();
        Exiv2::XmpData& xmpData = image->xmpData();

        // first retrieve jpeg comment
        std::string comment = image->comment();

        if (exifData.empty()) {
            // no exiv2 record found in image
            return;
        }

        // get date/time
        auto md = exifData.findKey(Exiv2::ExifKey("Exif.Photo.DateTimeOriginal"));
        if (md != exifData.end()) {
            value = sc->convert(md->toString());

            /// \todo convert date to ISO 8601 as required in the UPnP spec
            // from YYYY:MM:DD to YYYY-MM-DD
            if (value.length() >= 19) {
                value = fmt::format("{}-{}-{}T{}", value.substr(0, 4), value.substr(5, 2), value.substr(8, 2), value.substr(11, 8));
                log_debug("date: {}", value);
                item->addMetaData(MetadataFields::M_DATE, value);
            } else if (value.length() >= 11) {
                value = fmt::format("{}-{}-{}", value.substr(0, 4), value.substr(5, 2), value.substr(8, 2));
                log_debug("date: {}", value);
                item->addMetaData(MetadataFields::M_DATE, value);
            }
        }

        // if there was no jpeg comment, look if there is an exiv2 comment
        // should we override the normal jpeg comment, if there is an exiv2 one?
        if (comment.empty()) {
            md = exifData.findKey(Exiv2::ExifKey("Exif.Photo.UserComment"));
            if (md != exifData.end())
                comment = md->toString();
            log_debug("Comment: {}", comment);
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

        if (!comment.empty()) {
            item->addMetaData(MetadataFields::M_DESCRIPTION, sc->convert(comment));
        }

        // if there are any metadata tags that the user wants - add them
        if (!metaTags.empty()) {
            for (auto&& [metatag, metakey] : metaTags) {
                std::string metaval;
                log_debug("metatag: {} ", metatag.c_str());
                if (startswith(metatag, "Exif")) {
                    md = exifData.findKey(Exiv2::ExifKey(metatag));
                    if (md != exifData.end())
                        metaval = trimString(md->toString());
                } else if (startswith(metatag, "Xmp")) {
                    auto xmpMd = xmpData.findKey(Exiv2::XmpKey(metatag));
                    if (xmpMd != xmpData.end())
                        metaval = trimString(xmpMd->toString());
                } else {
                    log_warning("Invalid meta Tag {}", metatag.c_str());
                    continue;
                }
                if (!metaval.empty()) {
                    metaval = sc->convert(metaval);
                    item->addMetaData(metakey, metaval);
                    log_debug("Adding meta tag '{}' as '{}' with value '{}'", metatag, metakey, metaval);
                }
            }
        } else {
            log_debug("No aux data requested");
        }

        // if there are any auxilary tags that the user wants - add them
        if (!auxTags.empty()) {
            for (auto&& auxtag : auxTags) {
                std::string auxval;
                log_debug("auxtag: {} ", auxtag.c_str());
                if (startswith(auxtag, "Exif")) {
                    md = exifData.findKey(Exiv2::ExifKey(auxtag));
                    if (md != exifData.end())
                        auxval = trimString(md->toString());
                } else if (startswith(auxtag, "Xmp")) {
                    auto xmpMd = xmpData.findKey(Exiv2::XmpKey(auxtag));
                    if (xmpMd != xmpData.end())
                        auxval = trimString(xmpMd->toString());
                } else {
                    log_warning("Invalid Aux Tag {}", auxtag.c_str());
                    continue;
                }
                if (!auxval.empty()) {
                    auxval = sc->convert(auxval);
                    item->setAuxData(auxtag, auxval);
                    log_debug("Adding aux tag: {} with value {}", auxtag.c_str(), auxval.c_str());
                }
            }

        } else {
            log_debug("No aux data requested");
        }
    } catch (const Exiv2::AnyError& ex) {
        log_warning("Caught Exiv2 exception processing {}: '{}'", item->getLocation().c_str(), ex.what());
    }

    Exiv2::XmpProperties::unregisterNs();
}

std::unique_ptr<IOHandler> Exiv2Handler::serveContent(const std::shared_ptr<CdsObject>& obj, const std::shared_ptr<CdsResource>& resource)
{
    return nullptr;
}

#endif // HAVE_EXIV2
