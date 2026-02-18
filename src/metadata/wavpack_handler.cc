/*GRB*

    Gerbera - https://gerbera.io/

    wavpack_handler.cc - this file is part of Gerbera.

    Copyright (C) 2022-2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// @file metadata/wavpack_handler.cc

#ifdef HAVE_WAVPACK
#define GRB_LOG_FAC GrbLogFacility::wavpack
#include "wavpack_handler.h" // API

#include "cds/cds_item.h"
#include "config/config_val.h"
#include "iohandler/mem_io_handler.h"
#include "util/mime.h"
#include "util/string_converter.h"
#include "util/tools.h"

static const std::map<int, std::string> fileFormats {
    { WP_FORMAT_WAV, "WAV" },
    { WP_FORMAT_W64, "Wave64" },
    { WP_FORMAT_CAF, "CAF" },
    { WP_FORMAT_DFF, "DSDIFF" },
    { WP_FORMAT_DSF, "DSD" },
#ifdef WP_FORMAT_AIF
    { WP_FORMAT_AIF, "AIFF" },
#endif
};

static const auto propertyMap = std::map<std::string_view, MetadataFields> {
    { "Title", MetadataFields::M_TITLE },
    { "Artist", MetadataFields::M_ARTIST },
    { "AlbumArtist", MetadataFields::M_ALBUMARTIST },
    { "Album", MetadataFields::M_ALBUM },
    { "Comment", MetadataFields::M_DESCRIPTION },
    { "Disc", MetadataFields::M_PARTNUMBER },
    { "Track", MetadataFields::M_TRACKNUMBER },
    { "Genre", MetadataFields::M_GENRE },
    { "Publisher", MetadataFields::M_PRODUCER },
};

#define MAX_WV_TEXT_SIZE 1024
#define MAX_WV_IMAGE_SIZE (2048 * 2048)
#define ATTACHMENT_OPTION "albumArtTag"
#define ARTWORK_TAG "Cover"

WavPackHandler::WavPackHandler(const std::shared_ptr<Context>& context)
    : MediaMetadataHandler(
          context,
          ConfigVal::IMPORT_LIBOPTS_WAVPACK_ENABLED,
          ConfigVal::IMPORT_LIBOPTS_WAVPACK_CONTENT_LIST,
          ConfigVal::IMPORT_LIBOPTS_WAVPACK_METADATA_TAGS_LIST,
          ConfigVal::IMPORT_LIBOPTS_WAVPACK_AUXDATA_TAGS_LIST,
          ConfigVal::IMPORT_LIBOPTS_WAVPACK_COMMENT_ENABLED,
          ConfigVal::IMPORT_LIBOPTS_WAVPACK_COMMENT_LIST)
{
}

WavPackHandler::~WavPackHandler()
{
}

bool WavPackHandler::isSupported(
    const std::string& contentType,
    bool isOggTheora,
    const std::string& mimeType,
    ObjectType mediaType)
{
    return contentType == CONTENT_TYPE_WAVPACK;
}

/// @brief Wrapper class to interface with libwavpack
class WavPackObject {
private:
    char error[MAX_WV_TEXT_SIZE] { '\0' };

public:
    WavpackContext* wpContext;

    WavPackObject(const std::shared_ptr<CdsItem>& item)
    {
        /* WavpackContext * WavpackOpenFileInput (const char *inFileName, char *error, int flags, int norm_offset);
           - OPEN_TAGS: Attempt to read any ID3v1 or APEv2 tags appended to the end of the file. This obviously
             requires a seekable file to succeed because it takes place before decoding audio.
           - OPEN_2CH_MAX: This allows multichannel WavPack files to be opened with only one
             stream, which usually incorporates the front left and front right channels. This is provided to
             allow decoders that can only handle 2 channels to at least provide "something" when playing
             multichannel
           - OPEN_DSD_NATIVE: Open DSD files as bitstreams so that decoded audio is returned as 8-bit “samples”
             (which actually contain 8 discrete DSD samples, MSB first temporally) in 32-bit
             integers (like all WavPack audio data). If neither this flag nor the OPEN_DSD_AS_PCM flag are
             set, then DSD audio files will not be readable. Introduced with 5.0.0.
        */
        wpContext = WavpackOpenFileInput(item->getLocation().c_str(), error, OPEN_TAGS | OPEN_2CH_MAX | OPEN_DSD_NATIVE, 0);
        if (!wpContext) {
            log_error("Could not open {} as WavPack file: {}", item->getLocation().c_str(), error);
        }
    }
    /// @brief check if libexif information is available
    operator bool() const
    {
        return wpContext;
    }

    ~WavPackObject()
    {
        if (wpContext)
            wpContext = WavpackCloseFile(wpContext);
    }
};

/// @brief Wrapper class to interface with libwavpack
class WavPackEntry {
private:
    std::string location;
    std::shared_ptr<StringConverter> sc { nullptr };
    int size { 0 };
    char value[MAX_WV_IMAGE_SIZE + 1] { '\0' };

public:
    char tag[MAX_WV_TEXT_SIZE + 1] { '\0' };

    WavPackEntry(
        WavpackContext* wpContext,
        int index,
        const std::shared_ptr<StringConverter>& sc,
        const std::shared_ptr<CdsItem>& item)
        : location(item->getLocation())
        , sc(sc)
    {
        size = WavpackGetTagItemIndexed(wpContext, index, tag, MAX_WV_TEXT_SIZE);

        if (size > MAX_WV_TEXT_SIZE) {
            log_warning("file {}: wavpack tag {} truncated (full size {})", location, tag, size);
            size = 0;
        } else {
            size = WavpackGetTagItem(wpContext, tag, value, MAX_WV_IMAGE_SIZE);
            if (size > MAX_WV_IMAGE_SIZE) {
                log_warning("file {}: wavpack tag {} value {} truncated (full size {})", location, tag, value, size);
                size = MAX_WV_IMAGE_SIZE;
            }
        }
    }

    WavPackEntry(
        WavpackContext* wpContext,
        int index,
        const std::shared_ptr<CdsItem>& item)
        : location(item->getLocation())
    {
        size = WavpackGetBinaryTagItemIndexed(wpContext, index, tag, MAX_WV_TEXT_SIZE);

        if (size > MAX_WV_TEXT_SIZE) {
            log_warning("file {}: wavpack binary tag {} truncated (full size {})", location, tag, size);
            size = 0;
        } else {
            size = WavpackGetBinaryTagItem(wpContext, tag, value, MAX_WV_IMAGE_SIZE);
            if (size > MAX_WV_IMAGE_SIZE) {
                log_warning("file {}: wavpack binary tag {} value '{}' truncated (full size {})", location, tag, value, size);
                size = MAX_WV_IMAGE_SIZE;
            }
        }
    }

    WavPackEntry(
        WavpackContext* wpContext,
        const std::string& attmtTag,
        const std::shared_ptr<CdsItem>& item)
        : location(item->getLocation())
    {
        size = WavpackGetBinaryTagItem(wpContext, attmtTag.c_str(), value, MAX_WV_IMAGE_SIZE);

        if (size > MAX_WV_IMAGE_SIZE) {
            log_warning("file {}: wavpack binary tag {} value '{}' truncated (full size {})", location, attmtTag, value, size);
            size = MAX_WV_IMAGE_SIZE;
        }
    }

    /// @brief check if libexif information is available
    operator bool() const
    {
        return size != 0;
    }

    /// @brief get date/time
    std::string getDate()
    {
        auto [dateValue, err] = sc->convert(value);
        if (!err.empty()) {
            log_warning("{}: {}", location, err);
        }
        if (dateValue.length() == 4 && std::all_of(dateValue.begin(), dateValue.end(), ::isdigit) && std::stoi(dateValue) > 0) {
            log_debug("Identified metadata '{}': {}", tag, value);
            return dateValue;
        }
        return "";
    }

    /// @brief get key value from image
    std::string getValue()
    {
        if (!sc)
            return value;

        auto [val, err] = sc->convert(value);
        if (!err.empty()) {
            log_warning("{}: {}", location, err);
            return val;
        }
        return "";
    }

    /// @brief get image data
    std::unique_ptr<IOHandler> getRawValue()
    {
        return std::make_unique<MemIOHandler>(value + strlen(value) + 1, size - strlen(value) - 1);
    }

#ifdef HAVE_MAGIC
    /// @brief try to identify mime type from binary object
    std::string getMimeTypeFromByteVector(const std::shared_ptr<Mime>& mime) const
    {
        const char* data = value + strlen(value) + 1;
        int length = size - strlen(value) - 1;

        auto artMimetype = mime->bufferToMimeType(data, length);
        return artMimetype.empty() ? MIMETYPE_DEFAULT : artMimetype;
    }
#endif
};

bool WavPackHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item || !enabled)
        return false;

    WavPackObject wpObject(item);
    if (!wpObject) {
        return false;
    }
    WavpackContext* wpContext = wpObject.wpContext;
    bool result = false;
    result = getAttributes(wpContext, item) || result;
    result = getTags(wpContext, item) || result;
    result = getAttachments(wpContext, item) || result;

#ifdef UNUSED_WAVPACK
    auto bytesPerSample = WavpackGetBytesPerSample(wpContext);
    auto version = WavpackGetVersion(wpContext);
    /*
    int WavpackGetMode (WavpackContext *wpc);
       This returns a bitmask with the following values:
       - MODE_WVC: A .wvc file has been found and will be used for lossless decoding.
       - MODE_LOSSLESS: The file decoding is lossless (either pure or hybrid).
       - MODE_HYBRID: The file is in hybrid mode (may be either lossy or lossless).
       - MODE_FLOAT: The audio data is 32-bit ieee floating point.
       - MODE_VALID_TAG: The file contains a valid ID3v1 or APEv2 tag (OPEN_TAGS must be set above to get this status).
       - MODE_HIGH: The file was originally created in "high" mode (this is really only useful for reporting to the user)
       - MODE_FAST: The file was originally created in "fast" mode (this is really only useful for reporting to the user)
       - MODE_EXTRA: The file was originally created with the "extra" mode (this is really only useful for reporting to the user). The MODE_XMODE below can sometimes allow determination of the exact extra mode level.
       - MODE_XMODE: If the MODE_EXTRA bit above is set, this 3-bit field can sometimes allow the determination of the exact extra mode parameter specified by the user if the file was encoded with version 4.50 or later. If these three bits are zero then the extra mode level is unknown, otherwise is represents the extra mode level from 1-6.
       - MODE_APETAG: The file contains a valid APEv2 tag (OPEN_TAGS must be set in the "open" call for this to be true). Note that only APEv2 tags can be edited by the library. If a file that has an ID3v1 tag needs to be edited then it must either be done with another library or it must be converted (field by field) into a APEv2 tag (see the wvgain.c program for an example of this).
       - MODE_SFX: The file was created as a "self-extracting" executable (this is really only useful for reporting to the user). The creation of self-extracting files is no longer supported.
       - MODE_VERY_HIGH: The file was created in the "very high" mode (or in the "high" mode prior to 4.40).
       - MODE_MD5: The file contains an MD5 checksum.
       - MODE_DNS: The hybrid file was encoded with the dynamic noise shaping feature which was introduced in the 4.50 version of WavPack
    */
    auto mode = WavpackGetMode(wpContext);
    /*
    int WavpackGetQualifyMode (WavpackContext *wpc);
       This function obtains information about specific file features that were added for version 5, specifically
       qualifications added to support CAF and DSD files. These bits are meant to simply indicate the format
       of the data in the original source file and do not indicate how the library will return the data to the
       application (which is always the same). This means that in general an application that simply wants to
       play or process the audio data need not be concerned about these. Introduced with WavPack 5.0.0.
       If the file is DSD audio, then either of the QMODE_DSD_LSB_FIRST or QMODE_DSD_MSB_FIRST
       bits will be set (but native DSD audio is always returned to the caller MSB first). Checking for one of
       these two bits is the proper way to check for DSD audio.
       - QMODE_BIG_ENDIAN: big-endian data format (opposite of WAV format)
       - QMODE_SIGNED_BYTES: 8-bit audio data is signed (opposite of WAV format)
       - QMODE_UNSIGNED_WORDS: audio data (> 8-bit) is unsigned (opposite of WAV format)
       - QMODE_REORDERED_CHANS: source channels were reordered
       - QMODE_DSD_LSB_FIRST: DSD bytes, LSB first (most Sony .dsf files)
       - QMODE_DSD_MSB_FIRST: DSD bytes, MSB first (Philips .dff files)
       - QMODE_DSD_IN_BLOCKS: DSD data is blocked by channels (Sony .dsf only)
    */
    auto qualifyMode = WavpackGetQualifyMode(wpContext);
    auto redChannels = WavpackGetReducedChannels(wpContext);
    auto chMask = WavpackGetChannelMask(wpContext);
    auto nativeSampleRate = WavpackGetNativeSampleRate(wpContext);
    auto numSamples = WavpackGetNumSamples64(wpContext);
    auto ratio = WavpackGetRatio(wpContext);
#endif

    return result;
}

bool WavPackHandler::getAttributes(
    WavpackContext* wpContext,
    const std::shared_ptr<CdsItem>& item)
{
    auto resource = item->getResource(ContentHandler::DEFAULT);
    auto nrChannels = WavpackGetNumChannels(wpContext);
    resource->addAttribute(ResourceAttribute::NRAUDIOCHANNELS, fmt::to_string(nrChannels));
    auto sampleRate = WavpackGetSampleRate(wpContext);
    resource->addAttribute(ResourceAttribute::SAMPLEFREQUENCY, fmt::to_string(sampleRate));
    auto bitsPerSample = WavpackGetBitsPerSample(wpContext);
    resource->addAttribute(ResourceAttribute::BITS_PER_SAMPLE, fmt::to_string(bitsPerSample));
    /*
    int WavpackGetFileFormat (WavpackContext *wpc);
        Return the file format specified in the call to WavpackSetFileInformation() when the file
        was created. For all files created prior to WavPack 5 this will be zero (WP_FORMAT_WAV). Introduced
        in WavPack 5.0.0. The following formats are currently defined in wavpack.h:
        - WP_FORMAT_WAV: Microsoft Waveform Audio Format, including BWF and RF64
        - WP_FORMAT_W64: Sony Wave64
        - WP_FORMAT_CAF: Apple Core Audio
        - WP_FORMAT_DFF: Philips DSDIFF
        - WP_FORMAT_DSF: Sony DSD format
        - WP_FORMAT_AIF: Apple Image file format
    */
    auto fileFormat = WavpackGetFileFormat(wpContext);
    if (fileFormats.find(fileFormat) != fileFormats.end())
        resource->addAttribute(ResourceAttribute::FORMAT, fileFormats.at(fileFormat));
    else
        resource->addAttribute(ResourceAttribute::FORMAT, fmt::format("WP_FORMAT_{}", fileFormat));
    auto avgBitrate = WavpackGetAverageBitrate(wpContext, 0);
    resource->addAttribute(ResourceAttribute::BITRATE, fmt::to_string(avgBitrate));
    return true;
}

bool WavPackHandler::getAttachments(
    WavpackContext* wpContext,
    const std::shared_ptr<CdsItem>& item)
{
    auto tagCount = WavpackGetNumBinaryTagItems(wpContext);
    bool result = false;
    for (int index = 0; index < tagCount; index++) {
        WavPackEntry wavpackEntry(wpContext, index, item);
        if (wavpackEntry) {
#ifdef HAVE_MAGIC
            auto attmtMimetype = wavpackEntry.getMimeTypeFromByteVector(mime);
#else
            std::string attmtMimetype = MIMETYPE_DEFAULT;
#endif
            auto protocolInfo = renderProtocolInfo(attmtMimetype);
            log_debug("Identified metadata '{}': {}", wavpackEntry.tag, wavpackEntry.getValue());
            if (startswith(wavpackEntry.tag, ARTWORK_TAG)) {

                /* The convention for binary tag items in APEv2 tags is that the data starts with a NULLterminated
                   string representing a filename. After the terminating NULL, the actual binary data
                   starts. In the WavPack code this filename has only the extension of the actual file; the name
                   portion is made up of the tag item name.
                */
                log_debug("Adding album art '{}'", protocolInfo);
                auto resource = std::make_shared<CdsResource>(ContentHandler::WAVPACK, ResourcePurpose::Thumbnail);
                if (attmtMimetype != MIMETYPE_DEFAULT)
                    resource->addAttribute(ResourceAttribute::PROTOCOLINFO, protocolInfo);
                resource->addOption(ATTACHMENT_OPTION, wavpackEntry.tag);
                item->addResource(resource);
                result = true;
            } else {
                log_warning("file {}: wavpack binary tag {} unknown", item->getLocation().c_str(), wavpackEntry.tag);
                log_debug("Adding resource '{}'", protocolInfo);
                auto resource = std::make_shared<CdsResource>(ContentHandler::WAVPACK, ResourcePurpose::Content);
                resource->addAttribute(ResourceAttribute::PROTOCOLINFO, protocolInfo);
                resource->addOption(ATTACHMENT_OPTION, wavpackEntry.tag);
                item->addResource(resource);
                result = true;
            }
        }
    }
    return result;
}

bool WavPackHandler::getTags(
    WavpackContext* wpContext,
    const std::shared_ptr<CdsItem>& item)
{
    auto sc = converterManager->m2i(ConfigVal::IMPORT_LIBOPTS_WAVPACK_CHARSET, item->getLocation());
    auto tagCount = WavpackGetNumTagItems(wpContext);
    std::vector<std::string> snippets;
    bool result = false;
    for (int index = 0; index < tagCount; index++) {
        WavPackEntry wavpackEntry(wpContext, index, sc, item);
        if (wavpackEntry) {
            auto property = propertyMap.find(wavpackEntry.tag);
            if (property == propertyMap.end()) {
                if (std::string_view(wavpackEntry.tag) == "Year") {
                    auto dateValue = wavpackEntry.getDate();
                    if (!dateValue.empty()) {
                        log_debug("Identified metadata '{}': {}", wavpackEntry.tag, dateValue);
                        item->addMetaData(MetadataFields::M_DATE, fmt::format("{}-01-01", dateValue));
                        item->addMetaData(MetadataFields::M_UPNP_DATE, fmt::format("{}-01-01", dateValue));
                        result = true;
                    }
                } else {
                    log_warning("file {}: wavpack tag {} unknown", item->getLocation().string(), wavpackEntry.tag);
                }
            } else {
                auto val = wavpackEntry.getValue();
                if (!val.empty()) {
                    log_debug("Identified metadata '{}': {}", wavpackEntry.tag, val);
                    item->removeMetaData(propertyMap.at(wavpackEntry.tag)); // wavpack tags overwrite existing values
                    item->addMetaData(propertyMap.at(wavpackEntry.tag), val);
                    result = true;
                }
            }
            auto meta = metaTags.find(wavpackEntry.tag);
            if (meta != metaTags.end()) {
                if (std::string_view(wavpackEntry.tag) == "Year") {
                    auto dateValue = wavpackEntry.getDate();
                    if (!dateValue.empty()) {
                        log_debug("Identified metadata '{}': {}", wavpackEntry.tag, dateValue);
                        item->addMetaData(meta->first, fmt::format("{}-01-01", dateValue));
                        result = true;
                    }
                } else {
                    auto val = wavpackEntry.getValue();
                    if (!val.empty()) {
                        log_debug("Identified metadata '{}': {}", wavpackEntry.tag, val);
                        item->removeMetaData(metaTags.at(wavpackEntry.tag)); // wavpack tags overwrite existing values
                        item->addMetaData(metaTags.at(wavpackEntry.tag), val);
                        result = true;
                    }
                }
            }
            auto aux = std::find(auxTags.begin(), auxTags.end(), wavpackEntry.tag);
            if (aux != auxTags.end()) {
                auto val = wavpackEntry.getValue();
                if (!val.empty()) {
                    log_debug("Identified auxdata '{}': {}", wavpackEntry.tag, val);
                    item->setAuxData(wavpackEntry.tag, val);
                    result = true;
                }
            }
            auto comment = std::find_if(commentMap.begin(), commentMap.end(), [&](auto& c) { return c.second == wavpackEntry.tag; });
            if (comment != commentMap.end()) {
                auto val = wavpackEntry.getValue();
                if (!val.empty()) {
                    log_debug("Added comment {}: {}", comment->first, val);
                    snippets.push_back(fmt::format("{}: {}", comment->first, val));
                }
            }
        }
    }
    if (!snippets.empty() && item->getMetaData(MetadataFields::M_DESCRIPTION).empty() && isCommentEnabled) {
        auto comment = fmt::format("{}", fmt::join(snippets, ", "));
        log_debug("Fabricated Comment: {}", comment);
        item->addMetaData(MetadataFields::M_DESCRIPTION, comment);
        result = true;
    }
    return result;
}

std::unique_ptr<IOHandler> WavPackHandler::serveContent(
    const std::shared_ptr<CdsObject>& obj,
    const std::shared_ptr<CdsResource>& resource)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item && !enabled) // not streamable
        return {};
    if (!resource)
        return {};
    auto attmtTag = resource->getOption(ATTACHMENT_OPTION);
    if (attmtTag.empty())
        return {};

    WavPackObject wpObject(item);
    if (!wpObject) {
        return {};
    }

    WavPackEntry wavpackEntry(wpObject.wpContext, attmtTag, item);

    if (wavpackEntry && startswith(attmtTag, ARTWORK_TAG)) {
        log_debug("Found image '{}'", attmtTag);
        return wavpackEntry.getRawValue();
    }
    log_warning("file {}: wavpack binary tag {} unknown", item->getLocation().c_str(), attmtTag);

    return {};
}

#endif // HAVE_WAVPACK
