/*  tools.cc - this file is part of MediaTomb.

    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>

    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "tools.h"
#include <sys/stat.h>
#include <sys/time.h>
#include "md5/md5.h"


using namespace zmm;

static char *HEX_CHARS = "0123456789abcdef";

Ref<Array<StringBase> > split_string(String str, char sep)
{
    Ref<Array<StringBase> > ret(new Array<StringBase>());
    char *data = str.c_str();
    char *end = data + str.length();
    while (data < end)
    {
        char *pos = index(data, sep);
        if (pos == NULL)
        {
            String part(data);
            ret->append(part);
            data = end;
        }
        else if (pos == data)
        {
            data++;
        }
        else
        {
            String part(data, pos - data);
            ret->append(part);
            data = pos + 1;
        }
    }            
    return ret;
}

bool check_path(String path, bool needDir)
{
    int ret = 0;
    struct stat statbuf;

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0) return false;

    if ((needDir && (!S_ISDIR(statbuf.st_mode))) ||
       (!needDir && (S_ISDIR(statbuf.st_mode)))) return false;

    return true;
}

void check_path_ex(String path, bool needDir)
{
    int ret = 0;
    struct stat statbuf;

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0)
        throw Exception(path + " : " + strerror(errno));

    if (needDir && (!S_ISDIR(statbuf.st_mode)))
        throw Exception(String("Not a directory: ") + path);
    
    if (!needDir && (S_ISDIR(statbuf.st_mode)))
        throw Exception(String("Not a file: ") + path);

}

bool string_ok(String str)
{
    if ((str == nil) || (str == "")) return false;

    return true;
}

String http_redirect_to(String ip, String port, String page)
{
    return String("<html><head><meta http-equiv=\"Refresh\" content=\"0;URL=http://") + ip + ":" + port + "/" + page + "\"></head><body bgcolor=\"#408bff\"></body></html>";
}

String hex_encode(void *data, int len)
{
    unsigned char *chars;
    int i;
    unsigned char hi, lo;

    Ref<StringBuffer> buf(new StringBuffer(len * 2));
    
    chars = (unsigned char *)data;
    for (i = 0; i < len; i++)
    {
        unsigned char c = chars[i];
        hi = c >> 4;
        lo = c & 0xF;
        *buf << HEX_CHARS[hi] << HEX_CHARS[lo];
    }
    return buf->toString();
    
}

String hex_decode_string(String encoded)
{
	char *ptr = encoded.c_str();
	int len = encoded.length();
	
	Ref<StringBuffer> buf(new StringBuffer(len / 2));
	for (int i = 0; i < len; i += 2)
	{
		char *chi = index(HEX_CHARS, ptr[i]);
		char *clo = index(HEX_CHARS, ptr[i+1]);
		int hi, lo;
		
		if (chi)
			hi = chi - HEX_CHARS;
		else
			hi = 0;

		if (clo)
			lo = clo - HEX_CHARS;
		else
			lo = 0;
		char ch = (char)(hi << 4 | lo);
		*buf << ch;
	}
	return buf->toString();
}


struct randomizer
{
    struct timeval tv;
    int pid;
    int ppid;
};


String generate_random_id()
{
    struct randomizer st;
    gettimeofday(&st.tv, NULL);
    st.pid = rand();
    st.ppid = rand();

    char md5buf[16];

    md5_state_t ctx;
    md5_init(&ctx);
    md5_append(&ctx, (unsigned char *)(&st), sizeof(st));
    md5_finish(&ctx, (unsigned char *)md5buf);

    return hex_encode(md5buf, 16);
}

static const char *hex = "0123456789ABCDEF";

String url_escape(String str)
{
    char *data = str.c_str();
    int len = str.length();
    Ref<StringBuffer> buf(new StringBuffer(len));
    for (int i = 0; i < len; i++)
    {
        unsigned char c = (unsigned char)data[i];
        if ((c >= '0' && c <= '9') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z'))
        {
            *buf << (char)c;
        }
        else
        {
            int hi = c >> 4;
            int lo = c & 15;
            *buf << '%' << hex[hi] << hex[lo];
        }
    }
    return buf->toString();
}

String url_unescape(String str)
{
    char *data = str.c_str();
    int len = str.length();
    Ref<StringBuffer> buf(new StringBuffer(len));

    int i = 0;
    while (i < len)
    {
        char c = data[i++];
        if (c == '%')
        {
            if (i + 2 > len)
                break; // avoid buffer overrun
            char chi = data[i++];
            char clo = data[i++];
            int hi, lo;

            char *pos;

            pos = index(hex, chi);
            if (!pos)
                hi = 0;
            else
                hi = pos - hex;

            pos = index(hex, clo);
            if (!pos)
                lo = 0;
            else
                lo = pos - hex;

            int ascii = (hi << 4) | lo;
            *buf << (char)ascii;
        }
        else if (c == '+')
        {
            *buf << ' ';
        }
        else
        {
            *buf << c;
        }
    }
    return buf->toString();
}

String mime_types_to_CSV(Ref<Array<StringBase> > mimeTypes)
{
    Ref<StringBuffer> buf(new StringBuffer());
    for (int i = 0; i < mimeTypes->size(); i++)
    {
        if (i > 0)
            *buf << ",";
        String mimeType = mimeTypes->get(i);
        *buf << "http-get:*:" << mimeType << ":*";
    }

    return buf->toString();
}


