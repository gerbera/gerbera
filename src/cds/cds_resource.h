/*MT*

    MediaTomb - http://www.mediatomb.cc/

    cds_resource.h - this file is part of MediaTomb.

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

/// \file cds_resource.h

#ifndef __CDS_RESOURCE_H__
#define __CDS_RESOURCE_H__

#include <map>
#include <memory>

#include "common.h"
#include "util/enum_iterator.h"

/// \brief name for external urls that can appear in object resources (i.e.
/// a YouTube thumbnail)
#define RESOURCE_OPTION_URL "url"
#define RESOURCE_OPTION_FOURCC "4cc"

enum class ContentHandler;
class MetadataHandler;

class CdsResource {
public:
    enum class Purpose : int {
        Content = 0,
        Thumbnail,
        Subtitle,
        Transcode
    };
    enum class Attribute : int {
        SIZE = 0,
        DURATION,
        BITRATE,
        SAMPLEFREQUENCY,
        NRAUDIOCHANNELS,
        RESOLUTION,
        COLORDEPTH,
        PROTOCOLINFO,
        RESOURCE_FILE,
        TYPE,
        FANART_OBJ_ID,
        FANART_RES_ID,
        BITS_PER_SAMPLE,
        LANGUAGE,
        AUDIOCODEC,
        VIDEOCODEC,
        FORMAT,
        MAX
    };

    /// \brief creates a new resource object.
    ///
    /// The CdsResource object represents a <res> tag in the DIDL-Lite XML.
    ///
    /// \param ContentHandler handlerType of the associated handler
    /// \param Purpose purpose of the associated handler
    explicit CdsResource(ContentHandler handlerType, Purpose purpose, std::string_view options = {}, std::string_view parameters = {});
    CdsResource(ContentHandler handlerType, Purpose purpose,
        std::map<Attribute, std::string> attributes,
        std::map<std::string, std::string> parameters,
        std::map<std::string, std::string> options);

    int getResId() const { return resId; }
    void setResId(int rId) { resId = rId; }
    /// \brief Adds a resource attribute.
    ///
    /// This maps to an attribute of the <res> tag in the DIDL-Lite XML.
    ///
    /// \param name attribute name
    /// \param value attribute value
    void addAttribute(Attribute res, std::string value);

    /// \brief Merge existing attributes with new ones
    void mergeAttributes(const std::map<Attribute, std::string>& additional);

    /// \brief Adds a parameter (will be appended to the URL)
    ///
    /// The parameters will be appended to the object URL in the DIDL-Lite XML.
    /// This is useful for cases, where you need to identify specific options,
    /// i.e. something that is only relevant to a particular metadata handler
    /// and so on. The parameters will be automatically URL encoded.
    ///
    /// \param name parameter name
    /// \param value parameter value
    void addParameter(std::string name, std::string value);

    /// \brief Add an option to the resource.
    ///
    /// The options are internal, they do not appear in the URL or in the
    /// XML but can be used for any purpose.
    void addOption(std::string name, std::string value);

    /// \brief Type of resource handler
    ContentHandler getHandlerType() const { return handlerType; }

    const std::map<Attribute, std::string>& getAttributes() const;
    const std::map<std::string, std::string>& getParameters() const;
    const std::map<std::string, std::string>& getOptions() const;
    std::string getAttribute(Attribute attr) const;
    std::string getAttributeValue(Attribute attr) const;
    std::string getParameter(const std::string& name) const;
    std::string getOption(const std::string& name) const;

    static Purpose remapPurpose(int ip) { return static_cast<Purpose>(ip); }
    static std::string getPurposeDisplay(Purpose purpose);
    Purpose getPurpose() const { return purpose; }
    void setPurpose(Purpose purpose) { this->purpose = purpose; }

    bool equals(const std::shared_ptr<CdsResource>& other) const;
    std::shared_ptr<CdsResource> clone();

    static std::shared_ptr<CdsResource> decode(const std::string& serial);

    // FIXME Move out interval values to somewhere else? Options?
    static bool isPrivateAttribute(CdsResource::Attribute attribute)
    {
        switch (attribute) {
        case Attribute::RESOURCE_FILE:
        case Attribute::FANART_OBJ_ID:
        case Attribute::FANART_RES_ID:
        case Attribute::TYPE:
        case Attribute::FORMAT:
            return true;
        default:
            return false;
        }
    }

    static std::string getAttributeName(Attribute attr);
    static std::string getAttributeDisplay(Attribute attr);
    static std::string formatSizeValue(double value);
    static Attribute mapAttributeName(const std::string& name);

protected:
    Purpose purpose { Purpose::Content };
    ContentHandler handlerType;
    int resId { -1 };
    std::map<Attribute, std::string> attributes;
    std::map<std::string, std::string> parameters;
    std::map<std::string, std::string> options;

    inline static const std::map<Attribute, std::string> attrToName {
        { CdsResource::Attribute::SIZE, "size" },
        { CdsResource::Attribute::DURATION, "duration" },
        { CdsResource::Attribute::BITRATE, "bitrate" },
        { CdsResource::Attribute::SAMPLEFREQUENCY, "sampleFrequency" },
        { CdsResource::Attribute::NRAUDIOCHANNELS, "nrAudioChannels" },
        { CdsResource::Attribute::RESOLUTION, "resolution" },
        { CdsResource::Attribute::COLORDEPTH, "colorDepth" },
        { CdsResource::Attribute::PROTOCOLINFO, "protocolInfo" },
        { CdsResource::Attribute::RESOURCE_FILE, "resFile" },
        { CdsResource::Attribute::FANART_OBJ_ID, "fanArtObject" },
        { CdsResource::Attribute::FANART_RES_ID, "fanArtResource" },
        { CdsResource::Attribute::BITS_PER_SAMPLE, "bitsPerSample" },
        { CdsResource::Attribute::LANGUAGE, "dc:language" },
        { CdsResource::Attribute::AUDIOCODEC, "sec:acodec" },
        { CdsResource::Attribute::VIDEOCODEC, "sec:vcodec" },
        { CdsResource::Attribute::FORMAT, "format" },
        { CdsResource::Attribute::TYPE, "type" },
    };
    inline static const std::map<Attribute, std::string> attrToDisplay {
        { CdsResource::Attribute::SIZE, "size" },
        { CdsResource::Attribute::DURATION, "duration" },
        { CdsResource::Attribute::BITRATE, "bitrate" },
        { CdsResource::Attribute::SAMPLEFREQUENCY, "sampleFrequency" },
        { CdsResource::Attribute::NRAUDIOCHANNELS, "nrAudioChannels" },
        { CdsResource::Attribute::RESOLUTION, "resolution" },
        { CdsResource::Attribute::COLORDEPTH, "colorDepth" },
        { CdsResource::Attribute::PROTOCOLINFO, "protocolInfo" },
        { CdsResource::Attribute::RESOURCE_FILE, "resFile" },
        { CdsResource::Attribute::FANART_OBJ_ID, "fanArtObject" },
        { CdsResource::Attribute::FANART_RES_ID, "fanArtResource" },
        { CdsResource::Attribute::BITS_PER_SAMPLE, "bitsPerSample" },
        { CdsResource::Attribute::LANGUAGE, "language" },
        { CdsResource::Attribute::AUDIOCODEC, "audioCodec" },
        { CdsResource::Attribute::VIDEOCODEC, "videoCodec" },
        { CdsResource::Attribute::FORMAT, "format" },
        { CdsResource::Attribute::TYPE, "type" },
    };
    inline static const std::map<Purpose, std::string> purposeToDisplay {
        { CdsResource::Purpose::Content, "Content" },
        { CdsResource::Purpose::Thumbnail, "Thumbnail" },
        { CdsResource::Purpose::Subtitle, "Subtitle" },
        { CdsResource::Purpose::Transcode, "Transcode" },
    };
};

using ResourceAttributeIterator = EnumIterator<CdsResource::Attribute, CdsResource::Attribute::SIZE, CdsResource::Attribute::MAX>;

#endif // __CDS_RESOURCE_H__
