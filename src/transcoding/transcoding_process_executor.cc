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

#include "transcoding_process_executor.h" // API

TranscodingProcessExecutor::TranscodingProcessExecutor(const std::string& command, const std::vector<std::string>& arglist)
    : ProcessExecutor(command, arglist)
{
}

void TranscodingProcessExecutor::removeFile(const fs::path& filename)
{
    file_list.push_back(filename);
}

TranscodingProcessExecutor::~TranscodingProcessExecutor()
{
    kill();
    for (auto&& name : file_list) {
        fs::remove(name);
    }
}
