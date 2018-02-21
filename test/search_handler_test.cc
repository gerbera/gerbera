#include "gtest/gtest.h"
#include "search_handler.h"
#include "config_manager.h"
#include "zmm/exception.h"
#include <iostream>

using upVecUpST = std::unique_ptr<std::vector<std::unique_ptr<SearchToken>>>;
decltype(auto) getAllTokens(const std::string& input)
{
    SearchLexer lexer{input};
    upVecUpST searchTokens = std::make_unique<std::vector<std::unique_ptr<SearchToken>>>();
    std::unique_ptr<SearchToken> searchToken = nullptr;
    bool gotToken = false;
    do {
        searchToken = lexer.nextToken();
        gotToken = searchToken!=nullptr;
        if (searchToken)
            searchTokens->push_back(std::move(searchToken));
    } while (gotToken);
    return searchTokens;
}

::testing::AssertionResult executeSearchLexerTest(const std::string& input,
    const std::vector<std::pair<std::string, TokenType>>& expectedTokens)
{
    auto tokens = getAllTokens(input);
    if (expectedTokens.size() != tokens->size())
        return ::testing::AssertionFailure() << "Expected " << expectedTokens.size() << " tokens, got " << tokens->size();
    for (unsigned i = 0; i < tokens->size(); i++) {
        if (expectedTokens.at(i).first != tokens->at(i)->getValue())
            return ::testing::AssertionFailure() << "Token " << i << ": expected ["
                                                 << expectedTokens.at(i).first
                                                 << "], got ["
                                                 << tokens->at(i)->getValue() << "]";

        if (expectedTokens.at(i).second != tokens->at(i)->getType())
            return ::testing::AssertionFailure() << "Token type " << i << ": expected ["
                                                 << static_cast<int>(expectedTokens.at(i).second)
                                                 << "], got ["
                << static_cast<int>(tokens->at(i)->getType()) << "]";
    }
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult executeSearchParserTest(const SQLEmitter& emitter, const std::string& input,
    const std::string expectedOutput)
{
    std::cout << "Testing with input [" << input << "]" << std::endl;

    try {
        auto parser = SearchParser(emitter, input);
        auto rootNode = parser.parse();
        if (rootNode == nullptr)
            return ::testing::AssertionFailure() << "Failed to create AST";

        auto output = rootNode->emit();
        if (output != expectedOutput)
            return ::testing::AssertionFailure() << "\nExpected [" << expectedOutput << "]\nActual   [" << output << "]\n";

        return ::testing::AssertionSuccess();
    } catch (const zmm::Exception& e) {
        return ::testing::AssertionFailure() << e.getMessage().c_str();
    }
}

TEST(SearchLexer, OneSimpleTokenRecognized)
{
    auto tokens = getAllTokens("=");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::COMPAREOP,"="), *(tokens->at(0)));

    tokens = getAllTokens("!=");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::COMPAREOP,"!="), *(tokens->at(0)));

    tokens = getAllTokens(">");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::COMPAREOP,">"), *(tokens->at(0)));

    tokens = getAllTokens("(");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::LPAREN,"("), *(tokens->at(0)));
}


TEST(SearchLexer,OneComplexTokenRecognized)
{
    auto tokens = getAllTokens("\"");
    EXPECT_EQ(1, tokens->size());
    EXPECT_STREQ("\"", tokens->at(0)->getValue().c_str());
    EXPECT_EQ(1, tokens->at(0)->getValue().length());
    EXPECT_EQ(TokenType::DQUOTE, tokens->at(0)->getType());

    tokens = getAllTokens("true");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::BOOLVAL, "true"), *(tokens->at(0)));

    tokens = getAllTokens("FALSE");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::BOOLVAL, "FALSE"), *(tokens->at(0)));

    tokens = getAllTokens("and");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::AND, "and"), *(tokens->at(0)));

    tokens = getAllTokens("OR");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::OR, "OR"), *(tokens->at(0)));

    tokens = getAllTokens("exists");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::EXISTS, "exists"), *(tokens->at(0)));

    tokens = getAllTokens("@id");
    EXPECT_EQ(1, tokens->size());
    EXPECT_STREQ("@id", tokens->at(0)->getValue().c_str());
    EXPECT_EQ(TokenType::PROPERTY, tokens->at(0)->getType());

    tokens = getAllTokens("res@size");
    EXPECT_EQ(1, tokens->size());
    EXPECT_STREQ("res@size", tokens->at(0)->getValue().c_str());
    EXPECT_EQ(TokenType::PROPERTY, tokens->at(0)->getType());

    tokens = getAllTokens("dc:title");
    EXPECT_EQ(1, tokens->size());
    EXPECT_STREQ("dc:title", tokens->at(0)->getValue().c_str());
    EXPECT_EQ(TokenType::PROPERTY, tokens->at(0)->getType());
}

TEST(SearchLexer, MultipleTokens)
{
    std::string input = "x=a";
    std::vector<std::pair<std::string, TokenType>> expectedTokens = {
        {"x", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"a", TokenType::PROPERTY}
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "x = a";
    expectedTokens = {
        {"x", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"a", TokenType::PROPERTY}
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "xyz=abc";
    expectedTokens = {
        {"xyz", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"abc", TokenType::PROPERTY}
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "xyz=abc fg >= hi";
    expectedTokens = {
        {"xyz", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"abc", TokenType::PROPERTY},
        {"fg", TokenType::PROPERTY},
        {">=", TokenType::COMPAREOP},
        {"hi", TokenType::PROPERTY}
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "x=\"a\"";
    expectedTokens = {
        {"x", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"a", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE}
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "dc:creator = \"Kyuss\"";
    expectedTokens = {
        {"dc:creator", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"Kyuss", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE}
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "dc:creator = \"some band with \\\"a double-quote\"";
    expectedTokens = {
        {"dc:creator", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"some band with \"a double-quote", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE}
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "dc:creator = \"some band with \\\"a double-quote\\\"\"";
    expectedTokens = {
        {"dc:creator", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"some band with \"a double-quote\"", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE}
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "upnp:class derivedfrom \"object.item.audioItem\" and upnp:artist=\"King Krule\"";
    expectedTokens = {
        {"upnp:class", TokenType::PROPERTY},
        {"derivedfrom", TokenType::STRINGOP},
        {"\"", TokenType::DQUOTE},
        {"object.item.audioItem", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE},
        {"and", TokenType::AND},
        {"upnp:artist", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"King Krule", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE},
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "upnp:class derivedfrom \"object.item.audioItem\" and (upnp:artist=\"King Krule\" or dc:title=\"Heartattack and Vine\")";
    expectedTokens = {
        {"upnp:class", TokenType::PROPERTY},
        {"derivedfrom", TokenType::STRINGOP},
        {"\"", TokenType::DQUOTE},
        {"object.item.audioItem", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE},
        {"and", TokenType::AND},
        {"(", TokenType::LPAREN},
        {"upnp:artist", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"King Krule", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE},
        {"or", TokenType::OR},
        {"dc:title", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"Heartattack and Vine", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE},
        {")", TokenType::RPAREN},
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));
}

TEST(SearchParser, SimpleSearchCriteriaUsingEqualsOperatorForSqlite)
{
    SqliteEmitter sqlEmitter;
    // equalsOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
            "dc:title=\"Hospital Roll Call\"",
            "(m.property_name='dc:title' and lower(m.property_value)=lower('Hospital Roll Call') and c.upnp_class is not null)"));

    // equalsOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
            "upnp:album=\"Scraps At Midnight\"",
            "(m.property_name='upnp:album' and lower(m.property_value)=lower('Scraps At Midnight') and c.upnp_class is not null)"));

    // equalsOpExpr or equalsOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
            "upnp:album=\"Scraps At Midnight\" or dc:title=\"Hospital Roll Call\"",
            "(m.property_name='upnp:album' and lower(m.property_value)=lower('Scraps At Midnight') and c.upnp_class is not null) or (m.property_name='dc:title' and lower(m.property_value)=lower('Hospital Roll Call') and c.upnp_class is not null)"));

    // equalsOpExpr or equalsOpExpr or equalsOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
            "upnp:album=\"Scraps At Midnight\" or dc:title=\"Hospital Roll Call\" or upnp:artist=\"Deafheaven\"",
            "(m.property_name='upnp:album' and lower(m.property_value)=lower('Scraps At Midnight') and c.upnp_class is not null) or (m.property_name='dc:title' and lower(m.property_value)=lower('Hospital Roll Call') and c.upnp_class is not null) or (m.property_name='upnp:artist' and lower(m.property_value)=lower('Deafheaven') and c.upnp_class is not null)"));
}

TEST(SearchParser, SearchCriteriaUsingEqualsOperatorParenthesesForSqlite)
{
    SqliteEmitter sqlEmitter;
    // (equalsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
            "(upnp:album=\"Scraps At Midnight\")",
            "((m.property_name='upnp:album' and lower(m.property_value)=lower('Scraps At Midnight') and c.upnp_class is not null))"));

    // (equalsOpExpr or equalsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
            "(upnp:album=\"Scraps At Midnight\" or dc:title=\"Hospital Roll Call\")",
            "((m.property_name='upnp:album' and lower(m.property_value)=lower('Scraps At Midnight') and c.upnp_class is not null) or (m.property_name='dc:title' and lower(m.property_value)=lower('Hospital Roll Call') and c.upnp_class is not null))"));

    // (equalsOpExpr or equalsOpExpr) or equalsOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
            "(upnp:album=\"Scraps At Midnight\" or dc:title=\"Hospital Roll Call\") or upnp:artist=\"Deafheaven\"",
            "((m.property_name='upnp:album' and lower(m.property_value)=lower('Scraps At Midnight') and c.upnp_class is not null) or (m.property_name='dc:title' and lower(m.property_value)=lower('Hospital Roll Call') and c.upnp_class is not null)) or (m.property_name='upnp:artist' and lower(m.property_value)=lower('Deafheaven') and c.upnp_class is not null)"));

    // equalsOpExpr or (equalsOpExpr or equalsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
            "upnp:album=\"Scraps At Midnight\" or (dc:title=\"Hospital Roll Call\" or upnp:artist=\"Deafheaven\")",
            "(m.property_name='upnp:album' and lower(m.property_value)=lower('Scraps At Midnight') and c.upnp_class is not null) or ((m.property_name='dc:title' and lower(m.property_value)=lower('Hospital Roll Call') and c.upnp_class is not null) or (m.property_name='upnp:artist' and lower(m.property_value)=lower('Deafheaven') and c.upnp_class is not null))"));

    // equalsOpExpr and (equalsOpExpr or equalsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
            "upnp:album=\"Scraps At Midnight\" and (dc:title=\"Hospital Roll Call\" or upnp:artist=\"Deafheaven\")",
            "(m.property_name='upnp:album' and lower(m.property_value)=lower('Scraps At Midnight') and c.upnp_class is not null) and ((m.property_name='dc:title' and lower(m.property_value)=lower('Hospital Roll Call') and c.upnp_class is not null) or (m.property_name='upnp:artist' and lower(m.property_value)=lower('Deafheaven') and c.upnp_class is not null))"));

    // equalsOpExpr and (equalsOpExpr or equalsOpExpr or equalsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
            "upnp:album=\"Scraps At Midnight\" and (dc:title=\"Hospital Roll Call\" or upnp:artist=\"Deafheaven\" or upnp:artist=\"Pavement\")",
            "(m.property_name='upnp:album' and lower(m.property_value)=lower('Scraps At Midnight') and c.upnp_class is not null) and ((m.property_name='dc:title' and lower(m.property_value)=lower('Hospital Roll Call') and c.upnp_class is not null) or (m.property_name='upnp:artist' and lower(m.property_value)=lower('Deafheaven') and c.upnp_class is not null) or (m.property_name='upnp:artist' and lower(m.property_value)=lower('Pavement') and c.upnp_class is not null))"));

    // (equalsOpExpr or equalsOpExpr or equalsOpExpr) and equalsOpExpr and equalsOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
            "(dc:title=\"Hospital Roll Call\" or upnp:artist=\"Deafheaven\" or upnp:artist=\"Pavement\") and upnp:album=\"Nevermind\" and upnp:album=\"Sunbather\"",
            "((m.property_name='dc:title' and lower(m.property_value)=lower('Hospital Roll Call') and c.upnp_class is not null) or (m.property_name='upnp:artist' and lower(m.property_value)=lower('Deafheaven') and c.upnp_class is not null) or (m.property_name='upnp:artist' and lower(m.property_value)=lower('Pavement') and c.upnp_class is not null)) and (m.property_name='upnp:album' and lower(m.property_value)=lower('Nevermind') and c.upnp_class is not null) and (m.property_name='upnp:album' and lower(m.property_value)=lower('Sunbather') and c.upnp_class is not null)"));
}

TEST(SearchParser, SearchCriteriaUsingContainsOperator)
{
    SqliteEmitter sqlEmitter;
    // (containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album contains \"Midnight\"", "(m.property_name='upnp:album' and lower(m.property_value) like lower('%Midnight%') and c.upnp_class is not null)"));

    // (containsOpExpr or containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album contains \"Midnight\" or upnp:artist contains \"HEAVE\"", "(m.property_name='upnp:album' and lower(m.property_value) like lower('%Midnight%') and c.upnp_class is not null) or (m.property_name='upnp:artist' and lower(m.property_value) like lower('%HEAVE%') and c.upnp_class is not null)"));
}

TEST(SearchParser, SearchCriteriaUsingDoesNotContainOperator)
{
    SqliteEmitter sqlEmitter;
    // (containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album doesnotcontain \"Midnight\"", "(m.property_name='upnp:album' and lower(m.property_value) not like lower('%Midnight%') and c.upnp_class is not null)"));

    // (containsOpExpr or containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album doesNotContain \"Midnight\" or upnp:artist doesnotcontain \"HEAVE\"", "(m.property_name='upnp:album' and lower(m.property_value) not like lower('%Midnight%') and c.upnp_class is not null) or (m.property_name='upnp:artist' and lower(m.property_value) not like lower('%HEAVE%') and c.upnp_class is not null)"));
}

TEST(SearchParser, SearchCriteriaUsingStartsWithOperator)
{
    SqliteEmitter sqlEmitter;
    // (containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album startswith \"Midnight\"", "(m.property_name='upnp:album' and lower(m.property_value) like lower('Midnight%') and c.upnp_class is not null)"));

    // (containsOpExpr or containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album startsWith \"Midnight\" or upnp:artist startswith \"HEAVE\"", "(m.property_name='upnp:album' and lower(m.property_value) like lower('Midnight%') and c.upnp_class is not null) or (m.property_name='upnp:artist' and lower(m.property_value) like lower('HEAVE%') and c.upnp_class is not null)"));
}

TEST(SearchParser, SearchCriteriaUsingExistsOperator)
{
    SqliteEmitter sqlEmitter;
    // (containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album exists true", "(m.property_name='upnp:album' and m.property_value is not null and c.upnp_class is not null)"));

    // (containsOpExpr or containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album exists true or upnp:artist exists false", "(m.property_name='upnp:album' and m.property_value is not null and c.upnp_class is not null) or (m.property_name='upnp:artist' and m.property_value is null and c.upnp_class is not null)"));
}

TEST(SearchParser, SearchCriteriaWithExtendsOperator)
{
    SqliteEmitter sqlEmitter;
    // derivedfromOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:class derivedfrom \"object.item.audioItem\"",
            "c.upnp_class like lower('object.item.audioItem.%')"));

    // derivedfromOpExpr and (containsOpExpr or containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:class derivedfrom \"object.item.audioItem\" and (dc:title contains \"britain\" or dc:creator contains \"britain\"", "c.upnp_class like lower('object.item.audioItem.%') and ((m.property_name='dc:title' and lower(m.property_value) like lower('%britain%') and c.upnp_class is not null) or (m.property_name='dc:creator' and lower(m.property_value) like lower('%britain%') and c.upnp_class is not null))"));    

    // derivedFromOpExpr and (containsOpExpr or containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:class derivedFrom \"object.item.audioItem\" and (dc:title contains \"britain\" or dc:creator contains \"britain\"", "c.upnp_class like lower('object.item.audioItem.%') and ((m.property_name='dc:title' and lower(m.property_value) like lower('%britain%') and c.upnp_class is not null) or (m.property_name='dc:creator' and lower(m.property_value) like lower('%britain%') and c.upnp_class is not null))"));    
}

// select * from mt_cds_object where id=215291 or dc_title = 'Thong Song' or id in (194099,214889,215237,215239,215241,202037)
// select * from mt_cds_object where (upnp_class is null and dc_title='Hospital Roll Call') or id=204320 or id in (194099,203869,204303,204305,194717,194114)
// 194099,203869,204303,204305,194717,194114
