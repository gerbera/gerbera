/*MT*

    MediaTomb - http://www.mediatomb.cc/

    libexif_handler.cc - this file is part of MediaTomb.

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

/// \file libexif_handler.cc

#ifdef HAVE_LIBEXIF
#include "libexif_handler.h" // API

#include <array>

#include "cds_objects.h"
#include "iohandler/file_io_handler.h"
#include "iohandler/mem_io_handler.h"
#include "util/jpeg_resolution.h"
#include "util/string_converter.h"
#include "util/tools.h"

/// \brief Sets resolution for a given resource index, item must be a JPEG image
static void setJpegResolutionResource(const std::shared_ptr<CdsItem>& item, std::size_t resNum = 0)
{
    if (resNum >= item->getResourceCount()) {
        log_warning("Invalid resource {} index {}", item->getLocation().c_str(), resNum);
        return;
    }
    try {
        auto fioH = FileIOHandler(item->getLocation());
        fioH.open(UPNP_READ);
        auto resolution = getJpegResolution(fioH);

        item->getResource(resNum)->addAttribute(CdsResource::Attribute::RESOLUTION, resolution.string());

    } catch (const std::runtime_error& e) {
        log_error("Failed to parse EXIF {} resolution: {}", item->getLocation().c_str(), e.what());
    }
}

static const std::map<std::string, int> exifTagMap {
    { "EXIF_TAG_INTEROPERABILITY_INDEX", EXIF_TAG_INTEROPERABILITY_INDEX },
    { "EXIF_TAG_INTEROPERABILITY_VERSION", EXIF_TAG_INTEROPERABILITY_VERSION },
    { "EXIF_TAG_IMAGE_WIDTH", EXIF_TAG_IMAGE_WIDTH },
    { "EXIF_TAG_IMAGE_LENGTH", EXIF_TAG_IMAGE_LENGTH },
    { "EXIF_TAG_BITS_PER_SAMPLE", EXIF_TAG_BITS_PER_SAMPLE },
    { "EXIF_TAG_COMPRESSION", EXIF_TAG_COMPRESSION },
    { "EXIF_TAG_PHOTOMETRIC_INTERPRETATION", EXIF_TAG_PHOTOMETRIC_INTERPRETATION },
    { "EXIF_TAG_FILL_ORDER", EXIF_TAG_FILL_ORDER },
    { "EXIF_TAG_DOCUMENT_NAME", EXIF_TAG_DOCUMENT_NAME },
    { "EXIF_TAG_IMAGE_DESCRIPTION", EXIF_TAG_IMAGE_DESCRIPTION },
    { "EXIF_TAG_MAKE", EXIF_TAG_MAKE },
    { "EXIF_TAG_MODEL", EXIF_TAG_MODEL },
    { "EXIF_TAG_STRIP_OFFSETS", EXIF_TAG_STRIP_OFFSETS },
    { "EXIF_TAG_ORIENTATION", EXIF_TAG_ORIENTATION },
    { "EXIF_TAG_SAMPLES_PER_PIXEL", EXIF_TAG_SAMPLES_PER_PIXEL },
    { "EXIF_TAG_ROWS_PER_STRIP", EXIF_TAG_ROWS_PER_STRIP },
    { "EXIF_TAG_STRIP_BYTE_COUNTS", EXIF_TAG_STRIP_BYTE_COUNTS },
    { "EXIF_TAG_X_RESOLUTION", EXIF_TAG_X_RESOLUTION },
    { "EXIF_TAG_Y_RESOLUTION", EXIF_TAG_Y_RESOLUTION },
    { "EXIF_TAG_PLANAR_CONFIGURATION", EXIF_TAG_PLANAR_CONFIGURATION },
    { "EXIF_TAG_RESOLUTION_UNIT", EXIF_TAG_RESOLUTION_UNIT },
    { "EXIF_TAG_TRANSFER_FUNCTION", EXIF_TAG_TRANSFER_FUNCTION },
    { "EXIF_TAG_SOFTWARE", EXIF_TAG_SOFTWARE },
    { "EXIF_TAG_DATE_TIME", EXIF_TAG_DATE_TIME },
    { "EXIF_TAG_ARTIST", EXIF_TAG_ARTIST },
    { "EXIF_TAG_WHITE_POINT", EXIF_TAG_WHITE_POINT },
    { "EXIF_TAG_PRIMARY_CHROMATICITIES", EXIF_TAG_PRIMARY_CHROMATICITIES },
    { "EXIF_TAG_TRANSFER_RANGE", EXIF_TAG_TRANSFER_RANGE },
    { "EXIF_TAG_JPEG_PROC", EXIF_TAG_JPEG_PROC },
    { "EXIF_TAG_JPEG_INTERCHANGE_FORMAT", EXIF_TAG_JPEG_INTERCHANGE_FORMAT },
    { "EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH", EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH },
    { "EXIF_TAG_YCBCR_COEFFICIENTS", EXIF_TAG_YCBCR_COEFFICIENTS },
    { "EXIF_TAG_YCBCR_SUB_SAMPLING", EXIF_TAG_YCBCR_SUB_SAMPLING },
    { "EXIF_TAG_YCBCR_POSITIONING", EXIF_TAG_YCBCR_POSITIONING },
    { "EXIF_TAG_REFERENCE_BLACK_WHITE", EXIF_TAG_REFERENCE_BLACK_WHITE },
    { "EXIF_TAG_RELATED_IMAGE_FILE_FORMAT", EXIF_TAG_RELATED_IMAGE_FILE_FORMAT },
    { "EXIF_TAG_RELATED_IMAGE_WIDTH", EXIF_TAG_RELATED_IMAGE_WIDTH },
    { "EXIF_TAG_RELATED_IMAGE_LENGTH", EXIF_TAG_RELATED_IMAGE_LENGTH },
    { "EXIF_TAG_CFA_REPEAT_PATTERN_DIM", EXIF_TAG_CFA_REPEAT_PATTERN_DIM },
    { "EXIF_TAG_CFA_PATTERN", EXIF_TAG_CFA_PATTERN },
    { "EXIF_TAG_BATTERY_LEVEL", EXIF_TAG_BATTERY_LEVEL },
    { "EXIF_TAG_COPYRIGHT", EXIF_TAG_COPYRIGHT },
    { "EXIF_TAG_EXPOSURE_TIME", EXIF_TAG_EXPOSURE_TIME },
    { "EXIF_TAG_FNUMBER", EXIF_TAG_FNUMBER },
    { "EXIF_TAG_IPTC_NAA", EXIF_TAG_IPTC_NAA },
    { "EXIF_TAG_EXIF_IFD_POINTER", EXIF_TAG_EXIF_IFD_POINTER },
    { "EXIF_TAG_INTER_COLOR_PROFILE", EXIF_TAG_INTER_COLOR_PROFILE },
    { "EXIF_TAG_EXPOSURE_PROGRAM", EXIF_TAG_EXPOSURE_PROGRAM },
    { "EXIF_TAG_SPECTRAL_SENSITIVITY", EXIF_TAG_SPECTRAL_SENSITIVITY },
    { "EXIF_TAG_GPS_INFO_IFD_POINTER", EXIF_TAG_GPS_INFO_IFD_POINTER },
    { "EXIF_TAG_ISO_SPEED_RATINGS", EXIF_TAG_ISO_SPEED_RATINGS },
    { "EXIF_TAG_OECF", EXIF_TAG_OECF },
    { "EXIF_TAG_EXIF_VERSION", EXIF_TAG_EXIF_VERSION },
    { "EXIF_TAG_DATE_TIME_ORIGINAL", EXIF_TAG_DATE_TIME_ORIGINAL },
    { "EXIF_TAG_DATE_TIME_DIGITIZED", EXIF_TAG_DATE_TIME_DIGITIZED },
    { "EXIF_TAG_COMPONENTS_CONFIGURATION", EXIF_TAG_COMPONENTS_CONFIGURATION },
    { "EXIF_TAG_COMPRESSED_BITS_PER_PIXEL", EXIF_TAG_COMPRESSED_BITS_PER_PIXEL },
    { "EXIF_TAG_SHUTTER_SPEED_VALUE", EXIF_TAG_SHUTTER_SPEED_VALUE },
    { "EXIF_TAG_APERTURE_VALUE", EXIF_TAG_APERTURE_VALUE },
    { "EXIF_TAG_BRIGHTNESS_VALUE", EXIF_TAG_BRIGHTNESS_VALUE },
    { "EXIF_TAG_EXPOSURE_BIAS_VALUE", EXIF_TAG_EXPOSURE_BIAS_VALUE },
    { "EXIF_TAG_MAX_APERTURE_VALUE", EXIF_TAG_MAX_APERTURE_VALUE },
    { "EXIF_TAG_SUBJECT_DISTANCE", EXIF_TAG_SUBJECT_DISTANCE },
    { "EXIF_TAG_METERING_MODE", EXIF_TAG_METERING_MODE },
    { "EXIF_TAG_LIGHT_SOURCE", EXIF_TAG_LIGHT_SOURCE },
    { "EXIF_TAG_FLASH", EXIF_TAG_FLASH },
    { "EXIF_TAG_FOCAL_LENGTH", EXIF_TAG_FOCAL_LENGTH },
    { "EXIF_TAG_SUBJECT_AREA", EXIF_TAG_SUBJECT_AREA },
    { "EXIF_TAG_MAKER_NOTE", EXIF_TAG_MAKER_NOTE },
    { "EXIF_TAG_USER_COMMENT", EXIF_TAG_USER_COMMENT },
    /* { "EXIF_TAG_SUBSEC_TIME", EXIF_TAG_SUBSEC_TIME }, */
    { "EXIF_TAG_SUB_SEC_TIME_ORIGINAL", EXIF_TAG_SUB_SEC_TIME_ORIGINAL },
    { "EXIF_TAG_SUB_SEC_TIME_DIGITIZED", EXIF_TAG_SUB_SEC_TIME_DIGITIZED },
    { "EXIF_TAG_FLASH_PIX_VERSION", EXIF_TAG_FLASH_PIX_VERSION },
    { "EXIF_TAG_COLOR_SPACE", EXIF_TAG_COLOR_SPACE },
    { "EXIF_TAG_PIXEL_X_DIMENSION", EXIF_TAG_PIXEL_X_DIMENSION },
    { "EXIF_TAG_PIXEL_Y_DIMENSION", EXIF_TAG_PIXEL_Y_DIMENSION },
    { "EXIF_TAG_RELATED_SOUND_FILE", EXIF_TAG_RELATED_SOUND_FILE },
    { "EXIF_TAG_INTEROPERABILITY_IFD_POINTER", EXIF_TAG_INTEROPERABILITY_IFD_POINTER },
    { "EXIF_TAG_FLASH_ENERGY", EXIF_TAG_FLASH_ENERGY },
    { "EXIF_TAG_SPATIAL_FREQUENCY_RESPONSE", EXIF_TAG_SPATIAL_FREQUENCY_RESPONSE },
    { "EXIF_TAG_FOCAL_PLANE_X_RESOLUTION", EXIF_TAG_FOCAL_PLANE_X_RESOLUTION },
    { "EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION", EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION },
    { "EXIF_TAG_FOCAL_PLANE_RESOLUTION_UNIT", EXIF_TAG_FOCAL_PLANE_RESOLUTION_UNIT },
    { "EXIF_TAG_SUBJECT_LOCATION", EXIF_TAG_SUBJECT_LOCATION },
    { "EXIF_TAG_EXPOSURE_INDEX", EXIF_TAG_EXPOSURE_INDEX },
    { "EXIF_TAG_SENSING_METHOD", EXIF_TAG_SENSING_METHOD },
    { "EXIF_TAG_FILE_SOURCE", EXIF_TAG_FILE_SOURCE },
    { "EXIF_TAG_SCENE_TYPE", EXIF_TAG_SCENE_TYPE },
    { "EXIF_TAG_NEW_CFA_PATTERN", EXIF_TAG_NEW_CFA_PATTERN },
    { "EXIF_TAG_CUSTOM_RENDERED", EXIF_TAG_CUSTOM_RENDERED },
    { "EXIF_TAG_EXPOSURE_MODE", EXIF_TAG_EXPOSURE_MODE },
    { "EXIF_TAG_WHITE_BALANCE", EXIF_TAG_WHITE_BALANCE },
    { "EXIF_TAG_DIGITAL_ZOOM_RATIO", EXIF_TAG_DIGITAL_ZOOM_RATIO },
    { "EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM", EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM },
    { "EXIF_TAG_SCENE_CAPTURE_TYPE", EXIF_TAG_SCENE_CAPTURE_TYPE },
    { "EXIF_TAG_GAIN_CONTROL", EXIF_TAG_GAIN_CONTROL },
    { "EXIF_TAG_CONTRAST", EXIF_TAG_CONTRAST },
    { "EXIF_TAG_SATURATION", EXIF_TAG_SATURATION },
    { "EXIF_TAG_SHARPNESS", EXIF_TAG_SHARPNESS },
    { "EXIF_TAG_DEVICE_SETTING_DESCRIPTION", EXIF_TAG_DEVICE_SETTING_DESCRIPTION },
    { "EXIF_TAG_SUBJECT_DISTANCE_RANGE", EXIF_TAG_SUBJECT_DISTANCE_RANGE },
    { "EXIF_TAG_IMAGE_UNIQUE_ID", EXIF_TAG_IMAGE_UNIQUE_ID },
};

static int getTagFromString(const std::string& tag)
{
    auto result = getValueOrDefault(exifTagMap, tag, -1);

    if (result == -1)
        log_warning("Ignoring unknown libexif tag: {}", tag);
    return result;
}

void LibExifHandler::process_ifd(const ExifContent* content, const std::shared_ptr<CdsItem>& item,
    const std::unique_ptr<StringConverter>& sc, const std::vector<std::string>& auxtags, const std::map<std::string, std::string>& metatags)
{
    constexpr auto BUFLEN = 4096;
    std::array<char, BUFLEN> exif_entry_buffer;
#define exif_egv(arg) exif_entry_get_value(arg, exif_entry_buffer.data(), BUFLEN)

    for (std::size_t i = 0; i < content->count; i++) {
        ExifEntry* entry = content->entries[i];

        switch (entry->tag) {
        case EXIF_TAG_DATE_TIME_ORIGINAL: {
            auto value = trimString(exif_egv(entry));
            log_debug("Found exif date: {}", value);
            if (!value.empty()) {
                value = sc->convert(value);
                // convert date to ISO 8601 as required in the UPnP spec
                // from YYYY:MM:DD to YYYY-MM-DD
                if (value.length() >= 19) {
                    item->addMetaData(M_DATE, fmt::format("{}-{}-{}T{}", value.substr(0, 4), value.substr(5, 2), value.substr(8, 2), value.substr(11, 8)));
                } else if (value.length() >= 11) {
                    item->addMetaData(M_DATE, fmt::format("{}-{}-{}", value.substr(0, 4), value.substr(5, 2), value.substr(8, 2)));
                }
            }
            break;
        }
        case EXIF_TAG_USER_COMMENT: {
            auto value = trimString(exif_egv(entry));
            log_debug("Found exif comment: {}", value);
            if (!value.empty()) {
                item->addMetaData(M_DESCRIPTION, sc->convert(value));
            }
            break;
        }
        case EXIF_TAG_PIXEL_X_DIMENSION: {
            auto value = trimString(exif_egv(entry));
            log_debug("Found exif X dimension: {}", value);
            if (!value.empty()) {
                imageX = sc->convert(value);
            }
            break;
        }
        case EXIF_TAG_PIXEL_Y_DIMENSION: {
            auto value = trimString(exif_egv(entry));
            log_debug("Found exif Y dimension: {}", value);
            if (!value.empty()) {
                imageY = sc->convert(value);
            }
            break;
        }
        default:
            break;
        }

        // if there are any metadata tags that the user wants - add them
        for (auto&& [tag, key] : metatags) {
            if (tag.empty() || entry->tag != getTagFromString(tag))
                continue;

            auto value = trimString(exif_egv(entry));
            if (!value.empty()) {
                item->addMetaData(key, sc->convert(value));
                log_debug("Adding tag '{}' as '{}' with value '{}'", tag, key, value);
            }
        }
        // if there are any auxilary tags that the user wants - add them
        for (auto&& aux : auxtags) {
            if (aux.empty() || entry->tag != getTagFromString(aux))
                continue;

            auto value = trimString(exif_egv(entry));
            if (!value.empty()) {
                item->setAuxData(aux, sc->convert(value));
                log_debug("Adding aux: {} with value {}", aux, value);
            }
        }
    }
}

void LibExifHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return;

    auto sc = StringConverter::m2i(CFG_IMPORT_LIBOPTS_EXIF_CHARSET, item->getLocation(), config);
    ExifData* exifData = exif_data_new_from_file(item->getLocation().c_str());

    if (exifData == nullptr) {
        log_debug("Exif data not found, attempting to set resolution internally...");
        setJpegResolutionResource(item);
        return;
    }

    auto aux = config->getArrayOption(CFG_IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST);
    auto meta = config->getDictionaryOption(CFG_IMPORT_LIBOPTS_EXIF_METADATA_TAGS_LIST);
    for (auto* exifContent : exifData->ifd) {
        if (exifContent != nullptr) {
            process_ifd(exifContent, item, sc, aux, meta);
        }
    }

    // we got the image resolution so we can add our resource
    if (!imageX.empty() && !imageY.empty()) {
        item->getResource(ContentHandler::DEFAULT)->addAttribute(CdsResource::Attribute::RESOLUTION, fmt::format("{}x{}", imageX, imageY));
    } else {
        log_debug("EXIF resolution not found, falling back to parsing directly: {}", item->getLocation().c_str());
        setJpegResolutionResource(item);
    }

    if (exifData->size != 0U) {
        auto resource = std::make_shared<CdsResource>(ContentHandler::LIBEXIF, CdsResource::Purpose::Thumbnail);
        resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo(item->getMimeType()));
        item->addResource(resource);
        try {
            auto ioH = MemIOHandler(exifData->data, exifData->size);
            ioH.open(UPNP_READ);
            auto thResolution = getJpegResolution(ioH);
            log_debug("EXIF Thumb Resolution: {}", thResolution.string());
            resource->addAttribute(CdsResource::Attribute::RESOLUTION, thResolution.string());
        } catch (const std::runtime_error& e) {
            log_error("Failed to parse EXIF Thumbnail {} details: {}", item->getLocation().c_str(), e.what());
        }
    } // (exifData->size)
    exif_data_unref(exifData);
}

std::unique_ptr<IOHandler> LibExifHandler::serveContent(const std::shared_ptr<CdsObject>& obj, int resNum)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return nullptr;

    auto res = item->getResource(resNum);

    if (res->getPurpose() != CdsResource::Purpose::Thumbnail)
        throw_std_runtime_error("Resource {} is not a Thumbnail", resNum);

    ExifData* ed = exif_data_new_from_file(item->getLocation().c_str());
    if (!ed)
        throw_std_runtime_error("Resource {} has no exif information", resNum);

    if (!(ed->size))
        throw_std_runtime_error("Resource {} has no exif thumbnail", resNum);

    auto ioHandler = std::make_unique<MemIOHandler>(ed->data, ed->size);
    exif_data_unref(ed);
    return ioHandler;
}
#endif // HAVE_LIBEXIF
