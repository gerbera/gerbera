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
