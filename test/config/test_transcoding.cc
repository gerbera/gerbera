/*GRB*

    Gerbera - https://gerbera.io/

    test_transcoding.cc - this file is part of Gerbera.

    Copyright (C) 2026 Gerbera Contributors

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

#include "config/result/transcoding.h"

#include <gtest/gtest.h>

TEST(TranscodingProfileTest, SeekSizeFromBitrateOfExternalProfile)
{
    TranscodingProfile profile(true, TranscodingType::External, "seek");
    EXPECT_EQ(profile.getSeekSize("0:01:00.000"), -1);

    profile.setBitrate(8000000);
    // one second short of the duration, at 1 MB/s
    EXPECT_EQ(profile.getSeekSize("0:01:00.000"), 59000000);
    EXPECT_EQ(profile.getSeekSize(""), -1);
}

TEST(TranscodingProfileTest, NoSeekSizeForInternalProfile)
{
    TranscodingProfile profile(true, TranscodingType::Internal, "seek");
    profile.setBitrate(8000000);
    EXPECT_EQ(profile.getBitrate(), 8000000);
    EXPECT_EQ(profile.getSeekSize("0:01:00.000"), -1);
}
