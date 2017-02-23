/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpegdemux.h                                                *
 * Created:       2003-02-01 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2004-04-08 by Hampa Hug <hampa@hampa.ch>                   *
 * Copyright:     (C) 2003-2004 Hampa Hug <hampa@hampa.ch>                   *
 *****************************************************************************/

/*****************************************************************************
 * This program is free software. You can redistribute it and / or modify it *
 * under the terms of the GNU General Public License version 2 as  published *
 * by the Free Software Foundation.                                          *
 *                                                                           *
 * This program is distributed in the hope  that  it  will  be  useful,  but *
 * WITHOUT  ANY   WARRANTY,   without   even   the   implied   warranty   of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  General *
 * Public License for more details.                                          *
 *****************************************************************************/

/* $Id: mpegdemux.h 78 2004-04-08 18:57:31Z hampa $ */

// The code has been modified to use file descriptors instead of FILE streams.
// Only functionality needed in MediaTomb remains, all extra features are
// stripped out.


#ifndef MPEGDEMUX_INTERNAL_H
#define MPEGDEMUX_INTERNAL_H 1


#define PAR_STREAM_EXCLUDE 1

#define PAR_MODE_SCAN  0
#define PAR_MODE_LIST  1
#define PAR_MODE_REMUX 2
#define PAR_MODE_DEMUX 3


extern unsigned char par_stream[256];
extern unsigned char par_substream[256];
extern unsigned char par_invalid[256];
extern int           par_empty_pack;
extern int           par_drop;
extern int           par_dvdac3;

char *mpeg_get_name (const char *base, unsigned sid);
int mpeg_stream_excl (unsigned char sid, unsigned char ssid);
int mpeg_packet_check (mpeg_demux_t *mpeg);
int mpeg_copy (mpeg_demux_t *mpeg, int fd, unsigned n);


#endif
