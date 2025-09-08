/*GRB*

    Gerbera - https://gerbera.io/

    autoscan_setting.h - this file is part of Gerbera.

    Copyright (C) 2020-2025 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

    $Id$
*/

/// @file content/autoscan_setting.h
/// @brief Definitions of the AutoScanSetting class.

#ifndef __AUTOSCANSETING_H__
#define __AUTOSCANSETING_H__

#include "util/grb_fs.h"

#include <memory>
#include <vector>

class AutoscanDirectory;
class CdsObject;
class Config;

class AutoScanSetting {
public:
    bool followSymlinks = false;
    bool recursive = true;
    bool hidden = false;
    bool rescanResource = true;
    bool async = true;
    std::shared_ptr<AutoscanDirectory> adir;
    std::shared_ptr<CdsObject> changedObject;
    std::vector<std::string> resourcePatterns;

    void mergeOptions(const std::shared_ptr<Config>& config, const fs::path& location);
};

#endif
