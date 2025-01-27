/*MT*

    MediaTomb - http://www.mediatomb.cc/

    libexif_handler.cc - this file is part of MediaTomb.

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

/// \file libexif_handler.cc

#ifdef HAVE_LIBEXIF
#define GRB_LOG_FAC GrbLogFacility::exif
#include "libexif_handler.h" // API

#include "cds/cds_item.h"
#include "config/config_val.h"
#include "exceptions.h"
#include "iohandler/file_io_handler.h"
#include "iohandler/mem_io_handler.h"
#include "util/jpeg_resolution.h"
#include "util/string_converter.h"
#include "util/tools.h"

#include <array>

/// \brief Sets resolution for a given resource index, item must be a JPEG image
static void setJpegResolutionResource(
    const std::shared_ptr<CdsItem>& item,
    std::size_t resNum = 0)
{
    if (resNum >= item->getResourceCount()) {
        log_warning("Invalid resource {} index {}", item->getLocation().c_str(), resNum);
        return;
    }
    try {
        auto fioH = FileIOHandler(item->getLocation());
        fioH.open(UPNP_READ);
        auto resolution = getJpegResolution(fioH);

        item->getResource(resNum)->addAttribute(ResourceAttribute::RESOLUTION, resolution.string());

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

static void logfunc(ExifLog* log, ExifLogCode code, const char* domain, const char* format, va_list args, void* data)
{
    va_list args_copy;
    va_copy(args_copy, args);
    size_t len = std::vsnprintf(nullptr, 0, format, args_copy);
    std::vector<char> buf(len + 1); // need space for NUL
    std::vsnprintf(buf.data(), len + 1, format, args);
    std::string message = buf.data();
    log_debug("Exif: {}-{}, domain {}: {}", exif_log_code_get_title(code), exif_log_code_get_message(code), domain, message);
}

LibExifHandler::LibExifHandler(const std::shared_ptr<Context>& context)
    : MediaMetadataHandler(context,
          ConfigVal::IMPORT_LIBOPTS_EXIF_ENABLED,
          ConfigVal::IMPORT_LIBOPTS_EXIF_METADATA_TAGS_LIST,
          ConfigVal::IMPORT_LIBOPTS_EXIF_AUXDATA_TAGS_LIST,
          ConfigVal::IMPORT_LIBOPTS_EXIF_COMMENT_ENABLED,
          ConfigVal::IMPORT_LIBOPTS_EXIF_COMMENT_LIST)
{
    log = exif_log_new();
    exif_log_set_func(log, logfunc, nullptr);
}

LibExifHandler::~LibExifHandler()
{
    exif_log_free(log);
}

/// \brief Wrapper class to interface with LibExif
class LibExifObject {
private:
    static constexpr std::size_t BUFLEN = 4096;
    std::array<char, BUFLEN> exif_entry_buffer;
#define exif_egv(arg) exif_entry_get_value(arg, exif_entry_buffer.data(), BUFLEN)

public:
    std::string location;
    std::shared_ptr<StringConverter> sc;
    ExifData* exifData = nullptr;

    LibExifObject(const std::shared_ptr<ConverterManager>& converterManager, const std::shared_ptr<CdsItem>& item)
        : location(item->getLocation())
        , sc(converterManager->m2i(ConfigVal::IMPORT_LIBOPTS_EXIF_CHARSET, location))
    {
        exifData = exif_data_new_from_file(location.c_str());
    }

    ~LibExifObject()
    {
        // Close the media file
        if (exifData)
            exif_data_unref(exifData);
    }

    /// \brief check if libexif information is available
    operator bool() const
    {
        return exifData;
    }
    /// \brief check if thumbnail information is available
    bool hasThumb() const
    {
        return exifData && exifData->size > 0U;
    }

    /// get iohandler to serve thumbnail
    std::unique_ptr<IOHandler> getThumbnail()
    {
        if (!exifData)
            throw_std_runtime_error("Resource {} has no exif information", location);

        if (!hasThumb()) {
            throw_std_runtime_error("Resource {} has no exif thumbnail", location);
        }

        return std::make_unique<MemIOHandler>(exifData->data, exifData->size);
    }

    /// \brief get date/time
    std::string getDate(ExifEntry* entry)
    {
        auto value = trimString(exif_egv(entry));
        log_debug("Found exif date: {}", value);
        if (!value.empty()) {
            auto [val, err] = sc->convert(value);
            if (!err.empty()) {
                log_warning("{}: {}", location, err);
            }
            // convert date to ISO 8601 as required in the UPnP spec
            // from YYYY:MM:DD to YYYY-MM-DD
            if (val.length() >= 19) {
                return val;
            } else if (val.length() >= 11) {
                return val;
            }
            return "";
        }
        return "";
    }

    /// \brief get key value from image
    std::string getKey(ExifEntry* entry)
    {
        auto value = trimString(exif_egv(entry));
        log_debug("Found exif value: {}", value);
        if (!value.empty()) {
            auto [val, err] = sc->convert(value);
            if (!err.empty()) {
                log_warning("{}: {}", location, err);
            }
            return val;
        }
        return "";
    }

    /// \brief get resolution for thumbnail
    std::string getThumbResolution()
    {
        try {
            auto ioH = MemIOHandler(exifData->data, exifData->size);
            ioH.open(UPNP_READ);
            auto thResolution = getJpegResolution(ioH);
            log_debug("EXIF Thumb Resolution: {}", thResolution.string());
            return thResolution.string();
        } catch (const std::runtime_error& e) {
            log_error("Failed to parse EXIF Thumbnail {} details: {}", location, e.what());
        }
        return "";
    }
};

void LibExifHandler::process_ifd(
    const ExifContent* content,
    const std::shared_ptr<CdsItem>& item,
    LibExifObject& exifObject,
    std::vector<std::string>& snippets,
    std::string& imageX,
    std::string& imageY)
{
    for (std::size_t i = 0; i < content->count; i++) {
        ExifEntry* entry = content->entries[i];

        switch (entry->tag) {
        case EXIF_TAG_DATE_TIME_ORIGINAL: {
            auto val = exifObject.getDate(entry);
            log_debug("Found exif date: {}", val);
            if (!val.empty()) {
                // convert date to ISO 8601 as required in the UPnP spec
                // from YYYY:MM:DD to YYYY-MM-DD
                if (val.length() >= 19) {
                    val = fmt::format("{}-{}-{}T{}", val.substr(0, 4), val.substr(5, 2), val.substr(8, 2), val.substr(11, 8));
                } else { // otherwise getDate returned empty string
                    val = fmt::format("{}-{}-{}", val.substr(0, 4), val.substr(5, 2), val.substr(8, 2));
                }
                item->addMetaData(MetadataFields::M_DATE, val);
            }
            break;
        }
        case EXIF_TAG_USER_COMMENT: {
            auto value = exifObject.getKey(entry);
            log_debug("Found exif comment: {}", value);
            if (!value.empty()) {
                item->addMetaData(MetadataFields::M_DESCRIPTION, value);
            }
            break;
        }
        case EXIF_TAG_PIXEL_X_DIMENSION: {
            auto value = exifObject.getKey(entry);
            log_debug("Found exif X dimension: {}", value);
            if (!value.empty()) {
                imageX = value;
            }
            break;
        }
        case EXIF_TAG_PIXEL_Y_DIMENSION: {
            auto value = exifObject.getKey(entry);
            log_debug("Found exif Y dimension: {}", value);
            if (!value.empty()) {
                imageY = value;
            }
            break;
        }
        case EXIF_TAG_ORIENTATION: {
            auto o = exif_data_get_byte_order(entry->parent->parent);
            auto value = exif_get_short(entry->data, o);
            log_debug("Found exif orientation {}", value);
            item->getResource(ContentHandler::DEFAULT)->addAttribute(ResourceAttribute::ORIENTATION, fmt::format("{}", value));
            break;
        }
        default:
            break;
        }

        // if there are any metadata tags that the user wants - add them
        for (auto&& [tag, key] : metaTags) {
            if (tag.empty() || entry->tag != getTagFromString(tag))
                continue;

            auto value = exifObject.getKey(entry);
            if (!value.empty()) {
                item->addMetaData(key, value);
                log_debug("Adding tag '{}' as '{}' with value '{}'", tag, key, value);
            }
        }
        // if there are any auxilary tags that the user wants - add them
        for (auto&& aux : auxTags) {
            if (aux.empty() || entry->tag != getTagFromString(aux))
                continue;

            auto value = exifObject.getKey(entry);
            if (!value.empty()) {
                item->setAuxData(aux, value);
                log_debug("Adding aux: {} with value {}", aux, value);
            }
        }
        // if user want a fabricated comment
        for (auto&& [label, tag] : commentMap) {
            if (tag.empty() || entry->tag != getTagFromString(tag))
                continue;

            auto value = exifObject.getKey(entry);
            if (!value.empty()) {
                log_debug("Added comment {}: {}", label, value);
                snippets.push_back(fmt::format("{}: {}", label, value));
            }
        }
    }
}

void LibExifHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item || !isEnabled)
        return;

    LibExifObject exifObject(converterManager, item);

    if (!exifObject) {
        log_debug("Exif data not found, attempting to set resolution internally...");
        setJpegResolutionResource(item);
        return;
    }

    // image resolution in pixels
    // the problem is that I do not know when I encounter the
    // tags for X and Y, so I have to save the information
    // and in the very end - when I have both values - add the
    // appropriate resource
    std::string imageX;
    std::string imageY;

    std::vector<std::string> snippets;
    for (auto* exifContent : exifObject.exifData->ifd) {
        if (exifContent != nullptr) {
            process_ifd(exifContent, item, exifObject, snippets, imageX, imageY);
        }
    }
    if (!snippets.empty() && item->getMetaData(MetadataFields::M_DESCRIPTION).empty() && isCommentEnabled) {
        auto comment = fmt::format("{}", fmt::join(snippets, ", "));
        log_debug("Fabricated Comment: {}", comment);
        item->addMetaData(MetadataFields::M_DESCRIPTION, comment);
    }

    // we got the image resolution so we can add our resource
    if (!imageX.empty() && !imageY.empty()) {
        item->getResource(ContentHandler::DEFAULT)->addAttribute(ResourceAttribute::RESOLUTION, fmt::format("{}x{}", imageX, imageY));
    } else {
        log_debug("EXIF resolution not found, falling back to parsing directly: {}", item->getLocation().c_str());
        setJpegResolutionResource(item);
    }

    if (exifObject.hasThumb()) {
        auto resource = std::make_shared<CdsResource>(ContentHandler::LIBEXIF, ResourcePurpose::Thumbnail);
        resource->addAttribute(ResourceAttribute::PROTOCOLINFO, renderProtocolInfo(item->getMimeType()));
        item->addResource(resource);
        auto resolution = exifObject.getThumbResolution();
        if (!resolution.empty())
            resource->addAttribute(ResourceAttribute::RESOLUTION, resolution);
    }
}

std::unique_ptr<IOHandler> LibExifHandler::serveContent(
    const std::shared_ptr<CdsObject>& obj,
    const std::shared_ptr<CdsResource>& resource)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return nullptr;

    if (resource->getPurpose() != ResourcePurpose::Thumbnail)
        throw_std_runtime_error("Resource {} is not a Thumbnail", resource->getPurpose());

    LibExifObject exifObject(converterManager, item);

    return exifObject.getThumbnail();
}
#endif // HAVE_LIBEXIF
