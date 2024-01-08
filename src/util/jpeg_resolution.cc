/*MT*

    MediaTomb - http://www.mediatomb.cc/

    jpeg_resolution.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

    Copyright (C) 2006-2010 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>

    Copyright (C) 2016-2024 Gerbera Contributors

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

#include <climits>
#include <cstddef>
#include <fmt/core.h>

#include "exceptions.h"
#include "iohandler/io_handler.h"
#include "jpeg_resolution.h"
#include "metadata/resolution.h"

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

constexpr unsigned int ITEM_BUF_SIZE = 16;

static uint16_t getuint16(const std::byte* shrt)
{
    return std::to_integer<uint16_t>(shrt[0]) << CHAR_BIT | std::to_integer<uint8_t>(shrt[1]);
}

static std::uint8_t getUint8(IOHandler& ioh)
{
    std::byte byte {};
    auto ret = ioh.read(&byte, sizeof(std::byte));
    if (ret != 1) {
        throw_std_runtime_error("getJpegResolution: failed to read byte");
    }
    return std::to_integer<uint8_t>(byte);
}

static Resolution getJpegResolutionRaw(IOHandler& ioh)
{
    auto initMark = getUint8(ioh);
    if (initMark != 0xff || getUint8(ioh) != M_SOI) {
        throw_std_runtime_error("getJpegResolution: could not read jpeg specs");
    }

    while (true) {
        uint8_t marker = 0;
        std::byte data[ITEM_BUF_SIZE];

        for (std::size_t a = 0; a < 7; a++) {
            marker = getUint8(ioh);
            if (marker != 0xff) {
                break;
            }
            if (a >= 6) {
                throw_std_runtime_error("getJpegResolution: too many padding bytes");
            }
        }

        // 0xff is legal padding, but if we get that many, something's wrong.
        if (marker == 0xff) {
            throw_std_runtime_error("getJpegResolution: too many padding bytes");
        }

        // Read the length of the section.
        uint16_t lh = getUint8(ioh);
        auto ll = getUint8(ioh);

        uint16_t itemLen = (lh << CHAR_BIT) | ll;
        if (itemLen < 2) {
            throw_std_runtime_error("getJpegResolution: invalid marker");
        }

        off_t skip = 0;
        if (itemLen > ITEM_BUF_SIZE) {
            skip = itemLen - ITEM_BUF_SIZE;
            itemLen = ITEM_BUF_SIZE;
        }

        // Store first two pre-read bytes.
        data[0] = std::byte(lh);
        data[1] = std::byte(ll);

        auto got = ioh.read(data + 2, itemLen - 2);
        if (got < 0 && got != int(itemLen - 2)) {
            throw_std_runtime_error("getJpegResolution: Premature end of file?");
        }

        // Skip the rest
        ioh.seek(skip, SEEK_CUR);

        switch (marker) {
        case M_EOI: // in case it's a tables-only JPEG stream
            throw_std_runtime_error("getJpegResolution: No image in jpeg");
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
            // auto bitsPerColorComponent = std::to_integer<std::uint8_t>(data[2]);
            auto resX = getuint16(data + 5);
            auto resY = getuint16(data + 3);
            return { static_cast<uint64_t>(resX), static_cast<uint64_t>(resY) };
        }
    }
    throw_std_runtime_error("getJpegResolution: resolution not found");
}

// IOHandler must be opened
Resolution getJpegResolution(IOHandler& ioh)
{
    try {
        auto res = getJpegResolutionRaw(ioh);
        ioh.close();
        return res;
    } catch (const std::runtime_error& ex) {
        ioh.close();
        throw;
    }
}
