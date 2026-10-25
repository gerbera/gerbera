/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    Gerbera is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Gerbera is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gerbera. If not, see <http://www.gnu.org/licenses/>.
*/

#include "playlist_import_test.h"

#include "cds/cds_item.h"
#include "content/content.h"
#include "database/database.h"
#include "metadata/metadata_enums.h"
#include "upnp/upnp_common.h"

#include <fstream>
#include <gtest/gtest.h>
#include <json/json.h>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#ifdef HAVE_JS

enum class AsxResourceChecks { Database,
    Http };

class AsxImportTest : public PlaylistImportTest,
                      public ::testing::WithParamInterface<std::tuple<std::string, std::string>> {
protected:
    Json::Value manifest;
    std::map<std::string, std::vector<std::string>> expected;
    std::map<std::string, std::map<std::string, fs::path>> mediaFiles;
    std::map<fs::path, std::string> sourceBytes;
    std::map<fs::path, std::string> documents;
    std::map<std::string, std::pair<std::string, std::string>> external;
    std::string lastMismatch;

    static void replaceText(std::string& data, std::string from, std::string to)
    {
        // Preserve the corpus's UTF-16 byte order and BOM while replacing an
        // ASCII fixture path; XML decoding remains the actual parser's job.
        if (data.size() >= 2 && ((static_cast<unsigned char>(data[0]) == 0xff && static_cast<unsigned char>(data[1]) == 0xfe) || (static_cast<unsigned char>(data[0]) == 0xfe && static_cast<unsigned char>(data[1]) == 0xff))) {
            const bool littleEndian = static_cast<unsigned char>(data[0]) == 0xff;
            auto encode = [littleEndian](const std::string& text) {
                std::string result;
                for (char value : text) {
                    result += littleEndian ? value : '\0';
                    result += littleEndian ? '\0' : value;
                }
                return result;
            };
            from = encode(from);
            to = encode(to);
        }
        std::size_t position = 0;
        while ((position = data.find(from, position)) != std::string::npos) {
            data.replace(position, from.size(), to);
            position += to.size();
        }
    }

    void prepareCorpus(const std::string& name)
    {
        const fs::path corpus = fs::path(GERBERA_SOURCE_DIR) / "test/scripting/fixtures" / name;
        std::ifstream input(corpus / "cases.json");
        if (!input)
            throw std::runtime_error("Cannot read ASX corpus manifest");
        input >> manifest;
        for (const auto& test : manifest["cases"]) {
            const auto id = test["id"].asString();
            const auto directory = source / id;
            std::set<std::string> absent;
            for (const auto& key : test["absent_media"])
                absent.insert(key.asString());
            for (const auto& key : manifest["media"].getMemberNames()) {
                const auto& media = manifest["media"][key];
                if (!media.isMember("path") || absent.count(key))
                    continue;
                auto path = directory / media["path"].asString();
                writeBytes(path, key == "V" ? readBytes(fs::path(GERBERA_SOURCE_DIR) / "test/scripting/fixtures/xml-playlists/video.mp4") : waveBytes(media["title"].asString()));
                mediaFiles[id][key] = path;
            }
            auto data = readBytes(corpus / test["file"].asString());
            replaceText(data, "/fixture/asx", directory.string());
            documents[directory / test["file"].asString()] = data;
            for (const auto& key : test["expected"])
                expected[id].push_back(key.asString());
            expected.try_emplace(id);
        }
    }

    void addDocuments()
    {
        for (const auto& [path, bytes] : documents)
            writeBytes(path, bytes);
        rememberSources();
    }

    void rememberSources()
    {
        sourceBytes = readSources();
    }

    std::map<fs::path, std::string> readSources() const
    {
        std::map<fs::path, std::string> result;
        for (const auto& entry : fs::recursive_directory_iterator(source)) {
            if (entry.is_regular_file())
                result[entry.path()] = readBytes(entry.path());
        }
        return result;
    }

    std::map<std::string, std::vector<std::shared_ptr<CdsObject>>> playlistViews()
    {
        std::map<std::string, std::vector<std::shared_ptr<CdsObject>>> result;
        std::vector<int> pending { 0 };
        std::set<int> visited;
        while (!pending.empty()) {
            const auto parent = pending.back();
            pending.pop_back();
            if (!visited.insert(parent).second)
                continue;
            for (const auto& object : children(parent)) {
                if (!object->isContainer())
                    continue;
                if (object->getClass() == UPNP_CLASS_PLAYLIST_CONTAINER)
                    result[object->getTitle()].push_back(object);
                else
                    pending.push_back(object->getID());
            }
        }
        return result;
    }

    std::string expectedTitle(const std::string& id, const std::string& key, std::size_t) const
    {
        if (key == "EXTERNAL")
            return external.at(id).second;
        if (key == "LEGACY")
            return "Ã„ther";
        return key == "V" ? "v" : manifest["media"][key]["title"].asString();
    }

    bool viewsMatch()
    {
        const auto views = playlistViews();
        for (const auto& [title, containers] : views) {
            if (!expected.count(title)) {
                lastMismatch = "Unexpected playlist view: " + title;
                return false;
            }
        }
        for (const auto& [id, keys] : expected) {
            const auto found = views.find(id);
            const auto count = found == views.end() ? 0 : found->second.size();
            if (count != 2 && !(keys.empty() && !count)) {
                lastMismatch = id + ": expected two views, found " + std::to_string(count);
                return false;
            }
            if (found == views.end())
                continue;
            for (const auto& view : found->second) {
                auto items = children(view->getID());
                if (items.size() != keys.size()) {
                    lastMismatch = id + ": expected " + std::to_string(keys.size()) + " positions, found " + std::to_string(items.size());
                    return false;
                }
                for (std::size_t n = 0; n < keys.size(); ++n) {
                    const auto& key = keys[n];
                    lastMismatch = id + ": " + key + " at position " + std::to_string(n);
                    const auto& item = items[n];
                    if (key == "HTTPS") {
                        if (item->getLocation().string() != manifest["media"][key]["uri"].asString()) {
                            lastMismatch += ": expected external URI '" + manifest["media"][key]["uri"].asString() + "', got '" + item->getLocation().string() + "'";
                            return false;
                        }
                        continue;
                    }
                    const auto location = key == "EXTERNAL" ? external.at(id).first : key == "LEGACY" ? "https://media.example.invalid/legacy.mp3"
                                                                                                      : mediaFiles.at(id).at(key).string();
                    if (item->getLocation().string() != location) {
                        lastMismatch += ": expected location '" + location + "', got '" + item->getLocation().string() + "'";
                        return false;
                    }
                    // These are virtual playlist positions: the DIDL renderer
                    // emits getTitle(), including writeThrough overrides.
                    const auto title = expectedTitle(id, key, n);
                    if (item->getTitle() != title) {
                        lastMismatch += ": expected DIDL title '" + title + "', got '" + item->getTitle() + "' (embedded dc:title '" + item->getMetaData(MetadataFields::M_TITLE) + "')";
                        return false;
                    }
                    if (key != "EXTERNAL" && key != "LEGACY" && item->getClass().find(key == "V" ? "object.item.videoItem" : "object.item.audioItem") != 0) {
                        lastMismatch += ": wrong media class '" + item->getClass() + "'";
                        return false;
                    }
                }
            }
        }
        return true;
    }

    void verifyViews(const std::string& stage, AsxResourceChecks resources)
    {
        SCOPED_TRACE(stage);
        ASSERT_TRUE(waitFor([&] { return importsIdle() && viewsMatch(); }, stage)) << lastMismatch;
        for (const auto& [id, views] : playlistViews()) {
            SCOPED_TRACE(id);
            for (const auto& view : views) {
                const auto items = children(view->getID());
                for (std::size_t n = 0; n < items.size(); ++n) {
                    const auto& key = expected.at(id)[n];
                    SCOPED_TRACE("Position " + std::to_string(n) + ": " + key);
                    if (key == "HTTPS" || key == "EXTERNAL" || key == "LEGACY")
                        continue;
                    auto item = std::dynamic_pointer_cast<CdsItem>(items[n]);
                    ASSERT_NE(item, nullptr);
                    if (resources == AsxResourceChecks::Http) {
                        EXPECT_EQ(resourceBytes(item), readBytes(mediaFiles.at(id).at(key)));
                    }
                }
            }
        }
        EXPECT_EQ(readSources(), sourceBytes) << "Import and HTTP reads must not change, add or remove source files";
    }

    void rescan()
    {
        content->rescanDirectory(content->getAutoscanDirectory(source), content->ensurePathExistence(source));
    }

    std::string legacyBytes() const
    {
        auto title = std::get<1>(GetParam()) == "UTF-8"
            ? std::string("Ã„ther")
            : std::string("\xc3\x84ther");
        return "[playlist]\nFile1=https://media.example.invalid/legacy.mp3\nTitle1=" + title + "\n";
    }

    void exerciseUriCorpus(AsxResourceChecks resources)
    {
        prepareCorpus("asx-uris");
        for (const auto& [id, uri, title] : std::vector<std::tuple<std::string, std::string, std::string>> {
                 { "zutfxml", "https://media.example.invalid/xml.mp3", "Äther 東京" },
                 { "zmms", "mms://media.example.invalid/a.wmv?token=a+b", "Stream" },
                 { "zrtsp", "rtsp://media.example.invalid/a.wmv?token=Why%3F", "Stream" } }) {
            pugi::xml_document xml;
            auto entry = xml.append_child("ASX").append_child("ENTRY");
            entry.append_child("TITLE").text().set(title.c_str());
            entry.append_child("REF").append_attribute("HREF").set_value(uri.c_str());
            ASSERT_TRUE(xml.save_file((source / (id + ".asx")).c_str()));
            expected[id] = { "EXTERNAL" };
            external[id] = { uri, title };
        }
        writeBytes(source / "zlegacy.pls", legacyBytes());
        expected["zlegacy"] = { "LEGACY" };
        addDocuments();
        start(std::get<0>(GetParam()), std::get<1>(GetParam()));
        ASSERT_NO_FATAL_FAILURE(verifyViews("All39 URI documents, exact order and UTF-8", resources));
        writeBytes(source / "zafter.pls", legacyBytes());
        expected["zafter"] = { "LEGACY" };
        rememberSources();
        rescan();
        ASSERT_NO_FATAL_FAILURE(verifyViews("Rescan and later legacy converter reuse", resources));
        stop();
        start(std::get<0>(GetParam()), std::get<1>(GetParam()));
        ASSERT_NO_FATAL_FAILURE(verifyViews("Restart preserves both database views", resources));
    }
};

TEST_P(AsxImportTest, UriCorpusRescanAndRestartRetainDatabaseViews)
{
    ASSERT_NO_FATAL_FAILURE(exerciseUriCorpus(AsxResourceChecks::Database));
}

TEST_P(AsxImportTest, UriCorpusRescanAndRestartServeCompleteHttpResources)
{
#ifdef HAVE_CURL
    ASSERT_NO_FATAL_FAILURE(exerciseUriCorpus(AsxResourceChecks::Http));
#else
    GTEST_SKIP() << "HTTP byte validation requires CURL; the database test remains enabled";
#endif
}

INSTANTIATE_TEST_SUITE_P(ImportModesAndCharsets, AsxImportTest,
    ::testing::Combine(::testing::Values("mt", "grb"), ::testing::Values("UTF-8", "windows-1252")),
    [](const ::testing::TestParamInfo<AsxImportTest::ParamType>& info) {
        return std::get<0>(info.param) + (std::get<1>(info.param) == "UTF-8" ? "Utf8" : "Windows1252");
    });

#endif // HAVE_JS
