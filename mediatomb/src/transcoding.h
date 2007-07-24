/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    autoscan.h - this file is part of MediaTomb.
    
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

class TranscodingProfile : public zmm::Object
{
public:
    TranscodingProfile() { first_resource = false; }
    TranscodingProfile(zmm::String name) { this->name = name; }

    void setName(zmm::String name) { this->name = name; }
    zmm::String getName() { return name; }

    void setTargetMimeType(zmm::String tm) { this->tm = tm; }
    zmm::String getTargetMimeType() { return tm; }

    void setAcceptedMimeType(zmm::String am) { this->am = am; }
    zmm::String getAcceptedMimeType() { return am; }

    void setCommand(zmm::String command) { this->command = command; }
    zmm::String getCommand() { return command; }

    void setInputOptions(zmm::String in_opts) { this->in_opts = in_opts; }
    zmm::String getInputOptions() { return in_opts; }

    void setOutputOptions(zmm::String out_opts) { this->out_opts = out_opts; }
    zmm::String getOutputOptions() { return out_opts; }

    void setFirstResource(bool fr) { first_resource = fr; }
    bool firstResource() { return first_resource; }
    

protected:
    zmm::String name;
    zmm::String tm;
    zmm::String am;
    zmm::String command;
    zmm::String in_opts;
    zmm::String out_opts;
    bool first_resource;
};

class TranscodingProfileList : public zmm::Object
{
public:
    TranscodingProfileList();
    void add(zmm::Ref<TranscodingProfile> prof);
    zmm::Ref<TranscodingProfile> get(zmm::String acceptedMimeType);
    zmm::Ref<TranscodingProfile> get(int index);
    zmm::Ref<TranscodingProfile> getByName(zmm::String name);
protected:
    zmm::Ref<zmm::Array<TranscodingProfile> > list;
};

#endif//__TRANSCODING_H__

#endif//TRANSCODING
