#include "gtest/gtest.h"
#include "search_handler.h"

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

::testing::AssertionResult executeTest(const std::string& input,
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
    EXPECT_EQ(SearchToken(TokenType::ANDOR, "and"), *(tokens->at(0)));

    tokens = getAllTokens("OR");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::ANDOR, "OR"), *(tokens->at(0)));

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
    EXPECT_TRUE(executeTest(input, expectedTokens));

    input = "x = a";
    expectedTokens = {
        {"x", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"a", TokenType::PROPERTY}
    };
    EXPECT_TRUE(executeTest(input, expectedTokens));

    input = "xyz=abc";
    expectedTokens = {
        {"xyz", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"abc", TokenType::PROPERTY}
    };
    EXPECT_TRUE(executeTest(input, expectedTokens));

    input = "xyz=abc fg >= hi";
    expectedTokens = {
        {"xyz", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"abc", TokenType::PROPERTY},
        {"fg", TokenType::PROPERTY},
        {">=", TokenType::COMPAREOP},
        {"hi", TokenType::PROPERTY}
    };
    EXPECT_TRUE(executeTest(input, expectedTokens));

    input = "x=\"a\"";
    expectedTokens = {
        {"x", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"a", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE}
    };
    EXPECT_TRUE(executeTest(input, expectedTokens));

    input = "dc:creator = \"Kyuss\"";
    expectedTokens = {
        {"dc:creator", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"Kyuss", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE}
    };
    EXPECT_TRUE(executeTest(input, expectedTokens));

    input = "dc:creator = \"some band with \\\"a double-quote\"";
    expectedTokens = {
        {"dc:creator", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"some band with \"a double-quote", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE}
    };
    EXPECT_TRUE(executeTest(input, expectedTokens));

    input = "dc:creator = \"some band with \\\"a double-quote\\\"\"";
    expectedTokens = {
        {"dc:creator", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"some band with \"a double-quote\"", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE}
    };
    EXPECT_TRUE(executeTest(input, expectedTokens));

    input = "upnp:class derivedfrom \"object.item.audioItem\" and upnp:artist=\"King Krule\"";
    expectedTokens = {
        {"upnp:class", TokenType::PROPERTY},
        {"derivedfrom", TokenType::STRINGOP},
        {"\"", TokenType::DQUOTE},
        {"object.item.audioItem", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE},
        {"and", TokenType::ANDOR},
        {"upnp:artist", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"King Krule", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE},
    };
    EXPECT_TRUE(executeTest(input, expectedTokens));

    input = "upnp:class derivedfrom \"object.item.audioItem\" and (upnp:artist=\"King Krule\" or dc:title=\"Heartattack and Vine\")";
    expectedTokens = {
        {"upnp:class", TokenType::PROPERTY},
        {"derivedfrom", TokenType::STRINGOP},
        {"\"", TokenType::DQUOTE},
        {"object.item.audioItem", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE},
        {"and", TokenType::ANDOR},
        {"(", TokenType::LPAREN},
        {"upnp:artist", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"King Krule", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE},
        {"or", TokenType::ANDOR},
        {"dc:title", TokenType::PROPERTY},
        {"=", TokenType::COMPAREOP},
        {"\"", TokenType::DQUOTE},
        {"Heartattack and Vine", TokenType::ESCAPEDSTRING},
        {"\"", TokenType::DQUOTE},
        {")", TokenType::RPAREN},
    };
    EXPECT_TRUE(executeTest(input, expectedTokens));
}

