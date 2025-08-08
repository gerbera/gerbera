/*GRB*

    Gerbera - https://gerbera.io/

    inotify_handler.cc - this file is part of Gerbera.

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

/// \file inotify_handler.cc

#ifdef HAVE_INOTIFY
#include "content/inotify/inotify_types.h" // API

#include "util/tools.h"

#include <array>
#include <fmt/format.h>
#if FMT_VERSION >= 100202
#include <fmt/ranges.h>
#endif
#include <numeric>
#include <sys/inotify.h>
#include <vector>

#ifndef IN_MASK_CREATE
#define IN_MASK_CREATE 0x10000000
#endif

static constexpr std::array<std::pair<std::string_view, InotifyFlags>, 22> inotifyFlags {
    /* the following are legal, implemented events that user-space can watch for */
    std::pair("ACCESS", IN_ACCESS), /* File was accessed */
    std::pair("MODIFY", IN_MODIFY), /* File was modified */
    std::pair("ATTRIB", IN_ATTRIB), /* Metadata changed */
    std::pair("CLOSE_WRITE", IN_CLOSE_WRITE), /* Writable file was closed */
    std::pair("CLOSE_NOWRITE", IN_CLOSE_NOWRITE), /* Unwritable file closed */
    std::pair("OPEN", IN_OPEN), /* File was opened */
    std::pair("MOVED_FROM", IN_MOVED_FROM), /* File was moved from X */
    std::pair("MOVED_TO", IN_MOVED_TO), /* File was moved to Y */
    std::pair("CREATE", IN_CREATE), /* Subfile was created */
    std::pair("DELETE", IN_DELETE), /* Subfile was deleted */
    std::pair("DELETE_SELF", IN_DELETE_SELF), /* Self was deleted */
    std::pair("MOVE_SELF", IN_MOVE_SELF), /* Self was moved; Generates an IN_MOVED_FROM event for dir1, an IN_MOVED_TO event for dir2 */
    /* the following are legal events.  they are sent as needed to any watch */
    std::pair("UNMOUNT", IN_UNMOUNT), /* Backing fs was unmounted */
    std::pair("Q_OVERFLOW", IN_Q_OVERFLOW), /* Event queued overflowed */
    std::pair("IGNORED", IN_IGNORED), /* File was ignored */
    /* special flags */
    std::pair("ONLYDIR", IN_ONLYDIR), /* only watch the path if it is a directory */
    std::pair("DONT_FOLLOW", IN_DONT_FOLLOW), /* don't follow a sym link */
    std::pair("EXCL_UNLINK", IN_EXCL_UNLINK), /* exclude events on unlinked objects */
    std::pair("MASK_CREATE", IN_MASK_CREATE), /* only create watches */
    std::pair("MASK_ADD", IN_MASK_ADD), /* add to the mask of an already existing watch */
    std::pair("ISDIR", IN_ISDIR), /* event occurred against dir */
    std::pair("ONESHOT", IN_ONESHOT), /* only send event once */
};

InotifyFlags InotifyUtil::remapFlag(const std::string& flag)
{
    for (auto [qLabel, qFlag] : inotifyFlags) {
        if (flag == qLabel) {
            return qFlag;
        }
    }
    return stoulString(flag, 0, 0);
}

InotifyFlags InotifyUtil::makeFlags(const std::string& optValue)
{
    std::vector<std::string> flagsVector = splitString(optValue, '|', false);
    return std::accumulate(flagsVector.begin(), flagsVector.end(), 0ll, [](auto flg, auto&& i) { return flg | InotifyUtil::remapFlag(trimString(i)); });
}

std::string InotifyUtil::mapFlags(InotifyFlags flags)
{
    if (!flags)
        return "None";

    std::vector<std::string> myFlags;

    for (auto [qLabel, qFlag] : inotifyFlags) {
        if (flags & qFlag) {
            myFlags.emplace_back(qLabel);
            flags &= ~qFlag;
        }
    }

    if (flags) {
        myFlags.push_back(fmt::format("{:#04x}", flags));
    }

    return fmt::format("{}", fmt::join(myFlags, " | "));
}

#endif
