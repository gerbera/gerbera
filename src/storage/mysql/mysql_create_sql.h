/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    mysql_create_sql.h - this file is part of MediaTomb.
    
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

/// \file mysql_create_sql.h

#ifdef HAVE_MYSQL

#ifndef __MYSQL_CREATE_SQL_H__
#define __MYSQL_CREATE_SQL_H__
#define MS_CREATE_SQL_INFLATED_SIZE 4224
#define MS_CREATE_SQL_DEFLATED_SIZE 1084

/* begin binary data: */
const unsigned char mysql_create_sql[] = /* 1084 */
{0x78,0x9C,0xC5,0x57,0x51,0x8F,0x9B,0x38,0x10,0x7E,0xDF,0x5F,0xE1,0x7B,0x82
,0x54,0xF4,0x36,0xAC,0x52,0xA9,0xA7,0x6A,0xA5,0xE5,0x88,0xDB,0x46,0x25,0xB0
,0x05,0xD2,0x53,0xEF,0xC5,0x38,0xE0,0x6C,0xB8,0x25,0x10,0x81,0x89,0x9A,0x7F
,0x7F,0x63,0x08,0x01,0x82,0x93,0x26,0xD2,0xA9,0xF7,0xB2,0x4B,0x86,0xCF,0x9F
,0x3F,0x66,0xC6,0xE3,0x99,0xFB,0x37,0xBF,0x4D,0xC6,0xFA,0x58,0x47,0x1E,0xF6
,0xD1,0x93,0x63,0x4D,0x89,0xF9,0xD9,0x70,0x0D,0xD3,0xC7,0x2E,0x01,0x13,0x31
,0xAD,0x19,0xB6,0xFD,0xC7,0xA7,0x27,0x99,0x19,0xBD,0xB9,0xFF,0x70,0x77,0xFF
,0x13,0x06,0x17,0x7B,0x0B,0xCB,0xF7,0x06,0x14,0x07,0xFB,0x39,0x0E,0xC7,0xB2
,0x0C,0x7F,0xE6,0xD8,0xF0,0x64,0xDB,0xD8,0x14,0x8F,0x82,0x42,0x62,0x1E,0x32
,0xD8,0xC6,0x1C,0x7B,0xA8,0xE4,0xAB,0xF7,0xED,0xBB,0xB1,0x3E,0x69,0xD9,0x17
,0xF6,0xEC,0xEB,0x02,0x83,0x50,0x6C,0x7E,0x11,0xCA,0x7A,0xBF,0x35,0xD4,0x7F
,0x3D,0x3E,0x43,0xF2,0xD1,0x71,0xF1,0xEC,0x93,0x4D,0xBE,0xE0,0xEF,0x2D,0xD3
,0xD0,0xA8,0x21,0x09,0x70,0x7C,0xE6,0xB3,0xBD,0xAF,0x16,0x99,0x3B,0x53,0x0C
,0x4C,0xCD,0xA3,0x86,0x8E,0x46,0xC5,0x76,0x88,0xB1,0xF0,0x1D,0xF2,0xCD,0xB0
,0x40,0x1F,0x78,0xE1,0x6F,0xEC,0x3A,0x4A,0x87,0x4B,0x3F,0xE1,0xB2,0x1D,0x1F
,0x7B,0x07,0xB2,0xEA,0xB9,0x66,0xAB,0xCD,0xB5,0x08,0xD3,0xC5,0x86,0x8F,0x91
,0x6F,0xFC,0x69,0x61,0x14,0x6C,0x38,0x09,0xA3,0x82,0x64,0xCB,0x7F,0x58,0xC8
,0x03,0xA4,0xDE,0x21,0x14,0xC4,0x51,0x80,0xE2,0x94,0xAB,0xBA,0x3E,0x42,0xB0
,0x12,0xD9,0x0B,0xCB,0x42,0xB4,0xE4,0x19,0x89,0xD3,0x30,0x67,0x1B,0x96,0x72
,0x4D,0xE0,0x72,0xB6,0x22,0x5D,0x6C,0xC4,0x56,0xB4,0x4C,0x78,0x85,0xAF,0x00
,0x5B,0x9A,0x03,0x96,0x48,0xF9,0x1A,0xB0,0x32,0x56,0x2A,0x6C,0xAD,0x80,0xF0
,0xFD,0x96,0x05,0x88,0xC7,0xE9,0x5E,0xAC,0x98,0x8C,0x50,0x99,0x16,0xF1,0x4B
,0xCA,0xA2,0xE3,0xCA,0x0A,0x5D,0x6E,0xD3,0x2D,0x09,0x13,0x5A,0x14,0x01,0xDA
,0xD1,0x3C,0x5C,0xD3,0x5C,0x7D,0x3F,0x96,0x48,0x88,0x42,0xC2,0x63,0x9E,0xB0
,0x16,0xF6,0xF0,0xEE,0x9D,0x04,0x97,0x64,0x21,0xE5,0x71,0x96,0x06,0x68,0x99
,0x64,0xCB,0x9E,0x89,0xAC,0x69,0xB1,0x6E,0xBF,0xE0,0x28,0x68,0xC0,0xB1,0x61
,0x9C,0x46,0x94,0xD3,0x0E,0x07,0x2D,0x7F,0x9C,0x58,0x72,0x56,0x64,0x65,0x1E
,0xB2,0xA2,0x63,0x2B,0xB7,0x00,0x62,0xD7,0xF9,0x69,0x13,0x6F,0xD8,0xC1,0x4B
,0xCD,0x17,0x4D,0x64,0x1F,0xBE,0x4A,0xE8,0x4B,0x21,0x51,0x3D,0x24,0xD6,0x6B
,0x62,0x9E,0xD3,0xF0,0x95,0xA4,0xE5,0x66,0xC9,0xF2,0x0B,0x31,0x2D,0x58,0xBE
,0x8B,0xC3,0x5A,0xEC,0x45,0x97,0x3E,0xBB,0xB3,0xB9,0xE1,0x7E,0x47,0x70,0x0A
,0x10,0x52,0x45,0x52,0x8D,0x84,0x59,0xFC,0x0C,0xDA,0x94,0x23,0x4D,0x12,0xA9
,0x4D,0x3A,0x49,0x51,0x9D,0x4C,0x52,0x3B,0x69,0xA5,0xF5,0xD2,0x46,0x6B,0xA3
,0x2D,0x25,0xE9,0xA5,0x98,0xDA,0x5B,0xDA,0xE2,0x8F,0x51,0xAF,0x77,0x11,0xC0
,0x7E,0x22,0x68,0x9D,0xFD,0xA5,0xDB,0xF4,0x1D,0xA9,0xF6,0x1D,0x2B,0x5D,0xD1
,0xF5,0xA9,0xDA,0xF5,0x70,0x85,0x86,0xCA,0xE7,0xF9,0xAE,0x31,0x83,0xFA,0xDB
,0x3F,0xAE,0x24,0x5E,0xAE,0x5E,0x89,0x1E,0x34,0x05,0xA7,0xE2,0x6D,0xFD,0x88
,0x5C,0xFC,0x11,0xBB,0xD8,0x36,0xA1,0x36,0x0E,0xCE,0x79,0x15,0x0F,0x04,0xC5
,0x74,0x8A,0x2D,0x0C,0xE5,0xC0,0x34,0x3C,0xD3,0x98,0x62,0x61,0x59,0x3C,0x4F
,0x8D,0xD6,0x72,0x85,0x82,0x87,0x53,0x05,0x1D,0x07,0xFD,0x37,0x22,0xEE,0x46
,0x08,0xDB,0x9F,0x66,0x36,0x7E,0x9C,0xEF,0x67,0x9E,0x31,0x47,0xE2,0x6A,0x81
,0xC2,0xF7,0x28,0x6A,0xFE,0x87,0xBB,0x99,0xED,0x61,0xD7,0x47,0xA0,0xCF,0x19
,0x6C,0x52,0x95,0x4E,0x0F,0xA9,0x6F,0x75,0xAD,0xCA,0x4C,0xF8,0x3F,0xAE,0x9F
,0x2E,0xFF,0x39,0x80,0xFE,0x68,0x4D,0xA3,0xEB,0x36,0x1A,0x1F,0xF7,0xD1,0x35
,0xA5,0x7E,0xF9,0x7B,0x98,0xA5,0x9C,0xC6,0x29,0xCB,0x15,0x4D,0x71,0xB3,0x8C
,0x2B,0x37,0xEE,0x7B,0xF0,0xC6,0xE9,0x96,0xA2,0xF4,0x0B,0x1F,0x3E,0x42,0x71
,0x40,0x7F,0x7D,0x06,0x3F,0x1F,0x7E,0xEA,0xCA,0x75,0x5A,0xF5,0x66,0x4F,0xB9
,0xD4,0x67,0x13,0x4D,0xE3,0x1C,0xAC,0x59,0xBE,0xBF,0x55,0xB2,0xF4,0x9A,0xA1
,0x21,0x8F,0x77,0x90,0xD9,0x9C,0x6D,0x2E,0xDC,0x35,0x75,0xE5,0x0C,0xEB,0x72
,0xDC,0xAB,0x31,0x3D,0x44,0xC1,0xA1,0x68,0x5E,0x00,0x9C,0x29,0x40,0x92,0x64
,0xEE,0xC8,0x3A,0x73,0xA6,0x7E,0x59,0x2A,0x0F,0xDC,0x06,0xCE,0x61,0x79,0x4A
,0x13,0x28,0x12,0x1C,0xAE,0xC5,0x97,0x83,0xDF,0x5E,0xD9,0xBE,0x7F,0x01,0xF4
,0x5C,0xB3,0xA3,0x49,0x79,0x83,0x6B,0x04,0xD9,0xE8,0xC6,0x33,0x36,0xD4,0xD5
,0x24,0x95,0x12,0x2D,0xC9,0x8E,0xE5,0x05,0x84,0x0F,0x72,0x68,0xA2,0xC8,0x92
,0x41,0x74,0x13,0x45,0x48,0xD3,0x1B,0x3B,0x0E,0x70,0xF7,0xE5,0x8E,0x43,0x70
,0x92,0x84,0xED,0x58,0x12,0x20,0x06,0x25,0x57,0x55,0x96,0xB4,0x88,0x43,0xD0
,0xB1,0x2A,0x93,0x44,0x39,0xCD,0x20,0x81,0xDE,0x64,0x11,0x6B,0xC0,0x1C,0x2E
,0xD7,0x08,0xC0,0x71,0x9A,0xF1,0x78,0xB5,0x3F,0xC5,0xC3,0x51,0x28,0xE1,0xBB
,0x76,0xD7,0x74,0x28,0xEB,0x38,0x8A,0x58,0x7A,0x05,0xB0,0x72,0x24,0x04,0xEC
,0x9A,0x0E,0x03,0x1A,0x1E,0x2E,0x04,0xC7,0xAB,0x98,0x81,0x1B,0x96,0xF1,0x8B
,0x58,0xF3,0x30,0xBE,0xB4,0x66,0x2B,0x42,0x51,0xF0,0xEA,0x2E,0xBB,0x24,0x66
,0xD0,0x69,0x48,0x5A,0xA2,0x2D,0xE5,0x6B,0x08,0x40,0xB7,0x77,0xE1,0x59,0x19
,0xAE,0x85,0x98,0xEB,0xB8,0xEB,0x66,0xA3,0x9B,0x7F,0x41,0x7D,0xEB,0x35,0xC7
,0xB3,0xEE,0xC5,0xEB,0x37,0x9D,0x44,0x21,0x4D,0xE8,0xD5,0x26,0x09,0x64,0x87
,0xF9,0x88,0x96,0x9F,0xE2,0x66,0xE5,0xFF,0x73,0x92,0xDB,0xF6,0xF0,0xA6,0x9C
,0xAF,0xAB,0xD2,0xB9,0x32,0xB9,0xCD,0x33,0x08,0x30,0xDF,0x93,0x94,0x6E,0x2E
,0x9D,0xF8,0x16,0x78,0x4B,0x6D,0x38,0x89,0x4D,0x1D,0x94,0xC3,0x67,0x90,0xA3
,0x30,0xF5,0xA8,0x51,0x16,0x93,0x16,0x1F,0xAD,0x5E,0x87,0x85,0xB5,0x59,0xF9
,0x4B,0x62,0xD2,0x1B,0xC0,0xDA,0xD9,0xAB,0x3B,0x89,0x0D,0x87,0x3F,0xD9,0xDC
,0x27,0x9F,0x07,0x87,0x6B,0x4F,0x06,0xCF,0xC1,0x2C,0x3A,0x1C,0x0B,0xE5,0xE3
,0xF8,0xB9,0x41,0xFD,0x67,0xEB,0x8F,0xC3,0xF8,0xD9,0x39,0x5D,0xC2,0x20,0x1D
,0xC5,0xCF,0x0D,0xE9,0xC3,0x61,0xB4,0x33,0x87,0xF6,0xC6,0xD2,0x0A,0xF9,0x2F
,0x00,0xF5,0xFB,0x75};
/* end binary data. size = 1084 bytes */

#endif // __MYSQL_CREATE_SQL_H__

#endif // HAVE_MYSQL
