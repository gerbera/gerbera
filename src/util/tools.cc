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

#include "tools.h" // API

#include <arpa/inet.h>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <netdb.h>
#include <netinet/in.h>
#include <queue>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

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

#include <fmt/ostream.h>

#include "cds_objects.h"
#include "config/config_manager.h"
#include "contrib/md5.h"
#include "iohandler/file_io_handler.h"
#include "metadata/metadata_handler.h"

#define WHITE_SPACE " \t\r\n"

static constexpr const char* HEX_CHARS = "0123456789abcdef";

std::vector<std::string> splitString(const std::string& str, char sep, bool empty)
{
    std::vector<std::string> ret;
    const char* data = str.c_str();
    const char* end = data + str.length();
    while (data < end) {
        auto pos = std::strchr(data, sep);
        if (pos == nullptr) {
            std::string part = data;
            ret.push_back(part);
            data = end;
        } else if (pos == data) {
            data++;
            if ((data < end) && empty)
                ret.emplace_back("");
        } else {
            std::string part(data, pos - data);
            ret.push_back(part);
            data = pos + 1;
        }
    }
    return ret;
}

void leftTrimStringInPlace(std::string& str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

void rightTrimStringInPlace(std::string& str)
{
    str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(),
        str.end());
}

void trimStringInPlace(std::string& str)
{
    leftTrimStringInPlace(str);
    rightTrimStringInPlace(str);
}

std::string trimString(std::string str)
{
    if (str.empty())
        return str;

    leftTrimStringInPlace(str);
    rightTrimStringInPlace(str);
    return str;
}

bool startswith(const std::string& str, const std::string& check)
{
    return str.rfind(check, 0) == 0;
}

std::string toLower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

int stoiString(const std::string& str, int def, int base)
{
    if (str.empty() || !std::isdigit(*str.c_str()))
        return def;

    return std::stoi(str, nullptr, base);
}

std::string reduceString(std::string str, char ch)
{
    std::string::iterator new_end = std::unique(
        str.begin(), str.end(),
        [&](char lhs, char rhs) { return (lhs == rhs) && (lhs == ch); });

    str.erase(new_end, str.end());
    return str;
}

std::string& replaceString(std::string& str, std::string_view from, const std::string& to)
{
    size_t start_pos = str.find(from);
    if (start_pos != std::string::npos)
        str.replace(start_pos, from.length(), to);
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
        throw_std_runtime_error(fmt::format("{}: {}", strerror(errno), path.string()));
    }

    return statbuf.st_mtime;
}

bool isRegularFile(const fs::path& path)
{
    // unfortunately fs::is_regular_file(path, ec) does not to work for files >2GB on ARM 32bit systems (see #737)
    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        throw_std_runtime_error(fmt::format("{}: {}", strerror(errno), path.string()));
    }

    return S_ISREG(statbuf.st_mode);
}

bool isRegularFile(const fs::path& path, std::error_code& ec) noexcept
{
    // unfortunately fs::is_regular_file(path, ec) does not to work for files >2GB on ARM 32bit systems (see #737)
    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        ec = std::make_error_code(static_cast<std::errc>(errno));
        return false;
    }

    ec.clear();
    return S_ISREG(statbuf.st_mode);
}

off_t getFileSize(const fs::path& path)
{
    // unfortunately fs::file_size(path) does not to work for files >2GB on ARM 32bit systems (see #737)
    struct stat statbuf;
    int ret = stat(path.c_str(), &statbuf);
    if (ret != 0) {
        throw_std_runtime_error(fmt::format("{}: {}", strerror(errno), path.string()));
    }

    return statbuf.st_size;
}

bool isExecutable(const fs::path& path, int* err)
{
    int ret = access(path.c_str(), R_OK | X_OK);
    if (err != nullptr)
        *err = errno;

    return ret == 0;
}

fs::path findInPath(const fs::path& exec)
{
    std::string PATH = getenv("PATH");
    if (PATH.empty())
        return "";

    std::error_code ec;
    auto pathAr = splitString(PATH, ':');
    for (auto& path : pathAr) {
        fs::path check = fs::path(path) / exec;
        if (isRegularFile(check, ec))
            return check;
    }

    return "";
}

std::string renderWebUri(const std::string& ip, int port)
{
    std::ostringstream webUIAddr;
    if (ip.find(':') != std::string::npos) {
        webUIAddr << '[' << ip << ']';
    } else {
        webUIAddr << ip;
    }
    webUIAddr << ':' << port;
    return webUIAddr.str();
}

std::string httpRedirectTo(const std::string& addr, const std::string& page)
{
    return R"(<html><head><meta http-equiv="Refresh" content="0;URL=http://)" + addr + "/" + page + R"("></head><body bgcolor="#dddddd"></body></html>)";
}

std::string hexEncode(const void* data, int len)
{
    const unsigned char* chars;
    int i;
    unsigned char hi, lo;

    std::ostringstream buf;

    chars = static_cast<unsigned char*>(const_cast<void*>(data));
    for (i = 0; i < len; i++) {
        unsigned char c = chars[i];
        hi = c >> 4;
        lo = c & 0xF;
        buf << HEX_CHARS[hi] << HEX_CHARS[lo];
    }
    return buf.str();
}

std::string hexDecodeString(const std::string& encoded)
{
    auto ptr = const_cast<char*>(encoded.c_str());
    int len = encoded.length();

    std::ostringstream buf;
    for (int i = 0; i < len; i += 2) {
        auto chi = std::strchr(HEX_CHARS, ptr[i]);
        auto clo = std::strchr(HEX_CHARS, ptr[i + 1]);
        int hi = chi ? chi - HEX_CHARS : 0;
        int lo = clo ? clo - HEX_CHARS : 0;

        auto ch = static_cast<char>(hi << 4 | lo);
        buf << ch;
    }
    return buf.str();
}

std::string hexMd5(const void* data, int length)
{
    char md5buf[16];

    md5_state_t ctx;
    md5_init(&ctx);
    md5_append(&ctx, static_cast<unsigned char*>(const_cast<void*>(data)), length);
    md5_finish(&ctx, reinterpret_cast<unsigned char*>(md5buf));

    return hexEncode(md5buf, 16);
}
std::string hexStringMd5(const std::string& str)
{
    return hexMd5(str.c_str(), str.length());
}
std::string generateRandomId()
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

static constexpr const char* HEX_CHARS2 = "0123456789ABCDEF";

std::string urlEscape(const std::string& str)
{
    std::ostringstream buf;
    for (size_t i = 0; i < str.length();) {
        auto c = str[i];
        int cplen = 1;
        if ((c & 0xf8) == 0xf0)
            cplen = 4;
        else if ((c & 0xf0) == 0xe0)
            cplen = 3;
        else if ((c & 0xe0) == 0xc0)
            cplen = 2;
        if ((i + cplen) > str.length())
            cplen = 1;

        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '-') {
            buf << static_cast<char>(c);
        } else {
            int hi = c >> 4;
            int lo = c & 15;
            if (cplen > 1)
                buf << str.substr(i, cplen);
            else
                buf << '%' << HEX_CHARS2[hi] << HEX_CHARS2[lo];
        }
        i += cplen;
    }
    return buf.str();
}

std::string urlUnescape(const std::string& str)
{
    auto data = const_cast<char*>(str.c_str());
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

            pos = std::strchr(HEX_CHARS2, chi);
            hi = pos ? pos - HEX_CHARS2 : 0;

            pos = std::strchr(HEX_CHARS2, clo);
            lo = pos ? pos - HEX_CHARS2 : 0;

            int ascii = (hi << 4) | lo;
            buf << static_cast<char>(ascii);
        } else if (c == '+') {
            buf << ' ';
        } else {
            buf << c;
        }
    }
    return buf.str();
}

static std::string dictEncode(const std::map<std::string, std::string>& dict, char sep1, char sep2)
{
    std::ostringstream buf;
    for (auto it = dict.begin(); it != dict.end(); it++) {
        if (it != dict.begin())
            buf << sep1;
        buf << urlEscape(it->first) << sep2
            << urlEscape(it->second);
    }
    return buf.str();
}

std::string dictEncode(const std::map<std::string, std::string>& dict)
{
    return dictEncode(dict, '&', '=');
}

std::string dictEncodeSimple(const std::map<std::string, std::string>& dict)
{
    return dictEncode(dict, '/', '/');
}

void dictDecode(const std::string& url, std::map<std::string, std::string>* dict)
{
    auto data = url.c_str();
    auto dataEnd = data + url.length();
    while (data < dataEnd) {
        auto ampPos = reinterpret_cast<const char*>(std::strchr(data, '&'));
        if (!ampPos) {
            ampPos = dataEnd;
        }
        auto eqPos = std::strchr(data, '=');
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
void dictDecodeSimple(const std::string& url, std::map<std::string, std::string>* dict)
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

std::string mimeTypesToCsv(const std::vector<std::string>& mimeTypes)
{
    std::ostringstream buf;
    for (const auto& mimeType : mimeTypes) {
        buf << "http-get:*:" << mimeType << ":*"
            << ",";
    }

    return buf.str();
}

std::string readTextFile(const fs::path& path)
{
#ifdef __linux__
    auto f = ::fopen(path.c_str(), "rte");
#else
    auto f = ::fopen(path.c_str(), "rt");
#endif
    if (!f) {
        throw_std_runtime_error("could not open " + path.string() + " : " + strerror(errno));
    }
    std::ostringstream buf;
    std::array<char, 1024> buffer;
    size_t bytesRead;
    while ((bytesRead = std::fread(buffer.data(), 1, buffer.size(), f)) > 0) {
        buf << std::string(buffer.data(), bytesRead);
    }
    fclose(f);
    return buf.str();
}

void writeTextFile(const fs::path& path, const std::string& contents)
{
#ifdef __linux__
    auto f = ::fopen(path.c_str(), "wte");
#else
    auto f = ::fopen(path.c_str(), "wt");
#endif
    if (!f) {
        throw_std_runtime_error("could not open " + path.string() + " : " + strerror(errno));
    }

    size_t bytesWritten = std::fwrite(contents.c_str(), 1, contents.length(), f);
    if (bytesWritten < contents.length()) {
        fclose(f);

        throw_std_runtime_error("error writing to " + path.string() + " : " + strerror(errno));
    }
    fclose(f);
}

std::optional<std::vector<std::byte>> readBinaryFile(const fs::path& path)
{
    static_assert(sizeof(std::byte) == sizeof(std::ifstream::char_type));

    std::ifstream file { path, std::ios::in | std::ios::binary };
    if (!file)
        return {};

    auto& fb = *file.rdbuf();

    // Somewhat portable way to read file.
    // sgetn loops internally, so we need to check only the final result.
    // Also assume file size doesn't change while reading,
    // and no line conversion happens (therefore lseek returns result close to file size).
    auto size = fb.pubseekoff(0, std::ios::end);
    if (size < 0)
        throw_std_runtime_error(fmt::format("Can't determine file size of {}", path));

    fb.pubseekoff(0, std::ios::beg);

    std::vector<std::byte> result(size);
    size = fb.sgetn(reinterpret_cast<char*>(result.data()), size);
    if (size < 0 || !file)
        throw_std_runtime_error(fmt::format("Failed to read from file {}", path));

    result.resize(size);

    return result;
}

void writeBinaryFile(const fs::path& path, const std::byte* data, std::size_t size)
{
    static_assert(sizeof(std::byte) == sizeof(std::ifstream::char_type));

    std::ofstream file { path, std::ios::out | std::ios::binary | std::ios::trunc };
    if (!file)
        throw_std_runtime_error(fmt::format("Failed to open {}", path));

    file.rdbuf()->sputn(reinterpret_cast<const char*>(data), size);

    if (!file)
        throw_std_runtime_error(fmt::format("Failed to write to file {}", path));
}

std::string renderProtocolInfo(const std::string& mimetype, const std::string& protocol, const std::string& extend)
{
    if (!mimetype.empty() && !protocol.empty()) {
        if (!extend.empty())
            return protocol + ":*:" + mimetype + ":" + extend;
        return protocol + ":*:" + mimetype + ":*";
    }

    return "http-get:*:*:*";
}

std::string getMTFromProtocolInfo(const std::string& protocol)
{
    std::vector<std::string> parts = splitString(protocol, ':');
    if (parts.size() > 2)
        return parts[2];

    return "";
}

std::string getProtocol(const std::string& protocolInfo)
{
    size_t pos = protocolInfo.find(':');
    return (pos == std::string::npos || pos == 0) ? "http-get" : protocolInfo.substr(0, pos);
}

std::string millisecondsToHMSF(int milliseconds)
{
    auto ms = milliseconds % 1000;
    milliseconds /= 1000;

    auto s = milliseconds % 60;
    milliseconds /= 60;

    auto m = milliseconds % 60;
    auto h = milliseconds / 60;

    return fmt::format("{:01}:{:02}:{:02}.{:03}", h, m, s, ms);
}

int HMSFToMilliseconds(const std::string& time)
{
    if (time.empty()) {
        log_warning("Could not convert time representation to seconds!");
        return 0;
    }

    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int ms = 0;
    sscanf(time.c_str(), "%d:%d:%d.%d", &hours, &minutes, &seconds, &ms);

    return ((hours * 3600) + (minutes * 60) + seconds) * 1000 + ms;
}

bool checkResolution(const std::string& resolution, int* x, int* y)
{
    if (x != nullptr)
        *x = 0;

    if (y != nullptr)
        *y = 0;

    std::vector<std::string> parts = splitString(resolution, 'x');
    if (parts.size() != 2)
        return false;

    if (!parts[0].empty() && !parts[1].empty()) {
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

std::string fallbackString(const std::string& first, const std::string& fallback)
{
    return first.empty() ? fallback : first;
}

unsigned int stringHash(const std::string& str)
{
    unsigned int hash = 5381;
    auto data = str.c_str();
    int c;
    while ((c = *data++))
        hash = ((hash << 5) + hash) ^ c; /* (hash * 33) ^ c */
    return hash;
}

std::string getValueOrDefault(const std::map<std::string, std::string>& m, const std::string& key, const std::string& defval)
{
    return getValueOrDefault<std::string, std::string>(m, key, defval);
}

std::string toCSV(const std::shared_ptr<std::unordered_set<int>>& array)
{
    return array->empty() ? "" : join(*array, ",");
}

void getTimespecNow(struct timespec* ts)
{
    struct timeval tv;
    int ret = gettimeofday(&tv, nullptr);
    if (ret != 0)
        throw_std_runtime_error(fmt::format("gettimeofday failed: {}", strerror(errno)));

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

std::string ipToInterface(const std::string& ip)
{
    if (ip.empty()) {
        return "";
    }

    log_debug("Looking for '{}'", ip.c_str());

    struct ifaddrs *ifaddr, *ifa;
    int family, s, n;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        log_error("Could not getifaddrs: {}", strerror(errno));
    }

    for (ifa = ifaddr, n = 0; ifa != nullptr; ifa = ifa->ifa_next, n++) {
        if (ifa->ifa_addr == nullptr)
            continue;

        family = ifa->ifa_addr->sa_family;
        std::string name = ifa->ifa_name;

        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
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

bool validateYesNo(const std::string& value)
{
    return !((value != "yes") && (value != "no"));
}

std::vector<std::string> populateCommandLine(const std::string& line,
    const std::string& in,
    const std::string& out,
    const std::string& range,
    const std::string& title)
{
    log_debug("Template: '{}', in: '{}', out: '{}', range: '{}', title: '{}'", line, in, out, range, title);
    std::vector<std::string> params = splitString(line, ' ');
    if (in.empty() && out.empty())
        return params;

    for (auto& param : params) {
        size_t inPos = param.find("%in");
        if (inPos != std::string::npos) {
            std::string newParam = param.replace(inPos, 3, in);
        }

        size_t outPos = param.find("%out");
        if (outPos != std::string::npos) {
            std::string newParam = param.replace(outPos, 4, out);
        }

        size_t rangePos = param.find("%range");
        if (rangePos != std::string::npos) {
            std::string newParam = param.replace(rangePos, 6, range);
        }

        size_t titlePos = param.find("%title");
        if (titlePos != std::string::npos) {
            std::string newParam = param.replace(titlePos, 6, title);
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

    if (!XXXXXX || strncmp(XXXXXX, "XXXXXX", 6) != 0) {
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

            return "";
        }
    }

    /* We got out of the loop because we ran out of combinations to try.  */
    return "";
}

bool isTheora(const fs::path& ogg_filename)
{
    char buffer[7];

#ifdef __linux__
    auto f = ::fopen(ogg_filename.c_str(), "rbe");
#else
    auto f = ::fopen(ogg_filename.c_str(), "rb");
#endif
    if (!f) {
        throw_std_runtime_error("Error opening " + ogg_filename.string() + " : " + strerror(errno));
    }

    if (std::fread(buffer, 1, 4, f) != 4) {
        fclose(f);
        throw_std_runtime_error("Error reading " + ogg_filename.string());
    }

    if (memcmp(buffer, "OggS", 4) != 0) {
        fclose(f);
        return false;
    }

    if (fseek(f, 28, SEEK_SET) != 0) {
        fclose(f);
        throw_std_runtime_error("Incomplete file " + ogg_filename.string());
    }

    if (std::fread(buffer, 1, 7, f) != 7) {
        fclose(f);
        throw_std_runtime_error("Error reading " + ogg_filename.string());
    }

    if (memcmp(buffer, "\x80theora", 7) != 0) {
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}

fs::path getLastPath(const fs::path& path)
{
    fs::path ret;

    auto it = std::prev(path.end()); // filename
    if (it != path.end())
        it = std::prev(it); // last path
    if (it != path.end())
        ret = *it;

    return ret;
}

ssize_t getValidUTF8CutPosition(std::string str, ssize_t cutpos)
{
    ssize_t pos = -1;
    size_t len = str.length();

    if ((len == 0) || (cutpos > static_cast<ssize_t>(len)))
        return pos;

#if 0
    printf("Character at cut position: %0x\n", static_cast<char>(str.at(cutpos)));
    printf("Character at cut-1 position: %0x\n", static_cast<char>(str.at(cutpos - 1)));
    printf("Character at cut-2 position: %0x\n", static_cast<char>(str.at(cutpos - 2)));
    printf("Character at cut-3 position: %0x\n", static_cast<char>(str.at(cutpos - 3)));
#endif

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

std::string getDLNAprofileString(const std::string& contentType)
{
    std::string profile;
    if (contentType == CONTENT_TYPE_MP4)
        profile = UPNP_DLNA_PROFILE_AVC_MP4_EU;
    else if (contentType == CONTENT_TYPE_MKV)
        profile = UPNP_DLNA_PROFILE_MKV;
    else if (contentType == CONTENT_TYPE_AVI)
        profile = UPNP_DLNA_PROFILE_AVI;
    else if (contentType == CONTENT_TYPE_MPEG)
        profile = UPNP_DLNA_PROFILE_MPEG_PS_PAL;
    else if (contentType == CONTENT_TYPE_MP3)
        profile = UPNP_DLNA_PROFILE_MP3;
    else if (contentType == CONTENT_TYPE_PCM)
        profile = UPNP_DLNA_PROFILE_LPCM;
    else
        profile = "";

    if (!profile.empty())
        profile = std::string(UPNP_DLNA_PROFILE) + "=" + profile;
    return profile;
}

std::string getDLNAContentHeader(const std::shared_ptr<Config>& config, const std::string& contentType)
{
    std::string content_parameter;
    content_parameter = getDLNAprofileString(contentType);
    if (!content_parameter.empty())
        content_parameter += std::string(";");
    content_parameter += std::string(UPNP_DLNA_OP) + "=" + UPNP_DLNA_OP_SEEK_RANGE + ";";
    content_parameter += std::string(UPNP_DLNA_CONVERSION_INDICATOR) + "=" + UPNP_DLNA_NO_CONVERSION + ";";
    content_parameter += std::string(UPNP_DLNA_FLAGS "=" UPNP_DLNA_ORG_FLAGS_AV);
    return content_parameter;
}

std::string getDLNATransferHeader(const std::shared_ptr<Config>& config, const std::string& mimeType)
{
    std::string transfer_parameter;
    if (startswith(mimeType, "image"))
        transfer_parameter = UPNP_DLNA_TRANSFER_MODE_INTERACTIVE;
    else if (startswith(mimeType, "audio") || startswith(mimeType, "video"))
        transfer_parameter = UPNP_DLNA_TRANSFER_MODE_STREAMING;
    return transfer_parameter;
}

#ifndef HAVE_FFMPEG
std::string getAVIFourCC(const fs::path& avi_filename)
{
#define FCC_OFFSET 0xbc
    char* buffer;

#ifdef __linux__
    auto f = ::fopen(avi_filename.c_str(), "rbe");
#else
    auto f = ::fopen(avi_filename.c_str(), "rb");
#endif
    if (!f)
        throw_std_runtime_error("could not open file " + avi_filename.native() + " : " + strerror(errno));

    buffer = static_cast<char*>(malloc(FCC_OFFSET + 6));
    if (buffer == nullptr) {
        fclose(f);
        throw_std_runtime_error("Out of memory when allocating buffer for file " + avi_filename.native());
    }

    size_t rb = std::fread(buffer, 1, FCC_OFFSET + 4, f);
    fclose(f);
    if (rb != FCC_OFFSET + 4) {
        free(buffer);
        throw_std_runtime_error("could not read header of " + avi_filename.native() + " : " + strerror(errno));
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

    return fourcc;
}
#endif

std::string getHostName(const struct sockaddr* addr)
{
    char hoststr[NI_MAXHOST];
    char portstr[NI_MAXSERV];
    int len = addr->sa_family == AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
    int ret = getnameinfo(addr, len, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NOFQDN);
    if (ret != 0) {
        log_debug("could not determine getnameinfo: {}", strerror(errno));
    }

    return hoststr;
}

int sockAddrCmpAddr(const struct sockaddr* sa, const struct sockaddr* sb)
{
    if (sa->sa_family != sb->sa_family)
        return sa->sa_family - sb->sa_family;

    if (sa->sa_family == AF_INET) {
        return SOCK_ADDR_IN_ADDR(sa).s_addr - SOCK_ADDR_IN_ADDR(sb).s_addr;
    }

    if (sa->sa_family == AF_INET6) {
        return memcmp(reinterpret_cast<const char*>(&(SOCK_ADDR_IN6_ADDR(sa))), reinterpret_cast<const char*>(&(SOCK_ADDR_IN6_ADDR(sb))), sizeof(SOCK_ADDR_IN6_ADDR(sa)));
    }

    throw_std_runtime_error("unsupported address family :" + std::to_string(sa->sa_family));
}

std::string sockAddrGetNameInfo(const struct sockaddr* sa)
{
    char hoststr[NI_MAXHOST];
    char portstr[NI_MAXSERV];
    int len = sa->sa_family == AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in);
    int ret = getnameinfo(sa, len, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
    if (ret != 0) {
        throw_std_runtime_error(fmt::format("could not determine getnameinfo: {}", strerror(errno)));
    }

    return std::string(hoststr) + ":" + std::string(portstr);
}

#ifdef SOPCAST
/// \brief
int find_local_port(in_port_t range_min, in_port_t range_max)
{
    int fd;
    int retry_count = 0;
    in_port_t port;
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
            log_error("could not determine free port: error creating socket: {}", strerror(errno));
            return -1;
        }

        server = gethostbyname("127.0.0.1");
        if (server == nullptr) {
            log_error("could not resolve localhost");
            close(fd);
            return -1;
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
        server_addr.sin_port = htons(port);

        if (connect(fd, reinterpret_cast<struct sockaddr*>(&server_addr),
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
