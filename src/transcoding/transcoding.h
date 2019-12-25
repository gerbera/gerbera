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

#include <vector>
#include <string>

#include "zmm/zmmf.h"
#include "dictionary.h"
#include "object_dictionary.h"

#define SOURCE  (-1)
#define OFF       0

typedef enum 
{
    TR_None,
    TR_External,
    TR_Remote
} transcoding_type_t;

typedef enum 
{
    FCC_None,
    FCC_Process,
    FCC_Ignore
} avi_fourcc_listmode_t;

/// \brief this class keeps all data associated with one transcoding profile.
class TranscodingProfile : public zmm::Object
{
public:
    TranscodingProfile(transcoding_type_t tr_type, std::string name);

    /// \brief returns the transcoding type.
    transcoding_type_t getType() { return tr_type; }

    /// \brief set name of the transcoding profile
    void setName(std::string name) { this->name = name; }

    /// \brief get name of the transcoding profile
    std::string getName() { return name; }

    /// \brief set target mimetype
    ///
    /// The so called "target mimetype" is the mimetype of the media that will
    /// be produced by the transcoder and identifies the target format.
    void setTargetMimeType(std::string tm) { this->tm = tm; }

    /// \brief get target mimetype
    std::string getTargetMimeType() { return tm; }

    /// \brief sets the program name, i.e. the command line name of the
    /// transcoder that will be executed.
    void setCommand(std::string command) { this->command = command; }

    /// \brief gets the transcoders program name
    std::string getCommand() { return command; }

    /// \brief set buffering options
    /// \param bs the size of the buffer in bytes
    /// \param cs the maximum size of the chunks which are read by the buffer
    /// \param ifs the number of bytes which have to be in the buffer
    /// before the first read at the very beginning or after a seek returns;
    /// 0 disables the delay
    void setBufferOptions(size_t bs, size_t cs, size_t ifs);

    size_t getBufferSize() { return buffer_size; }
    size_t getBufferChunkSize() { return chunk_size; }
    size_t getBufferInitialFillSize() { return initial_fill_size; }

    /// \brief sets the arguments that will be fed to the transcoder,
    /// this is the string that comes right after the command.
    /// 
    /// The argument string must contain the special %out token and may contain
    /// the special %in token. The %in token is replaced by the filename of the
    /// appropriate item - this is the source media for the transcoder. The
    /// %out token is replaced by the fifo name that is generated when the
    /// transcoding process is launched. Transcoded data will be read by
    /// the server from the fifo and served via HTTP to the renderer.
    void setArguments(std::string args) { this->args = args; }

    /// \brief retrieves the argument string
    std::string getArguments() { return args; }

    /// \brief identifies if the profile should be set as the first resource
    void setFirstResource(bool fr) { first_resource = fr; }
    bool firstResource() { return first_resource; }

    /// \brief Adds a resource attribute.
    ///
    /// This maps to an attribute of the <res> tag in the DIDL-Lite XML.
    ///
    /// \param name attribute name 
    /// \param value attribute value
    void addAttribute(std::string name, std::string value);

    zmm::Ref<Dictionary> getAttributes();

    /// \brief Override for theora content.
    ///
    /// I could not find a more generic way for this, I guess this could
    /// be extended to a dictionary style options if more things like that
    /// become necessary; we need a possibility to have a more fine grained
    /// detection of content where mimetype alone is not enough.
    void setTheora(bool theora) { this->theora = theora; }
    bool isTheora() { return theora; }

    /// \brief Specifies if the transcoding profile directly accepts an URL
    /// or if we should proxy the data.
    void setAcceptURL(bool accept) { accept_url = accept; }
    bool acceptURL() { return accept_url; }

    /// \brief Specifies if the output of the profile is a thumbnail,
    /// this will add appropriate DLNA tags to the XML.
    void setThumbnail(bool th) { thumbnail = th; }
    bool isThumbnail() { return thumbnail; }

    /// \brief Specifies if the availability of this transcoding profile
    /// should enforce hiding of all original resources in the browse XML
    void setHideOriginalResource(bool hide) { hide_orig_res = hide; }
    bool hideOriginalResource() { return hide_orig_res; }

    /// \brief If the profile handles source content in the AVI container,
    /// we can specify a list of fourcc's; the list can be either processed
    /// (i.e. the profile will process only AVI files that have a fourcc which
    /// matches an entry in the list), or we can specify that the AVIs that
    /// match a fourcc in the list are ignored and not transcoded.
    ///
    /// \param list List of FourCC entries.
    /// \param mode Specifies if the FourCCs in the list are accepted or ignored
    void setAVIFourCCList(std::vector<std::string> list,
                          avi_fourcc_listmode_t mode = FCC_Ignore);

    /// \brief Retrieves the FourCC list
    std::vector<std::string> getAVIFourCCList();
    /// \brief Provides information on the mode of the list
    avi_fourcc_listmode_t getAVIFourCCListMode() { return fourcc_mode; }

    /// \brief Send out the data in chunked encoding
    void setChunked(bool chunked) { force_chunked = chunked; }
    bool getChunked() { return force_chunked; }

    /// \brief Sample frequency handling
    void setSampleFreq(int freq) { sample_frequency = freq; }
    int getSampleFreq() { return sample_frequency; }

    /// \brief Number of channels
    void setNumChannels(int chans) { number_of_channels = chans; }
    int getNumChannels() { return number_of_channels; }

protected:
    std::string name;
    std::string tm;
    std::string command;
    std::string args;
    bool first_resource;
    bool theora;
    bool accept_url;
    bool hide_orig_res;
    bool thumbnail;
    bool force_chunked;
    size_t buffer_size;
    size_t chunk_size;
    size_t initial_fill_size;
    transcoding_type_t tr_type;
    int number_of_channels;
    int sample_frequency;
    zmm::Ref<Dictionary> attributes;
    std::vector<std::string> fourcc_list;
    avi_fourcc_listmode_t fourcc_mode;
    TranscodingProfile();
};

/// \brief this class allows access to available transcoding profiles.
class TranscodingProfileList : public zmm::Object
{
public:
    TranscodingProfileList();
    void add(std::string sourceMimeType, zmm::Ref<TranscodingProfile> prof);
    zmm::Ref<ObjectDictionary<TranscodingProfile> > get(std::string sourceMimeType);
    zmm::Ref<ObjectDictionary<TranscodingProfile> > get(int index);
    zmm::Ref<TranscodingProfile> getByName(std::string name);
    inline int size() { return list->size(); }
protected:
    // outer dictionary is keyed by the source mimetype, inner dictionary by 
    // profile name; this whole construction is necessary to allow to transcode
    // to the same output format but vary things like resolution, bitrate, etc.
    zmm::Ref<ObjectDictionary<ObjectDictionary<TranscodingProfile> > > list;
};

class TranscodingProcess : public zmm::Object
{
public:
    TranscodingProcess(pid_t pid, std::string fname) 
                       { this->pid = pid; this->fname = fname; }

    pid_t getPID() { return pid; }
    std::string getFName() { return fname; }

protected:
    pid_t pid;
    std::string fname;
};



#endif//__TRANSCODING_H__

