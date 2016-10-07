/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     buffer.c                                                   *
 * Created:       2003-04-08 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-04-08 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: buffer.c 67 2004-01-02 18:20:15Z hampa $ */

// The code has been modified to use file descriptors instead of FILE streams.
// Only functionality needed in MediaTomb remains, all extra features are
// stripped out.


#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#ifdef HAVE_LIBDVDNAV

#include <stdlib.h>
#include <unistd.h>

#include "buffer.h"


void mpeg_buf_init (mpeg_buffer_t *buf)
{
  buf->buf = NULL;
  buf->max = 0;
  buf->cnt = 0;
}

void mpeg_buf_free (mpeg_buffer_t *buf)
{
  free (buf->buf);

  buf->buf = NULL;
  buf->cnt = 0;
  buf->max = 0;
}

void mpeg_buf_clear (mpeg_buffer_t *buf)
{
  buf->cnt = 0;
}

int mpeg_buf_set_max (mpeg_buffer_t *buf, unsigned max)
{
  if (buf->max == max) {
    return (0);
  }

  if (max == 0) {
    free (buf->buf);
    buf->max = 0;
    buf->cnt = 0;
    return (0);
  }

  buf->buf = (unsigned char *) realloc (buf->buf, max);
  if (buf->buf == NULL) {
    buf->max = 0;
    buf->cnt = 0;
    return (1);
  }

  buf->max = max;

  if (buf->cnt > max) {
    buf->cnt = max;
  }

  return (0);
}

int mpeg_buf_set_cnt (mpeg_buffer_t *buf, unsigned cnt)
{
  if (cnt > buf->max) {
    if (mpeg_buf_set_max (buf, cnt)) {
      return (1);
    }
  }

  buf->cnt = cnt;

  return (0);
}

int mpeg_buf_read (mpeg_buffer_t *buf, mpeg_demux_t *mpeg, unsigned cnt)
{
  if (mpeg_buf_set_cnt (buf, cnt)) {
    return (1);
  }

  buf->cnt = mpegd_read (mpeg, buf->buf, cnt);

  if (buf->cnt != cnt) {
    return (1);
  }

  return (0);
}

int mpeg_buf_write (mpeg_buffer_t *buf, int fd)
{
  if (buf->cnt > 0) {
    if (write (fd, buf->buf, buf->cnt) != buf->cnt) {
      return (1);
    }
  }

  return (0);
}

int mpeg_buf_write_clear (mpeg_buffer_t *buf, int fd)
{
  if (buf->cnt > 0) {
    if (write (fd, buf->buf, buf->cnt) != buf->cnt) {
      buf->cnt = 0;
      return (1);
    }
  }

  buf->cnt = 0;

  return (0);
}

#endif//HAVE_LIBDVDNAV
