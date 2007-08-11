/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    transcoding.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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
    
    $Id: transcoding.h $
*/

/// \file transcoding_profile.h
/// \brief Definitions of the Transcoding classes. 

#ifdef TRANSCODING

#ifndef __TRANSCODING_H__
#define __TRANSCODING_H__

#include "zmmf/zmmf.h"
#include "dictionary.h"

/// \brief this class keeps all data associated with one transcoding profile.
class TranscodingProfile : public zmm::Object
{
public:
    TranscodingProfile() { first_resource = false; }
    TranscodingProfile(zmm::String name) { this->name = name; }

    /// \brief set name of the transcoding profile
    void setName(zmm::String name) { this->name = name; }

    /// \brief get name of the transcoding profile
    zmm::String getName() { return name; }

    /// \brief set target mimetype
    ///
    /// The so called "target mimetype" is the mimetype of the media that will
    /// be produced by the transcoder and identifies the target format.
    void setTargetMimeType(zmm::String tm) { this->tm = tm; }

    /// \brief get target mimetype
    zmm::String getTargetMimeType() { return tm; }

    /// \brief sets the program name, i.e. the command line name of the
    /// transcoder that will be executed.
    void setCommand(zmm::String command) { this->command = command; }

    /// \brief gets the transcoders program name
    zmm::String getCommand() { return command; }

    /// \brief sets the arguments that will be fed to the transcoder,
    /// this is the string that comes right after the command.
    /// 
    /// The argument string must contain the special %out token and may contain
    /// the special %in token. The %in token is replaced by the filename of the
    /// appropriate item - this is the source media for the transcoder. The
    /// %out token is replaced by the fifo name that is generated when the
    /// transcoding process is launched. Transcoded data will be read by
    /// the server from the fifo and served via HTTP to the renderer.
    void setArguments(zmm::String args) { this->args = args; }

    /// \brief retrieves the argument string
    zmm::String getArguments() { return args; }

    /// \brief identifies if the profile should be set as the first resource
    void setFirstResource(bool fr) { first_resource = fr; }
    bool firstResource() { return first_resource; }
    

protected:
    zmm::String name;
    zmm::String tm;
    zmm::String command;
    zmm::String args;
    bool first_resource;
};

/// \brief this class allows access to available transcoding profiles.
class TranscodingProfileList : public zmm::Object
{
public:
    TranscodingProfileList();
    void add(zmm::Ref<TranscodingProfile> prof);
    zmm::Ref<TranscodingProfile> get(zmm::String acceptedMimeType);
    zmm::Ref<TranscodingProfile> get(int index);
    zmm::Ref<TranscodingProfile> getByName(zmm::String name);
//    void addMapping(zmm::String mimetype, zmm::String prname);
    void setMappings(zmm::Ref<Dictionary> mappings);
    inline int size() { return list->size(); }
protected:
    zmm::Ref<zmm::Array<TranscodingProfile> > list;
    zmm::Ref<Dictionary> mimetype_profile;
};

class TranscodingProcess : public zmm::Object
{
public:
    TranscodingProcess(pid_t pid, zmm::String fname) 
                       { this->pid = pid; this->fname = fname; }

    pid_t getPID() { return pid; }
    zmm::String getFName() { return fname; }

protected:
    pid_t pid;
    zmm::String fname;
};



#endif//__TRANSCODING_H__

#endif//TRANSCODING
