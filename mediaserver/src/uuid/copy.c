/*
 * copy.c --- copy UUIDs
 * 
 * Copyright (C) 1996, 1997 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU 
 * Library General Public License.
 * %End-Header%
 */

#include "uuidP.h"

void uuid_copy(uuid_t dst, const uuid_t src)
{
	unsigned char		*cp1;
	const unsigned char	*cp2;
	int			i;

	for (i=0, cp1 = dst, cp2 = src; i < 16; i++)
		*cp1++ = *cp2++;
}
