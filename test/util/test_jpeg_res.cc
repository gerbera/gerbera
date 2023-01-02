/*GRB*
    Gerbera - https://gerbera.io/

    test_jpeg_res.cc - this file is part of Gerbera.

    Copyright (C) 2022-2023 Gerbera Contributors

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
//
// Created by Ian Whyman on 19/04/2022.
//

#include <gtest/gtest.h>

#include "iohandler/file_io_handler.h"
#include "util/jpeg_resolution.h"

TEST(JpegParse, parse)
{
    auto file = FileIOHandler("testdata/Gerberas-Dmitry-Makeev-CC-BY-SA-4.0.jpg");
    file.open(UPNP_READ);

    auto res = getJpegResolution(file);

    file.close();

    EXPECT_EQ(res.x(), 290);
    EXPECT_EQ(res.y(), 183);
    EXPECT_EQ(res.string(), "290x183");


}
