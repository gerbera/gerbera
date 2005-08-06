/*  libexif_handler.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
    Sergey Bostandzhyan <jin@deadlock.dhs.org>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/// \file libexif_handler.cc
/// \brief Implementeation of the LibExifHandler class.

#ifdef HAVE_EXIF

#ifdef HAVE_EXTRACTOR
#include <extractor.h>
#endif


#include "libexif_handler.h"
#include "tools.h"
#include "config_manager.h"

using namespace zmm;
using namespace mxml;

LibExifHandler::LibExifHandler() : MetadataHandler()
{
}

static int getTagFromString(String tag)
{
    if (tag == "EXIF_TAG_INTEROPERABILITY_INDEX")
        return  EXIF_TAG_INTEROPERABILITY_INDEX;
    if (tag == "EXIF_TAG_INTEROPERABILITY_VERSION")
        return  EXIF_TAG_INTEROPERABILITY_VERSION;
    if (tag == "EXIF_TAG_IMAGE_WIDTH")
        return  EXIF_TAG_IMAGE_WIDTH; 
    if (tag == "EXIF_TAG_IMAGE_LENGTH")
        return  EXIF_TAG_IMAGE_LENGTH;
    if (tag == "EXIF_TAG_BITS_PER_SAMPLE")
        return  EXIF_TAG_BITS_PER_SAMPLE;
    if (tag == "EXIF_TAG_COMPRESSION")
        return  EXIF_TAG_COMPRESSION;
    if (tag == "EXIF_TAG_PHOTOMETRIC_INTERPRETATION")
        return  EXIF_TAG_PHOTOMETRIC_INTERPRETATION;
    if (tag == "EXIF_TAG_FILL_ORDER")
        return  EXIF_TAG_FILL_ORDER;
    if (tag == "EXIF_TAG_DOCUMENT_NAME")
        return  EXIF_TAG_DOCUMENT_NAME;
    if (tag == "EXIF_TAG_IMAGE_DESCRIPTION")
        return  EXIF_TAG_IMAGE_DESCRIPTION;
    if (tag == "EXIF_TAG_MAKE")
        return  EXIF_TAG_MAKE;
    if (tag == "EXIF_TAG_MODEL")
        return  EXIF_TAG_MODEL;
    if (tag == "EXIF_TAG_STRIP_OFFSETS")
        return  EXIF_TAG_STRIP_OFFSETS;
    if (tag == "EXIF_TAG_ORIENTATION")
        return  EXIF_TAG_ORIENTATION;
    if (tag == "EXIF_TAG_SAMPLES_PER_PIXEL")
        return  EXIF_TAG_SAMPLES_PER_PIXEL;
    if (tag == "EXIF_TAG_ROWS_PER_STRIP")
        return  EXIF_TAG_ROWS_PER_STRIP;
    if (tag == "EXIF_TAG_STRIP_BYTE_COUNTS")
        return  EXIF_TAG_STRIP_BYTE_COUNTS;
    if (tag == "EXIF_TAG_X_RESOLUTION")
        return  EXIF_TAG_X_RESOLUTION;
    if (tag == "EXIF_TAG_Y_RESOLUTION")
        return  EXIF_TAG_Y_RESOLUTION;
    if (tag == "EXIF_TAG_PLANAR_CONFIGURATION")
        return  EXIF_TAG_PLANAR_CONFIGURATION;
    if (tag == "EXIF_TAG_RESOLUTION_UNIT")
        return  EXIF_TAG_RESOLUTION_UNIT;
    if (tag == "EXIF_TAG_TRANSFER_FUNCTION")
        return  EXIF_TAG_TRANSFER_FUNCTION;
    if (tag == "EXIF_TAG_SOFTWARE")
        return  EXIF_TAG_SOFTWARE;
    if (tag == "EXIF_TAG_DATE_TIME")
        return  EXIF_TAG_DATE_TIME;
    if (tag == "EXIF_TAG_ARTIST")
        return  EXIF_TAG_ARTIST;
    if (tag == "EXIF_TAG_WHITE_POINT")
        return  EXIF_TAG_WHITE_POINT;
    if (tag == "EXIF_TAG_PRIMARY_CHROMATICITIES")
        return  EXIF_TAG_PRIMARY_CHROMATICITIES;
    if (tag == "EXIF_TAG_TRANSFER_RANGE")
        return  EXIF_TAG_TRANSFER_RANGE;
    if (tag == "EXIF_TAG_JPEG_PROC")
        return  EXIF_TAG_JPEG_PROC;
    if (tag == "EXIF_TAG_JPEG_INTERCHANGE_FORMAT")
        return  EXIF_TAG_JPEG_INTERCHANGE_FORMAT;
    if (tag == "EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH")
        return  EXIF_TAG_JPEG_INTERCHANGE_FORMAT_LENGTH;
    if (tag == "EXIF_TAG_YCBCR_COEFFICIENTS")
        return  EXIF_TAG_YCBCR_COEFFICIENTS;
    if (tag == "EXIF_TAG_YCBCR_SUB_SAMPLING")
        return  EXIF_TAG_YCBCR_SUB_SAMPLING;
    if (tag == "EXIF_TAG_YCBCR_POSITIONING")
        return  EXIF_TAG_YCBCR_POSITIONING;
    if (tag == "EXIF_TAG_REFERENCE_BLACK_WHITE")
        return  EXIF_TAG_REFERENCE_BLACK_WHITE;
    if (tag == "EXIF_TAG_RELATED_IMAGE_FILE_FORMAT")
        return  EXIF_TAG_RELATED_IMAGE_FILE_FORMAT;
    if (tag == "EXIF_TAG_RELATED_IMAGE_WIDTH")
        return  EXIF_TAG_RELATED_IMAGE_WIDTH;
    if (tag == "EXIF_TAG_RELATED_IMAGE_LENGTH")
        return  EXIF_TAG_RELATED_IMAGE_LENGTH;
    if (tag == "EXIF_TAG_CFA_REPEAT_PATTERN_DIM")
        return  EXIF_TAG_CFA_REPEAT_PATTERN_DIM;
    if (tag == "EXIF_TAG_CFA_PATTERN")
        return  EXIF_TAG_CFA_PATTERN;
    if (tag == "EXIF_TAG_BATTERY_LEVEL")
        return  EXIF_TAG_BATTERY_LEVEL;
    if (tag == "EXIF_TAG_COPYRIGHT")
        return  EXIF_TAG_COPYRIGHT;
    if (tag == "EXIF_TAG_EXPOSURE_TIME")
        return  EXIF_TAG_EXPOSURE_TIME;
    if (tag == "EXIF_TAG_FNUMBER")
        return  EXIF_TAG_FNUMBER;
    if (tag == "EXIF_TAG_IPTC_NAA")
        return  EXIF_TAG_IPTC_NAA;
    if (tag == "EXIF_TAG_EXIF_IFD_POINTER")
        return  EXIF_TAG_EXIF_IFD_POINTER;
    if (tag == "EXIF_TAG_INTER_COLOR_PROFILE")
        return  EXIF_TAG_INTER_COLOR_PROFILE;
    if (tag == "EXIF_TAG_EXPOSURE_PROGRAM")
        return  EXIF_TAG_EXPOSURE_PROGRAM;
    if (tag == "EXIF_TAG_SPECTRAL_SENSITIVITY")
        return  EXIF_TAG_SPECTRAL_SENSITIVITY;
    if (tag == "EXIF_TAG_GPS_INFO_IFD_POINTER")
        return  EXIF_TAG_GPS_INFO_IFD_POINTER;
    if (tag == "EXIF_TAG_ISO_SPEED_RATINGS")
        return  EXIF_TAG_ISO_SPEED_RATINGS;
    if (tag == "EXIF_TAG_OECF")
        return  EXIF_TAG_OECF;
    if (tag == "EXIF_TAG_EXIF_VERSION") 
        return  EXIF_TAG_EXIF_VERSION;
    if (tag == "EXIF_TAG_DATE_TIME_ORIGINAL")
        return  EXIF_TAG_DATE_TIME_ORIGINAL;
    if (tag == "EXIF_TAG_DATE_TIME_DIGITIZED")
        return  EXIF_TAG_DATE_TIME_DIGITIZED;
    if (tag == "EXIF_TAG_COMPONENTS_CONFIGURATION") 
        return  EXIF_TAG_COMPONENTS_CONFIGURATION;
    if (tag == "EXIF_TAG_COMPRESSED_BITS_PER_PIXEL")
        return  EXIF_TAG_COMPRESSED_BITS_PER_PIXEL;
    if (tag == "EXIF_TAG_SHUTTER_SPEED_VALUE")
        return  EXIF_TAG_SHUTTER_SPEED_VALUE;
    if (tag == "EXIF_TAG_APERTURE_VALUE")
        return  EXIF_TAG_APERTURE_VALUE;
    if (tag == "EXIF_TAG_BRIGHTNESS_VALUE")
        return  EXIF_TAG_BRIGHTNESS_VALUE;
    if (tag == "EXIF_TAG_EXPOSURE_BIAS_VALUE")
        return  EXIF_TAG_EXPOSURE_BIAS_VALUE;
    if (tag == "EXIF_TAG_MAX_APERTURE_VALUE")
        return  EXIF_TAG_MAX_APERTURE_VALUE;
    if (tag == "EXIF_TAG_SUBJECT_DISTANCE")
        return  EXIF_TAG_SUBJECT_DISTANCE;
    if (tag == "EXIF_TAG_METERING_MODE")
        return  EXIF_TAG_METERING_MODE;
    if (tag == "EXIF_TAG_LIGHT_SOURCE")
        return  EXIF_TAG_LIGHT_SOURCE;
    if (tag == "EXIF_TAG_FLASH")
        return  EXIF_TAG_FLASH;
    if (tag == "EXIF_TAG_FOCAL_LENGTH")
        return  EXIF_TAG_FOCAL_LENGTH;
    if (tag == "EXIF_TAG_SUBJECT_AREA")
        return  EXIF_TAG_SUBJECT_AREA;
    if (tag == "EXIF_TAG_MAKER_NOTE")
        return  EXIF_TAG_MAKER_NOTE;
    if (tag == "EXIF_TAG_USER_COMMENT")
        return  EXIF_TAG_USER_COMMENT;
    if (tag == "EXIF_TAG_SUBSEC_TIME")
        return  EXIF_TAG_SUBSEC_TIME;
    if (tag == "EXIF_TAG_SUB_SEC_TIME_ORIGINAL")
        return  EXIF_TAG_SUB_SEC_TIME_ORIGINAL;
    if (tag == "EXIF_TAG_SUB_SEC_TIME_DIGITIZED") 
        return  EXIF_TAG_SUB_SEC_TIME_DIGITIZED;
    if (tag == "EXIF_TAG_FLASH_PIX_VERSION") 
        return  EXIF_TAG_FLASH_PIX_VERSION;
    if (tag == "EXIF_TAG_COLOR_SPACE")
        return  EXIF_TAG_COLOR_SPACE;
    if (tag == "EXIF_TAG_PIXEL_X_DIMENSION")
        return  EXIF_TAG_PIXEL_X_DIMENSION;
    if (tag == "EXIF_TAG_PIXEL_Y_DIMENSION")
        return  EXIF_TAG_PIXEL_Y_DIMENSION;
    if (tag == "EXIF_TAG_RELATED_SOUND_FILE")
        return  EXIF_TAG_RELATED_SOUND_FILE;
    if (tag == "EXIF_TAG_INTEROPERABILITY_IFD_POINTER")
        return  EXIF_TAG_INTEROPERABILITY_IFD_POINTER;
    if (tag == "EXIF_TAG_FLASH_ENERGY")
        return  EXIF_TAG_FLASH_ENERGY;
    if (tag == "EXIF_TAG_SPATIAL_FREQUENCY_RESPONSE")
        return  EXIF_TAG_SPATIAL_FREQUENCY_RESPONSE;
    if (tag == "EXIF_TAG_FOCAL_PLANE_X_RESOLUTION")
        return  EXIF_TAG_FOCAL_PLANE_X_RESOLUTION;
    if (tag == "EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION")
        return  EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION;
    if (tag == "EXIF_TAG_FOCAL_PLANE_RESOLUTION_UNIT")
        return  EXIF_TAG_FOCAL_PLANE_RESOLUTION_UNIT;
    if (tag == "EXIF_TAG_SUBJECT_LOCATION")
        return  EXIF_TAG_SUBJECT_LOCATION;
    if (tag == "EXIF_TAG_EXPOSURE_INDEX")
        return  EXIF_TAG_EXPOSURE_INDEX;
    if (tag == "EXIF_TAG_SENSING_METHOD")
        return  EXIF_TAG_SENSING_METHOD;
    if (tag == "EXIF_TAG_FILE_SOURCE")
        return  EXIF_TAG_FILE_SOURCE;
    if (tag == "EXIF_TAG_SCENE_TYPE")
        return  EXIF_TAG_SCENE_TYPE;
    if (tag == "EXIF_TAG_NEW_CFA_PATTERN")
        return  EXIF_TAG_NEW_CFA_PATTERN;
    if (tag == "EXIF_TAG_CUSTOM_RENDERED")
        return  EXIF_TAG_CUSTOM_RENDERED;
    if (tag == "EXIF_TAG_EXPOSURE_MODE")
        return  EXIF_TAG_CUSTOM_RENDERED;
    if (tag == "EXIF_TAG_WHITE_BALANCE")
        return  EXIF_TAG_CUSTOM_RENDERED;
    if (tag == "EXIF_TAG_DIGITAL_ZOOM_RATIO")
        return  EXIF_TAG_CUSTOM_RENDERED;
    if (tag == "EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM")
        return  EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM;
    if (tag == "EXIF_TAG_SCENE_CAPTURE_TYPE")
        return  EXIF_TAG_SCENE_CAPTURE_TYPE;
    if (tag == "EXIF_TAG_GAIN_CONTROL")
        return  EXIF_TAG_GAIN_CONTROL;
    if (tag == "EXIF_TAG_CONTRAST")
        return  EXIF_TAG_CONTRAST;
    if (tag == "EXIF_TAG_SATURATION") 
        return  EXIF_TAG_SATURATION;
    if (tag == "EXIF_TAG_SHARPNESS")
        return  EXIF_TAG_SHARPNESS;
    if (tag == "EXIF_TAG_DEVICE_SETTING_DESCRIPTION")
        return  EXIF_TAG_DEVICE_SETTING_DESCRIPTION;
    if (tag == "EXIF_TAG_SUBJECT_DISTANCE_RANGE") 
        return  EXIF_TAG_DEVICE_SETTING_DESCRIPTION;
    if (tag == "EXIF_TAG_IMAGE_UNIQUE_ID") 
        return  EXIF_TAG_IMAGE_UNIQUE_ID;

    log_warning(("Ignoring unknown libexif tag: %s\n", tag.c_str()));
    return -1;
}

void LibExifHandler::process_ifd (ExifContent *content, Ref<CdsItem> item, Ref<StringConverter> sc, Ref<Array<StringBase> > auxtags)
{
    ExifEntry *e;
    unsigned int i;
    String value;
    String tmp;

    for (i = 0; i < content->count; i++) {
        e = content->entries[i];

//        log_info(("Processing entry: %d\n", i));
        
        switch (e->tag)
        {
            case EXIF_TAG_DATE_TIME:
                value = String((char *)exif_entry_get_value(e));
                if (string_ok(value))
                {
                    value = sc->convert(value);
                    //value = split_string(value, ' ');
                    // \TODO convert date to ISO 8601 as required in the UPnP spec
                    item->setMetadata(String(MT_KEYS[M_DATE].upnp), value);
                }
                break;

            case EXIF_TAG_USER_COMMENT:
                value = String((char *)exif_entry_get_value(e));
                if (string_ok(value))
                {
                    value = sc->convert(value);

                    item->setMetadata(String(MT_KEYS[M_DESCRIPTION].upnp), value);
                }
                break;

            case EXIF_TAG_PIXEL_X_DIMENSION:
                value = String((char *)exif_entry_get_value(e));
                if (string_ok(value))
                {
                    value = sc->convert(value);
                    imageX = value;
                }
                break;

            case EXIF_TAG_PIXEL_Y_DIMENSION:
                value = String((char *)exif_entry_get_value(e));
                if (string_ok(value))
                {
                    value = sc->convert(value);
                    imageY = value;
                }
                break;
        }

        // if there are any auxilary tags that the user wants - add them
        if (auxtags != nil)
        {
            for (int j = 0; j < auxtags->size(); j++)
            {

                tmp = auxtags->get(j);
                if (string_ok(tmp))
                {
                    if (e->tag == getTagFromString(tmp))
                    {
                        value = String((char *)exif_entry_get_value(e));
                        if (string_ok(value))
                        {
                            value = sc->convert(value);
                            item->setAuxData(tmp, value);
//                            log_debug(("Adding tag: %s with value %s\n", tmp.c_str(), value.c_str()));
                        }
                    }
                }
            }
        }
    }
}


void LibExifHandler::fillMetadata(Ref<CdsItem> item)
{
    ExifData    *ed;
    Ref<Array<StringBase> > aux;
    
    Ref<StringConverter> sc = StringConverter::m2i();

    ed = exif_data_new_from_file(item->getLocation().c_str());

    if (!ed)
        return;

    Ref<ConfigManager> cm = ConfigManager::getInstance();
    Ref<Element> e = cm->getElement("/import/library-options/libexif/auxdata");
    aux = cm->createArrayFromNodeset(e, "add", "tag");

    for (int i = 0; i < EXIF_IFD_COUNT; i++)
    {
        if (ed->ifd[i])
            process_ifd (ed->ifd[i], item, sc, aux);
    }

    // we got the image resolution so we can add our resource
    if (string_ok(imageX) && string_ok(imageY))
    {
        item->getResource(0)->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION),
                                           imageX + "x" + imageY);
    }

// we can only figure out information about the thumbnail if we have the
// libextractor library
#ifdef HAVE_EXTRACTOR    
    if (ed->size) 
    {
        //log_info(("EXIF data contains a thumbnail (%i bytes).\n", ed->size));
       
        int fd;
        char temp[] = "/tmp/.tombXXXXXX";

        fd = mkstemp(temp);

        String th_filename = temp;
        if (fd != -1)
        {
            write (fd, ed->data, ed->size);
            close(fd);

            EXTRACTOR_ExtractorList *extractors = EXTRACTOR_loadDefaultLibraries();
            EXTRACTOR_KeywordList *keywords = EXTRACTOR_getKeywords(extractors, th_filename.c_str());
    

            String th_mimetype = String((char *)EXTRACTOR_extractLast(EXTRACTOR_MIMETYPE, keywords));
            String th_resolution = String((char *)EXTRACTOR_extractLast(EXTRACTOR_SIZE, keywords));

            Ref<CdsResource> resource(new CdsResource());
            resource->addAttribute(String(RES_KEYS[R_PROTOCOLINFO].upnp), renderProtocolInfo(th_mimetype));
            resource->addAttribute(String(RES_KEYS[R_RESOLUTION].upnp), th_resolution);
            item->addResource(resource);

            EXTRACTOR_freeKeywords(keywords);
            EXTRACTOR_removeAll(extractors);

            if (remove(th_filename.c_str()) == -1)
                log_error(("Failed to remove temporary thumbnail file %s (%s)\n", th_filename.c_str(), strerror(errno))); 
        }
        else 
            log_error(("Could not open %s for writing thumbnail: %s\n", th_filename.c_str(), strerror(errno)));
    }
#endif
    
    exif_data_unref(ed);
}

Ref<IOHandler> LibExifHandler::serveContent(Ref<CdsItem> item, int resNum)
{
    return nil;
}

#endif // HAVE_EXIF

