/*GRB*

    Gerbera - https://gerbera.io/

    playlist_import_test.h - this file is part of Gerbera.

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

#ifndef GRB_PLAYLIST_IMPORT_TEST_H
#define GRB_PLAYLIST_IMPORT_TEST_H

#include "cds/cds_objects.h"
#include "config/result/autoscan.h"
#include "content/content.h"
#include "database/database.h"
#include "util/grb_fs.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <pugixml.hpp>
#include <string>
#include <vector>

class ConfigDefinition;
class ConfigManager;
class PlaylistImportServer;

// Real import, SQLite and resource delivery through the existing Server owner.
class PlaylistImportTest : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    void start(const std::string& importMode, const std::string& playlistCharset = "UTF-8",
        AutoscanScanMode scanMode = AutoscanScanMode::Timed);
    void stop();
    bool waitFor(const std::function<bool()>& predicate, const std::string& description,
        std::chrono::seconds timeout = std::chrono::seconds(30));
    bool waitForWatches(const std::vector<fs::path>& paths,
        std::chrono::seconds timeout = std::chrono::seconds(20));
    std::vector<std::shared_ptr<CdsObject>> children(int id) const;
    std::string resourceUrl(const std::shared_ptr<CdsObject>& item) const;
    std::string resourceBytes(const std::shared_ptr<CdsObject>& item) const;
    static std::string readBytes(const fs::path& path);
    static void writeBytes(const fs::path& path, const std::string& bytes);
    static std::string waveBytes(const std::string& title, std::size_t frames = 800);
    static void writeWave(const fs::path& path, const std::string& title, std::size_t frames = 800);

    fs::path root;
    fs::path source;
    pugi::xml_document document;
    std::shared_ptr<Database> database;
    std::shared_ptr<Content> content;

private:
    std::shared_ptr<ConfigDefinition> definition;
    std::shared_ptr<ConfigManager> config;
    std::shared_ptr<PlaylistImportServer> server;
};

#endif
