/*GRB*

    Gerbera - https://gerbera.io/

    test_upnp_map.cc - this file is part of Gerbera.

    Copyright (C) 2025 Gerbera Contributors

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

#include "cds/cds_item.h"
#include "content/import_service.h"
#include "upnp/upnp_common.h"

#include <gtest/gtest.h>

TEST(UpnpMapTest, StringTest)
{
    UpnpMap testMap1("video/", UPNP_CLASS_VIDEO_ITEM, { {"location", "=", "/path" } });
    UpnpMap testMap2("video/", UPNP_CLASS_VIDEO_ITEM, { {"location", "!=", "/path" } });
    UpnpMap testMap3("video/", UPNP_CLASS_VIDEO_ITEM, { {"location", "<", "/path" } });
    UpnpMap testMap4("video/", UPNP_CLASS_VIDEO_ITEM, { {"location", ">", "/path" } });
    UpnpMap testMap5("video/", UPNP_CLASS_VIDEO_ITEM, { {"upnp:artist", "==", "Artist" } });
    auto item = std::make_shared<CdsItem>();
    item->setID(1);
    item->setParentID(2);
    item->setRestricted(false);
    item->setTitle("Title");
    item->setMimeType("video/mp4");
    item->setClass(UPNP_CLASS_VIDEO_ITEM);
    item->setLocation("/path");
    item->addMetaData(MetadataFields::M_ARTIST, "Artist");

    EXPECT_TRUE(testMap1.isMatch(item, "video/mp4"));
    EXPECT_FALSE(testMap2.isMatch(item, "video/mp4"));
    EXPECT_FALSE(testMap3.isMatch(item, "video/mp4"));
    EXPECT_FALSE(testMap4.isMatch(item, "video/mp4"));
    EXPECT_TRUE(testMap5.isMatch(item, "video/mp4"));
}

TEST(UpnpMapTest, NumberTest)
{
    UpnpMap testMap1("video/", UPNP_CLASS_VIDEO_ITEM, { {"tracknumber", "=", "1" } });
    UpnpMap testMap2("video/", UPNP_CLASS_VIDEO_ITEM, { {"tracknumber", "!=", "1" } });
    UpnpMap testMap3("video/", UPNP_CLASS_VIDEO_ITEM, { {"tracknumber", "<", "1" } });
    UpnpMap testMap4("video/", UPNP_CLASS_VIDEO_ITEM, { {"tracknumber", "<", "2" } });
    UpnpMap testMap5("video/", UPNP_CLASS_VIDEO_ITEM, { {"tracknumber", ">", "1" } });
    UpnpMap testMap6("video/", UPNP_CLASS_VIDEO_ITEM, { {"tracknumber", ">", "0" } });
    auto item = std::make_shared<CdsItem>();
    item->setID(1);
    item->setParentID(2);
    item->setRestricted(false);
    item->setTitle("Title");
    item->setMimeType("video/mp4");
    item->setClass(UPNP_CLASS_VIDEO_ITEM);
    item->setLocation("/path");
    item->setTrackNumber(1);
    item->addMetaData(MetadataFields::M_ARTIST, "Artist");

    EXPECT_TRUE(testMap1.isMatch(item, "video/mp4"));
    EXPECT_FALSE(testMap2.isMatch(item, "video/mp4"));
    EXPECT_FALSE(testMap3.isMatch(item, "video/mp4"));
    EXPECT_TRUE(testMap4.isMatch(item, "video/mp4"));
    EXPECT_FALSE(testMap5.isMatch(item, "video/mp4"));
    EXPECT_TRUE(testMap6.isMatch(item, "video/mp4"));
}
