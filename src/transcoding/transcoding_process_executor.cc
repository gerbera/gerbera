/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    transcoding_process_executor.cc - this file is part of MediaTomb.
    
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

/// \file transcoding_process_executor.cc

#ifdef EXTERNAL_TRANSCODING

#include <unistd.h>
#include "transcoding_process_executor.h"

using namespace zmm;

TranscodingProcessExecutor::TranscodingProcessExecutor(String command, Ref<Array<StringBase> > arglist) : ProcessExecutor(command, arglist)
{
};


void TranscodingProcessExecutor::removeFile(String filename)
{
    if (file_list == nil)
        file_list = Ref<Array<StringBase> >(new Array<StringBase>(2));

    file_list->append(filename);
}

TranscodingProcessExecutor::~TranscodingProcessExecutor()
{
    kill();

    if (file_list != nil)
    {
        for (int i = 0; i < file_list->size(); i++)
        {
            String name = file_list->get(i);
            unlink(name.c_str());
        }
    }
}

#endif//EXTERNAL TRANSCODING
