/*GRB*

    Gerbera - https://gerbera.io/

    playlist_import_test.cc - this file is part of Gerbera.

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
#include "config/config_definition.h"
#include "config/config_generator.h"
#include "config/config_manager.h"
#include "database/db_param.h"
#include "server.h"
#include "upnp/upnp_common.h"
#include "upnp/upnp_service.h"
#include "upnp/xml_builder.h"
#include "util/tools.h"

#ifdef HAVE_CURL
#include "iohandler/curl_io_handler.h"
#endif

#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <thread>
#include <utility>

#ifdef __linux__
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#endif

class PlaylistImportServer : public Server {
public:
    using Server::Server;

    std::shared_ptr<Database> getDatabase() const { return database; }

    std::string resourceUrl(const std::shared_ptr<CdsItem>& item) const
    {
        return upnpXmlBuilder->getFirstResourcePath(item);
    }

    void finish()
    {
        if (database)
            shutdown();
        else if (timer)
            timer->shutdown();
        // Server keeps itself alive for callbacks; release that reference last.
        self.reset();
    }
};

static pugi::xml_node fixtureValue(pugi::xml_document& document, const std::string& path, const std::string& value)
{
    auto node = document.document_element();
    for (const auto& part : splitString(path, '/')) {
        auto next = node.child(part.c_str());
        node = next ? next : node.append_child(part.c_str());
    }
    node.text().set(value.c_str());
    return node;
}

void PlaylistImportTest::SetUp()
{
    root = fs::temp_directory_path() / ("grb-playlist-" + generateRandomId());
    source = root / "src";
    fs::create_directories(source);
    fs::create_directory(root / "web");
    definition = std::make_shared<ConfigDefinition>();
    definition->init(definition);
    ConfigGenerator generator(definition, "playlist-import-test", ConfigLevel::Base);
    const auto xml = generator.generate(root, ".", root, "");
    if (!document.load_string(xml.c_str()))
        throw std::runtime_error("Could not load generated playlist test configuration");
    fixtureValue(document, "server/home", root.string());
    fixtureValue(document, "server/webroot", (root / "web").string());
    fixtureValue(document, "server/port", "0");
    fixtureValue(document, "server/bookmark", (root / "bookmark.html").string());
    auto storage = document.document_element().child("server").child("storage");
    storage.child("sqlite3").attribute("enabled").set_value("yes");
    for (const auto* other : { "mysql", "postgresql" }) {
        if (auto backend = storage.child(other))
            backend.attribute("enabled").set_value("no");
    }
    fixtureValue(document, "server/storage/sqlite3/database-file", (root / "db.sqlite").string());
    const auto sql = fs::path(GERBERA_SOURCE_DIR) / "src/database/sqlite3";
    fixtureValue(document, "server/storage/sqlite3/init-sql-file", (sql / "sqlite3.sql").string());
    fixtureValue(document, "server/storage/sqlite3/upgrade-file", (sql / "sqlite3-upgrade.xml").string());
    fixtureValue(document, "server/storage/sqlite3/drop-file", (sql / "sqlite3-drop.sql").string());
    fixtureValue(document, "server/storage/sqlite3/backup", "").attribute("enabled").set_value("no");
    fixtureValue(document, "server/ui", "").attribute("enabled").set_value("yes");
    fixtureValue(document, "server/ui/accounts", "").attribute("enabled").set_value("no");
    fixtureValue(document, "import/filesystem-charset", "UTF-8");
    fixtureValue(document, "import/scripting/script-folder/common", (fs::path(GERBERA_SOURCE_DIR) / "scripts/js").string());
    fixtureValue(document, "import/scripting/script-folder/custom", "");
}

void PlaylistImportTest::start(const std::string& importMode, const std::string& playlistCharset, AutoscanScanMode scanMode)
{
    if (server)
        throw std::runtime_error("Playlist test server is already running");
    fixtureValue(document, "import/playlist-charset", playlistCharset);
    auto autoscan = fixtureValue(document, "import/autoscan", "");
    auto useInotify = autoscan.attribute("use-inotify");
    (useInotify ? useInotify : autoscan.append_attribute("use-inotify")).set_value(scanMode == AutoscanScanMode::INotify ? "yes" : "no");
    auto inotifyAttributes = autoscan.attribute("inotify-attrib");
    (inotifyAttributes ? inotifyAttributes : autoscan.append_attribute("inotify-attrib")).set_value("no");
    auto directory = autoscan.child("directory");
    if (!directory)
        directory = autoscan.append_child("directory");
    for (auto attribute : { "location", "mode", "interval", "recursive", "import-mode", "hidden-files" })
        if (!directory.attribute(attribute))
            directory.append_attribute(attribute);
    directory.attribute("location") = source.c_str();
    directory.attribute("mode") = AutoscanDirectory::mapScanmode(scanMode);
    directory.attribute("interval") = "3600";
    directory.attribute("recursive") = "yes";
    directory.attribute("import-mode") = importMode.c_str();
    directory.attribute("hidden-files") = "no";
    const auto configFile = root / "config.xml";
    if (!document.save_file(configFile.c_str()))
        throw std::runtime_error("Could not write playlist test configuration");
    config = std::make_shared<ConfigManager>(definition, configFile, root, ".", root, false);
    config->load(root, "");
    server = std::make_shared<PlaylistImportServer>(config);
    server->init(definition, false, false, false);
    // This returns after the owned listener and its request callbacks are installed.
    server->run();
    database = server->getDatabase();
    content = server->getContent();
}

void PlaylistImportTest::stop()
{
    content.reset();
    database.reset();
    if (server) {
        server->finish();
        server.reset();
    }
    config.reset();
}

void PlaylistImportTest::TearDown()
{
    stop();
    if (HasFailure()) {
        std::cerr << "Playlist import failure workspace: " << root << '\n';
        return;
    }
    if (!root.empty())
        fs::remove_all(root);
}

bool PlaylistImportTest::waitFor(const std::function<bool()>& predicate, const std::string& description, std::chrono::seconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::string lastError;
    do {
        try {
            if (predicate())
                return true;
        } catch (const std::runtime_error& error) {
            lastError = error.what();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } while (std::chrono::steady_clock::now() < deadline);
    ADD_FAILURE() << description << " did not converge within " << timeout.count()
                  << " seconds; last error: " << lastError << "; workspace: " << root;
    return false;
}

std::vector<std::shared_ptr<CdsObject>> PlaylistImportTest::children(int id) const
{
    const auto parent = database->loadObject(id);
    unsigned int flags = BROWSE_DIRECT_CHILDREN | BROWSE_ITEMS | BROWSE_CONTAINERS;
    if (startswith(parent->getClass(), UPNP_CLASS_PLAYLIST_CONTAINER) || startswith(parent->getClass(), UPNP_CLASS_MUSIC_ALBUM))
        flags |= BROWSE_TRACK_SORT;
    BrowseParam query(parent, flags);
    return database->browse(query);
}

std::string PlaylistImportTest::resourceUrl(const std::shared_ptr<CdsObject>& object) const
{
    const auto item = std::dynamic_pointer_cast<CdsItem>(object);
    if (!item)
        throw std::runtime_error("A media item is required for HTTP resource validation");
    return server->resourceUrl(item);
}

std::string PlaylistImportTest::resourceBytes(const std::shared_ptr<CdsObject>& object) const
{
#ifdef HAVE_CURL
    CurlIOHandler reader(config, resourceUrl(object), 65536, 0,
        std::chrono::seconds(5), std::chrono::seconds(5));
    reader.open(UPNP_READ);
    std::string result;
    std::array<std::byte, 4096> buffer;
    for (;;) {
        const auto count = reader.read(buffer.data(), buffer.size());
        if (count < 0)
            throw std::runtime_error("HTTP media resource read failed");
        if (count == GRB_READ_END)
            return result;
        result.append(reinterpret_cast<const char*>(buffer.data()), count);
    }
#else
    static_cast<void>(object);
    throw std::runtime_error("The separately named HTTP resource test requires HAVE_CURL");
#endif
}

static void appendWaveNumber(std::string& bytes, std::uint32_t value, unsigned int width)
{
    for (unsigned int index = 0; index < width; ++index)
        bytes.push_back(static_cast<char>((value >> (8 * index)) & 0xff));
}

std::string PlaylistImportTest::waveBytes(const std::string& title, std::size_t frames)
{
    const auto titleSize = title.size() + 1;
    if (titleSize > std::numeric_limits<std::uint32_t>::max() - 64
        || frames > (std::numeric_limits<std::uint32_t>::max() - titleSize - 64) / 2)
        throw std::runtime_error("Test WAV is too large");
    const auto dataSize = static_cast<std::uint32_t>(frames * 2);
    const auto infoSize = static_cast<std::uint32_t>(12 + titleSize + (titleSize % 2));
    std::string bytes = "RIFF";
    appendWaveNumber(bytes, 36 + dataSize + 8 + infoSize, 4);
    bytes += "WAVEfmt ";
    appendWaveNumber(bytes, 16, 4);
    appendWaveNumber(bytes, 1, 2);
    appendWaveNumber(bytes, 1, 2);
    appendWaveNumber(bytes, 8000, 4);
    appendWaveNumber(bytes, 16000, 4);
    appendWaveNumber(bytes, 2, 2);
    appendWaveNumber(bytes, 16, 2);
    bytes += "data";
    appendWaveNumber(bytes, dataSize, 4);
    bytes.append(dataSize, '\0');
    bytes += "LIST";
    appendWaveNumber(bytes, infoSize, 4);
    bytes += "INFOINAM";
    appendWaveNumber(bytes, titleSize, 4);
    bytes += title;
    bytes.push_back('\0');
    if (titleSize % 2)
        bytes.push_back('\0');
    return bytes;
}

void PlaylistImportTest::writeWave(const fs::path& path, const std::string& title, std::size_t frames)
{
    writeBytes(path, waveBytes(title, frames));
}

std::string PlaylistImportTest::readBytes(const fs::path& path)
{
    auto bytes = GrbFile(path).readBinaryFile();
    if (!bytes)
        throw std::runtime_error("Could not read test file: " + path.string());
    return { reinterpret_cast<const char*>(bytes->data()), bytes->size() };
}

void PlaylistImportTest::writeBytes(const fs::path& path, const std::string& bytes)
{
    if (!path.parent_path().empty())
        fs::create_directories(path.parent_path());
    GrbFile(path).writeBinaryFile(reinterpret_cast<const std::byte*>(bytes.data()), bytes.size());
}

bool PlaylistImportTest::waitForWatches(const std::vector<fs::path>& paths, std::chrono::seconds timeout)
{
#ifdef __linux__
    constexpr auto requiredMask = IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO | IN_CREATE | IN_DELETE;
    std::set<std::pair<unsigned long long, unsigned long long>> expected;
    std::string description = "Kernel watches for";
    for (const auto& path : paths) {
        struct stat info { };
        if (stat(path.c_str(), &info) != 0)
            throw std::runtime_error("Cannot inspect watch directory: " + path.string());
        const auto device = (static_cast<unsigned long long>(major(info.st_dev)) << 20) | minor(info.st_dev);
        expected.emplace(device, info.st_ino);
        description += " " + path.string();
    }
    return waitFor([&] {
        auto missing = expected;
        for (const auto& descriptor : fs::directory_iterator("/proc/self/fdinfo")) {
            std::ifstream input(descriptor.path());
            for (std::string line; std::getline(input, line);) {
                unsigned long long inode, device, mask;
                if (std::sscanf(line.c_str(), "inotify wd:%*x ino:%llx sdev:%llx mask:%llx", &inode, &device, &mask) == 3
                    && (mask & requiredMask) == requiredMask)
                    missing.erase({ device, inode });
            }
        }
        return missing.empty();
    },
        description, timeout);
#else
    static_cast<void>(paths);
    static_cast<void>(timeout);
    ADD_FAILURE() << "Real kernel-watch validation requires Linux /proc/self/fdinfo";
    return false;
#endif
}
