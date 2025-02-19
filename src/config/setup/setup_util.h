/*GRB*

    Gerbera - https://gerbera.io/

    setup_util.h - this file is part of Gerbera.

    Copyright (C) 2024-2025 Gerbera Contributors

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

/// \file setup_util.h
/// \brief Definitions of the setup utilities

#ifndef __GRB_SETUP_UTIL_H_
#define __GRB_SETUP_UTIL_H_

#include "util/tools.h"

template <class EditHelperConfig, class Setup, class Option, class Result>
bool updateConfig(std::shared_ptr<EditHelperConfig> list,
    const std::shared_ptr<Config>& config,
    Setup* setup,
    std::shared_ptr<Option>& value,
    const std::string& optItem,
    std::string& optValue,
    std::vector<std::size_t> indexList,
    const std::string& status)
{
    if (indexList.size() > 0) {
        auto statusList = splitString(status, ',');
        auto index = indexList.at(0);
        std::shared_ptr<Result> entry = list->get(index, true);
        if (!statusList.empty()) {
            if (!entry && (statusList.front() == STATUS_ADDED || statusList.front() == STATUS_MANUAL)) {
                entry = std::make_shared<Result>();
                list->add(entry, index);
            }
            if (entry && (statusList.front() == STATUS_REMOVED || statusList.front() == STATUS_KILLED)) {
                list->remove(index, true);
                return true;
            }
            if (entry && statusList.front() == STATUS_RESET) {
                list->add(entry, index);
            }
        }
        if (entry && setup->updateItem(indexList, optItem, config, entry, optValue, status)) {
            return true;
        }
    } else {
        indexList.push_back(0);
    }

    for (std::size_t index = 0; index < list->size(); index++) {
        std::shared_ptr<Result> entry = list->get(index, true);
        indexList[0] = index;
        if (setup->updateItem(indexList, optItem, config, entry, optValue, status)) {
            return true;
        }
    }
    return false;
}

#endif
