/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    tools.cc - this file is part of MediaTomb.
    
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

/// \file tools.cc

#include <arpa/inet.h>
#include <cerrno>
#include <climits>
#include <cstring>
#include <iterator>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ifaddrs.h>
#include <net/if.h>

#ifdef BSD_NATIVE_UUID
#include <uuid.h>
#else
#include <uuid/uuid.h>
#endif

#ifdef SOLARIS
#include <fcntl.h>
#include <sys/sockio.h>
#endif

#include "config_manager.h"
#include "contrib/md5.h"
#include "file_io_handler.h"
#include "metadata_handler.h"
#include "tools.h"

#define WHITE_SPACE " \t\r\n"

using namespace zmm;
using namespace std;

static const char* HEX_CHARS = "0123456789abcdef";

Ref<Array<StringBase>> split_string(String str, char sep, bool empty)
{
    Ref<Array<StringBase>> ret(new Array<StringBase>());
    const char* data = str.c_str();
    const char* end = data + str.length();
    while (data < end) {
        const char* pos = strchr(data, sep);
        if (pos == nullptr) {
            String part = data;
            ret->append(part);
            data = end;
        } else if (pos == data) {
            data++;
            if ((data < end) && empty)
                ret->append(_(""));
        } else {
            String part(data, pos - data);
            ret->append(part);
            data = pos + 1;
        }
    }
    return ret;
}

Ref<Array<StringBase>> split_path(String str)
{
    if (!string_ok(str))
        throw _Exception(_("invalid path given to split_path"));
    Ref<Array<StringBase>> ret(new Array<StringBase>());
    int pos = str.rindex(DIR_SEPARATOR);
    const char* data = str.c_str();

    if (pos < 0)
        throw _Exception(_("relative path given to split_path: ") + str);

    if (pos == 0) {
        /* there is only one separator at the beginning "/..." or "/" */
        ret->append(_(_DIR_SEPARATOR));
        String filename = data + 1;
        ret->append(filename);
    } else {
        String path(data, pos);
        ret->append(path);
        String filename = data + pos + 1;
        ret->append(filename);
    }
    return ret;
}

String trim_string(String str)
{
    if (str == nullptr)
        return nullptr;
    int i;
    int start = 0;
    int end = 0;
    int len = str.length();

    const char* buf = str.c_str();

    for (i = 0; i < len; i++) {
        if (!strchr(WHITE_SPACE, buf[i])) {
            start = i;
            break;
        }
    }
    if (i >= len)
        return _("");
    for (i = len - 1; i >= start; i--) {
        if (!strchr(WHITE_SPACE, buf[i])) {
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
    if (ret != 0)
        return false;

    if ((needDir && (!S_ISDIR(statbuf.st_mode))) || (!needDir && (S_ISDIR(statbuf.st_mode))))
        return false;

    return true;
}

time_t check_path_ex(String path, bool needDir, bool existenceUnneeded,
    off_t* filesize)
{
    int ret = 0;
    struct stat statbuf;

    if (filesize != nullptr)
        *filesize = 0;

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        if (existenceUnneeded && (errno == ENOENT))
            return 0;
        throw _Exception(mt_strerror(errno) + ": " + path + " (errno: " + errno + (int)existenceUnneeded + ")");
    }

    if (needDir && (!S_ISDIR(statbuf.st_mode)))
        throw _Exception(_("Not a directory: ") + path);

    if (!needDir && (S_ISDIR(statbuf.st_mode)))
        throw _Exception(_("Not a file: ") + path);

    if ((filesize != nullptr) && S_ISREG(statbuf.st_mode))
        *filesize = statbuf.st_size;

    return statbuf.st_mtime;
}

bool is_executable(String path, int* err)
{
    int ret = access(path.c_str(), R_OK | X_OK);
    if (err != nullptr)
        *err = errno;

    if (ret == 0)
        return true;
    else
        return false;
}

String find_in_path(String exec)
{
    String PATH = getenv("PATH");
    if (!string_ok(PATH))
        return nullptr;

    Ref<StringTokenizer> st(new StringTokenizer(PATH));
    String path = nullptr;
    String next;
    do {
        if (path == nullptr)
            path = st->nextToken(_(":"));
        next = st->nextToken(_(":"));

        if (path == nullptr)
            break;

        if ((next != nullptr) && !next.startsWith(_("/"))) {
            path = path + _(":") + next;
            next = nullptr;
        }

        String check = path + _("/") + exec;
        if (check_path(check))
            return check;

        if (next != nullptr)
            path = next;
        else
            path = nullptr;

    } while (path != nullptr);

    return nullptr;
}

bool string_ok(String str)
{
    if ((str == nullptr) || (str == ""))
        return false;
    return true;
}

void string_ok_ex(String str)
{
    if ((str == nullptr) || (str == ""))
        throw _Exception(_("Empty string"));
}

String http_redirect_to(String ip, String port, String page)
{
    return _("<html><head><meta http-equiv=\"Refresh\" content=\"0;URL=http://") + ip + ":" + port + "/" + page + "\"></head><body bgcolor=\"#dddddd\"></body></html>";
}

String hex_encode(const void* data, int len)
{
    const unsigned char* chars;
    int i;
    unsigned char hi, lo;

    std::ostringstream buf;

    chars = (unsigned char*)data;
    for (i = 0; i < len; i++) {
        unsigned char c = chars[i];
        hi = c >> 4;
        lo = c & 0xF;
        buf << HEX_CHARS[hi] << HEX_CHARS[lo];
    }
    return buf.str();
}

String hex_decode_string(String encoded)
{
    auto* ptr = const_cast<char*>(encoded.c_str());
    int len = encoded.length();

    std::ostringstream buf;
    for (int i = 0; i < len; i += 2) {
        const char* chi = strchr(const_cast<char*>(HEX_CHARS), ptr[i]);
        const char* clo = strchr(const_cast<char*>(HEX_CHARS), ptr[i + 1]);
        int hi, lo;

        if (chi)
            hi = chi - HEX_CHARS;
        else
            hi = 0;

        if (clo)
            lo = clo - HEX_CHARS;
        else
            lo = 0;
        auto ch = (char)(hi << 4 | lo);
        buf << ch;
    }
    return buf.str();
}

String hex_md5(const void* data, int length)
{
    char md5buf[16];

    md5_state_t ctx;
    md5_init(&ctx);
    md5_append(&ctx, (unsigned char*)data, length);
    md5_finish(&ctx, (unsigned char*)md5buf);

    return hex_encode(md5buf, 16);
}
String hex_string_md5(String str)
{
    return hex_md5(str.c_str(), str.length());
}
String generate_random_id()
{
#ifdef BSD_NATIVE_UUID
    char* uuid_str;
    uint32_t status;
#else
    char uuid_str[37];
#endif
    uuid_t uuid;

#ifdef BSD_NATIVE_UUID
    uuid_create(&uuid, &status);
    uuid_to_string(&uuid, &uuid_str, &status);
#else
    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_str);
#endif

    log_debug("Generated: %s\n", uuid_str);
    String uuid_String = String(uuid_str);
#ifdef BSD_NATIVE_UUID
    free(uuid_str);
#endif

    return uuid_String;
}

static const char* HEX_CHARS2 = "0123456789ABCDEF";

String url_escape(String str)
{
    const char* data = str.c_str();
    int len = str.length();
    std::ostringstream buf;
    for (int i = 0; i < len; i++) {
        auto c = (unsigned char)data[i];
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '-') {
            buf << (char)c;
        } else {
            int hi = c >> 4;
            int lo = c & 15;
            buf << '%' << HEX_CHARS2[hi] << HEX_CHARS2[lo];
        }
    }
    return buf.str();
}

String urlUnescape(String str)
{
    auto* data = const_cast<char*>(str.c_str());
    int len = str.length();
    std::ostringstream buf;

    int i = 0;
    while (i < len) {
        char c = data[i++];
        if (c == '%') {
            if (i + 2 > len)
                break; // avoid buffer overrun
            char chi = data[i++];
            char clo = data[i++];
            int hi, lo;

            const char* pos;

            pos = strchr(const_cast<char*>(HEX_CHARS2), chi);
            if (!pos)
                hi = 0;
            else
                hi = pos - HEX_CHARS2;

            pos = strchr(const_cast<char*>(HEX_CHARS2), clo);
            if (!pos)
                lo = 0;
            else
                lo = pos - HEX_CHARS2;

            int ascii = (hi << 4) | lo;
            buf << (char)ascii;
        } else if (c == '+') {
            buf << ' ';
        } else {
            buf << c;
        }
    }
    return buf.str();
}

String mime_types_to_CSV(Ref<Array<StringBase>> mimeTypes)
{
    std::ostringstream buf;
    for (int i = 0; i < mimeTypes->size(); i++) {
        if (i > 0)
            buf << ",";
        String mimeType = mimeTypes->get(i);
        buf << "http-get:*:" << mimeType << ":*";
    }

    return buf.str();
}

String mt_strerror(int mt_errno)
{
#ifdef DONT_USE_YET_HAVE_STRERROR_R
    char* buffer = (char*)MALLOC(512);
    char* err_str;
#ifdef STRERROR_R_CHAR_P
    err_str = strerror_r(errno, buffer, 512);
    if (err_str == NULL)
        err_str = buffer;
#else
    int ret = strerror_r(errno, buffer, 512);
    if (ret < 0)
        return _("cannot get error string: error while calling XSI-compliant strerror_r");
    err_str = buffer;
#endif
    String errStr(err_str);
    FREE(buffer);
    return errStr;
#else
    return strerror(errno);
#endif
}

String read_text_file(String path)
{
    FILE* f = fopen(path.c_str(), "r");
    if (!f) {
        throw _Exception(_("read_text_file: could not open ") + path + " : " + mt_strerror(errno));
    }
    std::ostringstream buf;
    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        buf << std::string(buffer, bytesRead);
    }
    fclose(f);
    return buf.str();
}
void write_text_file(String path, String contents)
{
    int bytesWritten;
    FILE* f = fopen(path.c_str(), "w");
    if (!f) {
        throw _Exception(_("write_text_file: could not open ") + path + " : " + mt_strerror(errno));
    }

    bytesWritten = fwrite(contents.c_str(), 1, contents.length(), f);
    if (bytesWritten < contents.length()) {
        fclose(f);
        if (bytesWritten >= 0)
            throw _Exception(_("write_text_file: incomplete write to ") + path + " : ");
        else
            throw _Exception(_("write_text_file: error writing to ") + path + " : " + mt_strerror(errno));
    }
    fclose(f);
}

void copy_file(String from, String to)
{
    FILE* f = fopen(from.c_str(), "r");
    if (!f) {
        throw _Exception(_("copy_file: could not open ") + from + " for read: " + mt_strerror(errno));
    }
    FILE* t = fopen(to.c_str(), "w");
    if (!t) {
        fclose(f);
        throw _Exception(_("copy_file: could not open ") + to + " for write: " + mt_strerror(errno));
    }
    auto* buffer = (char*)MALLOC(1024);
    size_t bytesRead = 0;
    size_t bytesWritten = 0;
    while (bytesRead == bytesWritten && !feof(f) && !ferror(f) && !ferror(t)
        && (bytesRead = fread(buffer, 1, 1024, f)) > 0) {
        bytesWritten = fwrite(buffer, 1, bytesRead, t);
    }
    FREE(buffer);
    if (ferror(f) || ferror(t)) {
        int my_errno = errno;
        fclose(f);
        fclose(t);
        throw _Exception(_("copy_file: error while copying ") + from + " to " + to + ": " + mt_strerror(my_errno));
    }

    fclose(f);
    fclose(t);
}

/* sorting */
int StringBaseComparator(void* arg1, void* arg2)
{
    return strcmp(((StringBase*)arg1)->data, ((StringBase*)arg2)->data);
}

static void quicksort_impl(COMPARABLE* a, int lo0, int hi0, COMPARATOR comparator)
{
    int lo = lo0;
    int hi = hi0;

    if (lo >= hi)
        return;
    if (lo == hi - 1) {
        // sort a two element list by swapping if necessary
        // if (a[lo] > a[hi])
        if (comparator(a[lo], a[hi]) > 0) {
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

    while (lo < hi) {
        /* Search forward from a[lo] until an element is found that
           is greater than the pivot or lo >= hi */
        // while (a[lo] <= pivot && lo < hi)
        while (comparator(a[lo], pivot) <= 0 && lo < hi) {
            lo++;
        }

        /* Search backward from a[hi] until element is found that
           is less than the pivot, or lo >= hi */
        // while (pivot <= a[hi] && lo < hi )
        while (comparator(pivot, a[hi]) <= 0 && lo < hi) {
            hi--;
        }

        /* Swap elements a[lo] and a[hi] */
        if (lo < hi) {
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
    quicksort_impl(a, lo0, lo - 1, comparator);
    quicksort_impl(a, hi + 1, hi0, comparator);
}

void quicksort(COMPARABLE* arr, int size, COMPARATOR comparator)
{
    quicksort_impl(arr, 0, size - 1, comparator);
}

String renderProtocolInfo(String mimetype, String protocol, String extend)
{
    if (string_ok(mimetype) && string_ok(protocol)) {
        if (string_ok(extend))
            return protocol + ":*:" + mimetype + ":" + extend;
        else
            return protocol + ":*:" + mimetype + ":*";
    } else
        return _("http-get:*:*:*");
}

String getMTFromProtocolInfo(String protocol)
{
    Ref<Array<StringBase>> parts = split_string(protocol, ':');
    if (parts->size() > 2)
        return parts->get(2);
    else
        return nullptr;
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
    // This fails if h goes over 999
    if (h > 999)
        h = 999;

    auto* str = (char*)malloc(10);
    sprintf(str, "%02d:%02d:%02d", h, m, s);
    return String::take(str);
}

int HMSToSeconds(String time)
{
    if (!string_ok(time)) {
        log_warning("Could not convert time representation to seconds!\n");
        return 0;
    }

    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    sscanf(time.c_str(), "%d:%d:%d", &hours, &minutes, &seconds);

    return (hours * 3600) + (minutes * 60) + seconds;
}

#ifdef HAVE_MAGIC
String getMIMETypeFromFile(String file)
{
    return getMIME(file, nullptr, -1);
}

String getMIMETypeFromBuffer(const void *buffer, size_t length)
{
    return getMIME(nullptr, buffer, length);
}

String getMIME(String filepath, const void *buffer, size_t length)
{
    /* MAGIC_MIME_TYPE tells magic to return ONLY the mimetype */
    magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);

    if (magic_cookie == NULL) {
        log_warning("Failed to initialize libmagic\n");
        return nullptr;
    }

    if (magic_load(magic_cookie, NULL) != 0) {
        log_warning("Failed to load magic database: %s\n", magic_error(magic_cookie));
        magic_close(magic_cookie);
        return nullptr;
    }

    const char* mime;
    if (!string_ok(filepath)) {
        mime = magic_buffer(magic_cookie, buffer, length);
    } else {
        mime = magic_file(magic_cookie, filepath.c_str());
    }

    String out = String::copy(mime);
    magic_close(magic_cookie);
    return out;
}
#endif

void set_jpeg_resolution_resource(Ref<CdsItem> item, int res_num)
{
    try {
        Ref<IOHandler> fio_h(new FileIOHandler(item->getLocation()));
        fio_h->open(UPNP_READ);
        String resolution = get_jpeg_resolution(fio_h);

        if (res_num >= item->getResourceCount())
            throw _Exception(_("Invalid resource index"));

        item->getResource(res_num)->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), resolution);
    } catch (const Exception& e) {
        e.printStackTrace();
    }
}

bool check_resolution(String resolution, int* x, int* y)
{
    if (x != nullptr)
        *x = 0;

    if (y != nullptr)
        *y = 0;

    Ref<Array<StringBase>> parts = split_string(resolution, 'x');
    if (parts->size() != 2)
        return false;

    if (string_ok(parts->get(0)) && string_ok(parts->get(1))) {
        int _x = _(parts->get(0)->data).toInt();
        int _y = _(parts->get(1)->data).toInt();

        if ((_x > 0) && (_y > 0)) {
            if (x != nullptr)
                *x = _x;

            if (y != nullptr)
                *y = _y;

            return true;
        }
    }

    return false;
}

String escape(String string, char escape_char, char to_escape)
{
    Ref<StringBase> stringBase(new StringBase(string.length() * 2));
    char* str = stringBase->data;
    int len = string.length();

    bool possible_more_esc = true;
    bool possible_more_char = true;

    int last = 0;
    do {
        int next_esc = -1;
        if (possible_more_esc) {
            next_esc = string.index(last, escape_char);
            if (next_esc < 0)
                possible_more_esc = false;
        }

        int next = -1;
        if (possible_more_char) {
            next = string.index(last, to_escape);
            if (next < 0)
                possible_more_char = false;
        }

        if (next < 0 || (next_esc >= 0 && next_esc < next)) {
            next = next_esc;
        }

        if (next < 0)
            next = len;
        int cpLen = next - last;
        if (cpLen > 0) {
            strncpy(str, string.charPtrAt(last), cpLen);
            str += cpLen;
        }
        if (next < len) {
            *(str++) = '\\';
            *(str++) = string.charAt(next);
        }
        last = next;
        last++;
    } while (last < len);
    *str = '\0';

    stringBase->len = strlen(stringBase->data);
    return stringBase->data;
}

String unescape(String string, char escape)
{
    Ref<StringBase> stringBase(new StringBase(string.length()));
    char* str = stringBase->data;
    int len = string.length();

    int last = -1;
    do {
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
    } while (last < len);
    *str = '\0';

    stringBase->len = strlen(stringBase->data);
    return String(stringBase);
}

/*
std::string xml_unescape(std::string_view sv)
{
    std::ostringstream buf;
    signed char *ptr = (signed char *)sv.data();
    while (*ptr)
    {
        if (*ptr == '&')
        {
            if ((*(ptr + 1) == 'l') && (*(ptr + 2) == 't') && 
                (*(ptr + 3) == ';'))
            {
                buf << '<';
                ptr = ptr + 3;
            }
            else if ((*(ptr + 1) == 'g') && (*(ptr + 2) == 't') && 
                     (*(ptr + 3) == ';'))
            {
                buf << '>';
                ptr = ptr + 3;
            }
            else if ((*(ptr + 1) == 'q') && (*(ptr + 2) == 'u') && 
                     (*(ptr + 3) == 'o') && (*(ptr + 4) == 't') &&
                     (*(ptr + 5) == ';'))
            {
                buf << '"';
                ptr = ptr + 5;
            }
            else if (*(ptr + 1) == 'a')
            {
                if ((*(ptr + 2) == 'm') && (*(ptr + 3) == 'p') && 
                    (*(ptr + 4) == ';'))
                    {
                        buf << '&';
                        ptr = ptr + 4;
                    }
                else if ((*(ptr + 2) == 'p') && (*(ptr + 3) == 'o') &&
                         (*(ptr + 4) == 's') && (*(ptr + 5) == ';'))
                {
                    buf << '\'';
                    ptr = ptr + 5;
                }
            }
            else
                buf << *ptr;
        }
        else
            buf << *ptr;

        ptr++;
    }

    return buf.str();
}
*/

String unescape_amp(String string)
{
    if (string == nullptr)
        return nullptr;
    Ref<StringBase> stringBase(new StringBase(string.length()));
    char* str = stringBase->data;
    int len = string.length();

    int last = 0;
    do {
        int skip = 0;
        int next = last - 1;
        do {
            next = string.index(next + 1, '&');
            if ((next < len) && (string.charAt(next + 1) == 'a') && (string.charAt(next + 2) == 'm') && (string.charAt(next + 3) == 'p') && (string.charAt(next + 4) == ';')) {
                skip = 4;
            }
        } while (next > 0 && skip == 0);

        if (next < 0)
            next = len;

        int cpLen = next - last + 1;
        strncpy(str, string.charPtrAt(last), cpLen);
        str += cpLen;
        last = next + skip + 1;
    } while (last <= len);

    stringBase->len = str - stringBase->data - 1;
    assert(stringBase->len == (int)strlen(stringBase->data));
    return String(stringBase);
}

String fallbackString(String first, String fallback)
{
    if (first == nullptr)
        return fallback;
    return first;
}

unsigned int stringHash(String str)
{
    unsigned int hash = 5381;
    auto* data = (unsigned char*)str.c_str();
    int c;
    while ((c = *data++))
        hash = ((hash << 5) + hash) ^ c; /* (hash * 33) ^ c */
    return hash;
}

String toCSV(shared_ptr<unordered_set<int>> array)
{
    if (array->empty())
        return nullptr;
    return join(*array, ",");
}

void getTimespecNow(struct timespec* ts)
{
    struct timeval tv;
    int ret = gettimeofday(&tv, nullptr);
    if (ret != 0)
        throw _Exception(_("gettimeofday failed: ") + mt_strerror(errno));

    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
}

long getDeltaMillis(struct timespec* first)
{
    struct timespec now;
    getTimespecNow(&now);
    return getDeltaMillis(first, &now);
}

long getDeltaMillis(struct timespec* first, struct timespec* second)
{
    return (second->tv_sec - first->tv_sec) * 1000 + (second->tv_nsec - first->tv_nsec) / 1000000;
}

void getTimespecAfterMillis(long delta, struct timespec* ret, struct timespec* start)
{
    struct timespec now;
    if (start == nullptr) {
        getTimespecNow(&now);
        start = &now;
    }
    ret->tv_sec = start->tv_sec + delta / 1000;
    ret->tv_nsec = (start->tv_nsec + (delta % 1000) * 1000000);
    if (ret->tv_nsec >= 1000000000) // >= 1 second
    {
        ret->tv_sec++;
        ret->tv_nsec -= 1000000000;
    }

    // log_debug("timespec: sec: %ld, nsec: %ld\n", ret->tv_sec, ret->tv_nsec);
}

String normalizePath(String path)
{
    log_debug("Normalizing path: %s\n", path.c_str());

    int length = path.length();

    Ref<StringBase> result(new StringBase(length));

    int avarageExpectedSlashes = length / 5;
    if (avarageExpectedSlashes < 3)
        avarageExpectedSlashes = 3;
    Ref<BaseStack<int>> separatorLocations(new BaseStack<int>(avarageExpectedSlashes, -1));
    char* str = result->data;
    //int len = string.length();

    if (path.charAt(0) != DIR_SEPARATOR)
        throw _Exception(_("Relative paths are not allowed!\n"));

    int next = 1;
    do {
        while (next < length && path.charAt(next) == DIR_SEPARATOR)
            next++;
        if (next >= length)
            break;

        int next_sep = path.index(next, DIR_SEPARATOR);
        if (next_sep < 0)
            next_sep = length;
        if (next_sep == next + 1 && path.charAt(next) == '.') {
            //  "." - can be ignored
        } else if (next_sep == next + 2 && next + 1 < length && path.charAt(next) == '.' && path.charAt(next + 1) == '.') {
            // ".."
            // go back one part
            int lastSepLocation = separatorLocations->pop();
            if (lastSepLocation < 0)
                lastSepLocation = 0;
            str = result->data + lastSepLocation;
        } else {
            // normal part
            separatorLocations->push(str - result->data);
            *(str++) = DIR_SEPARATOR;
            int cpLen = next_sep - next;
            strncpy(str, path.charPtrAt(next), cpLen);
            str += cpLen;
        }
        next = next_sep + 1;
    } while (next < length);

    if (str == result->data)
        *(str++) = DIR_SEPARATOR;

    *str = 0;
    result->len = strlen(result->data);
    return String(result);
}

String interfaceToIP(String interface)
{

    struct if_nameindex* iflist = nullptr;
    struct if_nameindex* iflist_free = nullptr;
    struct ifreq if_request;
    struct sockaddr_in local_address;
    int local_socket;

    if (!string_ok(interface))
        return nullptr;

    local_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (local_socket < 0) {
        log_error("Could not create local socket: %s\n",
            mt_strerror(errno).c_str());
        return nullptr;
    }

    iflist = iflist_free = if_nameindex();
    if (iflist == nullptr) {
        log_error("Could not get interface list: %s\n",
            mt_strerror(errno).c_str());
        close(local_socket);
        return nullptr;
    }

    while (iflist->if_index || iflist->if_name) {
        if (interface == iflist->if_name) {
            strncpy(if_request.ifr_name, iflist->if_name, IF_NAMESIZE);
            if (ioctl(local_socket, SIOCGIFADDR, &if_request) != 0) {
                log_error("Could not determine interface address: %s\n",
                    mt_strerror(errno).c_str());
                close(local_socket);
                if_freenameindex(iflist_free);
                return nullptr;
            }

            memcpy(&local_address, &if_request.ifr_addr, sizeof(if_request.ifr_addr));
            String ip = inet_ntoa(local_address.sin_addr);
            if_freenameindex(iflist_free);
            close(local_socket);
            return ip;
        }
        iflist++;
    }

    close(local_socket);
    if_freenameindex(iflist_free);
    return nullptr;
}

String ipToInterface(String ip)
{
    if (!string_ok(ip)) {
        return nullptr;
    } else {
        log_debug("Looking for '%s'\n", ip.c_str());
    }

    struct ifaddrs *ifaddr, *ifa;
    int family, s, n;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        log_error("Could not getifaddrs: %s\n", mt_strerror(errno).c_str());
    }

    for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;
        String name = String::copy(ifa->ifa_name);

        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                log_error("getnameinfo() failed: %s\n", gai_strerror(s));
                return nullptr;
            }

            String ipaddr = String::copy(host);
            // IPv6 link locals come back as fe80::351d:d7f4:6b17:3396%eth0
            if (ipaddr.startsWith(ip)) {
                return name;
            }
        }
    }

    freeifaddrs(ifaddr);
    log_warning("Failed to find interface for IP: %s\n", ip.c_str());
    return nullptr;
}

bool validateYesNo(String value)
{
    if ((value != "yes") && (value != "no"))
        return false;
    else
        return true;
}

Ref<Array<StringBase>> parseCommandLine(String line, String in, String out,
    String range)
{
    Ref<Array<StringBase>> params = split_string(line, ' ');
    if ((in == nullptr) && (out == nullptr))
        return params;

    for (int i = 0; i < params->size(); i++) {
        String param = params->get(i);
        String newParam = param.replace(_("%in"), in);
        newParam = newParam.replace(_("%out"), out);
        if (range != nullptr) {
            newParam = newParam.replace(_("%range"), range);
        }
        if (param != newParam) {
            params->set(newParam, i);
        }
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
String tempName(String leadPath, char* tmpl)
{
    char* XXXXXX;
    int count;
    static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const int NLETTERS = sizeof(letters) - 1;
    long value;
    struct timeval tv;
    static int counter = 0;
    struct stat statbuf;
    int ret = 0;

    /* find the last occurrence of "XXXXXX" */
    XXXXXX = strstr(tmpl, "XXXXXX");

    if (!XXXXXX || strncmp(XXXXXX, "XXXXXX", 6)) {
        return nullptr;
    }

    /* Get some more or less random data.  */
    gettimeofday(&tv, nullptr);
    value = (tv.tv_usec ^ tv.tv_sec) + counter++;

    for (count = 0; count < 100; value += 7777, ++count) {
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

        String check = leadPath + tmpl;
        ret = stat(check.c_str(), &statbuf);
        if (ret != 0) {
            if ((errno == ENOENT) || (errno == ENOTDIR))
                return check;
            else
                return nullptr;
        }
    }

    /* We got out of the loop because we ran out of combinations to try.  */
    return nullptr;
}

bool isTheora(String ogg_filename)
{
    FILE* f;
    char buffer[7];
    f = fopen(ogg_filename.c_str(), "rb");

    if (!f) {
        throw _Exception(_("Error opening ") + ogg_filename + _(" : ") + mt_strerror(errno));
    }

    if (fread(buffer, 1, 4, f) != 4) {
        fclose(f);
        throw _Exception(_("Error reading ") + ogg_filename);
    }

    if (memcmp(buffer, "OggS", 4) != 0) {
        fclose(f);
        return false;
    }

    if (fseek(f, 28, SEEK_SET) != 0) {
        fclose(f);
        throw _Exception(_("Incomplete file ") + ogg_filename);
    }

    if (fread(buffer, 1, 7, f) != 7) {
        fclose(f);
        throw _Exception(_("Error reading ") + ogg_filename);
    }

    if (memcmp(buffer, "\x80theora", 7) != 0) {
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}

String get_last_path(String location)
{
    String path;

    int last_slash = location.rindex(DIR_SEPARATOR);
    if (last_slash > 0) {
        path = location.substring(0, last_slash);
        int slash = path.rindex(DIR_SEPARATOR);
        if ((slash >= 0) && ((slash + 1) < path.length()))
            path = path.substring(slash + 1);
    }

    return path;
}

ssize_t getValidUTF8CutPosition(zmm::String str, ssize_t cutpos)
{
    ssize_t pos = -1;
    size_t len = str.length();

    if ((len == 0) || (cutpos > (ssize_t)len))
        return pos;

    printf("Character at cut position: %0x\n", (char)str.charAt(cutpos));
    printf("Character at cut-1 position: %0x\n", (char)str.charAt(cutpos - 1));
    printf("Character at cut-2 position: %0x\n", (char)str.charAt(cutpos - 2));
    printf("Character at cut-3 position: %0x\n", (char)str.charAt(cutpos - 3));

    // > 0x7f, we are dealing with a non-ascii character
    if (str.charAt(cutpos) & 0x80) {
        // check if we are at byte 2
        if (((cutpos - 1) >= 0) && (((str.charAt(cutpos - 1) & 0xc2) == 0xc2) || ((str.charAt(cutpos - 1) & 0xe2) == 0xe2) || ((str.charAt(cutpos - 1) & 0xf0) == 0xf0)))
            pos = cutpos - 1;
        // check if we are at byte 3
        else if (((cutpos - 2) >= 0) && (((str.charAt(cutpos - 2) & 0xe2) == 0xe2) || ((str.charAt(cutpos - 2) & 0xf0) == 0xf0)))
            pos = cutpos - 2;
        // we must be at byte 4 then...
        else if ((cutpos - 3) >= 0)
            pos = cutpos - 3;
    } else
        pos = cutpos;

    return pos;
}

String getDLNAprofileString(String contentType)
{
    String profile;
    if (contentType == CONTENT_TYPE_MP4)
        profile = _(D_PN_AVC_MP4_EU);
    else if (contentType == CONTENT_TYPE_MKV)
        profile = _(D_PN_MKV);
    else if (contentType == CONTENT_TYPE_AVI)
        profile = _(D_PN_AVI);
    else if (contentType == CONTENT_TYPE_MPEG)
        profile = _(D_PN_MPEG_PS_PAL);
    else if (contentType == CONTENT_TYPE_MP3)
        profile = _(D_MP3);
    else if (contentType == CONTENT_TYPE_PCM)
        profile = _(D_LPCM);
    else
        profile = _("");

    if (string_ok(profile))
        profile = _(D_PROFILE) + "=" + profile;
    return profile;
}

String getDLNAcontentHeader(String contentType)
{
    Ref<ConfigManager> config = ConfigManager::getInstance();
    if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO)) {
        String content_parameter;
        content_parameter = getDLNAprofileString(contentType);
        if (string_ok(content_parameter))
            content_parameter = _(D_PROFILE) + _("=") + content_parameter + ";";
        // enabling or disabling seek
        if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO_DLNA_SEEK))
            content_parameter = content_parameter + D_OP + "=" + D_OP_SEEK_ENABLED + ";";
        else
            content_parameter = content_parameter + D_OP + "=" + D_OP_SEEK_DISABLED + ";";
        content_parameter = content_parameter + D_CONVERSION_INDICATOR + "=" + D_NO_CONVERSION + ";";
        content_parameter = content_parameter + D_FLAGS "=" D_TR_FLAGS_AV;
        return _(D_HTTP_CONTENT_FEATURES_HEADER) + content_parameter;
    }
    return nullptr;
}

String getDLNAtransferHeader(String mimeType)
{
    if (ConfigManager::getInstance()->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO)) {
        String transfer_parameter;
        if (mimeType.startsWith(_("image")))
            transfer_parameter = _(D_HTTP_TRANSFER_MODE_INTERACTIVE);
        else if (mimeType.startsWith(_("audio")) || mimeType.startsWith(_("video")))
            transfer_parameter = _(D_HTTP_TRANSFER_MODE_STREAMING);

        if (string_ok(transfer_parameter)) {
            return _(D_HTTP_TRANSFER_MODE_HEADER) + transfer_parameter;
        }
    }

    return nullptr;
}

#ifndef HAVE_FFMPEG
String getAVIFourCC(zmm::String avi_filename)
{
#define FCC_OFFSET 0xbc
    char* buffer;
    FILE* f = fopen(avi_filename.c_str(), "rb");
    if (!f)
        throw _Exception(_("could not open file ") + avi_filename + " : " + mt_strerror(errno));

    buffer = (char*)MALLOC(FCC_OFFSET + 6);
    if (buffer == nullptr) {
        fclose(f);
        throw _Exception(_("Out of memory when allocating buffer for file ") + avi_filename);
    }

    size_t rb = fread(buffer, 1, FCC_OFFSET + 4, f);
    fclose(f);
    if (rb != FCC_OFFSET + 4) {
        free(buffer);
        throw _Exception(_("could not read header of ") + avi_filename + " : " + mt_strerror(errno));
    }

    buffer[FCC_OFFSET + 5] = '\0';

    if (strncmp(buffer, "RIFF", 4) != 0) {
        free(buffer);
        return nullptr;
    }

    if (strncmp(buffer + 8, "AVI ", 4) != 0) {
        free(buffer);
        return nullptr;
    }

    String fourcc = String(buffer + FCC_OFFSET, 4);
    free(buffer);

    if (string_ok(fourcc))
        return fourcc;
    else
        return nullptr;
}
#endif

#ifdef TOMBDEBUG

void profiling_thread_check(struct profiling_t* data)
{
    if (data->thread != pthread_self()) {
        log_debug("profiling_..() called from a different thread than profiling_init was called! (init: %d; this: %d) - aborting...\n", data->thread, pthread_self());
        print_backtrace();
        abort();
        return;
    }
}

void profiling_start(struct profiling_t* data)
{
    profiling_thread_check(data);
    if (data->running) {
        log_debug("profiling_start() called on an already running profile! - aborting...\n");
        print_backtrace();
        abort();
        return;
    }
    data->running = true;
    getTimespecNow(&(data->last_start));
}

void profiling_end(struct profiling_t* data)
{
    profiling_thread_check(data);
    struct timespec now;
    getTimespecNow(&now);
    if (!data->running) {
        log_debug("profiling_end() called on a not-running profile! - aborting...\n");
        print_backtrace();
        abort();
        return;
    }
    struct timespec* sum = &(data->sum);
    struct timespec* last_start = &(data->last_start);
    sum->tv_sec += now.tv_sec - last_start->tv_sec;
    //log_debug("!!!!!! adding %d sec\n", now.tv_sec - last_start->tv_sec);
    if (now.tv_nsec >= last_start->tv_nsec) {
        sum->tv_nsec += now.tv_nsec - last_start->tv_nsec;
        //log_debug("adding %ld nsec\n", now.tv_nsec - last_start->tv_nsec);
    } else {
        sum->tv_nsec += 1000000000L - last_start->tv_nsec + now.tv_nsec;
        sum->tv_sec--;
    }
    if (sum->tv_nsec >= 1000000000L) {
        sum->tv_nsec -= 1000000000L;
        sum->tv_sec++;
    }

    data->running = false;
}

void profiling_print(struct profiling_t* data)
{
    if (data->running) {
        log_debug("profiling_print() called on running profile! - aborting...\n");
        print_backtrace();
        abort();
        return;
    }
    //log_debug("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\nPROFILING: took %ld sec %ld nsec thread %u\n", data->sum.tv_sec, data->sum.tv_nsec, pthread_self());
    log_debug("PROFILING: took %ld sec %ld nsec\n", data->sum.tv_sec, data->sum.tv_nsec);
}

#endif

#ifdef SOPCAST
/// \brief
int find_local_port(unsigned short range_min, unsigned short range_max)
{
    int fd;
    int retry_count = 0;
    int port;
    struct sockaddr_in server_addr;
    struct hostent* server;

    if (range_min > range_max) {
        log_error("min port range > max port range!\n");
        return -1;
    }

    do {
        port = rand() % (range_max - range_min) + range_min;

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            log_error("could not determine free port: "
                      "error creating socket (%s)\n",
                mt_strerror(errno).c_str());
            return -1;
        }

        server = gethostbyname("127.0.0.1");
        if (server == NULL) {
            log_error("could not resolve localhost\n");
            close(fd);
            return -1;
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
        server_addr.sin_port = htons(port);

        if (connect(fd, (struct sockaddr*)&server_addr,
                sizeof(server_addr))
            == -1) {
            close(fd);
            return port;
        }

        retry_count++;

    } while (retry_count < USHRT_MAX);

    log_error("Could not find free port on localhost\n");

    return -1;
}
#endif
