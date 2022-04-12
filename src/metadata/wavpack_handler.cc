/*GRB*

    Gerbera - https://gerbera.io/

    wavpack_handler.cc - this file is part of Gerbera.

    Copyright (C) 2022 Gerbera Contributors

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

/// \file wavpack_handler.cc

#ifdef HAVE_WAVPACK
#include "wavpack_handler.h" // API

#include "cds_objects.h"
#include "iohandler/mem_io_handler.h"
#include "util/mime.h"

static const auto fileFormats = std::map<int, std::string_view> {
    { WP_FORMAT_WAV, "WAV" },
    { WP_FORMAT_W64, "Wave64" },
    { WP_FORMAT_CAF, "CAF" },
    { WP_FORMAT_DFF, "DSDIFF" },
    { WP_FORMAT_DSF, "DSD" },
};

static const auto metaTags = std::map<std::string_view, metadata_fields_t> {
    { "Title", M_TITLE },
    { "Artist", M_ARTIST },
    { "AlbumArtist", M_ALBUMARTIST },
    { "Album", M_ALBUM },
    { "Comment", M_DESCRIPTION },
    { "Disc", M_PARTNUMBER },
    { "Track", M_TRACKNUMBER },
    { "Genre", M_GENRE },
    { "Publisher", M_PRODUCER },
};

#define MAX_WV_TEXT_SIZE 256
#define MAX_WV_IMAGE_SIZE 1024 * 1024
#define ALBUMART_OPTION "albumArtTag"

WavPackHandler::~WavPackHandler()
{
    if (context)
        context = WavpackCloseFile(context);
}

void WavPackHandler::fillMetadata(const std::shared_ptr<CdsObject>& obj)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item)
        return;

    char error[MAX_WV_TEXT_SIZE];
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
    WavpackContext* context = WavpackOpenFileInput(item->getLocation().c_str(), error, OPEN_TAGS | OPEN_2CH_MAX | OPEN_DSD_NATIVE, 0);
    if (!context) {
        log_error("Could not open {} as WavPack file: {}", item->getLocation().c_str(), error);
        return;
    }
    getAttributes(context, item);
    getTags(context, item);
    getAttachments(context, item);

#ifdef UNUSED_WAVPACK
    auto bytesPerSample = WavpackGetBytesPerSample(context);
    auto version = WavpackGetVersion(context);
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
    auto mode = WavpackGetMode(context);
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
    auto qualifyMode = WavpackGetQualifyMode(context);
    auto redChannels = WavpackGetReducedChannels(context);
    auto chMask = WavpackGetChannelMask(context);
    auto nativeSampleRate = WavpackGetNativeSampleRate(context);
    auto numSamples = WavpackGetNumSamples64(context);
    auto ratio = WavpackGetRatio(context);
#endif
}

void WavPackHandler::getAttributes(WavpackContext* context, const std::shared_ptr<CdsItem>& item)
{
    auto resource = item->getResource(ContentHandler::DEFAULT);
    auto nrChannels = WavpackGetNumChannels(context);
    resource->addAttribute(CdsResource::Attribute::NRAUDIOCHANNELS, fmt::to_string(nrChannels));
    auto sampleRate = WavpackGetSampleRate(context);
    resource->addAttribute(CdsResource::Attribute::SAMPLEFREQUENCY, fmt::to_string(sampleRate));
    auto bitsPerSample = WavpackGetBitsPerSample(context);
    resource->addAttribute(CdsResource::Attribute::BITS_PER_SAMPLE, fmt::to_string(bitsPerSample));
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
    */
    auto fileFormat = WavpackGetFileFormat(context);
    resource->addAttribute(CdsResource::Attribute::FORMAT, fileFormats.at(fileFormat).data());
    auto avgBitrate = WavpackGetAverageBitrate(context, 0);
    resource->addAttribute(CdsResource::Attribute::BITRATE, fmt::to_string(avgBitrate));
}

std::string WavPackHandler::getContentTypeFromByteVector(const char* data, int size) const
{
#ifdef HAVE_MAGIC
    auto artMimetype = mime->bufferToMimeType(data, size);
    return artMimetype.empty() ? MIMETYPE_DEFAULT : artMimetype;
#else
    return MIMETYPE_DEFAULT;
#endif
}

void WavPackHandler::getAttachments(WavpackContext* context, const std::shared_ptr<CdsItem>& item)
{
    auto tagCount = WavpackGetNumBinaryTagItems(context);
    char tag[MAX_WV_TEXT_SIZE];
    char value[MAX_WV_IMAGE_SIZE];
    for (int index = 0; index < tagCount; index++) {
        auto size = WavpackGetBinaryTagItemIndexed(context, index, tag, MAX_WV_TEXT_SIZE);
        if (size > MAX_WV_TEXT_SIZE) {
            log_warning("file {}: wavpack binary tag {} truncated (full size {})", item->getLocation().c_str(), tag, size);
        } else {
            size = WavpackGetBinaryTagItem(context, tag, value, MAX_WV_IMAGE_SIZE);
            if (size > MAX_WV_IMAGE_SIZE) {
                log_warning("file {}: wavpack binary tag {} value '{}' truncated (full size {})", item->getLocation().c_str(), tag, value, size);
                size = MAX_WV_IMAGE_SIZE;
            }
            if (startswith(tag, "Cover Art")) {
                log_debug("Identified metadata '{}': {}", tag, value);

                /* The convention for binary tag items in APEv2 tags is that the data starts with a NULLterminated
                   string representing a filename. After the terminating NULL, the actual binary data
                   starts. In the WavPack code this filename has only the extension of the actual file; the name
                   portion is made up of the tag item name.
                */
                auto artMimetype = getContentTypeFromByteVector(value + strlen(value) + 1, size - strlen(value) - 1);
                if (artMimetype != MIMETYPE_DEFAULT) {
                    log_debug("Adding resource '{}'", renderProtocolInfo(artMimetype));
                    auto resource = std::make_shared<CdsResource>(ContentHandler::WAVPACK, CdsResource::Purpose::Thumbnail);
                    resource->addAttribute(CdsResource::Attribute::PROTOCOLINFO, renderProtocolInfo(artMimetype));
                    resource->addOption(ALBUMART_OPTION, tag);
                    item->addResource(resource);
                }
            } else {
                log_warning("file {}: wavpack binary tag {} unknown", item->getLocation().c_str(), tag);
            }
        }
    }
}

void WavPackHandler::getTags(WavpackContext* context, const std::shared_ptr<CdsItem>& item)
{
    auto tagCount = WavpackGetNumTagItems(context);
    char tag[MAX_WV_TEXT_SIZE];
    char value[MAX_WV_TEXT_SIZE];
    for (int index = 0; index < tagCount; index++) {
        auto size = WavpackGetTagItemIndexed(context, index, tag, MAX_WV_TEXT_SIZE);
        if (size > MAX_WV_TEXT_SIZE) {
            log_warning("file {}: wavpack tag {} truncated (full size {})", item->getLocation().c_str(), tag, size);
        } else {
            size = WavpackGetTagItem(context, tag, value, MAX_WV_TEXT_SIZE);
            if (size > MAX_WV_TEXT_SIZE) {
                log_warning("file {}: wavpack tag {} value {} truncated (full size {})", item->getLocation().c_str(), tag, value, size);
                size = MAX_WV_IMAGE_SIZE;
            }
            auto meta = metaTags.find(tag);
            if (meta == metaTags.end()) {
                if (std::string_view(tag) == "Year") {
                    auto dateValue = std::string(value);
                    if (dateValue.length() == 4 && std::all_of(dateValue.begin(), dateValue.end(), ::isdigit) && std::stoi(dateValue) > 0) {
                        log_debug("Identified metadata '{}': {}", tag, value);
                        item->addMetaData(M_DATE, fmt::format("{}-01-01", value).c_str());
                        item->addMetaData(M_UPNP_DATE, fmt::format("{}-01-01", value).c_str());
                    }
                } else {
                    log_warning("file {}: wavpack tag {} unknown", item->getLocation().c_str(), tag);
                }
            } else {
                log_debug("Identified metadata '{}': {}", tag, value);
                item->removeMetaData(metaTags.at(tag)); // wavpack tags overwrite existing values
                item->addMetaData(metaTags.at(tag), value);
            }
        }
    }
}

std::unique_ptr<IOHandler> WavPackHandler::serveContent(const std::shared_ptr<CdsObject>& obj, int resNum)
{
    auto item = std::dynamic_pointer_cast<CdsItem>(obj);
    if (!item) // not streamable
        return {};
    auto resource = item->getResource(resNum);
    if (!resource)
        return {};
    auto artTag = resource->getOption(ALBUMART_OPTION);
    if (artTag.empty())
        return {};

    char error[MAX_WV_TEXT_SIZE];
    context = WavpackOpenFileInput(item->getLocation().c_str(), error, OPEN_TAGS | OPEN_2CH_MAX | OPEN_DSD_NATIVE, 0);
    if (!context) {
        log_error("Could not open {} as WavPack file: {}", item->getLocation().c_str(), error);
    }

    char value[MAX_WV_IMAGE_SIZE];
    auto size = WavpackGetBinaryTagItem(context, artTag.c_str(), value, MAX_WV_IMAGE_SIZE);
    if (size > MAX_WV_IMAGE_SIZE) {
        log_warning("file {}: wavpack binary tag {} value '{}' truncated (full size {})", item->getLocation().c_str(), artTag, value, size);
        size = MAX_WV_IMAGE_SIZE;
    }
    if (startswith(artTag, "Cover Art")) {
        log_debug("Found image '{}': {}", artTag, value);
        return std::make_unique<MemIOHandler>(value + strlen(value) + 1, size - strlen(value) - 1);
    }
    log_warning("file {}: wavpack binary tag {} unknown", item->getLocation().c_str(), artTag);

    return {};
}

#endif // HAVE_WAVPACK
