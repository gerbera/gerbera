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

PESIOHandler::PESIOHandler(Ref<IOHandler> io_handler, int keep_audio_id)
{
    audio_stream_id = keep_audio_id;
    if (io_handler == nil)
        throw _Exception(_("Invalid IO handler given!"));
    this->io_handler = io_handler;
}

PESIOHandler::~PESIOHandler()
{
}

void PESIOHandler::open(IN enum UpnpOpenFileMode mode)
{
    io_handler->open(mode);
}

// The code in the read function was contributed by 
// Bernhard Ömer <Bernhard.Oemer@arcs.ac.at>
int PESIOHandler::read(OUT char *buf, IN size_t length)
{
    int bytes_read = 0;
    unsigned char *p;
    unsigned char *pend;
    unsigned char *q;
    unsigned char *r;
    unsigned char *buffer = (unsigned char *)buf;

    if (length & 0x7ff) 
        throw _Exception(_("Buffer must be multiple of 2048 bytes"));

    // aditionally checking 2048-byte alignment with tell might be in order
    bytes_read = io_handler->read(buf,length);

    p = buffer;
    pend = buffer + length;

    for (p = buffer ;p < pend-3; p++)
    {
        if(p[0] || p[1] || p[2] != 0x01 || p[3] < 0xbd || p[3] >= 0xf0) 
            continue;
        
        unsigned char *pnext= p < pend-5 ? p+6+((p[4]<<8)|p[5]) : pend;
        if (pnext > pend) 
            pnext=pend;

        if ((p[3]&0xe0) == 0xc0 && p[3] != 0xc0+audio_stream_id || 
             p[3] == 0xbd && p < pend - 8 && 
             (p[6]&0xc0) == 0x80 && (q = p+9+p[8]) < pend && 
              q[0]>=0x80 && q[0] != audio_stream_id) 
        {
                        
            p[3]=0xbe;  /* change SID to 0xBE (padding stream) */
            for (r = p+6; r < pnext; r++) *r=0xff;   /* blank stream with 1s */
                                            
        }
        p = pnext - 1;
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
    io_handler->close();
}

off_t PESIOHandler::length()
{
    return 0;
}

