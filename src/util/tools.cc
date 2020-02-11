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
#include <cctype>
#include <iterator>
#include <netdb.h>
#include <netinet/in.h>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <queue>
#include <filesystem>
namespace fs = std::filesystem;

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

#include "config/config_manager.h"
#include "contrib/md5.h"
#include "iohandler/file_io_handler.h"
#include "metadata/metadata_handler.h"
#include "tools.h"
#include "string_tokenizer.h"

#define WHITE_SPACE " \t\r\n"

using namespace std;

static const char* HEX_CHARS = "0123456789abcdef";

std::vector<std::string> split_string(std::string str, char sep, bool empty)
{
    std::vector<std::string> ret;
    const char* data = str.c_str();
    const char* end = data + str.length();
    while (data < end) {
        const char* pos = strchr(data, sep);
        if (pos == nullptr) {
            std::string part = data;
            ret.push_back(part);
            data = end;
        } else if (pos == data) {
            data++;
            if ((data < end) && empty)
                ret.push_back("");
        } else {
            std::string part(data, pos - data);
            ret.push_back(part);
            data = pos + 1;
        }
    }
    return ret;
}

std::string trim_string(std::string str)
{
    if (str.empty())
        return str;

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
        return "";
    for (i = len - 1; i >= start; i--) {
        if (!strchr(WHITE_SPACE, buf[i])) {
            end = i + 1;
            break;
        }
    }
    return str.substr(start, end - start);
}

bool startswith(std::string str, std::string check)
{
    return str.rfind(check, 0) == 0;
}

std::string tolower_string(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

int stoi_string(std::string str, int def)
{
    if (str.empty())
        return def;

    return std::stoi(str);
}

std::string reduce_string(std::string str, char ch)
{
    std::string::iterator new_end = std::unique(
        str.begin(), str.end(),
        [&](char lhs, char rhs) { return (lhs == rhs) && (lhs == ch); }
    );

    str.erase(new_end, str.end());
    return str;
}

time_t getLastWriteTime(const fs::path& path)
{
    // in future with C+20 we can replace this function too:
    // auto ftime = fs::last_write_time(p);
    // time_t cftime = decltype(ftime)::clock::to_time_t(ftime);

    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        throw std::runtime_error(mt_strerror(errno) + ": " + path.string());
    }

    return statbuf.st_mtime;
}

bool is_executable(const fs::path& path, int* err)
{
    int ret = access(path.c_str(), R_OK | X_OK);
    if (err != nullptr)
        *err = errno;

    if (ret == 0)
        return true;
    else
        return false;
}

fs::path find_in_path(const fs::path& exec)
{
    std::string PATH = getenv("PATH");
    if (!string_ok(PATH))
        return "";

    auto st = std::make_unique<StringTokenizer>(PATH);
    std::string path = "";
    std::string next;
    do {
        if (path.empty())
            path = st->nextToken(":");
        next = st->nextToken(":");

        if (path.empty())
            break;

        if ((!next.empty()) && !startswith(next, "/")) {
            path = path + ":" + next;
            next = "";
        }

        fs::path check = fs::path(path) / exec;
        if (fs::is_regular_file(check))
            return check;

        if (!next.empty())
            path = next;
        else
            path = "";

    } while (!path.empty());

    return "";
}

bool string_ok(std::string str)
{
    if (str.empty())
        return false;
    return true;
}

void string_ok_ex(std::string str)
{
    if (str.empty())
        throw std::runtime_error("Empty string");
}

std::string http_redirect_to(std::string ip, std::string port, std::string page)
{
    return R"(<html><head><meta http-equiv="Refresh" content="0;URL=http://)" + ip + ":" + port + "/" + page + R"("></head><body bgcolor="#dddddd"></body></html>)";
}

std::string hex_encode(const void* data, int len)
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

std::string hex_decode_string(std::string encoded)
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

std::string hex_md5(const void* data, int length)
{
    char md5buf[16];

    md5_state_t ctx;
    md5_init(&ctx);
    md5_append(&ctx, (unsigned char*)data, length);
    md5_finish(&ctx, (unsigned char*)md5buf);

    return hex_encode(md5buf, 16);
}
std::string hex_string_md5(std::string str)
{
    return hex_md5(str.c_str(), str.length());
}
std::string generate_random_id()
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

    log_debug("Generated: {}", uuid_str);
    std::string uuid_String = std::string(uuid_str);
#ifdef BSD_NATIVE_UUID
    free(uuid_str);
#endif

    return uuid_String;
}

static const char* HEX_CHARS2 = "0123456789ABCDEF";

std::string url_escape(std::string str)
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

std::string urlUnescape(std::string str)
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

static std::string dict_encode(const std::map<std::string,std::string>& dict, char sep1, char sep2)
{
    std::ostringstream buf;
    for (auto it = dict.begin(); it != dict.end(); it++) {
        if (it != dict.begin())
            buf << sep1;
        buf << url_escape(it->first) << sep2
            << url_escape(it->second);
    }
    return buf.str();
}

std::string dict_encode(const std::map<std::string,std::string>& dict)
{
    return dict_encode(dict, '&', '=');
}

std::string dict_encode_simple(const std::map<std::string,std::string>& dict)
{
    return dict_encode(dict, '/', '/');
}

void dict_decode(std::string url, std::map<std::string,std::string>* dict)
{
    const char* data = url.c_str();
    const char* dataEnd = data + url.length();
    while (data < dataEnd) {
        const char* ampPos = strchr(data, '&');
        if (!ampPos) {
            ampPos = dataEnd;
        }
        const char* eqPos = strchr(data, '=');
        if (eqPos && eqPos < ampPos) {
            std::string key(data, eqPos - data);
            std::string value(eqPos + 1, ampPos - eqPos - 1);
            key = urlUnescape(key);
            value = urlUnescape(value);

            dict->insert(std::pair(key, value));
        }
        data = ampPos + 1;
    }
}

// this is somewhat tricky as we need an exact amount of pairs
// object_id=720&res_id=0
void dict_decode_simple(std::string url, std::map<std::string,std::string>* dict)
{
    std::string encoded;
    size_t pos;
    size_t last_pos = 0;
    do {
        pos = url.find('/', last_pos);
        if (pos == std::string::npos || pos < last_pos + 1)
            break;

        std::string key = urlUnescape(url.substr(last_pos, pos - last_pos));
        last_pos = pos + 1;
        pos = url.find('/', last_pos);
        if (pos == std::string::npos)
            pos = url.length();
        if (pos < last_pos + 1)
            break;

        std::string value = urlUnescape(url.substr(last_pos, pos - last_pos));
        last_pos = pos + 1;

        dict->insert(std::pair(key, value));
    } while (last_pos < url.length());
}

std::string mime_types_to_CSV(std::vector<std::string> mimeTypes)
{
    std::ostringstream buf;
    for (size_t i = 0; i < mimeTypes.size(); i++) {
        if (i > 0)
            buf << ",";
        std::string mimeType = mimeTypes[i];
        buf << "http-get:*:" << mimeType << ":*";
    }

    return buf.str();
}

std::string mt_strerror(int mt_errno)
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
        return "cannot get error string: error while calling XSI-compliant strerror_r";
    err_str = buffer;
#endif
    std::string errStr(err_str);
    FREE(buffer);
    return errStr;
#else
    return strerror(errno);
#endif
}

std::string readTextFile(const fs::path& path)
{
    FILE* f = fopen(path.c_str(), "rt");
    if (!f) {
        throw std::runtime_error("readTextFile: could not open " + path.string() + " : " + mt_strerror(errno));
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

void writeTextFile(const fs::path& path, std::string contents)
{
    size_t bytesWritten;
    FILE* f = fopen(path.c_str(), "wt");
    if (!f) {
        throw std::runtime_error("writeTextFile: could not open " + path.string() + " : " + mt_strerror(errno));
    }

    bytesWritten = fwrite(contents.c_str(), 1, contents.length(), f);
    if (bytesWritten < contents.length()) {
        fclose(f);
        if (bytesWritten >= 0)
            throw std::runtime_error("writeTextFile: incomplete write to " + path.string() + " : ");
        else
            throw std::runtime_error("writeTextFile: error writing to " + path.string() + " : " + mt_strerror(errno));
    }
    fclose(f);
}

std::string renderProtocolInfo(std::string mimetype, std::string protocol, std::string extend)
{
    if (string_ok(mimetype) && string_ok(protocol)) {
        if (string_ok(extend))
            return protocol + ":*:" + mimetype + ":" + extend;
        else
            return protocol + ":*:" + mimetype + ":*";
    } else
        return "http-get:*:*:*";
}

std::string getMTFromProtocolInfo(std::string protocol)
{
    std::vector<std::string> parts = split_string(protocol, ':');
    if (parts.size() > 2)
        return parts[2];
    else
        return "";
}

std::string getProtocol(std::string protocolInfo)
{
    std::string protocol;
    size_t pos = protocolInfo.find(':');
    if (pos == std::string::npos || pos == 0)
        protocol = "http-get";
    else
        protocol = protocolInfo.substr(0, pos);

    return protocol;
}

std::string secondsToHMS(int seconds)
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

    // TOOD: optimize
    char* tmp = (char*)MALLOC(10);
    sprintf(tmp, "%02d:%02d:%02d", h, m, s);

    std::string res = tmp;
    FREE(tmp);

    return res;
}

int HMSToSeconds(std::string time)
{
    if (!string_ok(time)) {
        log_warning("Could not convert time representation to seconds!");
        return 0;
    }

    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    sscanf(time.c_str(), "%d:%d:%d", &hours, &minutes, &seconds);

    return (hours * 3600) + (minutes * 60) + seconds;
}

#ifdef HAVE_MAGIC
std::string getMIMETypeFromFile(const fs::path& file)
{
    return getMIME(file, nullptr, -1);
}

std::string getMIMETypeFromBuffer(const void *buffer, size_t length)
{
    return getMIME("", buffer, length);
}

std::string getMIME(const fs::path& filepath, const void *buffer, size_t length)
{
    /* MAGIC_MIME_TYPE tells magic to return ONLY the mimetype */
    magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);

    if (magic_cookie == NULL) {
        log_warning("Failed to initialize libmagic");
        return "";
    }

    if (magic_load(magic_cookie, NULL) != 0) {
        log_warning("Failed to load magic database: {}", magic_error(magic_cookie));
        magic_close(magic_cookie);
        return "";
    }

    const char* mime;
    if (!string_ok(filepath)) {
        mime = magic_buffer(magic_cookie, buffer, length);
    } else {
        mime = magic_file(magic_cookie, filepath.c_str());
    }

    std::string out = mime;
    magic_close(magic_cookie);
    return out;
}
#endif

void set_jpeg_resolution_resource(std::shared_ptr<CdsItem> item, int res_num)
{
    try {
        std::unique_ptr<IOHandler> fio_h = std::make_unique<FileIOHandler>(item->getLocation());
        fio_h->open(UPNP_READ);
        std::string resolution = get_jpeg_resolution(fio_h);

        if (res_num >= item->getResourceCount())
            throw std::runtime_error("Invalid resource index");

        item->getResource(res_num)->addAttribute(MetadataHandler::getResAttrName(R_RESOLUTION), resolution);
    } catch (const std::runtime_error& e) {
        log_error("Exception! {}", e.what());
    }
}

bool check_resolution(std::string resolution, int* x, int* y)
{
    if (x != nullptr)
        *x = 0;

    if (y != nullptr)
        *y = 0;

    std::vector<std::string> parts = split_string(resolution, 'x');
    if (parts.size() != 2)
        return false;

    if (string_ok(parts[0]) && string_ok(parts[1])) {
        int _x = std::stoi(parts[0]);
        int _y = std::stoi(parts[1]);

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

std::string escape(std::string string, char escape_char, char to_escape)
{
    std::ostringstream buf;
    size_t len = string.length();

    bool possible_more_esc = true;
    bool possible_more_char = true;

    size_t last = 0;
    do {
        size_t next_esc = std::string::npos;
        if (possible_more_esc) {
            next_esc = string.find(escape_char, last);
            if (next_esc == std::string::npos)
                possible_more_esc = false;
        }

        size_t next = std::string::npos;
        if (possible_more_char) {
            next = string.find(to_escape, last);
            if (next == std::string::npos)
                possible_more_char = false;
        }

        if (next == std::string::npos || (next_esc != std::string::npos && next_esc < next)) {
            next = next_esc;
        }

        if (next == std::string::npos)
            next = len;
        int cpLen = next - last;
        if (cpLen > 0) {
            buf.write(&string[last], cpLen);
        }
        if (next < len) {
            buf << '\\';
            buf << string.at(next);
        }
        last = next;
        last++;
    } while (last < len);

    return buf.str();
}

std::string unescape(std::string string, char escape)
{
    std::ostringstream buf;
    size_t len = string.length();

    size_t last = std::string::npos;
    do {
        size_t next = string.find(escape, last + 1);
        if (next == std::string::npos)
            next = len;
        if (last == std::string::npos)
            last = 0;
        int cpLen = next - last;
        if (cpLen > 0)
            buf.write(&string[last], cpLen);
        last = next;
        last++;
    } while (last < len);

    return buf.str();
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

std::string unescape_amp(std::string string)
{
    if (string.empty())
        return "";

    std::ostringstream buf;
    size_t len = string.length();

    size_t last = 0;
    do {
        int skip = 0;
        size_t next = last - 1;
        do {
            next = string.find('&', next + 1);
            if (next == std::string::npos) break;
            if ((next < len) && (string.at(next + 1) == 'a') && (string.at(next + 2) == 'm') && (string.at(next + 3) == 'p') && (string.at(next + 4) == ';')) {
                skip = 4;
            }
        } while (next > 0 && skip == 0);

        if (next == std::string::npos)
            next = len;

        int cpLen = next - last + 1;
        buf.write(&string[last], cpLen);
        last = next + skip + 1;
    } while (last <= len);

    return buf.str();
}

std::string fallbackString(std::string first, std::string fallback)
{
    if (first.empty())
        return fallback;
    return first;
}

unsigned int stringHash(std::string str)
{
    unsigned int hash = 5381;
    auto* data = (unsigned char*)str.c_str();
    int c;
    while ((c = *data++))
        hash = ((hash << 5) + hash) ^ c; /* (hash * 33) ^ c */
    return hash;
}

std::string getValueOrDefault(const std::map<std::string,std::string>& m, const std::string& key, const std::string& defval)
{
    return getValueOrDefault<std::string,std::string>(m, key, defval);
}

std::string toCSV(shared_ptr<unordered_set<int>> array)
{
    if (array->empty())
        return "";
    return join(*array, ",");
}

void getTimespecNow(struct timespec* ts)
{
    struct timeval tv;
    int ret = gettimeofday(&tv, nullptr);
    if (ret != 0)
        throw std::runtime_error("gettimeofday failed: " + mt_strerror(errno));

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

    // log_debug("timespec: sec: {}, nsec: {}", ret->tv_sec, ret->tv_nsec);
}

std::string interfaceToIP(std::string interface)
{

    struct if_nameindex* iflist = nullptr;
    struct if_nameindex* iflist_free = nullptr;
    struct ifreq if_request;
    struct sockaddr_in local_address;
    int local_socket;

    if (!string_ok(interface))
        return "";

    local_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (local_socket < 0) {
        log_error("Could not create local socket: {}",
            mt_strerror(errno).c_str());
        return "";
    }

    iflist = iflist_free = if_nameindex();
    if (iflist == nullptr) {
        log_error("Could not get interface list: {}",
            mt_strerror(errno).c_str());
        close(local_socket);
        return "";
    }

    while (iflist->if_index || iflist->if_name) {
        if (interface == iflist->if_name) {
            strncpy(if_request.ifr_name, iflist->if_name, IF_NAMESIZE);
            if (ioctl(local_socket, SIOCGIFADDR, &if_request) != 0) {
                log_error("Could not determine interface address: {}",
                    mt_strerror(errno).c_str());
                close(local_socket);
                if_freenameindex(iflist_free);
                return "";
            }

            memcpy(&local_address, &if_request.ifr_addr, sizeof(if_request.ifr_addr));
            std::string ip = inet_ntoa(local_address.sin_addr);
            if_freenameindex(iflist_free);
            close(local_socket);
            return ip;
        }
        iflist++;
    }

    close(local_socket);
    if_freenameindex(iflist_free);
    return "";
}

std::string ipToInterface(std::string ip)
{
    if (!string_ok(ip)) {
        return "";
    } else {
        log_debug("Looking for '{}'", ip.c_str());
    }

    struct ifaddrs *ifaddr, *ifa;
    int family, s, n;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        log_error("Could not getifaddrs: {}", mt_strerror(errno).c_str());
    }

    for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;
        std::string name = ifa->ifa_name;

        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                log_error("getnameinfo() failed: {}", gai_strerror(s));
                return "";
            }

            std::string ipaddr = host;
            // IPv6 link locals come back as fe80::351d:d7f4:6b17:3396%eth0
            if (startswith(ipaddr, ip)) {
                return name;
            }
        }
    }

    freeifaddrs(ifaddr);
    log_warning("Failed to find interface for IP: {}", ip.c_str());
    return "";
}

bool validateYesNo(std::string value)
{
    if ((value != "yes") && (value != "no"))
        return false;
    else
        return true;
}

std::vector<std::string> populateCommandLine(std::string line, std::string in, std::string out, std::string range)
{
    log_debug("Template: '{}', in: '{}', out: '{}', range: '{}'", line, in, out, range);
    std::vector<std::string> params = split_string(line, ' ');
    if (in.empty() && out.empty())
        return params;

    for (auto& param : params) {
        size_t inPos = param.find("%in");
        if (inPos != string::npos) {
            std::string newParam = param.replace(inPos, 3, in);
        }

        size_t outPos = param.find("%out");
        if (outPos != string::npos) {
            std::string newParam = param.replace(outPos, 4, out);
        }

        size_t rangePos = param.find("%range");
        if (rangePos != string::npos) {
            std::string newParam = param.replace(rangePos, 5, range);
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
fs::path tempName(const fs::path& leadPath, char* tmpl)
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
        return "";
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

        fs::path check = leadPath / tmpl;
        ret = stat(check.c_str(), &statbuf);
        if (ret != 0) {
            if ((errno == ENOENT) || (errno == ENOTDIR))
                return check;
            else
                return "";
        }
    }

    /* We got out of the loop because we ran out of combinations to try.  */
    return "";
}

bool isTheora(const fs::path& ogg_filename)
{
    FILE* f;
    char buffer[7];
    f = fopen(ogg_filename.c_str(), "rb");

    if (!f) {
        throw std::runtime_error("Error opening " + ogg_filename.string() + " : " + mt_strerror(errno));
    }

    if (fread(buffer, 1, 4, f) != 4) {
        fclose(f);
        throw std::runtime_error("Error reading " + ogg_filename.string());
    }

    if (memcmp(buffer, "OggS", 4) != 0) {
        fclose(f);
        return false;
    }

    if (fseek(f, 28, SEEK_SET) != 0) {
        fclose(f);
        throw std::runtime_error("Incomplete file " + ogg_filename.string());
    }

    if (fread(buffer, 1, 7, f) != 7) {
        fclose(f);
        throw std::runtime_error("Error reading " + ogg_filename.string());
    }

    if (memcmp(buffer, "\x80theora", 7) != 0) {
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}

fs::path get_last_path(const fs::path& path)
{
    fs::path ret;

    auto it = std::prev(path.end());    // filename
    if (it != path.end())
        it = std::prev(it);             // last path
    if (it != path.end())
        ret = *it;

    return ret;
}

ssize_t getValidUTF8CutPosition(std::string str, ssize_t cutpos)
{
    ssize_t pos = -1;
    size_t len = str.length();

    if ((len == 0) || (cutpos > (ssize_t)len))
        return pos;

    printf("Character at cut position: %0x\n", (char)str.at(cutpos));
    printf("Character at cut-1 position: %0x\n", (char)str.at(cutpos - 1));
    printf("Character at cut-2 position: %0x\n", (char)str.at(cutpos - 2));
    printf("Character at cut-3 position: %0x\n", (char)str.at(cutpos - 3));

    // > 0x7f, we are dealing with a non-ascii character
    if (str.at(cutpos) & 0x80) {
        // check if we are at byte 2
        if (((cutpos - 1) >= 0) && (((str.at(cutpos - 1) & 0xc2) == 0xc2) || ((str.at(cutpos - 1) & 0xe2) == 0xe2) || ((str.at(cutpos - 1) & 0xf0) == 0xf0)))
            pos = cutpos - 1;
        // check if we are at byte 3
        else if (((cutpos - 2) >= 0) && (((str.at(cutpos - 2) & 0xe2) == 0xe2) || ((str.at(cutpos - 2) & 0xf0) == 0xf0)))
            pos = cutpos - 2;
        // we must be at byte 4 then...
        else if ((cutpos - 3) >= 0)
            pos = cutpos - 3;
    } else
        pos = cutpos;

    return pos;
}

std::string getDLNAprofileString(std::string contentType)
{
    std::string profile;
    if (contentType == CONTENT_TYPE_MP4)
        profile = D_PN_AVC_MP4_EU;
    else if (contentType == CONTENT_TYPE_MKV)
        profile = D_PN_MKV;
    else if (contentType == CONTENT_TYPE_AVI)
        profile = D_PN_AVI;
    else if (contentType == CONTENT_TYPE_MPEG)
        profile = D_PN_MPEG_PS_PAL;
    else if (contentType == CONTENT_TYPE_MP3)
        profile = D_MP3;
    else if (contentType == CONTENT_TYPE_PCM)
        profile = D_LPCM;
    else
        profile = "";

    if (string_ok(profile))
        profile = std::string(D_PROFILE) + "=" + profile;
    return profile;
}

std::string getDLNAContentHeader(std::shared_ptr<ConfigManager> config, std::string contentType)
{
    if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO)) {
        std::string content_parameter;
        content_parameter = getDLNAprofileString(contentType);
        if (string_ok(content_parameter))
            content_parameter = D_PROFILE + std::string("=") + content_parameter + ";";
        // enabling or disabling seek
        if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO_DLNA_SEEK))
            content_parameter = content_parameter + D_OP + "=" + D_OP_SEEK_ENABLED + ";";
        else
            content_parameter = content_parameter + D_OP + "=" + D_OP_SEEK_DISABLED + ";";
        content_parameter = content_parameter + D_CONVERSION_INDICATOR + "=" + D_NO_CONVERSION + ";";
        content_parameter = content_parameter + D_FLAGS "=" D_TR_FLAGS_AV;
        return content_parameter;
    }
    return "";
}

std::string getDLNATransferHeader(std::shared_ptr<ConfigManager> config, std::string mimeType)
{
    if (config->getBoolOption(CFG_SERVER_EXTEND_PROTOCOLINFO)) {
        std::string transfer_parameter;
        if (startswith(mimeType, "image"))
            transfer_parameter = D_HTTP_TRANSFER_MODE_INTERACTIVE;
        else if (startswith(mimeType, "audio") || startswith(mimeType, "video"))
            transfer_parameter = D_HTTP_TRANSFER_MODE_STREAMING;

        if (string_ok(transfer_parameter)) {
            return transfer_parameter;
        }
    }
    return "";
}

#ifndef HAVE_FFMPEG
std::string getAVIFourCC(const fs::path& avi_filename)
{
#define FCC_OFFSET 0xbc
    char* buffer;
    FILE* f = fopen(avi_filename.c_str(), "rb");
    if (!f)
        throw std::runtime_error("could not open file " + avi_filename + " : " + mt_strerror(errno));

    buffer = (char*)MALLOC(FCC_OFFSET + 6);
    if (buffer == nullptr) {
        fclose(f);
        throw std::runtime_error("Out of memory when allocating buffer for file " + avi_filename);
    }

    size_t rb = fread(buffer, 1, FCC_OFFSET + 4, f);
    fclose(f);
    if (rb != FCC_OFFSET + 4) {
        free(buffer);
        throw std::runtime_error("could not read header of " + avi_filename + " : " + mt_strerror(errno));
    }

    buffer[FCC_OFFSET + 5] = '\0';

    if (strncmp(buffer, "RIFF", 4) != 0) {
        free(buffer);
        return "";
    }

    if (strncmp(buffer + 8, "AVI ", 4) != 0) {
        free(buffer);
        return "";
    }

    std::string fourcc = std::string(buffer + FCC_OFFSET, 4);
    free(buffer);

    if (string_ok(fourcc))
        return fourcc;
    else
        return "";
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
        log_error("min port range > max port range!");
        return -1;
    }

    do {
        port = rand() % (range_max - range_min) + range_min;

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            log_error("could not determine free port: error creating socket ({})\n",
                mt_strerror(errno).c_str());
            return -1;
        }

        server = gethostbyname("127.0.0.1");
        if (server == NULL) {
            log_error("could not resolve localhost");
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

    log_error("Could not find free port on localhost");

    return -1;
}
#endif
