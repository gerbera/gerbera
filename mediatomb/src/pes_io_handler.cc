/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    pes_io_handler.cc - this dvd is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2008 Gena Batyan <bgeradz@mediatomb.cc>,
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
    
    $Id: pes_io_handler.h 1698 2008-02-23 20:48:30Z lww $
*/

/// \dvd pes_io_handler.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "pes_io_handler.h"

using namespace zmm;

#define MAX_PS_PACKET_SIZE   2048
#define INTERNAL_BUFFER_SIZE MAX_PS_PACKET_SIZE

PESIOHandler::PESIOHandler(Ref<IOHandler> io_handler, int keep_audio_id)
{
    audio_stream_id = keep_audio_id;
    if (io_handler == nil)
        throw _Exception(_("Invalid IO handler given!"));
    this->io_handler = io_handler;
//    buffer = (unsigned char *)MALLOC(INTERNAL_BUFFER_SIZE);
//    if (!buffer)
//        throw _Exception(_("Could not allocate memory for internal buffer!"));

   for (int i = 0; i < STREAMS; i++)
       streams[i] = 0;
}

PESIOHandler::~PESIOHandler()
{
//    if (buffer)
//        free(buffer);
}

void PESIOHandler::open(IN enum UpnpOpenFileMode mode)
{
    io_handler->open(mode);
}

int PESIOHandler::read(OUT char *buf, IN size_t length)
{
    int bytes_read = 0;
    unsigned char* p;
    unsigned char *buffer = (unsigned char *)buf;
    int index = 0;

    if (length < MAX_PS_PACKET_SIZE)
        throw _Exception(_("Buffer must be at least 2048 bytes"));

    bytes_read = io_handler->read(buf, INTERNAL_BUFFER_SIZE);
        for (int i = 0; i < bytes_read-3; i++)
        {
            if ((buffer[i]   != 0x00) || (buffer[i+1] != 0x00) || 
                (buffer[i+2] != 0x01) || (buffer[i+3] <  0xbd) ||
                (buffer[i+3] > 0xDF))
                continue;

            int packet_length = (int)((buffer[i+4]<<8)|buffer[i+5]);
//            printf("PES PACKET SIZE: %d\n", length);
            if ((packet_length == 0) || (packet_length > MAX_PS_PACKET_SIZE))
            {
//                printf("INVALID PES PACKET SIZE!!!\n");
                continue;
            }

            if (i < bytes_read-6)
            {
                int id = 0;
                id = (int)buffer[i+3];

//                printf("Stream id: %0x (%0x): ", id, buffer[i+3]);
                if ((id >= 0xC0) && (id <= 0xDF))
                {
                    printf("AUDIO");
                    streams[id - 0xc0] = id;
                }
//                else if ((id >= 0xE0) && (id <= 0xEF))
//                    printf("VIDEO");
//                else if (id == 0xBA)
//                    printf("PACK HEADER");
                else if (id == 0xBD)
                {
                    printf("PRIVATE 1");
                    // 16 + ?? stuffing
                    p = buffer+i+14;
//                    printf(" Looking for stuff at %0x ", p[0]);
                    int restlen = packet_length - 14;
                    // skip stuffing bytes
                    int count = 0;
                    while (restlen > 0)
                    {
                        if (*p != 0xFF)
                            break;
                        p++;
                        restlen--;
                        count++;
                    }
//                    printf("Skipped %d stuffing bytes\n", count);

                    // now p should be pointing at the substream id
                    int substream_id = (int)p[0];
                    printf(" Substream ID: %0x", substream_id);
                    int aid = (int)p[0];
                    if((aid & 0xE0) == 0x20)
                    {
                        printf(" subtitle");
                    }
                    else if((aid >= 0x80 && aid <= 0x8F) || (aid >= 0x98 && aid <= 0xAF) || (aid >= 0xC0 && aid <= 0xCF)) 
                    {
                        // 1110 0000 b  (high 3 bit: type  low 5: id)
                        switch(aid & 0xE0)
                        {
                            case 0x00:
                                printf(" MPEG ");
                                break;
                            case 0xA0:
                                printf(" DVD PCM ");
                            case 0x80:
                                if((aid & 0xF8) == 0x88)
                                    printf(" DTS ");
                                else
                                    printf(" AC3 ");
                                break;
                            default:
                                printf(" Could not recognize substream ");
                                break;
                        }
                    }

                    if ((substream_id != 0x80) && 
                        (substream_id != 0x81) && 
                        (substream_id != 0x8a))
                    {
                        i = i+packet_length;
                        continue;
                    }
                    if (substream_id != audio_stream_id) 
                    {
                        // make a padding stream out of it
                        buffer[i+3] = 0xBE;
                        printf("Need to padd %d bytes ", packet_length);
                        memset(buffer+i+5, 0xFF, packet_length-5);
                    }
                }
//                else if (id == 0xBE)
//                    printf("PADDING");
                else if (id == 0xBF)
                    printf("PRIVATE 2 (NAVI)");
//                else
//                    printf("UNKNOWN");

                printf("\n");


                i = i+packet_length;
            }
        }
        return bytes_read;
}
   
int PESIOHandler::write(OUT char *buf, IN size_t length)
{
    return -1;
}

void PESIOHandler::seek(IN off_t offset, IN int whence)
{
}

void PESIOHandler::close()
{
    printf("Streams: ");
    for (int i = 0; i < STREAMS; i ++)
        if (streams[i] > 0)
            printf("[%d - %0x] ", i, streams[i]);
    printf("\n");
}

off_t PESIOHandler::length()
{
    return 0;
}

