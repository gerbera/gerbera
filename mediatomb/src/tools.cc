/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    tools.cc - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
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

/// \file tools.cc

#ifdef HAVE_CONFIG_H
    #include "autoconfig.h"
#endif

#include "tools.h"
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <netdb.h>
#include <string.h>

#ifndef SOLARIS
    #include <net/if.h>
#else
    #include <fcntl.h>
    #include <net/if.h>
    #include <sys/sockio.h>
#endif

#include "md5/md5.h"
#include "file_io_handler.h"
#include "metadata_handler.h"

#define WHITE_SPACE " \t\r\n"

using namespace zmm;

static char *HEX_CHARS = "0123456789abcdef";

Ref<Array<StringBase> > split_string(String str, char sep, bool empty)
{
    Ref<Array<StringBase> > ret(new Array<StringBase>());
    char *data = str.c_str();
    char *end = data + str.length();
    while (data < end)
    {
        char *pos = strchr(data, sep);
        if (pos == NULL)
        {
            String part(data);
            ret->append(part);
            data = end;
        }
        else if (pos == data)
        {
            data++;
            if ((data < end) && empty)
                ret->append(String(""));
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

Ref<Array<StringBase> > split_path(String str)
{
    if (! string_ok(str))
        throw _Exception(_("invalid path given to split_path"));
    Ref<Array<StringBase> > ret(new Array<StringBase>());
    int pos = str.rindex(DIR_SEPARATOR);
    const char *data = str.c_str();
    
    if (pos < 0)
        throw _Exception(_("relative path given to split_path: ") + str);
    
    if (pos == 0)
    {
        /* there is only one separator at the beginning "/..." or "/" */
        ret->append(_(_DIR_SEPARATOR));
        String filename(data+1);
        ret->append(filename);
    }
    else
    {
        String path(data, pos);
        ret->append(path);
        String filename(data + pos + 1);
        ret->append(filename);
    }
    return ret;
}

String trim_string(String str)
{
    if (str == nil)
        return nil;
    int i;
    int start = 0;
    int end = 0;
    int len = str.length();

    char *buf = str.c_str();

    for (i = 0; i < len; i++)
    {
        if (! strchr(WHITE_SPACE, buf[i]))
        {
            start = i;
            break;
        }
    }
    if (i >= len)
        return _("");
    for (i = len - 1; i >= start; i--)
    {
        if (! strchr(WHITE_SPACE, buf[i]))
        {
            end = i + 1;
            break;
        }
    }
    return str.substring(start, end - start);
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

time_t check_path_ex(String path, bool needDir, bool existenceUnneeded)
{
    int ret = 0;
    struct stat statbuf;

    ret = stat(path.c_str(), &statbuf);
    log_debug("path: %s, ret: %d; errno: %d\n", path.c_str(), ret, errno);
    
    if (ret != 0)
    {
        if (existenceUnneeded && (errno == ENOENT))
            return 0;
        
        throw _Exception(path + " : " + errno + (int)existenceUnneeded+ " x " + mt_strerror(errno));
    }

    if (needDir && (!S_ISDIR(statbuf.st_mode)))
        throw _Exception(_("Not a directory: ") + path);
    
    if (!needDir && (S_ISDIR(statbuf.st_mode)))
        throw _Exception(_("Not a file: ") + path);

    return statbuf.st_mtime;
}

bool string_ok(String str)
{
    if ((str == nil) || (str == ""))
        return false;
    return true;
}

bool string_ok(Ref<StringBuffer> str)
{
    
    if ((str == nil) || (str->length()<=0))
        return false;
    else
        return true;
}

void string_ok_ex(String str)
{
    if ((str == nil) || (str == ""))
        throw _Exception(_("Empty string"));
}

String http_redirect_to(String ip, String port, String page)
{
    return _("<html><head><meta http-equiv=\"Refresh\" content=\"0;URL=http://") + ip + ":" + port + "/" + page + "\"></head><body bgcolor=\"#dddddd\"></body></html>";
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
        char *chi = strchr(HEX_CHARS, ptr[i]);
        char *clo = strchr(HEX_CHARS, ptr[i + 1]);
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
    int salt;
};
String hex_md5(void *data, int length)
{
    char md5buf[16];

    md5_state_t ctx;
    md5_init(&ctx);
    md5_append(&ctx, (unsigned char *)data, length);
    md5_finish(&ctx, (unsigned char *)md5buf);

    return hex_encode(md5buf, 16);
}
String hex_string_md5(String str)
{
    return hex_md5(str.c_str(), str.length());
}
String generate_random_id()
{
    struct randomizer st;
    gettimeofday(&st.tv, NULL);
    st.salt = rand();
    return hex_md5(&st, sizeof(st));
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
            (c >= 'a' && c <= 'z') ||
            c == '_' ||
            c == '-')
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

            pos = strchr(hex, chi);
            if (!pos)
                hi = 0;
            else
                hi = pos - hex;

            pos = strchr(hex, clo);
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

String mt_strerror(int mt_errno)
{
    /*
    char *buffer = (char *)MALLOC(512);
    char *err_str = strerror_r(errno, buffer, 512);
    String errStr(err_str);
    FREE(buffer);
    return errStr;
    */
    return String(strerror(errno));
}

String read_text_file(String path)
{
    FILE *f = fopen(path.c_str(), "r");
    if (!f)
    {
        throw _Exception(_("read_text_file: could not open ") +
                        path + " : " + mt_strerror(errno));
    }
    Ref<StringBuffer> buf(new StringBuffer()); 
    char *buffer = (char *)MALLOC(1024);
    size_t bytesRead;    
    while((bytesRead = fread(buffer, 1, 1024, f)) > 0)
    {
        buf->concat(buffer, bytesRead);
    }
    fclose(f);
    FREE(buffer);
    return buf->toString();
}
void write_text_file(String path, String contents)
{
    int bytesWritten;
    FILE *f = fopen(path.c_str(), "w");
    if (!f)
    {
        throw _Exception(_("write_text_file: could not open ") +
                        path + " : " + mt_strerror(errno));
    }
    
    bytesWritten = fwrite(contents.c_str(), 1, contents.length(), f);
    if (bytesWritten < contents.length())
    {
        fclose(f);
        if (bytesWritten >= 0)
            throw _Exception(_("write_text_file: incomplete write to ") +
                            path + " : ");
        else
            throw _Exception(_("write_text_file: error writing to ") +
                            path + " : " + mt_strerror(errno));
    }
    fclose(f);
}

void copy_file(String from, String to)
{
    FILE *f = fopen(from.c_str(), "r");
    if (!f)
    {
        throw _Exception(_("copy_file: could not open ") +
                        from + " for read: " + mt_strerror(errno));
    }
    FILE *t = fopen(to.c_str(), "w");
    if (!t)
    {
        fclose(f);
        throw _Exception(_("copy_file: could not open ") +
                        to + " for write: " + mt_strerror(errno));
    }
    char *buffer = (char *)MALLOC(1024);
    size_t bytesRead = 0;
    size_t bytesWritten = 0;
    while(bytesRead == bytesWritten && ! feof(f) && ! ferror(f) && ! ferror(t)
        && (bytesRead = fread(buffer, 1, 1024, f)) > 0)
    {
        bytesWritten = fwrite(buffer, 1, bytesRead, t);
    }
    if (ferror(f) || ferror(t))
    {
        fclose(f);
        fclose(t);
        throw _Exception(_("copy_file: error while copying ") + from + " to " +
                        to + ": " + mt_strerror(errno));
    }
    FREE(buffer);
}

/* sorting */
int StringBaseComparator(void *arg1, void *arg2)
{
    return strcmp(((StringBase *)arg1)->data, ((StringBase *)arg2)->data); 
}

static void quicksort_impl(COMPARABLE *a, int lo0, int hi0, COMPARATOR comparator)
{
    int lo = lo0;
    int hi = hi0;

    if (lo >= hi)
        return;
    if( lo == hi - 1 )
    {
        // sort a two element list by swapping if necessary 
        // if (a[lo] > a[hi])
        if (comparator(a[lo], a[hi]) > 0)
        {
            COMPARABLE T = a[lo];
            a[lo] = a[hi];
            a[hi] = T;
        }
        return;
    }

    // Pick a pivot and move it out of the way
    COMPARABLE pivot = a[(lo + hi) / 2];
    a[(lo + hi) / 2] = a[hi];
    a[hi] = pivot;

    while( lo < hi )
    {
        /* Search forward from a[lo] until an element is found that
           is greater than the pivot or lo >= hi */ 
        // while (a[lo] <= pivot && lo < hi)
        while (comparator(a[lo], pivot) <= 0 && lo < hi)
        {
            lo++;
        }

        /* Search backward from a[hi] until element is found that
           is less than the pivot, or lo >= hi */
        // while (pivot <= a[hi] && lo < hi )
        while (comparator(pivot, a[hi]) <= 0 && lo < hi)
        {
            hi--;
        }

        /* Swap elements a[lo] and a[hi] */
        if( lo < hi )
        {
            COMPARABLE T = a[lo];
            a[lo] = a[hi];
            a[hi] = T;
        }
    }

    /* Put the median in the "center" of the list */
    a[hi0] = a[hi];
    a[hi] = pivot;

    /*
     *  Recursive calls, elements a[lo0] to a[lo-1] are less than or
     *  equal to pivot, elements a[hi+1] to a[hi0] are greater than
     *  pivot.
     */
    quicksort_impl(a, lo0, lo-1, comparator);
    quicksort_impl(a, hi+1, hi0, comparator);
}

void quicksort(COMPARABLE *arr, int size, COMPARATOR comparator)
{
    quicksort_impl(arr, 0, size - 1, comparator);
}

String renderProtocolInfo(String mimetype, String protocol, String extend)
{
    if (string_ok(mimetype) && string_ok(protocol))
    {
        if (string_ok(extend))
            return protocol + ":*:" + mimetype + ":" + extend;
        else
            return protocol + ":*:" + mimetype + ":*";
    }
    else
        return _("http-get:*:*:*");
}

String getMTFromProtocolInfo(String protocol)
{
    Ref<Array<StringBase> > parts = split_string(protocol, ':');
    if (parts->size() > 2)
        return parts->get(2);
    else
        return nil;
}

String getProtocol(String protocolInfo)
{
    String protocol;
    int pos = protocolInfo.index(':');
    if (pos <= 0)
        protocol = _("http-get");
    else
        protocol = protocolInfo.substring(0, pos);

    return protocol;
}

String secondsToHMS(int seconds)
{
    int h, m, s;
    
    s = seconds % 60;
    seconds /= 60;

    m = seconds % 60;
    h = seconds / 60;

    // XXX:XX:XX
    char *str = (char *)malloc(10);
    sprintf(str, "%02d:%02d:%02d", h, m, s);
    return String::take(str);
}

#ifdef HAVE_MAGIC
String get_mime_type(magic_set *ms, Ref<RExp> reMimetype, String file)
{
    if (ms == NULL)
        return nil;

    char *mt = (char *)magic_file(ms, file.c_str());
    if (mt == NULL)
    {
        log_error("magic_file: %s\n", magic_error(ms));
        return nil;
    }

    String mime_type(mt);

    Ref<Matcher> matcher = reMimetype->matcher(mime_type, 2);
    if (matcher->next())
        return matcher->group(1);

    log_warning("filemagic returned invalid mimetype for %s\n%s\n",
                file.c_str(), mt);
    return nil;
}

String get_mime_type_from_buffer(magic_set *ms, Ref<RExp> reMimetype, 
                                 void *buffer, size_t length)
{
    if (ms == NULL)
        return nil;

    char *mt = (char *)magic_buffer(ms, buffer, length);
    if (mt == NULL)
    {
        log_error("magic_file: %s\n", magic_error(ms));
        return nil;
    }

    String mime_type(mt);

    Ref<Matcher> matcher = reMimetype->matcher(mime_type, 2);
    if (matcher->next())
        return matcher->group(1);

    log_warning("filemagic returned invalid mimetype for the given buffer%s\n",
                mt);
    return nil;
}
#endif 

void set_jpeg_resolution_resource(Ref<CdsItem> item, int res_num)
{
    try
    {
        Ref<IOHandler> fio_h(new FileIOHandler(item->getLocation()));
        fio_h->open(UPNP_READ);
        String resolution = get_jpeg_resolution(fio_h);

        if (res_num >= item->getResourceCount())
            throw _Exception(_("Invalid resource index"));
            
        item->getResource(res_num)->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), resolution);
    }
    catch (Exception e)
    {
        e.printStackTrace();
    }
}

bool check_resolution(String resolution)
{
    Ref<Array<StringBase> > parts = split_string(resolution, 'x');
    if (parts->size() != 2)
        return false;

    if (string_ok(parts->get(0)) && 
        string_ok(parts->get(1)) &&
       (String(parts->get(0)->data).toInt() > 0) && 
       (String(parts->get(1)->data).toInt() > 0))
        return true;
    else
        return false;
}


String escape(String string, char escape_char, char to_escape)
{
    Ref<StringBase> stringBase(new StringBase(string.length() * 2));
    char *str = stringBase->data;
    int len = string.length();
    
    bool possible_more_esc = true;
    bool possible_more_char = true;
    
    int last = 0;
    do
    {
        int next_esc = -1;
        if (possible_more_esc)
        {
            next_esc = string.index(last, escape_char);
            if (next_esc < 0)
                possible_more_esc = false;
        }
        
        int next = -1;
        if (possible_more_char)
        {
            next = string.index(last, to_escape);
            if (next < 0)
                possible_more_char = false;
        }
        
        if (next < 0 || (next_esc >= 0 && next_esc < next))
        {
            next = next_esc;
        }
        
        if (next < 0)
            next = len;
        int cpLen = next - last;
        if (cpLen > 0)
        {
            strncpy(str, string.charPtrAt(last), cpLen);
            str += cpLen;
        }
        if (next < len)
        {
            *(str++) = '\\';
            *(str++) = string.charAt(next);
        }
        last = next;
        last++;
    }
    while (last < len);
    *str = '\0';
    
    stringBase->len = strlen(stringBase->data);
    return String(stringBase->data);
}

String unescape(String string, char escape)
{
    Ref<StringBase> stringBase(new StringBase(string.length()));
    char *str = stringBase->data;
    int len = string.length();
    
    int last = -1;
    do
    {
        int next = string.index(last + 1, escape);
        if (next < 0)
            next = len;
        if (last < 0)
            last = 0;
        int cpLen = next - last;
        if (cpLen > 0)
            strncpy(str, string.charPtrAt(last), cpLen);
        str += cpLen;
        last = next;
        last++;
    }
    while (last < len);
    *str = '\0';
    
    stringBase->len = strlen(stringBase->data);
    return String(stringBase);
}

/*
String xml_unescape(String string)
{
    Ref<StringBuffer> buf(new StringBuffer(string.length()));
    signed char *ptr = (signed char *)string.c_str();
    while (*ptr)
    {
        if (*ptr == '&')
        {
            if ((*(ptr + 1) == 'l') && (*(ptr + 2) == 't') && 
                (*(ptr + 3) == ';'))
            {
                *buf << '<';
                ptr = ptr + 3;
            }
            else if ((*(ptr + 1) == 'g') && (*(ptr + 2) == 't') && 
                     (*(ptr + 3) == ';'))
            {
                *buf << '>';
                ptr = ptr + 3;
            }
            else if ((*(ptr + 1) == 'q') && (*(ptr + 2) == 'u') && 
                     (*(ptr + 3) == 'o') && (*(ptr + 4) == 't') &&
                     (*(ptr + 5) == ';'))
            {
                *buf << '"';
                ptr = ptr + 5;
            }
            else if (*(ptr + 1) == 'a')
            {
                if ((*(ptr + 2) == 'm') && (*(ptr + 3) == 'p') && 
                    (*(ptr + 4) == ';'))
                    {
                        *buf << '&';
                        ptr = ptr + 4;
                    }
                else if ((*(ptr + 2) == 'p') && (*(ptr + 3) == 'o') &&
                         (*(ptr + 4) == 's') && (*(ptr + 5) == ';'))
                {
                    *buf << '\'';
                    ptr = ptr + 5;
                }
            }
            else
                *buf << *ptr;
        }
        else
            *buf << *ptr;

        ptr++;
    }

    return buf->toString();
}
*/

String unescape_amp(String string)
{
    if (string == nil)
        return nil;
    Ref<StringBase> stringBase(new StringBase(string.length()));
    char *str = stringBase->data;
    int len = string.length();

    int last = 0;
    do
    {
        int skip = 0;
        int next = last - 1;
        do
        {
            next = string.index(next + 1, '&');
            if ((next < len) && (string.charAt(next + 1) == 'a') && 
                (string.charAt(next + 2) == 'm') &&
                (string.charAt(next + 3) == 'p') &&
                (string.charAt(next + 4) == ';'))
            {
                skip = 4;
            }
        }
        while(next > 0 && skip == 0);
        
        if (next < 0)
            next = len;
        
        int cpLen = next - last + 1;
        strncpy(str, string.charPtrAt(last), cpLen);
        str += cpLen;
        last = next + skip + 1;
    }
    while (last <= len);
    
    stringBase->len = str - stringBase->data - 1;
    assert(stringBase->len == (int)strlen(stringBase->data));
    return String(stringBase);
}

String fallbackString(String first, String fallback)
{
    if (first==nil)
        return fallback;
    return first;
}

unsigned int stringHash(String str)
{
    unsigned int hash = 5381;
    unsigned char *data = (unsigned char *)str.c_str();
    int c;
    while ((c = *data++))
        hash = ((hash << 5) + hash) ^ c; /* (hash * 33) ^ c */
    return hash;
}

String intArrayToCSV(int *array, int size)
{
    if (size <= 0)
        return nil;
    Ref<StringBuffer> buf(new StringBuffer());
    for (int i = 0; i < size; i++)
        *buf << ',' << array[i];
    return buf->toString(1);
}

void getTimespecNow(struct timespec *ts)
{
#ifdef HAVE_CLOCK_GETTIME 
    clock_gettime(CLOCK_REALTIME, ts);
#else
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
#endif
}

long getDeltaMillis(struct timespec *first)
{
    struct timespec now;
    getTimespecNow(&now);
    return getDeltaMillis(first, &now);
}

long getDeltaMillis(struct timespec *first, struct timespec *second)
{
    return (second->tv_sec - first->tv_sec) * 1000 + (second->tv_nsec - first->tv_nsec) / 1000000;
}

void getTimespecAfterMillis(long delta, struct timespec *ret, struct timespec *start)
{
    struct timespec now;
    if (start == NULL)
    {
        getTimespecNow(&now);
        start = &now;
    }
    ret->tv_sec = start->tv_sec + delta / 1000;
    ret->tv_nsec = (start->tv_nsec + (delta % 1000) * 1000000);
    if (ret->tv_nsec >= 1000000000) // >= 1 second
    {
        ret->tv_sec ++;
        ret->tv_nsec -= 1000000000;
    }
    
    //log_debug("timespec: sec: %ld, nsec: %ld\n", ret->tv_sec, ret->tv_nsec);
}

int compareTimespecs(struct timespec *a,  struct timespec *b)
{
    if (a->tv_sec < b->tv_sec)
        return 1;
    if (a->tv_sec > b->tv_sec)
        return -1;
    if (a->tv_nsec < b->tv_nsec)
        return 1;
    if (a->tv_nsec > b->tv_nsec)
        return -1;
    return 0;
}

String normalizePath(String path)
{
    log_debug("Normalizing path: %s\n", path.c_str());
    
    int length = path.length();
    
    Ref<StringBase> result(new StringBase(length));
    
    int avarageExpectedSlashes = length/5;
    if (avarageExpectedSlashes < 3)
        avarageExpectedSlashes = 3;
    Ref<BaseStack<int> > separatorLocations(new BaseStack<int>(avarageExpectedSlashes, -1));
    char *str = result->data;
    //int len = string.length();

#ifndef __CYGWIN__
    if (path.charAt(0) != DIR_SEPARATOR)
#else
    #warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! this function is not finished for Cygwin
    #warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! this function is not finished for Cygwin
    #warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! this function is not finished for Cygwin
    #warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! this function is not finished for Cygwin
    #warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! this function is not finished for Cygwin
    #warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! this function is not finished for Cygwin
    #warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! this function is not finished for Cygwin
    #warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! this function is not finished for Cygwin
    for (int i = 0; i < 20; i++)
        print_backtrace();
    /// \todo this doesn't seem to be correct...
    if ((!path.length() > 1) && (path.charAt(1) != ':'))
#endif
        throw _Exception(_("Relative paths are not allowed!\n"));
    
    int next = 1;
    do
    {
        while (next < length && path.charAt(next) == DIR_SEPARATOR)
            next++;
        if (next >= length)
            break;
        
        int next_sep = path.index(next, DIR_SEPARATOR);
        if (next_sep < 0)
            next_sep = length;
        if (next_sep == next + 1 && path.charAt(next) == '.')
        {
            //  "." - can be ignored
        }
        else if (next_sep == next + 2 &&
            next + 1 < length &&
            path.charAt(next) == '.' &&
            path.charAt(next + 1) == '.')
        {
            // ".."
            // go back one part
            int lastSepLocation = separatorLocations->pop();
            if (lastSepLocation < 0)
                lastSepLocation = 0;
            str = result->data + lastSepLocation;
        }
        else
        {
            // normal part
            separatorLocations->push(str - result->data);
            *(str++) = DIR_SEPARATOR;
            int cpLen = next_sep - next;
            strncpy(str, path.charPtrAt(next), cpLen);
            str += cpLen;
        }
        next = next_sep + 1;
    }
    while(next < length);
    
    if (str == result->data)
        *(str++) = DIR_SEPARATOR;
    
    *str = 0;
    
    return String(result);
}

String interfaceToIP(String interface)
{
#if defined(__CYGWIN__)
    struct hostent *h=NULL;
    struct sockaddr_in LocalAddr;
    char *hostname = (char *)MALLOC(256);
    if (!hostname)
        return nil;

    gethostname(hostname, 255);
    hostname[255] = '\0';
    h=gethostbyname(hostname);
    free(hostname);
    if (h != NULL)
    {
        memcpy(&LocalAddr.sin_addr, h->h_addr_list[0],4);
        return String(inet_ntoa(LocalAddr.sin_addr));
    }
    return nil;
#else

    struct if_nameindex *iflist = NULL;
    struct if_nameindex *iflist_free = NULL;
    struct ifreq if_request;
    struct sockaddr_in local_address;
    int local_socket;

    if (!string_ok(interface))
            return nil;

    local_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (local_socket < 0)
    {
        log_error("Could not create local socket\n");
        return nil;
    }

    iflist = iflist_free = if_nameindex();
    if (iflist == NULL)
    {
        log_error("Could not get interface list\n");
        close(local_socket);
        return nil;
    }

    while (iflist->if_index || iflist->if_name)
    {
        if (interface == iflist->if_name)
        {
            strncpy(if_request.ifr_name, iflist->if_name, IF_NAMESIZE);
            if (ioctl(local_socket, SIOCGIFADDR, &if_request) != 0)
            {
                log_error("Could not determine interface address\n");
                close(local_socket);
                if_freenameindex(iflist_free);
                return nil;
            }

            memcpy(&local_address, &if_request.ifr_addr, sizeof(if_request.ifr_addr));
            String ip = String(inet_ntoa(local_address.sin_addr));
            if_freenameindex(iflist_free);
            close(local_socket);
            return ip;
        }
        iflist++;
    }

    close(local_socket);
    if_freenameindex(iflist_free);
    return nil;
#endif
}

bool validateYesNo(String value)
{
    if ((value != "yes") && (value != "no"))
        return false;
    else
        return true;
}

Ref<Array<StringBase> > parseCommandLine(String line, String in, String out)
{
    Ref<Array<StringBase> > params = split_string(line, ' ');
    for (int i = 0; i < params->size(); i++)
    {
        String param = params->get(i);
        String newParam = param.replace(_("%in"), in);
        newParam = newParam.replace(_("%out"), out);
        if (param != newParam)
            params->set(newParam, i);
    }

    return params;
}

// The tempName() function is borrowed from gfileutils.c from the glibc package

/* gfileutils.c - File utility functions
 *
 *  Copyright 2000 Red Hat, Inc.
 *
 * GLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * GLib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GLib; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *   Boston, MA 02111-1307, USA.
 */
/*
 * create_temp_file based on the mkstemp implementation from the GNU C library.
 * Copyright (C) 1991,92,93,94,95,96,97,98,99 Free Software Foundation, Inc.
 */
// tempName is based on create_temp_file, see (C) above
String tempName(char *tmpl)
{
    char *XXXXXX;
    int count;
    static const char letters[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const int NLETTERS = sizeof (letters) - 1;
    long value;
    struct timeval tv;
    static int counter = 0;
    struct stat statbuf;
    int ret = 0;

    /* find the last occurrence of "XXXXXX" */
    XXXXXX = strstr (tmpl, "XXXXXX");

    if (!XXXXXX || strncmp (XXXXXX, "XXXXXX", 6))
    {
        return nil;
    }

    /* Get some more or less random data.  */
    gettimeofday(&tv, NULL);
    value = (tv.tv_usec ^ tv.tv_sec) + counter++;

    for (count = 0; count < 100; value += 7777, ++count)
    {
        long v = value;

        /* Fill in the random bits.  */
        XXXXXX[0] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[1] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[2] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[3] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[4] = letters[v % NLETTERS];
        v /= NLETTERS;
        XXXXXX[5] = letters[v % NLETTERS];


        ret = stat(tmpl, &statbuf);
        if (ret != 0)
        {
            if ((errno == ENOENT) ||
                    (errno == ENOTDIR))
                return String(tmpl);
            else
                return nil;
        }
    }

    /* We got out of the loop because we ran out of combinations to try.  */
    return nil;
}

bool isTheora(String ogg_filename)
{
    FILE *f;
    char buffer[7];
    f = fopen(ogg_filename.c_str(), "rb");
    
    if (fread(buffer, 1, 4, f) != 4)
    {
        fclose(f);
        throw _Exception(_("Error reading ") + ogg_filename);
    }

    if (memcmp(buffer, "OggS", 4) != 0)
    {
        fclose(f);
        return false;
    }

    if (fseek(f, 28, SEEK_SET) != 0)
    {
        fclose(f);
        throw _Exception(_("Incomplete file ") + ogg_filename);
    }

    if (fread(buffer, 1, 7, f) != 7)
    {
        fclose(f);
        throw _Exception(_("Error reading ") + ogg_filename);
    }

    if (memcmp(buffer, "\x80theora", 7) != 0)
    {
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}


#ifdef LOG_TOMBDEBUG

void profiling_thread_check(struct profiling_t *data)
{
    if (data->thread != pthread_self())
    {
        log_debug("profiling_..() called from a different thread than profiling_init was called! (init: %d; this: %d) - aborting...\n", data->thread, pthread_self());
        print_backtrace();
        abort();
        return;
    }
}


void profiling_start(struct profiling_t *data)
{
    profiling_thread_check(data);
    if (data->running)
    {
        log_debug("profiling_start() called on an already running profile! - aborting...\n");
        print_backtrace();
        abort();
        return;
    }
    data->running = true;
    getTimespecNow(&(data->last_start));
}

void profiling_end(struct profiling_t *data)
{
    profiling_thread_check(data);
    struct timespec now;
    getTimespecNow(&now);
    if (! data->running)
    {
        log_debug("profiling_end() called on a not-running profile! - aborting...\n");
        print_backtrace();
        abort();
        return;
    }
    struct timespec *sum = &(data->sum);
    struct timespec *last_start = &(data->last_start);
    sum->tv_sec += now.tv_sec - last_start->tv_sec;
    //log_debug("!!!!!! adding %d sec\n", now.tv_sec - last_start->tv_sec);
    if (now.tv_nsec >= last_start->tv_nsec)
    {
        sum->tv_nsec += now.tv_nsec - last_start->tv_nsec;
        //log_debug("adding %ld nsec\n", now.tv_nsec - last_start->tv_nsec);
    }
    else
    {
        sum->tv_nsec += 1000000000 + now.tv_nsec - last_start->tv_nsec;
        //log_debug("adding 1 sec %ld nsec\n", 1000000000 + now.tv_nsec - last_start->tv_nsec);
        sum->tv_sec ++;
    }
    if (sum->tv_nsec >= 1000000000)
    {
        sum->tv_nsec -= 1000000000;
        sum->tv_sec ++;
    }
    
    data->running = false;
}

void profiling_print(struct profiling_t *data)
{
    if (data->running)
    {
        log_debug("profiling_print() called on running profile! - aborting...\n");
        print_backtrace();
        abort();
        return;
    }
    //log_debug("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\nPROFILING: took %ld sec %ld nsec thread %u\n", data->sum.tv_sec, data->sum.tv_nsec, pthread_self());
    log_debug("PROFILING: took %ld sec %ld nsec\n", data->sum.tv_sec, data->sum.tv_nsec);
}

#endif
