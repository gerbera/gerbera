/*MT*

    MediaTomb - http://www.mediatomb.cc/

    exiv2_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2025 Gerbera Contributors

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

#include "cds/cds_item.h"
#include "config/config_val.h"
#include "context.h"
#include "iohandler/io_handler.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include <exiv2/exiv2.hpp>

#if EXIV2_TEST_VERSION(0, 28, 0)
#define AnyError Error
#endif

Exiv2Handler::Exiv2Handler(const std::shared_ptr<Context>& context)
    : MediaMetadataHandler(context,
          ConfigVal::IMPORT_LIBOPTS_EXIV2_ENABLED,
          ConfigVal::IMPORT_LIBOPTS_EXIV2_METADATA_TAGS_LIST,
          ConfigVal::IMPORT_LIBOPTS_EXIV2_AUXDATA_TAGS_LIST,
          ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_ENABLED,
          ConfigVal::IMPORT_LIBOPTS_EXIV2_COMMENT_LIST)
{
    // silence exiv2 messages without debug
    Exiv2::LogMsg::setHandler([](auto, auto s) { log_debug("Exiv2: {}", s); });
}

/// \brief Wrapper class to interface with EXIV2
class Exiv2Object {
public:
    std::string location;
    std::shared_ptr<StringConverter> sc;
#if EXIV2_TEST_VERSION(0, 28, 0)
    Exiv2::Image::UniquePtr image;
#else
    Exiv2::Image::AutoPtr image;
#endif
    Exiv2::ExifData* exifData;
    Exiv2::XmpData* xmpData;
    std::string comment;

    Exiv2Object(const std::shared_ptr<ConverterManager>& converterManager, const std::shared_ptr<CdsItem>& item)
        : location(item->getLocation())
        , sc(converterManager->m2i(ConfigVal::IMPORT_LIBOPTS_EXIV2_CHARSET, location))
        , image(Exiv2::ImageFactory::open(location))
    {
#if EXIV2_TEST_VERSION(0, 28, 0)
        if (!image) { // is unique_ptr now
            log_warning("Exiv2::ImageFactory could not open {}", location);
            return;
        }
#else
        if (image.get() == nullptr) { // used to be auto_ptr
            log_warning("Exiv2::ImageFactory could not open {}", location);
            return;
        }
#endif
        image->readMetadata();
        exifData = &(image->exifData());
        xmpData = &(image->xmpData());
        // first retrieve jpeg comment
        comment = image->comment();
    }

    /// \brief get date/time
    std::string getDate() const
    {
        std::string value = getKey("Exif.Photo.DateTimeOriginal");
        if (!value.empty()) {
            // convert date to ISO 8601 as required in the UPnP spec
            // from YYYY:MM:DD to YYYY-MM-DD
            if (value.length() >= 19) {
                value = fmt::format("{}-{}-{}T{}", value.substr(0, 4), value.substr(5, 2), value.substr(8, 2), value.substr(11, 8));
            } else if (value.length() >= 11) {
                value = fmt::format("{}-{}-{}", value.substr(0, 4), value.substr(5, 2), value.substr(8, 2));
            }
            log_debug("{} date: {}", location, value);
        }
        return value;
    }

    /// \brief get key value from image
    std::string getKey(const std::string& key) const
    {
        log_debug("key: {} ", key);
        std::string keyValue;
        if (startswith(key, "Exif")) {
            auto md = exifData->findKey(Exiv2::ExifKey(key));
            if (md != exifData->end())
                keyValue = md->toString();
        } else if (startswith(key, "Xmp")) {
            auto xmpMd = xmpData->findKey(Exiv2::XmpKey(key));
            if (xmpMd != xmpData->end())
                keyValue = xmpMd->toString();
        } else {
            log_warning("Invalid key {}", key);
        }
        if (!keyValue.empty()) {
            auto [value, err] = sc->convert(keyValue);
            if (!err.empty()) {
                log_warning("{} {}: {}", key, location, err);
            }
            return trimString(value);
        }
        return "";
    }
};

void Exiv2Handler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item || !isEnabled)
        return;

    try {
        Exiv2Object exivObj(converterManager, item);
        if (exivObj.exifData->empty()) {
            // no exiv2 record found in image
            return;
        }

        auto date = exivObj.getDate();
        if (!date.empty()) {
            item->addMetaData(MetadataFields::M_DATE, date);
        }

        auto orientation = exivObj.getKey("Exif.Image.Orientation");
        if (!orientation.empty()) {
            item->getResource(ContentHandler::DEFAULT)->addAttribute(ResourceAttribute::ORIENTATION, orientation);
        }

        // if there was no jpeg comment, look if there is an exiv2 comment
        // should we override the normal jpeg comment, if there is an exiv2 one?
        if (exivObj.comment.empty()) {
            exivObj.comment = exivObj.getKey("Exif.Photo.UserComment");
        }

        // if the image has no comment, compose something nice out of the exiv information
        if (exivObj.comment.empty() && isCommentEnabled) {
            std::vector<std::string> snippets;
            for (auto&& [label, tag] : commentMap) {
                std::string value = exivObj.getKey(tag);
                if (!value.empty()) {
                    snippets.push_back(fmt::format("{}: {}", label, value));
                }
            }
            exivObj.comment = fmt::format("{}", fmt::join(snippets, ", "));
            log_debug("Fabricated Comment: {}", exivObj.comment);
        }

        if (!exivObj.comment.empty()) {
            item->addMetaData(MetadataFields::M_DESCRIPTION, exivObj.comment);
        }

        // if there are any metadata tags that the user wants - add them
        if (!metaTags.empty()) {
            for (auto&& [metatag, metakey] : metaTags) {
                log_debug("metatag: {} ", metatag);
                std::string metaval = exivObj.getKey(metatag);
                if (!metaval.empty()) {
                    item->addMetaData(metakey, metaval);
                    log_debug("Adding meta tag '{}' as '{}' with value '{}'", metatag, metakey, metaval);
                }
            }
        } else {
            log_debug("No metadata requested");
        }

        // if there are any auxilary tags that the user wants - add them
        if (!auxTags.empty()) {
            for (auto&& auxtag : auxTags) {
                log_debug("auxtag: {} ", auxtag);
                std::string auxval = exivObj.getKey(auxtag);
                if (!auxval.empty()) {
                    item->setAuxData(auxtag, auxval);
                    log_debug("Adding aux tag: {} with value {}", auxtag, auxval);
                }
            }
        } else {
            log_debug("No aux data requested");
        }
    } catch (const Exiv2::AnyError& ex) {
        log_warning("Caught Exiv2 exception processing {}: '{}'", item->getLocation().string(), ex.what());
    } catch (const std::runtime_error& e) {
        log_warning("Caught exception processing {}: '{}'", item->getLocation().string(), e.what());
    }

    Exiv2::XmpProperties::unregisterNs();
}

std::unique_ptr<IOHandler> Exiv2Handler::serveContent(
    const std::shared_ptr<CdsObject>& obj,
    const std::shared_ptr<CdsResource>& resource)
{
    return nullptr;
}

#endif // HAVE_EXIV2
