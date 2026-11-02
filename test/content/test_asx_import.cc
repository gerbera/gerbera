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
#ifdef HAVE_JS

#include "playlist_import_test.h"

#include "cds/cds_item.h"
#include "cds/cds_resource.h"
#include "content/content.h"
#include "database/database.h"
#include "metadata/metadata_enums.h"
#include "upnp/upnp_common.h"
#include "util/grb_time.h"
#include "util/tools.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <gtest/gtest.h>
#include <json/json.h>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

enum class AsxResourceChecks {
    Database,
    Http
};

class AsxRemoteTrap {
public:
    AsxRemoteTrap()
    {
        listener = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (listener < 0 || ::bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0
            || ::listen(listener, 8) != 0) {
            if (listener >= 0)
                ::close(listener);
            throw std::runtime_error("Cannot bind the isolated ASX remote-request trap");
        }
        socklen_t length = sizeof(address);
        if (::getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length) != 0) {
            ::close(listener);
            throw std::runtime_error("Cannot obtain the ASX request-trap port");
        }
        location = fmt::format("http://127.0.0.1:{}", ntohs(address.sin_port));
        worker = std::thread([this] {
            for (;;) {
                fd_set readable;
                FD_ZERO(&readable);
                FD_SET(listener, &readable);
                timeval timeout { 0, 100000 };
                if (::select(listener + 1, &readable, nullptr, nullptr, &timeout) > 0) {
                    const auto client = ::accept(listener, nullptr, nullptr);
                    if (client >= 0) {
                        ++requests;
                        ::close(client);
                    }
                } else if (stopping) {
                    break;
                }
            }
        });
    }

    ~AsxRemoteTrap()
    {
        finish();
        ::close(listener);
    }

    const std::string& baseUrl() const { return location; }
    unsigned int finish()
    {
        stopping = true;
        if (worker.joinable())
            worker.join();
        return requests;
    }

private:
    int listener { -1 };
    std::string location;
    std::atomic<bool> stopping {};
    std::atomic<unsigned int> requests {};
    std::thread worker;
};

class AsxImportTest : public PlaylistImportTest,
                      public ::testing::WithParamInterface<std::tuple<std::string, std::string>> {
protected:
    Json::Value manifest;
    std::map<std::string, std::vector<std::string>> expected;
    std::map<std::string, std::map<std::string, fs::path>> mediaFiles;
    std::map<fs::path, std::string> sourceBytes;
    std::map<fs::path, std::string> documents;
    std::map<std::string, std::pair<std::string, std::string>> external;
    std::map<std::pair<std::string, std::string>, std::string> titleOverrides;
    std::set<std::string> deleted;
    std::string corpusName;
    std::string lastMismatch;
    std::string remoteHost;

    static void replaceText(std::string& data, std::string from, std::string to)
    {
        // Preserve the corpus's UTF-16 byte order and BOM while replacing an
        // ASCII fixture path; XML decoding remains the actual parser's job.
        if (data.size() >= 2
            && ((static_cast<unsigned char>(data[0]) == 0xff && static_cast<unsigned char>(data[1]) == 0xfe)
                || (static_cast<unsigned char>(data[0]) == 0xfe && static_cast<unsigned char>(data[1]) == 0xff))) {
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
        replaceAllString(data, from, to);
    }

    void prepareCorpus(const std::string& name)
    {
        corpusName = name;
        const fs::path corpus = fs::path(GERBERA_SOURCE_DIR) / "test/scripting/fixtures" / name;
        std::ifstream input(corpus / "cases.json");
        if (!input)
            throw std::runtime_error("Cannot read ASX corpus manifest");
        input >> manifest;
        try {
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
                if (name == "asx-semantics") {
                    replaceText(data, "\"sub/", "\".sub/");
                    replaceText(data, "\"depth/", "\".depth/");
                    replaceText(data, "https://media.example.invalid/a.mp3?title=Why%3F&amp;token=a+b", fmt::format("{}/a.mp3?title=Why%3F&amp;token=a+b", remoteHost));
                    replaceText(data, "https://playlist.example.invalid/list.asx", fmt::format("{}/child.asx", remoteHost));
                    replaceText(data, "https://presentation.example.invalid/image", fmt::format("{}/image", remoteHost));
                    replaceText(data, "https://log.example.invalid/", fmt::format("{}/log", remoteHost));
                    for (const auto& child : manifest["dependencies"].getMemberNames()) {
                        auto contents = readBytes(corpus / child);
                        replaceText(contents, "\"sub/", "\".sub/");
                        replaceText(contents, "\"depth/", "\".depth/");
                        writeBytes(directory / ("." + child), contents);
                    }
                }
                documents[directory / test["file"].asString()] = data;
                for (const auto& key : test["expected"])
                    expected[id].push_back(key.asString());
                expected.try_emplace(id);
            }
            if (name == "asx-semantics")
                manifest["media"]["HTTPS"]["uri"] = fmt::format("{}/a.mp3?title=Why%3F&token=a+b", remoteHost);
        } catch (const std::exception& ex) {
            std::cerr << ex.what() << std::endl;
            throw;
        }
    }

    void addDocuments()
    {
        try {
            for (const auto& [path, bytes] : documents)
                writeBytes(path, bytes);
            rememberSources();
        } catch (const std::exception& ex) {
            std::cerr << ex.what() << std::endl;
            throw;
        }
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
                if (!object) {
                    std::cerr << "object null" << std::endl;
                    continue;
                }
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

    std::string expectedTitle(const std::string& id, const std::string& key, std::size_t position) const
    {
        if (key == "EXTERNAL")
            return external.at(id).second;
        if (key == "LEGACY")
            return corpusName == "asx-uris" ? "Ã„ther" : "Äther";
        auto override = titleOverrides.find({ id, key });
        if (override != titleOverrides.end())
            return override->second;
        if (corpusName == "asx-semantics" && id == "f35" && position == 0)
            return "Override";
        return key == "V" ? "v" : manifest["media"][key]["title"].asString();
    }

    bool viewsMatch()
    {
        const auto views = playlistViews();
        for (const auto& [title, containers] : views) {
            if (!expected.count(title)) {
                lastMismatch = fmt::format("Unexpected playlist view: {}", title);
                return false;
            }
        }
        for (const auto& [id, keys] : expected) {
            const auto found = views.find(id);
            const auto count = found == views.end() ? 0 : found->second.size();
            if ((deleted.count(id) && count) || (!deleted.count(id) && count != 2 && !(keys.empty() && !count))) {
                lastMismatch = fmt::format("{}: expected two views, found {}", id, count);
                return false;
            }
            if (found == views.end())
                continue;
            for (const auto& view : found->second) {
                if (!view)
                    continue;
                auto items = children(view->getID());
                if (items.size() != keys.size()) {
                    lastMismatch = fmt::format("{}: expected {} positions, found {}", id, keys.size(), items.size());
                    return false;
                }
                for (std::size_t n = 0; n < keys.size(); ++n) {
                    const auto& key = keys[n];
                    lastMismatch = fmt::format("{}: {} at position {}", id, key, n);
                    const auto& item = items[n];
                    if (!item)
                        continue;
                    if (key == "HTTPS") {
                        if (item->getLocation().string() != manifest["media"][key]["uri"].asString()) {
                            lastMismatch += fmt::format(": expected external URI '{}', got '{}#", manifest["media"][key]["uri"].asString(), item->getLocation().string());
                            return false;
                        }
                        continue;
                    }
                    const auto location = key == "EXTERNAL" ? external.at(id).first : key == "LEGACY" ? "https://media.example.invalid/legacy.mp3"
                                                                                                      : mediaFiles.at(id).at(key).string();
                    if (item->getLocation().string() != location) {
                        lastMismatch += fmt::format(": expected location '{}', got '{}'", location, item->getLocation().string());
                        return false;
                    }
                    // These are virtual playlist positions: the DIDL renderer
                    // emits getTitle(), including writeThrough overrides.
                    const auto title = expectedTitle(id, key, n);
                    if (item->getTitle() != title) {
                        lastMismatch += fmt::format(": expected DIDL title '{}', got '{}' (embedded dc:title '{}')", title, item->getTitle(), item->getMetaData(MetadataFields::M_TITLE));
                        return false;
                    }
                    if (key != "EXTERNAL" && key != "LEGACY" && item->getClass().find(key == "V" ? "object.item.videoItem" : "object.item.audioItem") != 0) {
                        lastMismatch += fmt::format(": wrong media class '{}'", item->getClass());
                        return false;
                    }
                    if (corpusName == "asx-semantics" && id == "f25") {
                        if (item->getMetaData(MetadataFields::M_AUTHOR) != (n == 0 ? "Entry author" : "Root author")
                            || item->getMetaData("dc:rights") != "Root rights"
                            || (n == 0 && item->getMetaData(MetadataFields::M_DESCRIPTION) != "Entry description")) {
                            lastMismatch += fmt::format(": scoped metadata mismatch: author='{}', rights='{}', description='{}'", item->getMetaData(MetadataFields::M_AUTHOR), item->getMetaData("dc:rights"), item->getMetaData(MetadataFields::M_DESCRIPTION));
                            return false;
                        }
                    }
                    if (corpusName == "asx-semantics" && id == "f27") {
                        if (item->getResources().empty()) {
                            lastMismatch += ": no resource available for duration validation";
                            return false;
                        }
                        const auto duration = item->getResources().front()->getAttribute(ResourceAttribute::DURATION);
                        if (duration.empty() || std::abs(HMSFToMilliseconds(duration) - 100) >= 10) {
                            lastMismatch += fmt::format(": expected original duration0.1s, got '{}'", duration);
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

    void verifyViews(const std::string& stage, AsxResourceChecks resources)
    {
        SCOPED_TRACE(stage);
        ASSERT_TRUE(waitFor([&] { return importsIdle() && viewsMatch(); }, stage, std::chrono::seconds(60))) << lastMismatch;
        for (const auto& [id, views] : playlistViews()) {
            SCOPED_TRACE(id);
            for (const auto& view : views) {
                ASSERT_NE(view, nullptr);
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
            ? (corpusName == "asx-uris" ? std::string("Ã„ther") : std::string("Äther"))
            : (corpusName == "asx-uris" ? std::string("\xc3\x84ther") : std::string("\xc4ther"));
        return fmt::format("[playlist]\nFile1=https://media.example.invalid/legacy.mp3\nTitle1={}\n", title);
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

    void exerciseSemanticsCorpus(AsxResourceChecks resources, AutoscanScanMode scanMode)
    {
        AsxRemoteTrap trap;
        remoteHost = trap.baseUrl();
        prepareCorpus("asx-semantics");
        const auto inotify = scanMode == AutoscanScanMode::INotify;
        if (inotify)
            addDocuments();
        start(std::get<0>(GetParam()), std::get<1>(GetParam()), scanMode);
        if (inotify) {
            ASSERT_TRUE(waitForWatches({ source, source / "f01", mediaFiles.at("f01").at("A").parent_path(), source / "f15" }));
        } else {
            ASSERT_TRUE(waitFor([&] {
                for (const auto& [id, files] : mediaFiles) {
                    for (const auto& [key, path] : files) {
                        if (!database->findObjectByPath(path, ""))
                            return false;
                    }
                }
                return true;
            },
                "Index physical media before structural ASX imports"));
            addDocuments();
            rescan();
        }
        ASSERT_NO_FATAL_FAILURE(verifyViews("All35 structural documents, exact order, scoped metadata and duration", resources));
        writeBytes(source / "zlegacy.pls", legacyBytes());
        expected["zlegacy"] = { "LEGACY" };
        rememberSources();
        rescan();
        ASSERT_NO_FATAL_FAILURE(verifyViews("Legacy import after XML restores configured charset", resources));
        if (!inotify) {
            rescan();
            ASSERT_NO_FATAL_FAILURE(verifyViews("Unchanged rescan retains all semantic positions", resources));
            stop();
            start(std::get<0>(GetParam()), std::get<1>(GetParam()));
            ASSERT_NO_FATAL_FAILURE(verifyViews("Restart retains all semantic positions", resources));
            stop();
            EXPECT_EQ(trap.finish(), 0U) << "ASX import must not probe remote media, children, banners or logging URLs";
            return;
        }
        const auto parent = source / "f01/f01.asx";
        writeBytes(parent, "<ASX><ENTRY><REF HREF=\"media/b.wav\"/></ENTRY><REPEAT COUNT=\"2\"><ENTRY><REF HREF=\"media/a.wav\"/></ENTRY></REPEAT></ASX>");
        expected["f01"] = { "B", "A", "A" };
        rememberSources();
        ASSERT_NO_FATAL_FAILURE(verifyViews("Inotify updates ordinary parent playlist edits", resources));
        const auto media = mediaFiles.at("f01").at("A");
        writeBytes(media, waveBytes("Changed source title"));
        titleOverrides[{ "f01", "A" }] = "Changed source title";
        rememberSources();
        ASSERT_NO_FATAL_FAILURE(verifyViews("Inotify media edits preserve duplicate playlist positions", resources));
        {
            std::fstream writable(media, std::ios::in | std::ios::out | std::ios::binary);
            ASSERT_TRUE(writable.good());
        }
        // Preserve the original unchanged-close observation interval.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ASSERT_NO_FATAL_FAILURE(verifyViews("Unchanged writable media close retains expanded positions", resources));
        writeBytes(source / "f15/.sub/child.asx", "<ASX><ENTRY><REF HREF=\"../media/c.wav\"/></ENTRY><ENTRY><REF HREF=\"../media/b.wav\"/></ENTRY></ASX>");
        expected["f15"] = { "A", "C", "B", "A" };
        // Parent-mtime invalidation is the existing supported child-refresh policy.
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        fs::last_write_time(source / "f15/f15.asx", fs::file_time_type::clock::now());
        rememberSources();
        rescan();
        ASSERT_NO_FATAL_FAILURE(verifyViews("Child edits refresh through parent modification and rescan", resources));
        stop();
        start(std::get<0>(GetParam()), std::get<1>(GetParam()), scanMode);
        ASSERT_TRUE(waitForWatches({ source, source / "f01", media.parent_path(), source / "f15" }));
        ASSERT_NO_FATAL_FAILURE(verifyViews("Restart retains expanded lists and metadata", resources));
        writeBytes(parent, "<ASX/>");
        expected["f01"].clear();
        rememberSources();
        rescan();
        ASSERT_NO_FATAL_FAILURE(verifyViews("Empty parent removes obsolete positions", resources));
        fs::remove(parent);
        deleted.insert("f01");
        rememberSources();
        rescan();
        ASSERT_NO_FATAL_FAILURE(verifyViews("Actual parent deletion leaves other playlists intact", resources));
        stop();
        EXPECT_EQ(trap.finish(), 0U) << "ASX import must not probe remote media, children, banners or logging URLs";
    }
};

TEST_P(AsxImportTest, UriCorpusRescanAndRestartRetainDatabaseViews)
{
    ASSERT_NO_FATAL_FAILURE(exerciseUriCorpus(AsxResourceChecks::Database));
}

#if defined(HAVE_CURL)
TEST_P(AsxImportTest, UriCorpusRescanAndRestartServeCompleteHttpResources)
{
    ASSERT_NO_FATAL_FAILURE(exerciseUriCorpus(AsxResourceChecks::Http));
}
#endif

TEST_P(AsxImportTest, SemanticsCorpusRescanAndRestartRetainDatabaseViews)
{
    ASSERT_NO_FATAL_FAILURE(exerciseSemanticsCorpus(AsxResourceChecks::Database, AutoscanScanMode::Timed));
}

#if defined(HAVE_CURL)
TEST_P(AsxImportTest, SemanticsCorpusRescanAndRestartServeCompleteHttpResources)
{
    ASSERT_NO_FATAL_FAILURE(exerciseSemanticsCorpus(AsxResourceChecks::Http, AutoscanScanMode::Timed));
}
#endif

#if defined(HAVE_INOTIFY) && defined(__linux__)
TEST_P(AsxImportTest, SemanticsCorpusInotifyMediaAndPlaylistUpdatesRetainDatabaseViews)
{
    ASSERT_NO_FATAL_FAILURE(exerciseSemanticsCorpus(AsxResourceChecks::Database, AutoscanScanMode::INotify));
}
#endif

#if defined(HAVE_CURL) && defined(HAVE_INOTIFY) && defined(__linux__)
TEST_P(AsxImportTest, SemanticsCorpusInotifyMediaAndPlaylistUpdatesServeCompleteHttpResources)
{
    ASSERT_NO_FATAL_FAILURE(exerciseSemanticsCorpus(AsxResourceChecks::Http, AutoscanScanMode::INotify));
}
#endif

INSTANTIATE_TEST_SUITE_P(ImportModesAndCharsets, AsxImportTest,
    ::testing::Combine(::testing::Values("mt", "grb"), ::testing::Values("UTF-8", "windows-1252")),
    [](const ::testing::TestParamInfo<AsxImportTest::ParamType>& info) {
        return fmt::format("{}_{}", std::get<0>(info.param), (std::get<1>(info.param) == "UTF-8" ? "Utf8" : "Windows1252"));
    });

#endif // HAVE_JS
