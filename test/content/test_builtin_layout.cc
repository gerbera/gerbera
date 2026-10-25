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

#include "../mock/config_mock.h"
#include "../mock/database_mock.h"
#include "cds/cds_item.h"
#include "cds/cds_objects.h"
#include "config/config_option_enum.h"
#include "config/config_setup.h"
#include "config/result/autoscan.h"
#include "config/result/box_layout.h"
#include "content/content_manager.h"
#include "content/layout/builtin_layout.h"
#include "context.h"
#include "metadata/metadata_enums.h"
#include "upnp/clients.h"
#include "upnp/upnp_common.h"
#include "util/string_converter.h"

#ifdef ONLINE_SERVICES
#include "content/onlineservice/online_service.h"
#endif

#ifdef HAVE_INOTIFY
#include "content/inotify/autoscan_inotify.h"
#ifdef HAVE_JS
#include "content/inotify/scripting_inotify.h"
#endif
#endif

#include <array>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

using TitleReplacementRules = std::vector<std::vector<std::pair<std::string, std::string>>>;

class BuiltinTitleConfig : public ConfigMock {
public:
    explicit BuiltinTitleConfig(TitleReplacementRules rules)
        : rules(std::move(rules))
        , boxes(std::make_shared<BoxLayoutList>())
    {
        for (auto key : std::array {
                 BoxKeys::videoRoot, BoxKeys::videoAll,
                 BoxKeys::imageRoot, BoxKeys::imageAll,
                 BoxKeys::audioRoot, BoxKeys::audioAll,
                 BoxKeys::audioAllTracks, BoxKeys::audioAllArtists,
                 BoxKeys::audioAllAlbums, BoxKeys::audioAllGenres,
                 BoxKeys::audioAllComposers, BoxKeys::audioAllYears,
                 BoxKeys::audioAllDirectories, BoxKeys::audioArtistChronology,
                 BoxKeys::audioAllSongs }) {
            auto box = std::make_shared<BoxLayout>(key, BoxLayout::getBoxKey(key));
            box->setEnabled(key == BoxKeys::audioAll || key == BoxKeys::audioAllTracks);
            boxes->EditHelperBoxLayout::add(box);
        }
    }

    std::string getOption(ConfigVal option) const override
    {
        if (option == ConfigVal::IMPORT_FILESYSTEM_CHARSET || option == ConfigVal::IMPORT_METADATA_CHARSET)
            return "UTF-8";
        return {};
    }

    std::shared_ptr<ConfigOption> getConfigOption(ConfigVal option) const override
    {
#ifdef HAVE_JS
        if (option == ConfigVal::IMPORT_SCRIPTING_SCAN_MODE)
            return std::make_shared<EnumOption<AutoscanScanMode>>(AutoscanScanMode::Manual, "manual");
#endif
        return ConfigMock::getConfigOption(option);
    }

    TitleReplacementRules getVectorOption(ConfigVal option) const override
    {
        if (option == ConfigVal::IMPORT_SCRIPTING_AUDIO_TITLE_REPLACEMENTS)
            return rules;
        return ConfigMock::getVectorOption(option);
    }

    std::shared_ptr<BoxLayoutList> getBoxLayoutListOption(ConfigVal) const override { return boxes; }

private:
    TitleReplacementRules rules;
    std::shared_ptr<BoxLayoutList> boxes;
};

class BuiltinTitleContent : public ContentManager {
public:
    explicit BuiltinTitleContent(const std::shared_ptr<Context>& context)
        : ContentManager(context, nullptr, nullptr)
    {
    }

    std::pair<int, bool> addContainerTree(const std::vector<std::shared_ptr<CdsObject>>&,
        const std::shared_ptr<CdsObject>&, int) override
    {
        constexpr bool isNewContainer = true;
        return { nextContainerId++, isNewContainer };
    }

    void addObject(const std::shared_ptr<CdsObject>& object, bool) override
    {
        object->setID(nextObjectId++);
        auto saved = CdsObject::createObject(object->getEntryType());
        object->copyTo(saved);
        added.push_back(std::move(saved));
    }

    std::vector<std::shared_ptr<CdsObject>> added;

private:
    int nextContainerId { 100 };
    int nextObjectId { 1000 };
};

class BuiltinTitleLayout : public BuiltinLayout {
public:
    using BuiltinLayout::BuiltinLayout;
    using BuiltinLayout::replaceAudioTitle;
};

class BuiltinAudioTitleTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        source = std::make_shared<CdsItem>(CdsEntryType::File);
        source->setID(42);
        source->setTitle("Physical file");
        source->setClass(UPNP_CLASS_MUSIC_TRACK);
        source->setMimeType("audio/mpeg");
        source->setLocation("/music/track.mp3", CdsEntryType::File);
        source->addMetaData(MetadataFields::M_TITLE, "Track");
        source->addMetaData(MetadataFields::M_ARTIST, "Artist");
        source->addMetaData(MetadataFields::M_ARTIST, "Guest");
        source->addMetaData(MetadataFields::M_ALBUMARTIST, "Various Artists");
        source->addMetaData(MetadataFields::M_ALBUM, "Album");
        source->addMetaData(MetadataFields::M_GENRE, "Techno");
        source->addMetaData(MetadataFields::M_GENRE, "Trance");
        source->addMetaData(MetadataFields::M_COMPOSER, "Composer");
        source->addMetaData(MetadataFields::M_DATE, "2026-01-01");
    }

    std::unique_ptr<BuiltinTitleLayout> makeLayout(TitleReplacementRules rules = {})
    {
        auto config = std::make_shared<BuiltinTitleConfig>(std::move(rules));
        auto database = std::make_shared<DatabaseMock>(config);
        auto converters = std::make_shared<ConverterManager>(config);
        auto context = std::make_shared<Context>(nullptr, config, nullptr, nullptr, database, nullptr, converters);
        content = std::make_shared<BuiltinTitleContent>(context);
        return std::make_unique<BuiltinTitleLayout>(content);
    }

    std::shared_ptr<CdsItem> source;
    std::shared_ptr<BuiltinTitleContent> content;
};

TEST_F(BuiltinAudioTitleTest, EmptyRulesKeepTitleAndMetadata)
{
    auto layout = makeLayout();
    const auto metadata = source->getMetaData();
    EXPECT_EQ(layout->replaceAudioTitle(*source, "Track"), "Track");
    EXPECT_EQ(source->getTitle(), "Physical file");
    EXPECT_EQ(source->getMetaData(), metadata);
}

TEST_F(BuiltinAudioTitleTest, LiteralRulesPreserveUnicodeAndTreatDollarAndBracketsLiterally)
{
    auto layout = makeLayout({
        { { "from", u8"\u00C4" }, { "to", u8"\u00D6" } },
        { { "from", "_" }, { "to", " " } },
        { { "from", "$1" }, { "to", u8"\u97F3\u697D" }, { "regex", "false" } },
        { { "from", "[x]" }, { "to", "literal" } },
    });
    EXPECT_EQ(layout->replaceAudioTitle(*source, u8"\u00C4ther_$1 [x]"), u8"\u00D6ther \u97F3\u697D literal");
}

TEST_F(BuiltinAudioTitleTest, RegexReplacesAllAsciiMatchesAndExpandsCapturesCaseSensitively)
{
    auto layout = makeLayout({ { { "from", "(A) (B)" }, { "to", "$2/$1" }, { "regex", "yes" } } });
    EXPECT_EQ(layout->replaceAudioTitle(*source, "A B a B A B"), "B/A a B B/A");
}

TEST_F(BuiltinAudioTitleTest, OrderedRulesRequireAllConditionsAndCanMatchAnyMetadataValue)
{
    auto layout = makeLayout({
        { { "from", "Track" }, { "to", "Pending" }, { "if-title", "Track" },
            { "if-artist", "Guest" }, { "if-albumartist", "Various Artists" },
            { "if-albumtitle", "Album" }, { "if-genre", "Trance" },
            { "if-composer", "Composer" }, { "if-year", "2026" } },
        { { "from", "Pending" }, { "to", "Final" } },
        { { "from", "Final" }, { "to", "wrong" }, { "if-artist", "guest" } },
    });
    const auto metadata = source->getMetaData();
    EXPECT_EQ(layout->replaceAudioTitle(*source, "Track"), "Final");
    EXPECT_EQ(source->getMetaData(), metadata);
}

TEST_F(BuiltinAudioTitleTest, ConditionsUseRawTitleAndDateYearAndRejectMissingMetadata)
{
    auto layout = makeLayout({
        { { "from", "Display" }, { "to", "Matched" }, { "if-title", "Track" }, { "if-year", "2026" } },
    });
    EXPECT_EQ(layout->replaceAudioTitle(*source, "Display"), "Matched");
    source->removeMetaData(MetadataFields::M_DATE);
    EXPECT_EQ(layout->replaceAudioTitle(*source, "Display"), "Display");
}

TEST_F(BuiltinAudioTitleTest, InvalidRegexDoesNotPreventLaterLiteralRules)
{
    auto layout = makeLayout({
        { { "from", "(" }, { "to", "bad" }, { "regex", "yes" } },
        { { "from", "_" }, { "to", " " } },
    });
    EXPECT_EQ(layout->replaceAudioTitle(*source, u8"Track_\u00C4ther"), u8"Track \u00C4ther");
}

TEST_F(BuiltinAudioTitleTest, RejectsBrokenUtf8CandidateBeforeApplyingTheNextRule)
{
    auto layout = makeLayout({
        { { "from", "^." }, { "to", "" }, { "regex", "yes" } },
        { { "from", u8"\u00C4ther" }, { "to", "Safe" } },
    });
    EXPECT_EQ(layout->replaceAudioTitle(*source, u8"\u00C4ther"), "Safe");
}

TEST_F(BuiltinAudioTitleTest, EmptyPatternsAreIgnoredAndEmptyFinalOutputKeepsOriginalTitle)
{
    auto layout = makeLayout({
        { { "from", "" }, { "to", "ignored" } },
        { { "from", "Track" }, { "to", "" } },
    });
    EXPECT_EQ(layout->replaceAudioTitle(*source, "Track"), "Track");
}

TEST_F(BuiltinAudioTitleTest, NativeRegexDollarZeroExpandsTheWholeMatch)
{
    auto layout = makeLayout({ { { "from", "(A)" }, { "to", "<$0>" }, { "regex", "true" } } });
    EXPECT_EQ(layout->replaceAudioTitle(*source, "XAY"), "X<A>Y");
}

TEST_F(BuiltinAudioTitleTest, NativeRegexMissingTenthCaptureExpandsToEmpty)
{
    auto layout = makeLayout({ { { "from", "(A)" }, { "to", "$10" }, { "regex", "yes" } } });
    EXPECT_EQ(layout->replaceAudioTitle(*source, "XAY"), "XY");
}

TEST_F(BuiltinAudioTitleTest, ProcessAudioFormatsEachViewOnceAndPreservesPhysicalMetadata)
{
    source->removeMetaData(MetadataFields::M_TITLE);
    source->addMetaData(MetadataFields::M_TITLE, "A");
    source->removeMetaData(MetadataFields::M_ARTIST);
    source->addMetaData(MetadataFields::M_ARTIST, "Performer");
    source->removeMetaData(MetadataFields::M_ALBUM);
    source->addMetaData(MetadataFields::M_ALBUM, "Record");
    const auto metadata = source->getMetaData();
    const auto entryType = source->getEntryType();
    auto layout = makeLayout({ { { "from", "A" }, { "to", "AA" } } });
    std::vector<int> previousReferences;
    layout->processCdsObject(source, nullptr, "/music", CONTENT_TYPE_MP3, {}, previousReferences);
    ASSERT_EQ(content->added.size(), 2U);
    EXPECT_EQ(content->added[0]->getTitle(), "AA");
    EXPECT_EQ(content->added[1]->getTitle(), "Performer - Record - AA");
    EXPECT_EQ(content->added[0]->getMetaData(MetadataFields::M_TITLE), "A");
    EXPECT_EQ(content->added[1]->getMetaData(MetadataFields::M_TITLE), "A");
    EXPECT_NE(content->added[0]->getParentID(), content->added[1]->getParentID());
    EXPECT_EQ(content->added[0]->getRefID(), 42);
    EXPECT_EQ(content->added[1]->getRefID(), 42);
    EXPECT_EQ(source->getMetaData(), metadata);
    EXPECT_EQ(source->getTitle(), "Physical file");
    EXPECT_EQ(source->getID(), 42);
    EXPECT_EQ(source->getEntryType(), entryType);
}

TEST_F(BuiltinAudioTitleTest, AllTracksReplacementsSeeTheCompletedPrefixAndKeepPhysicalMetadata)
{
    auto layout = makeLayout({ { { "from", "Artist - Album - " }, { "to", "" } } });
    source->removeMetaData(MetadataFields::M_ARTIST);
    source->addMetaData(MetadataFields::M_ARTIST, "Artist");
    const auto metadata = source->getMetaData();
    std::vector<int> previousReferences;
    layout->processCdsObject(source, nullptr, "/music", CONTENT_TYPE_MP3, {}, previousReferences);
    ASSERT_EQ(content->added.size(), 2U);
    EXPECT_EQ(content->added[0]->getTitle(), "Track");
    EXPECT_EQ(content->added[1]->getTitle(), "Track");
    EXPECT_EQ(source->getMetaData(), metadata);
    EXPECT_EQ(source->getTitle(), "Physical file");
}

} // namespace
