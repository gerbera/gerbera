#ifndef __CURL_STRTOOFFT_H__
#define __CURL_STRTOOFFT_H__
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2004, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * $Id: strtoofft.h,v 1.14 2004/12/17 18:33:09 giva Exp $
 ***************************************************************************/

/* This code has been adapted for libupnp 
 * by Sergey 'Jin' Bostandzhyan <jin@mediatomb.org> 
 */

#include <stddef.h>
#include "upnpconfig.h"

#ifdef HAVE_STRTOLL
    #define str_to_offt strtoll
#else
    long long upnp_strtoll(const char *nptr, char **endptr, int base);
    #define str_to_offt upnp_strtoll
    #define NEED_UPNP_STRTOLL
#endif

#ifndef LLONG_MIN
    #define LLONG_MIN 0x8000000000000000LL
#endif

#ifndef LLONG_MAX
    #define LLONG_MAX 0x7FFFFFFFFFFFFFFFLL
#endif

#if 0
#if (SIZEOF_CURL_OFF_T > 4) && (SIZEOF_LONG < 8)
#if HAVE_STRTOLL
#define curlx_strtoofft strtoll
#else /* HAVE_STRTOLL */

/* For MSVC7 we can use _strtoi64() which seems to be a strtoll() clone */
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#define curlx_strtoofft _strtoi64
#else /* MSVC7 or later */
curl_off_t curlx_strtoll(const char *nptr, char **endptr, int base);
#define curlx_strtoofft curlx_strtoll
#define NEED_CURL_STRTOLL
#endif /* MSVC7 or later */

#endif /* HAVE_STRTOLL */
#else /* (SIZEOF_CURL_OFF_T > 4) && (SIZEOF_LONG < 8) */
/* simply use strtol() to get numbers, either 32 or 64 bit */
#define curlx_strtoofft strtol
#endif

#if defined(_MSC_VER) || defined(__WATCOMC__)
#define CURL_LLONG_MIN 0x8000000000000000i64
#define CURL_LLONG_MAX 0x7FFFFFFFFFFFFFFFi64
#elif defined(HAVE_LL)
#define CURL_LLONG_MIN 0x8000000000000000LL
#define CURL_LLONG_MAX 0x7FFFFFFFFFFFFFFFLL
#else
#define CURL_LLONG_MIN 0x8000000000000000L
#define CURL_LLONG_MAX 0x7FFFFFFFFFFFFFFFL
#endif

#endif // #if 0
#endif
