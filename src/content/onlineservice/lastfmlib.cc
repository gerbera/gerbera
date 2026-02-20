/*GRB*
    Gerbera - https://gerbera.io/

    lastfmlib.cc - this file is part of Gerbera.

    Copyright (C) 2025-2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
*/

/// @file content/onlineservice/lastfmlib.cc
/// @brief Implementation of the LastFmScrobbler class.

#ifdef HAVE_CURL
#define GRB_LOG_FAC GrbLogFacility::online

#include "lastfmlib.h" // API

#include "exceptions.h"
#include "util/logger.h"
#include "util/tools.h"

#include <curl/curl.h>
#include <fmt/format.h>
#if FMT_VERSION >= 100202
#include <fmt/ranges.h>
#endif
#include <pugixml.hpp>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    log_debug("got {}", *((std::string*)userp));
    return size * nmemb;
}

LastFmScrobbler::LastFmScrobbler(const std::string& apiKey, const std::string& apiSecret)
    : apiKey(apiKey)
    , apiSecret(apiSecret)
{
    stopRequested = true;
    if (scrobbleThread.joinable())
        scrobbleThread.join();
}

LastFmScrobbler::LastFmScrobbler(const std::string& apiKey, const std::string& apiSecret, const std::string& sessionKey)
    : apiKey(apiKey)
    , apiSecret(apiSecret)
    , sessionKey(sessionKey)
{
    stopRequested = true;
    if (scrobbleThread.joinable())
        scrobbleThread.join();
}

void LastFmScrobbler::authenticate()
{
    if (apiKey.empty())
        throw_std_runtime_error("last.fm apiKey not set");
    if (apiSecret.empty())
        throw_std_runtime_error("last.fm apiSecret not set");
    if (sessionKey.empty())
        throw_std_runtime_error("last.fm sessionKey not set");
}

std::string LastFmScrobbler::fetchAuthToken()
{
    std::vector<std::pair<std::string, std::string>> params = {
        { "api_key", apiKey },
        { "method", "auth.getToken" },
    };
    params.emplace_back("api_sig", buildApiSig(params));
    std::string response = getRequest(params);

    pugi::xml_document doc;
    if (doc.load_string(response.c_str())) {
        auto lfm = doc.child("lfm");
        std::string status = lfm.attribute("status").as_string();
        log_debug("status = {}", status);
        if (status == "failed") {
            auto error = lfm.child("error");
            log_error("Fetch Auth Token failed\ncode = {}\nmessage = {}", error.attribute("code").as_int(), error.text().as_string());
        } else {
            auto token = lfm.child("token");
            if (token)
                return token.child_value();
        }
    }
    return {};
}

std::string LastFmScrobbler::getAuthURL(const std::string& token) const
{
    return fmt::format("{}?api_key={}&token={}", authUrl, apiKey, token);
}

bool LastFmScrobbler::fetchSessionKey(const std::string& token)
{
    std::vector<std::pair<std::string, std::string>> params = {
        { "method", "auth.getSession" },
        { "api_key", apiKey },
        { "token", token },
    };
    params.emplace_back("api_sig", buildApiSig(params));
    std::string response = getRequest(params);

    pugi::xml_document doc;
    if (doc.load_string(response.c_str())) {
        auto lfm = doc.child("lfm");
        std::string status = lfm.attribute("status").as_string();
        log_debug("status = {}", status);
        if (status == "failed") {
            auto error = lfm.child("error");
            log_error("Fetch Session Key failed\ncode = {}\nmessage = {}", error.attribute("code").as_int(), error.text().as_string());
        } else {
            auto key = lfm.child("session").child("key");
            if (key) {
                sessionKey = key.child_value();
                return true;
            }
        }
    }
    return false;
}

bool LastFmScrobbler::updateNowPlaying(
    const std::string& artist,
    const std::string& track,
    const std::string& album,
    int duration)
{
    std::vector<std::pair<std::string, std::string>> params = {
        { "method", "track.updateNowPlaying" },
        { "api_key", apiKey },
        { "sk", sessionKey },
        { "artist", artist },
        { "track", track },
    };
    if (!album.empty())
        params.emplace_back("album", album);
    if (duration > 0)
        params.emplace_back("duration", std::to_string(duration));
    params.emplace_back("api_sig", buildApiSig(params));
    std::string response = postRequest(params);

    pugi::xml_document doc;
    if (doc.load_string(response.c_str())) {
        auto lfm = doc.child("lfm");
        std::string status = lfm.attribute("status").as_string();
        log_debug("status = {}", status);
        if (status == "failed") {
            auto error = lfm.child("error");
            log_error("Update Now Playing failed\ncode = {}\nmessage = {}", error.attribute("code").as_int(), error.text().as_string());
            return false;
        } else {
            return status == "ok";
        }
    }
    return false;
}

void LastFmScrobbler::startedPlaying(const SubmissionInfo& info)
{
    stopRequested = false;

    std::string artist = info.getArtist();
    std::string track = info.getTrack();
    std::string album = info.getAlbum();
    int duration = info.getTrackLength();

    std::time_t startTime = std::time(nullptr);

    updateNowPlaying(artist, track, album, duration);

    int scrobbleDelay = 30;
    if (duration > 0)
        scrobbleDelay = std::min(duration / 2, 30);

    if (scrobbleThread.joinable())
        scrobbleThread.join();

    scrobbleThread = std::thread([=]() {
        std::this_thread::sleep_for(std::chrono::seconds(scrobbleDelay));
        if (!stopRequested.load()) {
            scrobbleTrack(
                artist,
                track,
                startTime,
                album,
                duration);
        }
    });
    scrobbleThread.detach();
}

void LastFmScrobbler::finishedPlaying()
{
}

bool LastFmScrobbler::scrobbleTrack(
    const std::string& artist,
    const std::string& track,
    std::time_t timestamp,
    const std::string& album,
    int duration)
{
    std::vector<std::pair<std::string, std::string>> params = {
        { "method", "track.scrobble" },
        { "api_key", apiKey },
        { "sk", sessionKey },
        { "artist", artist },
        { "track", track },
        { "timestamp", std::to_string(timestamp) },
    };
    if (!album.empty())
        params.emplace_back("album", album);
    if (duration > 0)
        params.emplace_back("duration", std::to_string(duration));
    params.emplace_back("api_sig", buildApiSig(params));
    std::string response = postRequest(params);

    pugi::xml_document doc;
    if (doc.load_string(response.c_str())) {
        auto lfm = doc.child("lfm");
        std::string status = lfm.attribute("status").as_string();
        log_debug("status = {}", status);
        if (status == "failed") {
            auto error = lfm.child("error");
            log_error("Scrobble failed\ncode = {}\nmessage = {}", error.attribute("code").as_int(), error.text().as_string());
            return false;
        } else {
            return status == "ok";
        }
    }
    return false;
}

std::string LastFmScrobbler::buildApiSig(const std::vector<std::pair<std::string, std::string>>& params) const
{
    std::map<std::string, std::string> sorted;
    for (const auto& [key, val] : params) {
        if (key == "format" || key == "api_sig")
            continue;
        sorted[key] = val;
    }
    std::string sig;
    for (const auto& [key, val] : sorted) {
        sig.append(fmt::format("{}{}", key, val));
    }
    sig.append(apiSecret);
    return hexStringMd5(sig);
}

std::string LastFmScrobbler::getRequest(const std::vector<std::pair<std::string, std::string>>& params) const
{
    CURL* curl = curl_easy_init();
    if (!curl) {
        log_error("curl error !");
        return "";
    }

    std::vector<std::string> postFields;
    for (auto&& [key, val] : params) {
        if (val.empty()) {
            postFields.emplace_back(key);
        } else {
            char* escaped = curl_easy_escape(curl, val.c_str(), 0);
            postFields.emplace_back(fmt::format("{}={}", key, escaped ? escaped : ""));
            if (escaped)
                curl_free(escaped);
        }
    }

    std::string response;
    auto request = fmt::format("{}?{}", scrobbleUrl, fmt::join(postFields, "&"));
    log_debug("Posting {}", request);
    curl_easy_setopt(curl, CURLOPT_URL, request.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        log_error("curl error: {}", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    return response;
}

std::string LastFmScrobbler::postRequest(const std::vector<std::pair<std::string, std::string>>& params) const
{
    CURL* curl = curl_easy_init();
    if (!curl) {
        log_error("curl error !");
        return "";
    }

    std::vector<std::string> postFields;
    for (auto&& [key, val] : params) {
        if (val.empty()) {
            postFields.emplace_back(key);
        } else {
            char* escaped = curl_easy_escape(curl, val.c_str(), 0);
            postFields.emplace_back(fmt::format("{}={}", key, escaped ? escaped : ""));
            if (escaped)
                curl_free(escaped);
        }
    }

    std::string response;
    auto postData = fmt::format("{}", fmt::join(postFields, "&"));
    curl_easy_setopt(curl, CURLOPT_URL, scrobbleUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    log_debug("Trying to scrobble with this post data: {}", postData);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        log_error("curl error: {}", curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    return response;
}

#endif // HAVE_CURL
