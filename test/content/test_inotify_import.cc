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

#ifdef HAVE_INOTIFY
#include "content/inotify/mt_inotify.h"
#include <sys/inotify.h>
#endif

#include "metadata/metadata_enums.h"
#include "util/tools.h"

#include <gtest/gtest.h>
#include <unistd.h>

#if defined(HAVE_INOTIFY) && defined(__linux__)

class InotifyImportTest : public PlaylistImportTest,
                          public ::testing::WithParamInterface<std::string> {
protected:
    bool waitForMedia(const fs::path& path, const std::string& title)
    {
        return waitFor([&] {
            auto item = database->findObjectByPath(path, "", DbFileType::File);
            return item && item->getMetaData(MetadataFields::M_TITLE) == title;
        },
            "Media metadata at " + path.string());
    }

    bool drainEvents()
    {
        // This close follows earlier kernel events on the same instance.
        auto sentinel = source / ("sentinel-" + generateRandomId() + ".wav");
        writeWave(sentinel, "Event barrier");
        return waitForMedia(sentinel, "Event barrier")
            && waitFor([&] { return importsIdle(); }, "Queued reconciliation completion");
    }

    void replaceDirectoryWithTrack(const fs::path& track, const std::string& title)
    {
        auto replacement = root / "Replacement";
        auto replacementTrack = replacement / track.filename();
        writeWave(replacementTrack, title);
        fs::last_write_time(replacementTrack, fs::last_write_time(track));
        fs::remove_all(track.parent_path());
        fs::rename(replacement, track.parent_path());
    }
};

TEST_F(PlaylistImportTest, InotifyBatchesBelongToTheirInstance)
{
    fs::create_directory(source / "a");
    fs::create_directory(source / "b");
    Inotify first;
    Inotify second;
    ASSERT_GE(first.addWatch(source / "a", IN_CREATE), 0);
    ASSERT_GE(second.addWatch(source / "b", IN_CREATE), 0);
    writeBytes(source / "a/first", "one");
    writeBytes(source / "a/remaining", "two");
    writeBytes(source / "b/own", "three");
    first.stop();
    auto event = first.nextEvent();
    ASSERT_NE(event, nullptr);
    ASSERT_STREQ(event->name, "first");
    second.stop();
    event = second.nextEvent();
    ASSERT_NE(event, nullptr);
    ASSERT_STREQ(event->name, "own");
    first.stop();
    event = first.nextEvent();
    ASSERT_NE(event, nullptr);
    EXPECT_STREQ(event->name, "remaining");
}

TEST_F(PlaylistImportTest, NewInotifyInstanceDoesNotInheritDiscardedBatch)
{
    fs::create_directory(source / "old");
    fs::create_directory(source / "new");
    {
        Inotify old;
        ASSERT_GE(old.addWatch(source / "old", IN_CREATE), 0);
        writeBytes(source / "old/first", "one");
        writeBytes(source / "old/discarded", "two");
        old.stop();
        auto event = old.nextEvent();
        ASSERT_NE(event, nullptr);
        ASSERT_STREQ(event->name, "first");
    }
    Inotify current;
    ASSERT_GE(current.addWatch(source / "new", IN_CREATE), 0);
    writeBytes(source / "new/current", "three");
    current.stop();
    auto event = current.nextEvent();
    ASSERT_NE(event, nullptr);
    EXPECT_STREQ(event->name, "current");
}

TEST_F(PlaylistImportTest, KernelWatchIdsDistinguishReplacementFilesDirectoriesAndHardlinks)
{
    for (bool directory : { false, true }) {
        auto path = source / (directory ? "directory" : "file");
        if (directory)
            fs::create_directory(path);
        else
            writeBytes(path, "old");
        GrbFile oldHandle(path);
        ASSERT_NE(oldHandle.open("rb"), nullptr);
        Inotify notifications;
        auto oldWd = notifications.addWatch(path, IN_DELETE_SELF);
        ASSERT_GE(oldWd, 0);
        EXPECT_EQ(notifications.addWatch(path, IN_DELETE_SELF | IN_MASK_ADD), oldWd);
        if (!directory) {
            auto linked = source / "linked";
            fs::create_hard_link(path, linked);
            EXPECT_EQ(notifications.addWatch(linked, IN_DELETE_SELF | IN_MASK_ADD), oldWd);
            fs::remove(linked);
        }
        auto renamed = path;
        renamed += "-renamed";
        fs::rename(path, renamed);
        EXPECT_EQ(notifications.addWatch(renamed, IN_DELETE_SELF | IN_MASK_ADD), oldWd);
        fs::rename(renamed, path);
        fs::remove(path);
        if (directory)
            fs::create_directory(path);
        else
            writeBytes(path, "new");
        auto newWd = notifications.addWatch(path, IN_DELETE_SELF | IN_MASK_ADD);
        ASSERT_GE(newWd, 0);
        EXPECT_NE(oldWd, newWd);
        EXPECT_EQ(notifications.addWatch(path, IN_DELETE_SELF | IN_MASK_ADD), newWd);
        notifications.stop();
        EXPECT_EQ(notifications.nextEvent(), nullptr);
        ASSERT_TRUE(oldHandle.close());
        notifications.stop();
        auto event = notifications.nextEvent();
        ASSERT_NE(event, nullptr);
        EXPECT_EQ(event->wd, oldWd);
        EXPECT_EQ(event->mask & IN_DELETE_SELF, IN_DELETE_SELF);
        EXPECT_EQ(notifications.addWatch(path, IN_DELETE_SELF | IN_MASK_ADD), newWd);
    }
}

TEST_P(InotifyImportTest, DelayedOldDirectoryDeletionPreservesReplacementContents)
{
    auto album = source / "Album";
    auto disc = album / "Disc";
    auto track = disc / "track.wav";
    writeWave(track, "Old recording");
    start(GetParam(), "UTF-8", AutoscanScanMode::INotify);
    ASSERT_TRUE(waitForWatches({ source, album, disc }));
    ASSERT_TRUE(waitForMedia(track, "Old recording"));
    ASSERT_TRUE(waitFor([&] { return importsIdle(); }, "Initial import completion"));
    GrbFile oldDirectory(disc);
    GrbFile oldFile(track);
    ASSERT_NE(oldDirectory.open("rb"), nullptr);
    ASSERT_NE(oldFile.open("r+b"), nullptr);
    fs::remove_all(album);
    writeWave(track, "Replacement recording");
    ASSERT_TRUE(waitForWatches({ album, disc }));
    ASSERT_TRUE(waitForMedia(track, "Replacement recording"));
    ASSERT_TRUE(oldFile.close());
    ASSERT_TRUE(oldDirectory.close());
    ASSERT_TRUE(drainEvents());
    auto replacement = database->findObjectByPath(track, "", DbFileType::File);
    ASSERT_NE(replacement, nullptr);
    EXPECT_EQ(replacement->getMetaData(MetadataFields::M_TITLE), "Replacement recording");
    EXPECT_EQ(readBytes(track), waveBytes("Replacement recording"));
    auto renamed = source / "Renamed";
    fs::rename(album, renamed);
    ASSERT_TRUE(waitForMedia(renamed / "Disc/track.wav", "Replacement recording"));
    ASSERT_TRUE(drainEvents());
    EXPECT_EQ(database->findObjectByPath(track, "", DbFileType::File), nullptr);
    fs::remove_all(renamed);
    ASSERT_TRUE(drainEvents());
    EXPECT_EQ(database->findObjectByPath(track, "", DbFileType::File), nullptr);
    EXPECT_EQ(database->findObjectByPath(album, "", DbFileType::Directory), nullptr);
    EXPECT_EQ(database->findObjectByPath(renamed / "Disc/track.wav", "", DbFileType::File), nullptr);
    EXPECT_EQ(database->findObjectByPath(renamed, "", DbFileType::Directory), nullptr);
}

TEST_P(InotifyImportTest, RecreatedAutoscanRootRebuildsItsWatches)
{
    auto track = source / "track.wav";
    auto removedTrack = source / "removed.wav";
    writeWave(track, "Old root");
    writeWave(removedTrack, "Removed with old root");
    start(GetParam(), "UTF-8", AutoscanScanMode::INotify);
    ASSERT_TRUE(waitForWatches({ source }));
    ASSERT_TRUE(waitForMedia(track, "Old root"));
    ASSERT_TRUE(waitForMedia(removedTrack, "Removed with old root"));
    GrbFile oldRoot(source);
    GrbFile oldTrack(track);
    ASSERT_NE(oldRoot.open("rb"), nullptr);
    ASSERT_NE(oldTrack.open("rb"), nullptr);
    replaceDirectoryWithTrack(track, "Recreated root");
    ASSERT_TRUE(oldTrack.close());
    ASSERT_TRUE(oldRoot.close());
    ASSERT_TRUE(waitForMedia(track, "Recreated root"));
    ASSERT_TRUE(waitForWatches({ source, track }));
    ASSERT_TRUE(drainEvents());
    EXPECT_NE(database->findObjectByPath(track, "", DbFileType::File), nullptr);
    EXPECT_EQ(database->findObjectByPath(removedTrack, "", DbFileType::File), nullptr);
}

TEST_P(InotifyImportTest, DelayedOldFileDeletionPreservesReplacementRenameAndHardlink)
{
    auto track = source / "track.wav";
    writeWave(track, "Old file");
    start(GetParam(), "UTF-8", AutoscanScanMode::INotify);
    ASSERT_TRUE(waitForWatches({ source, track }));
    ASSERT_TRUE(waitForMedia(track, "Old file"));
    GrbFile old(track);
    ASSERT_NE(old.open("r+b"), nullptr);
    fs::remove(track);
    writeWave(track, "New file");
    ASSERT_TRUE(waitForWatches({ track }));
    ASSERT_TRUE(waitForMedia(track, "New file"));
    ASSERT_TRUE(old.close());
    ASSERT_TRUE(drainEvents());
    ASSERT_NE(database->findObjectByPath(track, "", DbFileType::File), nullptr);
    auto renamed = source / "renamed.wav";
    fs::rename(track, renamed);
    ASSERT_TRUE(waitForMedia(renamed, "New file"));
    ASSERT_TRUE(drainEvents());
    EXPECT_EQ(database->findObjectByPath(track, "", DbFileType::File), nullptr);
    auto linked = source / "linked.wav";
    fs::create_hard_link(renamed, linked);
    // Hard-link publication has no writer close: use music-import's completion signal.
    GrbFile linkHandle(linked);
    ASSERT_NE(linkHandle.open("r+b"), nullptr);
    ASSERT_TRUE(linkHandle.close());
    ASSERT_TRUE(waitForMedia(linked, "New file"));
    fs::remove(renamed);
    ASSERT_TRUE(drainEvents());
    EXPECT_EQ(database->findObjectByPath(renamed, "", DbFileType::File), nullptr);
    EXPECT_NE(database->findObjectByPath(linked, "", DbFileType::File), nullptr);
    fs::remove(linked);
    ASSERT_TRUE(drainEvents());
    EXPECT_EQ(database->findObjectByPath(linked, "", DbFileType::File), nullptr);
}

TEST_P(InotifyImportTest, NestedAutoscanCleanupPreservesParentAndReplacementWatches)
{
    auto nested = source / "Nested";
    auto track = nested / "track.wav";
    writeWave(track, "Old nested root");
    auto import = document.document_element().child("import");
    auto autoscan = import.child("autoscan");
    if (!autoscan)
        autoscan = import.append_child("autoscan");
    if (!autoscan.child("directory"))
        autoscan.append_child("directory");
    auto directory = autoscan.append_child("directory");
    directory.append_attribute("location") = nested.c_str();
    directory.append_attribute("mode") = "inotify";
    directory.append_attribute("recursive") = "yes";
    directory.append_attribute("import-mode") = GetParam().c_str();
    start(GetParam(), "UTF-8", AutoscanScanMode::INotify);
    ASSERT_TRUE(waitForWatches({ source, nested }));
    ASSERT_TRUE(waitForMedia(track, "Old nested root"));
    GrbFile old(nested);
    ASSERT_NE(old.open("rb"), nullptr);
    replaceDirectoryWithTrack(track, "New nested root");
    ASSERT_TRUE(waitForMedia(track, "New nested root"));
    ASSERT_TRUE(old.close());
    ASSERT_TRUE(drainEvents());
    ASSERT_TRUE(waitForWatches({ source, nested, track }));
    EXPECT_NE(database->findObjectByPath(track, "", DbFileType::File), nullptr);
}

TEST_P(InotifyImportTest, ReplacementTypeAndExcludedSymlinkAreReconciled)
{
    auto entry = source / "entry.wav";
    writeWave(entry, "Old file");
    auto import = document.document_element().child("import");
    auto follow = import.attribute("follow-symlinks");
    (follow ? follow : import.append_attribute("follow-symlinks")).set_value("no");
    start(GetParam(), "UTF-8", AutoscanScanMode::INotify);
    ASSERT_TRUE(waitForMedia(entry, "Old file"));
    GrbFile old(entry);
    ASSERT_NE(old.open("rb"), nullptr);
    fs::remove(entry);
    writeWave(entry / "child.wav", "Directory replacement");
    ASSERT_TRUE(waitForMedia(entry / "child.wav", "Directory replacement"));
    ASSERT_TRUE(old.close());
    ASSERT_TRUE(drainEvents());
    EXPECT_EQ(database->findObjectByPath(entry, "", DbFileType::File), nullptr);
    EXPECT_NE(database->findObjectByPath(entry, "", DbFileType::Directory), nullptr);
    fs::remove_all(entry);
    writeWave(entry, "Another file");
    ASSERT_TRUE(waitForMedia(entry, "Another file"));
    auto excluded = root / "excluded.wav";
    writeWave(excluded, "Outside watched tree");
    fs::remove(entry);
    fs::create_symlink(excluded, entry);
    ASSERT_TRUE(drainEvents());
    EXPECT_EQ(database->findObjectByPath(entry, "", DbFileType::File), nullptr);
}

TEST_P(InotifyImportTest, UnreadableSubdirectoryDoesNotPurgeIndexAndLaterEventsRecover)
{
    if (geteuid() == 0)
        GTEST_SKIP() << "Run this permission regression as a non-root user";
    auto album = source / "Album";
    auto track = album / "track.wav";
    writeWave(track, "Retained recording");
    start(GetParam(), "UTF-8", AutoscanScanMode::INotify);
    ASSERT_TRUE(waitForMedia(track, "Retained recording"));
    ASSERT_TRUE(waitFor([&] { return importsIdle(); }, "Initial import completion"));
    fs::permissions(album, fs::perms::none);
    auto adir = content->getAutoscanDirectory(source);
    ASSERT_NE(adir, nullptr);
    content->rescanDirectory(adir, adir->getObjectID(), source, false);
    bool completed = waitFor([&] { return importsIdle(); }, "Failed enumeration completion");
    fs::permissions(album, fs::perms::owner_all);
    ASSERT_TRUE(completed);
    auto retained = database->findObjectByPath(track, "", DbFileType::File);
    ASSERT_NE(retained, nullptr);
    EXPECT_EQ(retained->getMetaData(MetadataFields::M_TITLE), "Retained recording");
    writeWave(album / "later.wav", "After access error");
    ASSERT_TRUE(waitForMedia(album / "later.wav", "After access error"));
}

INSTANTIATE_TEST_SUITE_P(ImportModes, InotifyImportTest, ::testing::Values("mt", "grb"),
    [](const ::testing::TestParamInfo<std::string>& info) { return info.param; });

#endif
