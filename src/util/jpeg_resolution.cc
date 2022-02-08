/*MT*

    MediaTomb - http://www.mediatomb.cc/

    jpeg_resolution.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

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

/// \file jpeg_resolution.cc

#include "tools.h" // API

#include <cstddef>

#include "iohandler/io_handler.h"

using uchar = unsigned char;

#define M_SOF0 0xC0 // Start Of Frame N
#define M_SOF1 0xC1 // N indicates which compression process
#define M_SOF2 0xC2 // Only SOF0-SOF2 are now in common use
#define M_SOF3 0xC3
#define M_SOF5 0xC5 // NB: codes C4 and CC are NOT SOF markers
#define M_SOF6 0xC6
#define M_SOF7 0xC7
#define M_SOF9 0xC9
#define M_SOF10 0xCA
#define M_SOF11 0xCB
#define M_SOF13 0xCD
#define M_SOF14 0xCE
#define M_SOF15 0xCF
#define M_SOI 0xD8 // Start Of Image (beginning of datastream)
#define M_EOI 0xD9 // End Of Image (end of datastream)
#define M_SOS 0xDA // Start Of Scan (begins compressed data)
#define M_JFIF 0xE0 // Jfif marker
#define M_EXIF 0xE1 // Exif marker
#define M_COM 0xFE // COMment
#define M_DQT 0xDB
#define M_DHT 0xC4
#define M_DRI 0xDD

#define ITEM_BUF_SIZE 16
static int get16m(const std::byte* shrt)
{
    return std::to_integer<int>((shrt[0] << 8) | shrt[1]);
}

static int iohFgetc(const std::unique_ptr<IOHandler>& ioh)
{
    std::byte c[1] {};
    int ret = ioh->read(reinterpret_cast<char*>(c), sizeof(char));
    if (ret < 0)
        return ret;
    return int(c[0]);
}

static std::pair<int, int> getJpegResolution(const std::unique_ptr<IOHandler>& ioh)
{
    int a = iohFgetc(ioh);

    if (a != 0xff || iohFgetc(ioh) != M_SOI)
        throw_std_runtime_error("get_jpeg_resolution: could not read jpeg specs");

    for (;;) {
        int marker = 0;
        std::byte data[ITEM_BUF_SIZE];

        for (a = 0; a < 7; a++) {
            marker = iohFgetc(ioh);
            if (marker != 0xff)
                break;

            if (a >= 6)
                throw_std_runtime_error("get_jpeg_resolution: too many padding bytes");
        }

        // 0xff is legal padding, but if we get that many, something's wrong.
        if (marker == 0xff)
            throw_std_runtime_error("get_jpeg_resolution: too many padding bytes");

        // Read the length of the section.
        int lh = iohFgetc(ioh);
        int ll = iohFgetc(ioh);

        int itemlen = (lh << 8) | ll;
        if (itemlen < 2)
            throw_std_runtime_error("get_jpeg_resolution: invalid marker");

        off_t skip = 0;
        if (itemlen > ITEM_BUF_SIZE) {
            skip = itemlen - ITEM_BUF_SIZE;
            itemlen = ITEM_BUF_SIZE;
        }

        // Store first two pre-read bytes.
        data[0] = std::byte(lh);
        data[1] = std::byte(ll);

        int got = ioh->read(reinterpret_cast<char*>(data + 2), itemlen - 2);
        if (got != itemlen - 2)
            throw_std_runtime_error("get_jpeg_resolution: Premature end of file?");

        ioh->seek(skip, SEEK_CUR);

        switch (marker) {
        case M_EOI: // in case it's a tables-only JPEG stream
            throw_std_runtime_error("get_jpeg_resolution: No image in jpeg");
        case M_SOF0:
        case M_SOF1:
        case M_SOF2:
        case M_SOF3:
        case M_SOF5:
        case M_SOF6:
        case M_SOF7:
        case M_SOF9:
        case M_SOF10:
        case M_SOF11:
        case M_SOF13:
        case M_SOF14:
        case M_SOF15:
            return { get16m(data + 5), get16m(data + 3) };
        }
    }
    throw_std_runtime_error("get_jpeg_resolution: resolution not found");
}

// IOHandler must be opened
std::string get_jpeg_resolution(std::unique_ptr<IOHandler> ioh)
{
    auto wh = std::pair<int, int>();
    try {
        wh = getJpegResolution(ioh);
    } catch (const std::runtime_error&) {
        ioh->close();
        throw;
    }
    ioh->close();

    return fmt::format("{}x{}", wh.first, wh.second);
}
