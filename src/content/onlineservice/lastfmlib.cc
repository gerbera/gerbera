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

#include "lastfmlib.h" // API

#include "exceptions.h"
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
    return size * nmemb;
}

LastFmScrobbler::LastFmScrobbler(const std::string& apiKey, const std::string& apiSecret)
    : apiKey(apiKey)
    , apiSecret(apiSecret)
{
}

LastFmScrobbler::LastFmScrobbler(const std::string& apiKey, const std::string& apiSecret, const std::string& sessionKey)
    : apiKey(apiKey)
    , apiSecret(apiSecret)
    , sessionKey(sessionKey)
{
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
        { "method", "auth.getToken" },
        { "api_key", apiKey }
    };
    params.emplace_back("api_sig", buildApiSig(params));
    std::string response = postRequest(params);

    pugi::xml_document doc;
    if (doc.load_string(response.c_str())) {
        auto token = doc.child("lfm").child("token");
        if (token)
            return token.child_value();
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
        { "token", token }
    };
    params.emplace_back("api_sig", buildApiSig(params));
    std::string response = postRequest(params);

    pugi::xml_document doc;
    if (doc.load_string(response.c_str())) {
        auto key = doc.child("lfm").child("session").child("key");
        if (key) {
            sessionKey = key.child_value();
            return true;
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
        { "track", track }
    };
    if (!album.empty())
        params.emplace_back("album", album);
    if (duration > 0)
        params.emplace_back("duration", std::to_string(duration));
    params.emplace_back("api_sig", buildApiSig(params));
    std::string response = postRequest(params);

    pugi::xml_document doc;
    return doc.load_string(response.c_str()) && std::string(doc.child("lfm").attribute("status").value()) == "ok";
}

void LastFmScrobbler::startedPlaying(const SubmissionInfo& info)
{
    scrobbleTrack(
        info.getArtist(),
        info.getTrack(),
        0,
        info.getAlbum(),
        info.getTrackLength());
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
        { "timestamp", std::to_string(timestamp) }
    };
    if (!album.empty())
        params.emplace_back("album", album);
    if (duration > 0)
        params.emplace_back("duration", std::to_string(duration));
    params.emplace_back("api_sig", buildApiSig(params));
    std::string response = postRequest(params);

    pugi::xml_document doc;
    return doc.load_string(response.c_str()) && std::string(doc.child("lfm").attribute("status").value()) == "ok";
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
    return hexStringMd5(fmt::format("{}{}", sig, apiSecret));
}

std::string LastFmScrobbler::postRequest(const std::vector<std::pair<std::string, std::string>>& params) const
{
    std::vector<std::string> postFields;
    for (auto&& [key, val] : params) {
        postFields.emplace_back(fmt::format("{}={}", key, curl_easy_escape(nullptr, val.c_str(), 0)));
    }

    CURL* curl = curl_easy_init();
    std::string response;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, scrobbleUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fmt::format("{}", fmt::join(postFields, "&")).c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return response;
}

#endif // HAVE_CURL
