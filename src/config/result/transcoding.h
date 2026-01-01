/*MT*

    MediaTomb - http://www.mediatomb.cc/

    transcoding.h - this file is part of MediaTomb.

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

/// @file config/result/transcoding.h
/// @brief Definitions of the Transcoding classes.

#ifndef __TRANSCODING_H__
#define __TRANSCODING_H__

#include "cds/cds_enums.h"
#include "common.h"
#include "metadata/metadata_enums.h"
#include "upnp/quirks.h"
#include "util/grb_fs.h"

#include <map>
#include <utility>
#include <vector>

enum class TranscodingType {
    None,
    External,
    Remote
};

enum class AviFourccListmode {
    None,
    Process,
    Ignore
};

class TranscodingMimeProperty {
public:
    TranscodingMimeProperty(std::string key, ResourceAttribute attribute, MetadataFields metadata)
        : key(std::move(key))
        , attribute(attribute)
        , metadata(metadata)
    {
    }
    ResourceAttribute getAttribute() const { return attribute; }
    MetadataFields getMetadata() const { return metadata; }
    std::string getKey() const { return key; }

private:
    std::string key;
    ResourceAttribute attribute = ResourceAttribute::MAX;
    MetadataFields metadata = MetadataFields::M_MAX;
};

/// @brief this class keeps all data associated with one transcoding profile.
class TranscodingProfile {
public:
    TranscodingProfile(bool enabled, TranscodingType trType, std::string name);

    /// @brief Client Flags required to match
    QuirkFlags getClientFlags() const { return clientFlags; }
    void setClientFlags(QuirkFlags clientFlags) { this->clientFlags = clientFlags; }

    /// @brief Clienta without flags match
    bool matchesWithOut() const { return matchWithOut; }
    void setMatchWithOut(bool matchWithOut) { this->matchWithOut = matchWithOut; }

    /// @brief returns the transcoding type.
    TranscodingType getType() const { return trType; }

    /// @brief set type of the transcoding profile
    void setType(TranscodingType type) { this->trType = type; }

    /// @brief set name of the transcoding profile
    void setName(const std::string& name) { this->name = name; }

    /// @brief get name of the transcoding profile
    std::string getName() const { return name; }

    /// @brief set target mimetype
    ///
    /// The so called "target mimetype" is the mimetype of the media that will
    /// be produced by the transcoder and identifies the target format.
    void setTargetMimeType(const std::string& tm) { this->tm = tm; }

    /// @brief get target mimetype
    std::string getTargetMimeType() const { return tm; }
    std::vector<TranscodingMimeProperty> getTargetMimeProperties() const { return mimeProperties; }
    void addTargetMimeProperty(TranscodingMimeProperty property) { mimeProperties.push_back(std::move(property)); }

    /// @brief sets the program name, i.e. the command line name of the
    /// transcoder that will be executed.
    void setCommand(const fs::path& command) { this->command = command; }

    /// @brief gets the transcoders program name
    fs::path getCommand() const { return command; }

    /// @brief set buffering options
    /// @param bs the size of the buffer in bytes
    /// @param cs the maximum size of the chunks which are read by the buffer
    /// @param ifs the number of bytes which have to be in the buffer
    /// before the first read at the very beginning or after a seek returns;
    /// 0 disables the delay
    void setBufferOptions(std::size_t bs, std::size_t cs, std::size_t ifs);

    std::size_t getBufferSize() const { return bufferSize; }
    std::size_t getBufferChunkSize() const { return chunkSize; }
    std::size_t getBufferInitialFillSize() const { return initialFillSize; }

    /// @brief sets the arguments that will be fed to the transcoder,
    /// this is the string that comes right after the command.
    ///
    /// The argument string must contain the special %out token and may contain
    /// the special %in token. The %in token is replaced by the filename of the
    /// appropriate item - this is the source media for the transcoder. The
    /// %out token is replaced by the fifo name that is generated when the
    /// transcoding process is launched. Transcoded data will be read by
    /// the server from the fifo and served via HTTP to the renderer.
    void setArguments(const std::string& args) { this->args = args; }

    /// @brief retrieves the argument string
    std::string getArguments() const { return args; }
    void setEnviron(const std::map<std::string, std::string>& environ) { this->environment = environ; }

    const std::map<std::string, std::string>& getEnviron() const { return environment; }

    /// @brief identifies if the profile should be set as the first resource
    void setFirstResource(bool fr) { firstResource = fr; }
    bool getFirstResource() const { return firstResource; }

    /// @brief Adds or overwrites a resource attribute.
    ///
    /// This maps to an attribute of the \<res\> tag in the DIDL-Lite XML.
    ///
    /// @param attribute attribute name
    /// @param value attribute value
    void setAttributeOverride(ResourceAttribute attribute, const std::string& value);
    std::string getAttributeOverride(ResourceAttribute attribute) const;

    std::map<ResourceAttribute, std::string> getAttributeOverrides() const;

    /// @brief Override for theora content.
    ///
    /// I could not find a more generic way for this, I guess this could
    /// be extended to a dictionary style options if more things like that
    /// become necessary; we need a possibility to have a more fine grained
    /// detection of content where mimetype alone is not enough.
    /// @param theora new value of the flag
    void setTheora(bool theora) { this->theora = theora; }
    bool isTheora() const { return theora; }

    /// @brief Specifies if the transcoding profile directly accepts an URL
    /// or if we should proxy the data.
    void setAcceptURL(bool accept) { acceptUrl = accept; }
    bool getAcceptURL() const { return acceptUrl; }

    void setDlnaProfile(const std::string& dlna) { dlnaProf = dlna; }
    std::string getDlnaProfile() const { return dlnaProf; }

    /// @brief Specifies if the output of the profile is a thumbnail,
    /// this will add appropriate DLNA tags to the XML.
    void setThumbnail(bool th) { thumbnail = th; }
    bool isThumbnail() const { return thumbnail; }

    /// @brief Specifies if the availability of this transcoding profile
    /// should enforce hiding of all original resources in the browse XML
    void setHideOriginalResource(bool hide) { hideOrigRes = hide; }
    bool hideOriginalResource() const { return hideOrigRes; }

    /// @brief If the profile handles source content in the AVI container,
    /// we can specify a list of fourcc's; the list can be either processed
    /// (i.e. the profile will process only AVI files that have a fourcc which
    /// matches an entry in the list), or we can specify that the AVIs that
    /// match a fourcc in the list are ignored and not transcoded.
    ///
    /// @param list List of FourCC entries.
    /// @param mode Specifies if the FourCCs in the list are accepted or ignored
    void setAVIFourCCList(const std::vector<std::string>& list,
        AviFourccListmode mode = AviFourccListmode::Ignore);

    /// @brief Retrieves the FourCC list
    const std::vector<std::string>& getAVIFourCCList() const;
    /// @brief Provides information on the mode of the list
    AviFourccListmode getAVIFourCCListMode() const { return fourccMode; }

    /// @brief Send out the data in chunked encoding
    void setEnabled(bool newEnabled) { enabled = newEnabled; }
    bool isEnabled() const { return enabled; }

    /// @brief Sample frequency handling
    void setSampleFreq(int freq) { sampleFrequency = freq; }
    int getSampleFreq() const { return sampleFrequency; }

    /// @brief Number of channels
    void setNumChannels(int chans) { numberOfChannels = chans; }
    int getNumChannels() const { return numberOfChannels; }

protected:
    bool enabled { true };
    std::string name;
    std::string tm;
    fs::path command;
    std::string args;
    bool firstResource {};
    bool theora {};
    bool acceptUrl { true };
    bool hideOrigRes {};
    bool thumbnail {};
    bool forceChunked { true };
    std::size_t bufferSize {};
    std::size_t chunkSize {};
    std::size_t initialFillSize {};
    TranscodingType trType { TranscodingType::None };
    int numberOfChannels { SOURCE };
    int sampleFrequency { SOURCE };
    std::map<ResourceAttribute, std::string> attributeOverrides;
    std::map<std::string, std::string> environment;
    std::vector<std::string> fourccList;
    AviFourccListmode fourccMode { AviFourccListmode::None };
    QuirkFlags clientFlags { 0 };
    bool matchWithOut;
    std::string dlnaProf;
    std::vector<TranscodingMimeProperty> mimeProperties;
};

class TranscodingFilter {
public:
    TranscodingFilter(std::string mimeType, std::string transcoder);

    /// @brief mimetype to search for
    std::string getMimeType() const { return mimeType; }
    void setMimeType(const std::string& mimeType) { this->mimeType = mimeType; }

    /// @brief mimetype not to be transcoded
    const std::vector<std::string>& getNoTranscodingMimeTypes() const { return noTranscodingMimeTypes; }
    void setNoTranscodingMimeTypes(const std::vector<std::string>& mimeTypes) { this->noTranscodingMimeTypes = mimeTypes; }

    /// @brief transcoding profile name
    std::string getTranscoderName() const { return transcoder; }
    void setTranscoderName(const std::string& transcoder) { this->transcoder = transcoder; }

    /// @brief client flags this filter is enabling
    QuirkFlags getClientFlags() const { return clientFlags; }
    void setClientFlags(QuirkFlags clientFlags) { this->clientFlags = clientFlags; }

    /// @brief Clienta without flags match
    bool matchesWithOut() const { return matchWithOut; }
    void setMatchWithOut(bool matchWithOut) { this->matchWithOut = matchWithOut; }

    /// @brief source dlna profile
    std::string getSourceProfile() const { return sourceProf; }
    void setSourceProfile(const std::string& dlna) { this->sourceProf = dlna; }

    /// @brief transcoder
    void setTranscodingProfile(const std::shared_ptr<TranscodingProfile>& profile) { this->transcodingProfile = profile; }
    std::shared_ptr<TranscodingProfile> getTranscodingProfile() const { return transcodingProfile; }

protected:
    std::string mimeType;
    std::string transcoder;
    std::string sourceProf;
    QuirkFlags clientFlags { 0 };
    bool matchWithOut;
    std::vector<std::string> noTranscodingMimeTypes;
    std::shared_ptr<TranscodingProfile> transcodingProfile;
};

/// @brief this class allows access to available transcoding profiles.
class TranscodingProfileList {
public:
    void add(const std::shared_ptr<TranscodingFilter>& filter) { filterList.push_back(filter); }

    const std::vector<std::shared_ptr<TranscodingFilter>>& getFilterList() const { return filterList; }
    std::shared_ptr<TranscodingProfile> getByName(const std::string& name, bool getAll = false) const;
    std::size_t size() const { return filterList.size(); }

protected:
    std::vector<std::shared_ptr<TranscodingFilter>> filterList;
};

#endif //__TRANSCODING_H__
