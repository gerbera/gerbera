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

#include "common.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "tools.h"
typedef unsigned char uchar;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

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

using namespace zmm;

#define ITEM_BUF_SIZE 16
static int Get16m(const void* Short)
{
    return (((uchar*)Short)[0] << 8) | ((uchar*)Short)[1];
}

static int ioh_fgetc(Ref<IOHandler> ioh)
{
    uchar c[1] = { 0 };
    int ret = ioh->read((char*)c, sizeof(char));
    if (ret < 0)
        return ret;
    return (int)c[0];
}

static void get_jpeg_resolution(Ref<IOHandler> ioh, int* w, int* h)
{
    int a;

    a = ioh_fgetc(ioh);

    if (a != 0xff || ioh_fgetc(ioh) != M_SOI)
        throw _Exception(_("get_jpeg_resolution: could not read jpeg specs"));

    for (;;) {
        int itemlen;
        off_t skip;
        int marker = 0;
        int ll, lh, got;
        uchar Data[ITEM_BUF_SIZE];

        for (a = 0; a < 7; a++) {
            marker = ioh_fgetc(ioh);
            if (marker != 0xff)
                break;

            if (a >= 6)
                throw _Exception(_("get_jpeg_resolution: too many padding bytes"));
        }

        // 0xff is legal padding, but if we get that many, something's wrong.
        if (marker == 0xff)
            throw _Exception(_("get_jpeg_resolution: too many padding bytes!"));

        // Read the length of the section.
        lh = ioh_fgetc(ioh);
        ll = ioh_fgetc(ioh);

        itemlen = (lh << 8) | ll;

        if (itemlen < 2)
            throw _Exception(_("get_jpeg_resolution: invalid marker"));

        skip = 0;
        if (itemlen > ITEM_BUF_SIZE) {
            skip = itemlen - ITEM_BUF_SIZE;
            itemlen = ITEM_BUF_SIZE;
        }

        // Store first two pre-read bytes.
        Data[0] = (uchar)lh;
        Data[1] = (uchar)ll;

        got = ioh->read((char*)(Data + 2), itemlen - 2);
        if (got != itemlen - 2)
            throw _Exception(_("get_jpeg_resolution: Premature end of file?"));

        ioh->seek(skip, SEEK_CUR);

        switch (marker) {
        case M_EOI: // in case it's a tables-only JPEG stream
            throw _Exception(_("get_jpeg_resolution: No image in jpeg!"));
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
            *w = Get16m(Data + 5);
            *h = Get16m(Data + 3);
            return;
        }
    }
    throw _Exception(_("get_jpeg_resolution: resolution not found"));
}

// IOHandler must be opened
String get_jpeg_resolution(Ref<IOHandler> ioh)
{
    int w, h;
    try {
        get_jpeg_resolution(ioh, &w, &h);
    } catch (const Exception& e) {
        ioh->close();
        throw e;
    }
    ioh->close();

    return String::from(w) + "x" + h;
}
