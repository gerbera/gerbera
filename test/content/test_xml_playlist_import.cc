/*GRB*

    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

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

#include "playlist_import_test.h"

#include "cds/cds_item.h"
#include "common.h"
#include "content/content.h"
#include "metadata/metadata_enums.h"
#include "upnp/upnp_common.h"
#include "util/tools.h"

#include <chrono>
#include <fstream>
#include <gtest/gtest.h>
#include <json/json.h>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

enum class XmlResourceValidation {
    Metadata,
    HttpBytes,
};

class XmlPlaylistImportTest : public PlaylistImportTest,
                              public ::testing::WithParamInterface<std::string> {
protected:
    std::map<std::string, fs::path> media;
    std::map<std::string, std::string> titles;
    std::map<std::string, std::vector<std::string>> expected;
    const std::map<std::string, std::string> remoteLocations {
        { "legacycp", "https://media.example.invalid/legacy.mp3" },
        { "w09", "https://media.example.invalid/a.mp3?x=1&y=2" },
        { "x17", "https://media.example.invalid/a.mp3?token=one%26two&x=1" },
        { "x19", "https://media.example.invalid/media/a.wav" },
        { "x23", "https://media.example.invalid/a.mp3?title=Why%3F&token=a+b" },
    };

    void prepareInputs()
    {
        const auto corpus = fs::path(XML_PLAYLIST_FIXTURES);
        Json::Value manifest;
        std::ifstream input(corpus / "cases.json");
        ASSERT_TRUE(input.good());
        input >> manifest;
        ASSERT_EQ(manifest["cases"].size(), 45);
        fs::create_directories(source / "pl");
        for (const auto& key : manifest["media"].getMemberNames()) {
            const auto path = source / manifest["media"][key].asString();
            fs::create_directories(path.parent_path());
            if (key == "V") {
                fs::copy_file(corpus / "video.mp4", path);
                titles[key] = "v";
            } else {
                titles[key] = key == "U" ? "Äther 東京" : "Track " + key;
                writeWave(path, titles[key]);
            }
            media[key] = path;
        }
        for (const auto& entry : manifest["cases"]) {
            const fs::path name = entry["file"].asString();
            auto bytes = readBytes(corpus / name);
            // The UTF-16 fixture contains relative paths only; preserve its BOM
            // and declaration byte for byte while adapting absolute UTF-8 paths.
            replaceAllString(bytes, "/fixture/xml", source.string());
            writeBytes(source / "pl" / name, bytes);
            auto& positions = expected[name.stem().string()];
            if (entry["outcome"].asString() == "import") {
                for (const auto& key : entry["expected"])
                    positions.push_back(key.asString());
            }
        }
        for (const auto& [from, to] : std::vector<std::pair<const char*, const char*>> {
                 { "w01.wpl", "double.wpl.wpl" }, { "x01.xspf", "double.xspf.xspf" } }) {
            fs::copy_file(source / "pl" / from, source / "pl" / to);
            expected[fs::path(to).stem().string()] = expected.at(fs::path(from).stem().string());
        }
        const std::vector<std::string> order { "A", "V", "B", "V", "A" };
        for (const auto* extension : { "m3u", "m3u8", "pls", "asx" }) {
            const auto name = std::string("legacy") + extension;
            const auto kind = std::string(extension);
            std::string bytes = kind == "pls" ? "[playlist]\n" : kind == "asx" ? "<asx version=\"3.0\">"
                                                                               : "";
            for (std::size_t position = 0; position < order.size(); ++position) {
                const auto path = "../" + manifest["media"][order[position]].asString();
                if (kind == "asx")
                    bytes += "<entry><ref href=\"" + path + "\"/></entry>";
                else if (kind == "pls")
                    bytes += "File" + std::to_string(position + 1) + "=" + path + "\n";
                else
                    bytes += path + "\n";
            }
            if (kind == "asx")
                bytes += "</asx>";
            writeBytes(source / "pl" / (name + "." + extension), bytes);
            expected[name] = order;
        }
        writeBytes(source / "pl/legacycp.m3u", "#EXTINF:-1,\xC4"
                                               "ther\nhttps://media.example.invalid/legacy.mp3\n");
        expected["legacycp"] = { "HTTPS" };
        auto mappings = document.select_node("/config/import/mappings/extension-mimetype").node();
        ASSERT_TRUE(mappings);
        auto ignoreUnknown = mappings.attribute("ignore-unknown");
        (ignoreUnknown ? ignoreUnknown : mappings.append_attribute("ignore-unknown")) = "no";
        auto asx = mappings.find_child_by_attribute("map", "from", "asx");
        if (!asx) {
            asx = mappings.append_child("map");
            asx.append_attribute("from") = "asx";
            asx.append_attribute("to");
        }
        asx.attribute("to") = MIME_TYPE_ASX_PLAYLIST;
    }

    ::testing::AssertionResult viewsMatch(XmlResourceValidation validation)
    {
        std::vector<int> pending;
        for (const auto& object : children(CDS_ID_ROOT)) {
            if (object->getTitle() == "Playlists")
                pending.push_back(object->getID());
        }
        if (pending.size() != 1)
            return ::testing::AssertionFailure() << "Expected one Playlists root, found " << pending.size();
        std::map<std::string, std::vector<int>> found;
        while (!pending.empty()) {
            const auto id = pending.back();
            pending.pop_back();
            for (const auto& object : children(id)) {
                if (object->getClass() == UPNP_CLASS_PLAYLIST_CONTAINER)
                    found[object->getTitle()].push_back(object->getID());
                else if (object->isContainer())
                    pending.push_back(object->getID());
            }
        }
        for (const auto& [name, views] : found) {
            if (expected.find(name) == expected.end())
                return ::testing::AssertionFailure() << "Unexpected playlist container: " << name;
        }
        for (const auto& [name, positions] : expected) {
            auto views = found[name];
            if (views.size() != 2 && !(positions.empty() && views.empty()))
                return ::testing::AssertionFailure() << name << ": expected two views (or no views for empty input), got " << views.size();
            for (int id : views) {
                auto items = children(id);
                if (items.size() != positions.size())
                    return ::testing::AssertionFailure() << name << ": expected " << positions.size() << " positions, got " << items.size();
                for (std::size_t position = 0; position < positions.size(); ++position) {
                    const auto& key = positions[position];
                    auto item = std::dynamic_pointer_cast<CdsItem>(items[position]);
                    if (!item)
                        return ::testing::AssertionFailure() << name << ": position " << position + 1 << " is not an item";
                    if (key == "HTTPS" || key == "RELATIVE_HTTPS") {
                        auto remote = remoteLocations.find(name);
                        if (remote == remoteLocations.end() || resourceUrl(item) != remote->second)
                            return ::testing::AssertionFailure() << name << ": external resource URL changed at position " << position + 1;
                        if (name == "legacycp" && item->getTitle() != "Äther")
                            return ::testing::AssertionFailure() << "Legacy windows-1252 title was not restored after XML import";
                        if (name == "x23" && item->getTitle() != "Why? Äther 東京")
                            return ::testing::AssertionFailure() << "XML Unicode external title changed";
                        continue; // External fixture addresses must never be fetched.
                    }
                    if (item->getTitle() != titles.at(key))
                        return ::testing::AssertionFailure() << name << ": title at position " << position + 1 << " is " << item->getTitle() << ", expected " << titles.at(key);
                    const auto expectedClass = key == "V" ? UPNP_CLASS_VIDEO_ITEM : UPNP_CLASS_AUDIO_ITEM;
                    if (!startswith(item->getClass(), expectedClass))
                        return ::testing::AssertionFailure() << name << ": wrong media class at position " << position + 1 << ": " << item->getClass();
                    if (item->getLocation() != media.at(key))
                        return ::testing::AssertionFailure() << name << ": wrong media path at position " << position + 1;
#ifdef HAVE_CURL
                    if (validation == XmlResourceValidation::HttpBytes && resourceBytes(item) != readBytes(media.at(key)))
                        return ::testing::AssertionFailure() << name << ": full HTTP media bytes differ at position " << position + 1;
#else
                    static_cast<void>(validation);
#endif
                }
            }
        }
        return ::testing::AssertionSuccess();
    }

    void requireViews(const std::string& stage, XmlResourceValidation validation)
    {
        SCOPED_TRACE(stage);
        ASSERT_TRUE(waitFor([this, validation] {
            // An unchanged old view can match while its replacement is queued.
            // Finish that import before HTTP requests can mark those items played.
            if (!importsIdle())
                return false;
            const auto result = viewsMatch(validation);
            if (!result)
                throw std::runtime_error(result.message());
            return true;
        },
            stage, std::chrono::seconds(30)));
        ASSERT_TRUE(viewsMatch(validation));
    }

    void rescan()
    {
        auto directory = content->getAutoscanDirectory(source);
        ASSERT_NE(directory, nullptr);
        content->rescanDirectory(directory, directory->getObjectID());
    }

    void runLifecycle(XmlResourceValidation validation)
    {
        ASSERT_NO_FATAL_FAILURE(prepareInputs());
        start(GetParam(), "windows-1252");
        ASSERT_NO_FATAL_FAILURE(requireViews("45 XML cases, four legacy formats, double suffixes and charset recovery", validation));
        // Existing import timestamps have whole-second precision. Keep the
        // former fixture's mtime transition before rewriting the playlists.
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        for (const auto* name : { "w01.wpl", "x01.xspf", "double.wpl.wpl", "double.xspf.xspf" }) {
            const auto path = source / "pl" / name;
            writeBytes(path, readBytes(path) + "\n");
        }
        ASSERT_NO_FATAL_FAILURE(rescan());
        ASSERT_NO_FATAL_FAILURE(requireViews("playlist source refresh preserves names and all sequences", validation));
        ASSERT_NO_FATAL_FAILURE(rescan());
        ASSERT_NO_FATAL_FAILURE(requireViews("unchanged rescan preserves all sequences", validation));
        ASSERT_NO_FATAL_FAILURE(rescan());
        ASSERT_NO_FATAL_FAILURE(requireViews("second unchanged rescan is idempotent", validation));
        stop();
        start(GetParam(), "windows-1252");
        ASSERT_NO_FATAL_FAILURE(requireViews("restart retains all playlist contents", validation));
    }
};

TEST_P(XmlPlaylistImportTest, RealImportRefreshRescansAndRestartPreserveEveryCorpusPosition)
{
    runLifecycle(XmlResourceValidation::Metadata);
}

TEST_P(XmlPlaylistImportTest, HttpResourcesRemainCompleteAcrossRefreshRescansAndRestart)
{
#ifdef HAVE_CURL
    runLifecycle(XmlResourceValidation::HttpBytes);
#else
    GTEST_SKIP() << "HTTP verification requires existing CurlIOHandler; real database/import lifecycle test remains enabled";
#endif
}

INSTANTIATE_TEST_SUITE_P(ImportModes, XmlPlaylistImportTest, ::testing::Values("mt", "grb"),
    [](const ::testing::TestParamInfo<std::string>& info) { return info.param; });
