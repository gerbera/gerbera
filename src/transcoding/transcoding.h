/*MT*

    MediaTomb - http://www.mediatomb.cc/

    transcoding.h - this file is part of MediaTomb.

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

/// \file transcoding.h
/// \brief Definitions of the Transcoding classes.

#ifndef __TRANSCODING_H__
#define __TRANSCODING_H__

#include <map>
#include <vector>

#include "cds_resource.h"
#include "util/grb_fs.h"

#define SOURCE (-1)
#define OFF 0

enum transcoding_type_t {
    TR_None,
    TR_External,
    TR_Remote
};

enum avi_fourcc_listmode_t {
    FCC_None,
    FCC_Process,
    FCC_Ignore
};

/// \brief this class keeps all data associated with one transcoding profile.
class TranscodingProfile {
public:
    TranscodingProfile(bool enabled, transcoding_type_t trType, std::string name);

    int getClientFlags() const { return clientFlags; }
    void setClientFlags(int clientFlags) { this->clientFlags = clientFlags; }

    /// \brief returns the transcoding type.
    transcoding_type_t getType() const { return tr_type; }

    /// \brief set type of the transcoding profile
    void setType(transcoding_type_t type) { this->tr_type = type; }

    /// \brief set name of the transcoding profile
    void setName(std::string name) { this->name = std::move(name); }

    /// \brief get name of the transcoding profile
    std::string getName() const { return name; }

    /// \brief set target mimetype
    ///
    /// The so called "target mimetype" is the mimetype of the media that will
    /// be produced by the transcoder and identifies the target format.
    void setTargetMimeType(std::string tm) { this->tm = std::move(tm); }

    /// \brief get target mimetype
    std::string getTargetMimeType() const { return tm; }

    /// \brief sets the program name, i.e. the command line name of the
    /// transcoder that will be executed.
    void setCommand(fs::path command) { this->command = std::move(command); }

    /// \brief gets the transcoders program name
    fs::path getCommand() const { return command; }

    /// \brief set buffering options
    /// \param bs the size of the buffer in bytes
    /// \param cs the maximum size of the chunks which are read by the buffer
    /// \param ifs the number of bytes which have to be in the buffer
    /// before the first read at the very beginning or after a seek returns;
    /// 0 disables the delay
    void setBufferOptions(std::size_t bs, std::size_t cs, std::size_t ifs);

    std::size_t getBufferSize() const { return buffer_size; }
    std::size_t getBufferChunkSize() const { return chunk_size; }
    std::size_t getBufferInitialFillSize() const { return initial_fill_size; }

    /// \brief sets the arguments that will be fed to the transcoder,
    /// this is the string that comes right after the command.
    ///
    /// The argument string must contain the special %out token and may contain
    /// the special %in token. The %in token is replaced by the filename of the
    /// appropriate item - this is the source media for the transcoder. The
    /// %out token is replaced by the fifo name that is generated when the
    /// transcoding process is launched. Transcoded data will be read by
    /// the server from the fifo and served via HTTP to the renderer.
    void setArguments(std::string args) { this->args = std::move(args); }

    /// \brief retrieves the argument string
    std::string getArguments() const { return args; }
    void setEnviron(std::map<std::string, std::string> environ) { this->environment = std::move(environ); }

    const std::map<std::string, std::string>& getEnviron() const { return environment; }

    /// \brief identifies if the profile should be set as the first resource
    void setFirstResource(bool fr) { first_resource = fr; }
    bool firstResource() const { return first_resource; }

    /// \brief Adds or overwrites a resource attribute.
    ///
    /// This maps to an attribute of the <res> tag in the DIDL-Lite XML.
    ///
    /// \param name attribute name
    /// \param value attribute value
    void setAttributeOverride(CdsResource::Attribute, const std::string& value);
    std::string getAttributeOverride(CdsResource::Attribute attribute) const;

    std::map<CdsResource::Attribute, std::string> getAttributeOverrides() const;

    /// \brief Override for theora content.
    ///
    /// I could not find a more generic way for this, I guess this could
    /// be extended to a dictionary style options if more things like that
    /// become necessary; we need a possibility to have a more fine grained
    /// detection of content where mimetype alone is not enough.
    void setTheora(bool theora) { this->theora = theora; }
    bool isTheora() const { return theora; }

    /// \brief Specifies if the transcoding profile directly accepts an URL
    /// or if we should proxy the data.
    void setAcceptURL(bool accept) { accept_url = accept; }
    bool acceptURL() const { return accept_url; }

    void setDlnaProfile(std::string dlna) { dlnaProf = std::move(dlna); }
    std::string getDlnaProfile() const { return dlnaProf; }

    /// \brief Specifies if the output of the profile is a thumbnail,
    /// this will add appropriate DLNA tags to the XML.
    void setThumbnail(bool th) { thumbnail = th; }
    bool isThumbnail() const { return thumbnail; }

    /// \brief Specifies if the availability of this transcoding profile
    /// should enforce hiding of all original resources in the browse XML
    void setHideOriginalResource(bool hide) { hide_orig_res = hide; }
    bool hideOriginalResource() const { return hide_orig_res; }

    /// \brief If the profile handles source content in the AVI container,
    /// we can specify a list of fourcc's; the list can be either processed
    /// (i.e. the profile will process only AVI files that have a fourcc which
    /// matches an entry in the list), or we can specify that the AVIs that
    /// match a fourcc in the list are ignored and not transcoded.
    ///
    /// \param list List of FourCC entries.
    /// \param mode Specifies if the FourCCs in the list are accepted or ignored
    void setAVIFourCCList(const std::vector<std::string>& list,
        avi_fourcc_listmode_t mode = FCC_Ignore);

    /// \brief Retrieves the FourCC list
    const std::vector<std::string>& getAVIFourCCList() const;
    /// \brief Provides information on the mode of the list
    avi_fourcc_listmode_t getAVIFourCCListMode() const { return fourcc_mode; }

    /// \brief Send out the data in chunked encoding
    void setEnabled(bool new_enabled) { enabled = new_enabled; }
    bool isEnabled() const { return enabled; }

    /// \brief Sample frequency handling
    void setSampleFreq(int freq) { sample_frequency = freq; }
    int getSampleFreq() const { return sample_frequency; }

    /// \brief Number of channels
    void setNumChannels(int chans) { number_of_channels = chans; }
    int getNumChannels() const { return number_of_channels; }

    static std::string mapFourCcMode(avi_fourcc_listmode_t mode);

protected:
    bool enabled { true };
    std::string name;
    std::string tm;
    fs::path command;
    std::string args;
    bool first_resource {};
    bool theora {};
    bool accept_url { true };
    bool hide_orig_res {};
    bool thumbnail {};
    bool force_chunked { true };
    std::size_t buffer_size {};
    std::size_t chunk_size {};
    std::size_t initial_fill_size {};
    transcoding_type_t tr_type;
    int number_of_channels { SOURCE };
    int sample_frequency { SOURCE };
    std::map<CdsResource::Attribute, std::string> attributeOverrides;
    std::map<std::string, std::string> environment;
    std::vector<std::string> fourcc_list;
    avi_fourcc_listmode_t fourcc_mode { FCC_None };
    int clientFlags { 0 };
    std::string dlnaProf;
};

class TranscodingFilter {
public:
    TranscodingFilter(std::string mimeType, std::string transcoder);

    /// \brief mimetype
    std::string getMimeType() const { return mimeType; }
    void setMimeType(std::string mimeType) { this->mimeType = std::move(mimeType); }

    /// \brief transcoding profile name
    std::string getTranscoderName() const { return transcoder; }
    void setTranscoderName(std::string transcoder) { this->transcoder = std::move(transcoder); }

    /// \brief client flags
    int getClientFlags() const { return clientFlags; }
    void setClientFlags(int clientFlags) { this->clientFlags = clientFlags; }

    /// \brief source dlna profile
    std::string getSourceProfile() const { return sourceProf; }
    void setSourceProfile(std::string dlna) { this->sourceProf = std::move(dlna); }

    /// \brief transcoder
    void setTranscodingProfile(std::shared_ptr<TranscodingProfile> profile) { this->transcodingProfile = std::move(profile); }
    std::shared_ptr<TranscodingProfile> getTranscodingProfile() const { return transcodingProfile; }

protected:
    std::string mimeType;
    std::string transcoder;
    std::string sourceProf;
    int clientFlags { 0 };
    std::shared_ptr<TranscodingProfile> transcodingProfile;
};

/// \brief this class allows access to available transcoding profiles.
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
