/*GRB*
    Gerbera - https://gerbera.io/

    test_autoscan_list.cc - this file is part of Gerbera.

    Copyright (C) 2016-2024 Gerbera Contributors

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
#include <gtest/gtest.h>

#include "config/result/autoscan.h"
#include "content/autoscan_list.h"

TEST(AutoscanListTest, create)
{
    auto list = AutoscanList();

    auto adir = std::make_shared<AutoscanDirectory>();
    adir->setLocation({"/path/to/thing"});

    list.add(adir);

    EXPECT_EQ(list.size(), 1);

    EXPECT_EQ(list.get({"/path/to/thing"}), adir);

    EXPECT_EQ(list.get({"/path/to/other"}), nullptr);
}
