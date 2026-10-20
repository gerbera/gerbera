/*GRB*
    Gerbera - https://gerbera.io/

    Copyright (C) 2026 Gerbera Contributors

    This file is part of Gerbera, licensed under the GNU General Public
    License version 2 or later. See <http://www.gnu.org/licenses/>.
*/

#include "util/asx_playlist.h"

#include <gtest/gtest.h>
#include <stdexcept>

class AsxPlaylistTest : public ::testing::Test {
protected:
    pugi::xml_document document;
    std::map<fs::path, std::string> children;
    std::vector<fs::path> requested;

    AsxPlaylistPlan parse(const std::string& body)
    {
        auto result = document.load_string(("<ASX VERSION='3.0'>" + body + "</ASX>").c_str());
        if (!result)
            throw std::runtime_error(result.description());
        return readAsxPlaylistPlan(
            document.document_element(), "/asx/main.asx",
            [&](const auto& href, const auto& source) {
                auto path = (source.parent_path() / href).lexically_normal();
                requested.push_back(path);
                auto found = children.find(path);
                if (found == children.end())
                    return AsxPlaylistDocument { {}, {}, "Missing local ENTRYREF" };
                auto xml = std::make_unique<pugi::xml_document>();
                auto parsed = xml->load_string(found->second.c_str());
                if (!parsed)
                    throw std::runtime_error(parsed.description());
                return AsxPlaylistDocument { path, std::move(xml), {} };
            });
    }

    static std::string entry(const std::string& href)
    {
        return "<ENTRY><REF HREF='" + href + "'/></ENTRY>";
    }

    static std::vector<std::string> firstReferences(const AsxPlaylistPlan& plan)
    {
        std::vector<std::string> result;
        for (const auto& entry : plan.entries)
            result.push_back(entry.references.front().href);
        return result;
    }
};

TEST_F(AsxPlaylistTest, PreservesSourceOrderAndAlternativeGroups)
{
    auto plan = parse("<ENTRY><REF HREF='a.wav'/><REF HREF='b.wav' "
                      "WRITETHROUGH='1'/><REF HREF='a.wav'/></ENTRY>"
        + entry("c.wav") + entry("a.wav"));
    ASSERT_EQ(plan.entries.size(), 3);
    EXPECT_EQ(plan.entries[0].references.size(), 3);
    EXPECT_EQ(plan.entries[0].references[0].href, "a.wav");
    EXPECT_EQ(plan.entries[0].references[1].href, "b.wav");
    EXPECT_EQ(plan.entries[0].references[1].writeThrough, 1);
    EXPECT_EQ(plan.entries[0].references[2].href, "a.wav");
    EXPECT_EQ(plan.entries[0].source, "/asx/main.asx");
    EXPECT_EQ(firstReferences(plan),
        (std::vector<std::string> { "a.wav", "c.wav", "a.wav" }));
}

TEST_F(AsxPlaylistTest, RepeatsFiniteSequenceWithDuplicatePositions)
{
    auto plan = parse(entry("before.wav") + "<REPEAT COUNT='2'>" + entry("a.wav") + entry("b.wav") + "</REPEAT>" + entry("after.wav"));
    EXPECT_EQ(firstReferences(plan),
        (std::vector<std::string> { "before.wav", "a.wav", "b.wav", "a.wav",
            "b.wav", "after.wav" }));
}

TEST_F(AsxPlaylistTest, ZeroAndOneCountsBothKeepOnePass)
{
    for (auto count : { "0", "1" }) {
        SCOPED_TRACE(count);
        auto plan = parse(std::string("<REPEAT COUNT='") + count + "'>" + entry("a.wav") + "</REPEAT>");
        EXPECT_EQ(firstReferences(plan), (std::vector<std::string> { "a.wav" }));
        EXPECT_TRUE(plan.warnings.empty());
    }
}

TEST_F(AsxPlaylistTest, UnboundedRepeatKeepsOnePassAndReportsPolicy)
{
    auto plan = parse("<REPEAT>" + entry("a.wav") + "</REPEAT>" + entry("b.wav"));
    EXPECT_EQ(firstReferences(plan),
        (std::vector<std::string> { "a.wav", "b.wav" }));
    ASSERT_EQ(plan.warnings.size(), 1);
    EXPECT_NE(plan.warnings[0].find("unbounded REPEAT imported once"),
        std::string::npos);
}

TEST_F(AsxPlaylistTest, RejectsInvalidCountsBeforeReturningAnyPlan)
{
    for (auto count : { "-1", "two", "2.0", "", "+2", " 2", "10001",
             "184467440737095516160000" }) {
        SCOPED_TRACE(count);
        EXPECT_THROW(parse(entry("before.wav") + "<REPEAT COUNT='" + count + "'>" + entry("a.wav") + "</REPEAT>"),
            std::runtime_error);
    }
}

TEST_F(AsxPlaylistTest, BoundsCompleteExpandedSequence)
{
    auto boundary = "<REPEAT COUNT='10000'>" + entry("a.wav") + "</REPEAT>";
    EXPECT_EQ(parse(boundary).entries.size(), 10000);
    EXPECT_THROW(parse(entry("before.wav") + boundary), std::runtime_error);
    EXPECT_THROW(parse("<REPEAT COUNT='5001'>" + entry("a.wav") + entry("b.wav") + "</REPEAT>"),
        std::runtime_error);
}

TEST_F(AsxPlaylistTest, IgnoresSubsequentAndNestedRepeatBlocksWithDiagnostic)
{
    auto plan = parse("<REPEAT COUNT='2'>" + entry("a.wav") + "<REPEAT COUNT='2'>" + entry("nested.wav") + "</REPEAT></REPEAT><REPEAT COUNT='2'>" + entry("later.wav") + "</REPEAT>" + entry("after.wav"));
    EXPECT_EQ(firstReferences(plan),
        (std::vector<std::string> { "a.wav", "a.wav", "after.wav" }));
    ASSERT_EQ(plan.warnings.size(), 1);
    EXPECT_NE(plan.warnings[0].find("REPEAT ignored"), std::string::npos);
}

TEST_F(AsxPlaylistTest, ExpandsChildAtReferencePositionUsingItsOwnContext)
{
    children["/asx/sub/child.asx"] = "<ASX>" + entry("../b.wav") + entry("c.wav") + "</ASX>";
    auto plan = parse(entry("a.wav") + "<ENTRYREF HREF='sub/child.asx'/>" + entry("after.wav"));
    EXPECT_EQ(
        firstReferences(plan),
        (std::vector<std::string> { "a.wav", "../b.wav", "c.wav", "after.wav" }));
    EXPECT_EQ(plan.entries[1].source, "/asx/sub/child.asx");
    EXPECT_EQ(plan.entries[2].source, "/asx/sub/child.asx");
    EXPECT_EQ(plan.entries[3].source, "/asx/main.asx");
}

TEST_F(AsxPlaylistTest,
    RepeatedChildIsNotACycleAndRepeatAppliesToItsCompleteSequence)
{
    children["/asx/child.asx"] = "<ASX>" + entry("b.wav") + entry("c.wav") + "</ASX>";
    auto plan = parse("<REPEAT COUNT='2'><ENTRYREF "
                      "HREF='child.asx'/></REPEAT><ENTRYREF HREF='child.asx'/>");
    EXPECT_EQ(firstReferences(plan),
        (std::vector<std::string> { "b.wav", "c.wav", "b.wav", "c.wav",
            "b.wav", "c.wav" }));
}

TEST_F(AsxPlaylistTest, RepeatValidityIsScopedToEachDocument)
{
    children["/asx/child.asx"] = "<ASX><REPEAT COUNT='2'>" + entry("b.wav") + "</REPEAT></ASX>";
    auto plan = parse("<REPEAT COUNT='2'>" + entry("a.wav") + "</REPEAT><ENTRYREF HREF='child.asx'/>");
    EXPECT_EQ(firstReferences(plan),
        (std::vector<std::string> { "a.wav", "a.wav", "b.wav", "b.wav" }));
}

TEST_F(AsxPlaylistTest, SkipIfRefOnlySkipsIncludedEntries)
{
    const std::string skip = "<ENTRY SKIPIFREF='yEs'><REF HREF='skip.wav'/></ENTRY>";
    children["/asx/child.asx"] = "<ASX>" + skip + entry("keep.wav") + "</ASX>";
    auto plan = parse(skip + "<ENTRYREF HREF='child.asx'/>");
    EXPECT_EQ(firstReferences(plan),
        (std::vector<std::string> { "skip.wav", "keep.wav" }));
}

TEST_F(AsxPlaylistTest, RejectsCyclesWithoutReturningSurroundingEntries)
{
    children["/asx/child.asx"] = "<ASX>" + entry("child.wav") + "<ENTRYREF HREF='child.asx'/></ASX>";
    EXPECT_THROW(parse(entry("before.wav") + "<ENTRYREF HREF='child.asx'/>" + entry("after.wav")),
        std::runtime_error);
}

TEST_F(AsxPlaylistTest, ChecksCanonicalLoaderPathForAliasCycles)
{
    ASSERT_TRUE(document.load_string("<ASX><ENTRYREF HREF='alias.asx'/></ASX>"));
    EXPECT_THROW(readAsxPlaylistPlan(document.document_element(), "/asx/main.asx",
                     [](const auto&, const auto&) {
                         auto child = std::make_unique<pugi::xml_document>();
                         child->load_string("<ASX/>");
                         return AsxPlaylistDocument {
                             "/asx/main.asx", std::move(child), {}
                         };
                     }),
        std::runtime_error);
}

TEST_F(AsxPlaylistTest, AllowsFiveNestedDocumentsAndRejectsSix)
{
    for (int i = 1; i <= 5; ++i)
        children["/asx/" + std::to_string(i) + ".asx"] = i == 5
            ? "<ASX>" + entry("a.wav") + "</ASX>"
            : "<ASX><ENTRYREF HREF='" + std::to_string(i + 1) + ".asx'/></ASX>";
    EXPECT_EQ(firstReferences(parse("<ENTRYREF HREF='1.asx'/>")),
        (std::vector<std::string> { "a.wav" }));
    children["/asx/5.asx"] = "<ASX><ENTRYREF HREF='6.asx'/></ASX>";
    children["/asx/6.asx"] = "<ASX>" + entry("a.wav") + "</ASX>";
    EXPECT_THROW(parse("<ENTRYREF HREF='1.asx'/>"), std::runtime_error);
}

TEST_F(AsxPlaylistTest, MissingChildrenKeepSurroundingEntriesAndDiagnostic)
{
    auto plan = parse(entry("before.wav") + "<ENTRYREF HREF='missing.asx'/>" + entry("after.wav"));
    EXPECT_EQ(firstReferences(plan),
        (std::vector<std::string> { "before.wav", "after.wav" }));
    ASSERT_EQ(plan.warnings.size(), 1);
    EXPECT_NE(plan.warnings[0].find("Missing local ENTRYREF"), std::string::npos);
}

TEST_F(AsxPlaylistTest, BoundsTraversalWorkEvenWhenChildrenHaveNoPositions)
{
    children["/asx/empty.asx"] = "<ASX/>";
    std::string body;
    for (int i = 0; i < 10000; ++i)
        body += "<ENTRYREF HREF='empty.asx'/>";
    EXPECT_TRUE(parse(body).entries.empty());
    EXPECT_THROW(parse(body + "<ENTRYREF HREF='empty.asx'/>"), std::runtime_error);
}

TEST_F(AsxPlaylistTest, MalformedChildRejectsCompletePlan)
{
    children["/asx/broken.asx"] = "<ASX><ENTRY>";
    EXPECT_THROW(parse(entry("before.wav") + "<ENTRYREF HREF='broken.asx'/>" + entry("after.wav")),
        std::runtime_error);
}

TEST_F(AsxPlaylistTest, MapsScopedMetadataWithoutInheritingThePlaylistTitle)
{
    auto plan = parse(
        "<TITLE>Root title</TITLE><AUTHOR>Root author</AUTHOR><COPYRIGHT>Root "
        "rights</COPYRIGHT><ABSTRACT>Root abstract</ABSTRACT>"
        "<ENTRY><TITLE>Entry title</TITLE><TITLE>Ignored "
        "title</TITLE><AUTHOR>Entry author</AUTHOR><AUTHOR>Ignored "
        "author</AUTHOR><ABSTRACT>Entry abstract</ABSTRACT><REF "
        "HREF='a.wav'/></ENTRY>"
        + entry("b.wav"));
    ASSERT_EQ(plan.entries.size(), 2);
    EXPECT_EQ(plan.metadata.title, "Root title");
    EXPECT_EQ(plan.entries[0].metadata.title, "Entry title");
    EXPECT_EQ(plan.entries[0].metadata.author, "Entry author");
    EXPECT_EQ(plan.entries[0].metadata.copyright, "Root rights");
    EXPECT_EQ(plan.entries[0].metadata.description, "Entry abstract");
    EXPECT_TRUE(plan.entries[1].metadata.title.empty());
    EXPECT_EQ(plan.entries[1].metadata.author, "Root author");
    EXPECT_EQ(plan.entries[1].metadata.description, "Root abstract");
}

TEST_F(AsxPlaylistTest, ChildDefaultsDoNotLeakIntoParentOrOtherChildDocuments)
{
    children["/asx/child.asx"] = "<ASX><AUTHOR>Child author</AUTHOR>" + entry("child.wav") + "</ASX>";
    children["/asx/plain.asx"] = "<ASX>" + entry("plain.wav") + "</ASX>";
    auto plan = parse("<AUTHOR>Root author</AUTHOR><ENTRYREF "
                      "HREF='child.asx'/><ENTRYREF HREF='plain.asx'/>"
        + entry("parent.wav"));
    EXPECT_EQ(plan.entries[0].metadata.author, "Child author");
    EXPECT_TRUE(plan.entries[1].metadata.author.empty());
    EXPECT_EQ(plan.entries[2].metadata.author, "Root author");
}

TEST_F(AsxPlaylistTest, EntryBaseOverridesRootBaseWithoutLeaking)
{
    auto plan = parse("<BASE HREF='media'/><ENTRY><BASE HREF='other'/><REF "
                      "HREF='a.wav'/></ENTRY>"
        + entry("b.wav"));
    EXPECT_EQ(plan.entries[0].base, "other");
    EXPECT_EQ(plan.entries[1].base, "media");
}

TEST_F(AsxPlaylistTest,
    RetainsParamsAndAlternativeWriteThroughWithoutStateLeak)
{
    auto plan = parse("<PARAM NAME='protocol' VALUE='http-get'/><ENTRY><PARAM "
                      "NAME='mimetype' VALUE='audio/wav'/><PARAM NAME='custom' "
                      "VALUE='Äther 東京'/><REF HREF='a.wav' WRITETHROUGH='1'/></ENTRY>"
        + entry("b.wav"));
    ASSERT_EQ(plan.entries.size(), 2);
    EXPECT_EQ(plan.entries[0].metadata.parameters.at("protocol"), "http-get");
    EXPECT_EQ(plan.entries[0].metadata.parameters.at("mimetype"), "audio/wav");
    EXPECT_EQ(plan.entries[0].metadata.parameters.at("custom"), "Äther 東京");
    EXPECT_EQ(plan.entries[0].references[0].writeThrough, 1);
    EXPECT_EQ(plan.entries[1].metadata.parameters.size(), 1);
    EXPECT_EQ(plan.entries[1].references[0].writeThrough, -1);
}

TEST_F(AsxPlaylistTest,
    UnsupportedPlayerElementsAreDiagnosedWithoutFollowingThem)
{
    auto plan = parse("<ENTRY CLIENTSKIP='NO'><DURATION VALUE='00:02'/><REF "
                      "HREF='a.wav'><STARTTIME VALUE='00:01'/></REF></ENTRY>"
                      "<EVENT><ENTRYREF HREF='never.asx'/>"
        + entry("event.wav") + "</EVENT><BANNER HREF='https://invalid.example/banner'/><LOGURL "
                               "HREF='https://invalid.example/log'/>");
    EXPECT_EQ(firstReferences(plan), (std::vector<std::string> { "a.wav" }));
    EXPECT_TRUE(requested.empty());
    EXPECT_EQ(plan.warnings.size(), 5);
}

TEST_F(AsxPlaylistTest, CaseInsensitiveElementsAndAttributesAndEmptyEntries)
{
    auto plan = parse("<entry/><eNtRy><TiTlE>Title</TiTlE><ReF HrEf='a.wav' "
                      "WrItEtHrOuGh='1'/></eNtRy>");
    ASSERT_EQ(plan.entries.size(), 1);
    EXPECT_EQ(plan.entries[0].metadata.title, "Title");
    EXPECT_EQ(plan.entries[0].references[0].href, "a.wav");
    EXPECT_EQ(plan.entries[0].references[0].writeThrough, 1);
}

TEST_F(AsxPlaylistTest, InvalidRootAndVersionAreRejected)
{
    for (auto xml : { "<WPL/>", "<ASX VERSION='2.0'/>" }) {
        ASSERT_TRUE(document.load_string(xml));
        EXPECT_THROW(
            readAsxPlaylistPlan(document.document_element(), "/asx/main.asx", {}),
            std::runtime_error);
    }
}
