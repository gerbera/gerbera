/*MT*

    MediaTomb - http://www.mediatomb.cc/

    process_executor.h - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2023 Gerbera Contributors

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

/// \file process_executor.h

#ifndef __PROCESS_EXECUTOR_H__
#define __PROCESS_EXECUTOR_H__

#include <map>
#include <memory>
#include <vector>

#include <unistd.h>

#include "util/executor.h"
#include "util/grb_fs.h"

class ProcessExecutor final : public Executor {
public:
    ProcessExecutor(const std::string& command, const std::vector<std::string>& arglist, const std::map<std::string, std::string>& env, std::vector<fs::path> tempPaths);
    ~ProcessExecutor() override;

    ProcessExecutor(const ProcessExecutor&) = delete;
    ProcessExecutor& operator=(const ProcessExecutor&) = delete;

    bool isAlive() override;
    bool kill() override;
    int getStatus() override;

protected:
    std::vector<fs::path> tempPaths;
    pid_t pid;
    int exitStatus {};
};

#endif // __PROCESS_EXECUTOR_H__
