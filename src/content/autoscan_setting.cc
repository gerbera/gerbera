/*GRB*

    Gerbera - https://gerbera.io/

    autoscan_setting.cc - this file is part of Gerbera.

    Copyright (C) 2020-2024 Gerbera Contributors

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

/// \file autoscan_setting.cc
///\brief Implementation of the AutoScanSetting class.

#define GRB_LOG_FAC GrbLogFacility::content
#include "autoscan_setting.h" // API

#include "config/config.h"
#include "config/config_val.h"
#include "config/result/directory_tweak.h"

void AutoScanSetting::mergeOptions(const std::shared_ptr<Config>& config, const fs::path& location)
{
    auto tweak = config->getDirectoryTweakOption(ConfigVal::IMPORT_DIRECTORIES_LIST)->getKey(location);
    if (!tweak)
        return;

    if (tweak->hasFollowSymlinks())
        followSymlinks = tweak->getFollowSymlinks();
    if (tweak->hasHidden())
        hidden = tweak->getHidden();
    if (tweak->hasRecursive())
        recursive = tweak->getRecursive();
    if (tweak->hasFanArtFile())
        resourcePatterns.push_back(tweak->getFanArtFile());
    if (tweak->hasSubTitleFile())
        resourcePatterns.push_back(tweak->getSubTitleFile());
    if (tweak->hasResourceFile())
        resourcePatterns.push_back(tweak->getResourceFile());
}
