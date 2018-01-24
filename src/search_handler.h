/// \file search_handler.h
/// \brief Definitions of classes supporting implementation of search
/// Implements functionality defined in standardizeddcps\MediaServer_4 and  MediaRenderer_3\UPnP-av-ContentDirectory-v4-Service-20150630.pdf
/// Document available in upnpresources.zip, download from here:
/// https://openconnectivity.org/developer/specifications/upnp-resources/upnp
#ifndef __SEARCH_HANDLER_H__
#define __SEARCH_HANDLER_H__

#include "cds_objects.h"
#include "memory.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

class Namee
{
public:
    //! Default constructor
    Namee() = default;

    //! Copy constructor
    Namee(const Namee &other) = default;

    //! Move constructor
    Namee(Namee &&other) noexcept = default;

    //! Destructor
    virtual ~Namee() noexcept = default;

    //! Copy assignment operator
    Namee& operator=(const Namee &other) = default;

    //! Move assignment operator
    Namee& operator=(Namee &&other) noexcept = default;



protected:
private:
};
class SearchParam {
protected:
    const std::string& containerID;
    const std::string& searchCriteria;
    int startingIndex;
    int requestedCount;
    
public:
    SearchParam(const std::string& containerID, const std::string& searchCriteria, int startingIndex, int requestedCount)
        : containerID(containerID)
        , searchCriteria(searchCriteria)
        , startingIndex(startingIndex)
        , requestedCount(requestedCount)
    {}
};

enum class TokenType
{
    ASTERISK, DQUOTE, ESCAPEDSTRING, PROPERTY, BOOLVAL, EXISTS, STRINGOP, COMPAREOP, AND, OR, LPAREN, RPAREN, INVALID
};

class SearchToken {
public:
    SearchToken(TokenType type, std::string value)
        : type(type), value(std::move(value)) {}
    virtual ~SearchToken(){};
    
    const TokenType& getType() const { return type; }
    const std::string& getValue() const { return value; }

    friend bool operator==(const SearchToken& lhs, const SearchToken& rhs) {
        return lhs.type==rhs.type && lhs.value==rhs.value;
    }
    
protected:
    const TokenType type;
    const std::string value;
};


class SearchLexer {
public:
    SearchLexer(const std::string& input) : input(input), currentPos(0), inQuotes(false) {};
    virtual ~SearchLexer(){};

    std::unique_ptr<SearchToken> nextToken();

    SearchLexer& operator=(const SearchLexer&) = delete;
    SearchLexer(const SearchLexer&) = delete;
    SearchLexer& operator=(const SearchLexer&&) = delete;
    SearchLexer(const SearchLexer&&) = delete;
protected:
    std::string nextStringToken(const std::string& input);
    std::unique_ptr<SearchToken> makeToken(const std::string& tokenStr);
    std::string getQuotedValue(const std::string& input);

    const std::string& input;
    unsigned currentPos;
    bool inQuotes;
};


// NOTES
// ASTNode needs node type (new enum or will TokenType suffice?)
// Need ASTNode factory so that each node is built with relevant 'emitter'
// Will need an emitter for testing correct tree AST is getting built
//
class ASTNode {
public:
    ASTNode(std::unique_ptr<SQLEmitter> emitter)
        : sqlEmitter(std:move(emitter))
    {};
    virtual ~ASTNode() = default;

protected:
    virtual std::string emit() = 0;
    std::unique_ptr<SQLEmitter> sqlEmitter;
};

class ASTAsterisk : public ASTNode {
public:
    ASTAsterisk(std::string value) : value(std::move(value)){};
protected:
    std::string value;
};

class ASTProperty : public ASTNode {
public:
    ASTProperty(std::string value) : value(std::move(value)){};
protected:
    std::string value;
};

class ASTBoolean : public ASTNode {
public:
    ASTBoolean(std::string value) : value(std::move(value)){};
protected:
    std::string value;
};

class ASTParenthesis : public ASTNode {
public:
    ASTParenthesis(std::unique_ptr<ASTNode> node)
        : node(std::move(node))
    {};
protected:
    std::unique_ptr<ASTNode> node;
};

class ASTDQuote : public ASTNode {
public:
    ASTDQuote(std::string value) : value(std::move(value)){};
protected:
    std::string value;
};

class ASTEscapedString : public ASTNode {
public:
    ASTEscapedString(std::string value) : value(std::move(value)){};
protected:
    std::string value;
};

class ASTQuotedString : public ASTNode {
public:
    ASTQuotedString(std::unique_ptr<ASTDQuote> openQuote, std::unique_ptr<ASTEscapedString> escapedString, std::unique_ptr<ASTDQuote> closeQuote)
        : openQuote(std::move(openQuote))
        , escapedString(std::move(escapedString))
        , closeQuote(std::move(closeQuote))
    {};
protected:
    std::unique_ptr<ASTDQuote> openQuote;
    std::unique_ptr<ASTEscapedString> escapedString;
    std::unique_ptr<ASTDQuote> closeQuote;
};

/// \brief Represents a comparison operator such as =, >=, <
class ASTCompareOperator : public ASTNode {
public:
    ASTCompareOperator(std::string value) : value(std::move(value)){};
protected:
    std::string value;
};

/// \brief Represents an expression using a comparison operator
class ASTCompareExpression : public ASTNode {
public:
    ASTCompareExpression(std::unique_ptr<ASTProperty> lhs, std::unique_ptr<ASTCompareOperator> operatr, std::unique_ptr<ASTQuotedString> rhs)
        : lhs(std::move(lhs))
        , operatr(std::move(operatr))
        , rhs(std::move(rhs))
    {};
protected:
    std::unique_ptr<ASTProperty> lhs;
    std::unique_ptr<ASTCompareOperator> operatr;
    std::unique_ptr<ASTQuotedString> rhs;
};

/// \brief Represents an operator defined by a string such as contains, derivedFrom
class ASTStringOperator : public ASTNode {
public:
    ASTStringOperator(std::string value) : value(std::move(value)){};
protected:
    std::string value;
};

/// \brief Represents an expression using an operator defined by a string
class ASTStringExpression : public ASTNode {
public:
    ASTStringExpression(std::unique_ptr<ASTProperty> lhs, std::unique_ptr<ASTStringOperator> operatr, std::unique_ptr<ASTQuotedString> rhs)
        : lhs(std::move(lhs))
        , operatr(std::move(operatr))
        , rhs(std::move(rhs))
    {};
protected:
    std::unique_ptr<ASTProperty> lhs;
    std::unique_ptr<ASTStringOperator> operatr;
    std::unique_ptr<ASTQuotedString> rhs;
};

class ASTExistsOperator : public ASTNode {
public:
    ASTExistsOperator(std::string value) : value(std::move(value)){};
protected:
    std::string value;
};

/// \brief Represents an expression using the exists operator
class ASTExistsExpression : public ASTNode {
public:
    ASTExistsExpression(std::unique_ptr<ASTProperty> lhs, std::unique_ptr<ASTExistsOperator> operatr, std::unique_ptr<ASTBoolean> rhs)
        : lhs(std::move(lhs))
        , operatr(std::move(operatr))
        , rhs(std::move(rhs))
    {};
protected:
    std::unique_ptr<ASTProperty> lhs;
    std::unique_ptr<ASTExistsOperator> operatr;
    std::unique_ptr<ASTBoolean> rhs;
};

class ASTAndOperator : public ASTNode {
public:
    ASTAndOperator(std::unique_ptr<ASTNode> lhs, std::unique_ptr<ASTNode> rhs)
        : lhs(std::move(lhs))
        , rhs(std::move(rhs))
    {};
protected:
    std::unique_ptr<ASTNode> lhs;
    std::unique_ptr<ASTNode> rhs;
};

class ASTOrOperator : public ASTNode {
public:
    ASTOrOperator(std::unique_ptr<ASTNode> lhs, std::unique_ptr<ASTNode> rhs)
        : lhs(std::move(lhs))
        , rhs(std::move(rhs))
    {};
protected:
    std::unique_ptr<ASTNode> lhs;
    std::unique_ptr<ASTNode> rhs;
};

class SearchParser {
public:
    SearchParser(const std::string& input);
    std::unique_ptr<ASTNode> parse();

protected:
    void getNextToken();
    std::unique_ptr<ASTNode> parseSearchExpression();
    std::unique_ptr<ASTNode> parseRelationshipExpression();
    std::unique_ptr<ASTNode> parseParenthesis();
    std::unique_ptr<ASTQuotedString> parseQuotedString();
    void checkIsExpected(TokenType tokenType, const std::string& tokenTypeDescription);

private:
    std::unique_ptr<SearchToken> currentToken;
    std::unique_ptr<SearchLexer> lexer;
};

using uvCdsObject = std::unique_ptr<std::vector<zmm::Ref<CdsObject>>>;
class SearchHandler {
public:
    inline SearchHandler(){};
    inline virtual ~SearchHandler(){};
    uvCdsObject executeSearch(SearchParam& searchParam);
};

#endif // __SEARCH_HANDLER_H__
