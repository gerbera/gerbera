/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpegdemux.c                                                *
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

/* $Id: mpegdemux.c 78 2004-04-08 18:57:31Z hampa $ */

// The code has been modified to use file descriptors instead of FILE streams.
// Only functionality needed in MediaTomb remains, all extra features are
// stripped out.


#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_LIBDVDNAV

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mpeg_parse.h"
#include "mpeg_remux.h"
#include "mpegdemux_internal.h"

// thanks to mru for the following defines
#define ISVIDEO(id)     ((id & 0xf0) == 0xe0)
#define ISMPEGAUDIO(id) ((id & 0xe0) == 0xc0)
#define ISAC3(id)       ((id & 0xf8) == 0x80)
#define ISDTS(id)       ((id & 0xf8) == 0x88)
#define ISPCM(id)       ((id & 0xf8) == 0xa0)
#define ISSPU(id)       ((id & 0xe0) == 0x20)

#define STREAM_ID_PRIVATE      0xbd

static int     par_inp = -1;
static int     par_out = -1;

unsigned char   par_stream[256];
unsigned char   par_substream[256];
unsigned char   par_invalid[256];

int             par_empty_pack = 0;
int             par_drop = 1;
int             par_dvdac3 = 0;




int mpeg_stream_excl (unsigned char sid, unsigned char ssid)
{
  if (par_stream[sid] & PAR_STREAM_EXCLUDE) {
    return (1);
  }

  if (sid == 0xbd) {
    if (par_substream[ssid] & PAR_STREAM_EXCLUDE) {
      return (1);
    }
  }

  return (0);
}

/* check if packet is valid. returns 0 if it is. */
int mpeg_packet_check (mpeg_demux_t *mpeg)
{
  return ((par_invalid[mpeg->packet.sid] & PAR_STREAM_EXCLUDE) == 0);
}

int mpeg_copy (mpeg_demux_t *mpeg, int fd, unsigned n)
{
  unsigned char buf[4096];
  unsigned      i, j;

  while (n > 0) {
    i = (n < 4096) ? n : 4096;

    j = mpegd_read (mpeg, buf, i);

    if (j > 0) {
      if (write (fd, buf, j) != j) {
        return (1);
      }
    }

    if (i != j) {
      return (1);
    }

    n -= i;
  }

  return (0);
}

int remux_mpeg(int fd_in, int fd_out, unsigned char keep_video_id, unsigned char keep_audio_id)
{
  unsigned i;
  int      r;

  if (!ISVIDEO(keep_video_id))
      return 1;

  for (i = 0; i < 256; i++) {
    par_stream[i] = PAR_STREAM_EXCLUDE;
    par_substream[i] = PAR_STREAM_EXCLUDE;
    par_invalid[i] = PAR_STREAM_EXCLUDE;
  }

  par_stream[keep_video_id] &= ~PAR_STREAM_EXCLUDE;

  if (ISAC3(keep_audio_id) || ISDTS(keep_audio_id) || ISPCM(keep_audio_id))
  {
      par_stream[STREAM_ID_PRIVATE] &= ~PAR_STREAM_EXCLUDE;
      par_substream[keep_audio_id] &= ~PAR_STREAM_EXCLUDE; 
      par_dvdac3 = 1;
  }
  else
  {
      par_stream[keep_audio_id] &= ~PAR_STREAM_EXCLUDE;
  }

 
  par_inp = fd_in;
  par_out = fd_out;

  if ((par_inp == -1) || (par_out == -1))
  {
      return 1;
  }

  r = mpeg_remux (par_inp, par_out);

  if (r) {
    return (1);
  }

  return (0);
}

#endif//HAVE_LIBDVDNAV
