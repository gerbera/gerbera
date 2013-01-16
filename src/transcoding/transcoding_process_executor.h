/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    transcoding_process_executor.h - this file is part of MediaTomb.
    
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

/// \file transcoding_process_executor.h

#ifdef EXTERNAL_TRANSCODING

#ifndef __TRANSCODING_PROCESS_EXECUTOR_H__
#define __TRANSCODING_PROCESS_EXECUTOR_H__

#include "process_executor.h"

class TranscodingProcessExecutor : public ProcessExecutor
{
public:
    TranscodingProcessExecutor(zmm::String command,
                               zmm::Ref<zmm::Array<zmm::StringBase> > arglist);
    /// \brief This function adds a filename to a list, files in that list
    /// will be removed once the class is destroyed.
    void removeFile(zmm::String filename);

    virtual ~TranscodingProcessExecutor();

protected:
    /// \brief The files in this list will be removed once the class is no
    /// longer in use.
    zmm::Ref<zmm::Array<zmm::StringBase> > file_list;
};

#endif // __TRANSCODING_PROCESS_EXECUTOR_H__

#endif//EXTERNAL_TRANSCODING
