/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpeg_remux.c                                               *
 * Created:       2003-02-02 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-09-10 by Hampa Hug <hampa@hampa.ch>                   *
 * Copyright:     (C) 2003 by Hampa Hug <hampa@hampa.ch>                     *
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

/* $Id: mpeg_remux.c 67 2004-01-02 18:20:15Z hampa $ */

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

#include "buffer.h"
#include "mpeg_parse.h"
#include "mpeg_remux.h"
#include "mpegdemux_internal.h"


static mpeg_buffer_t shdr = { NULL, 0, 0 };
static mpeg_buffer_t pack = { NULL, 0, 0 };
static mpeg_buffer_t packet = { NULL, 0, 0 };

static
int mpeg_remux_system_header (mpeg_demux_t *mpeg)
{
  if (mpeg_buf_write_clear (&pack, mpeg->ext)) {
    return (1);
  }

  if (mpeg_buf_read (&shdr, mpeg, mpeg->shdr.size)) {
    return (1);
  }

  if (mpeg_buf_write_clear (&shdr, mpeg->ext)) {
    return (1);
  }

  return (0);
}

static
int mpeg_remux_packet (mpeg_demux_t *mpeg)
{
  int      r;
  unsigned sid, ssid;

  sid = mpeg->packet.sid;
  ssid = mpeg->packet.ssid;

  if (mpeg_stream_excl (sid, ssid)) {
    return (0);
  }

  r = 0;

  if (mpeg_buf_read (&packet, mpeg, mpeg->packet.size)) {
    fprintf(stderr, "remux: incomplete packet (sid=%02x size=%u/%u)\n",
      sid, packet.cnt, mpeg->packet.size
    );

    if (par_drop) {
      mpeg_buf_clear (&packet);
      return (1);
    }

    r = 1;
  }

  if (mpeg_buf_write_clear (&pack, mpeg->ext)) {
    return (1);
  }

  if (mpeg_buf_write_clear (&packet, mpeg->ext)) {
    return (1);
  }

  return (r);
}

static
int mpeg_remux_pack (mpeg_demux_t *mpeg)
{
  if (mpeg_buf_read (&pack, mpeg, mpeg->pack.size)) {
    return (1);
  }

  if (par_empty_pack) {
    if (mpeg_buf_write_clear (&pack, mpeg->ext)) {
      return (1);
    }
  }

  return (0);
}

static
int mpeg_remux_end (mpeg_demux_t *mpeg)
{
  if (mpeg_copy (mpeg, mpeg->ext, 4)) {
    return (1);
  }

  return (0);
}

int mpeg_remux (int inp, int out)
{
  int          r;
  mpeg_demux_t *mpeg;

  mpeg = mpegd_open_fd (NULL, inp, 0);
  if (mpeg == NULL) {
    return (1);
  }

  mpeg->ext = out;
  mpeg->mpeg_system_header = &mpeg_remux_system_header;
  mpeg->mpeg_pack = &mpeg_remux_pack;
  mpeg->mpeg_packet = &mpeg_remux_packet;
  mpeg->mpeg_packet_check = &mpeg_packet_check;
  mpeg->mpeg_end = &mpeg_remux_end;

  mpeg_buf_init (&shdr);
  mpeg_buf_init (&pack);
  mpeg_buf_init (&packet);

  r = mpegd_parse (mpeg);

  mpegd_close (mpeg);

  mpeg_buf_free (&shdr);
  mpeg_buf_free (&pack);
  mpeg_buf_free (&packet);

  return (r);
}

#endif//HAVE_LIBDVDNAV
