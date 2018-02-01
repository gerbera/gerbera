/// \file search_handler.h
/// \brief Definitions of classes supporting implementation of UPnP search
/// Implements functionality defined in standardizeddcps\MediaServer_4 and MediaRenderer_3\UPnP-av-ContentDirectory-v4-Service-20150630.pdf
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

class SearchParam {
protected:
    const std::string& containerID;
    const std::string& searchCriteria;
    int startingIndex;
    int requestedCount;
    
public:
    SearchParam(const std::string& containerID, const std::string& searchCriteria, int startingIndex,
        int requestedCount)
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
class SQLEmitter;

class ASTNode {
public:
    ASTNode(const SQLEmitter& emitter)
        : sqlEmitter(emitter)
    {};

    virtual std::string emit() = 0;

    virtual ~ASTNode() = default;
protected:
    const SQLEmitter& sqlEmitter;
};

class ASTAsterisk : public ASTNode {
public:
    ASTAsterisk(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value)){};
    virtual std::string emit();
    virtual ~ASTAsterisk() = default;
protected:
    std::string value;
};

class ASTProperty : public ASTNode {
public:
    ASTProperty(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value)){};
    virtual std::string emit();
    virtual ~ASTProperty() = default;
protected:
    std::string value;
};

class ASTBoolean : public ASTNode {
public:
    ASTBoolean(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value)){};
    virtual std::string emit();
    virtual ~ASTBoolean() = default;
protected:
    std::string value;
};

class ASTParenthesis : public ASTNode {
public:
    ASTParenthesis(const SQLEmitter& sqlEmitter, std::shared_ptr<ASTNode> node)
        : ASTNode(sqlEmitter), bracketedNode(std::move(node))
    {};
    virtual std::string emit();
    virtual ~ASTParenthesis() = default;
protected:
    std::shared_ptr<ASTNode> bracketedNode;
};

class ASTDQuote : public ASTNode {
public:
    ASTDQuote(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value)){};
    virtual std::string emit();
    virtual ~ASTDQuote() = default;
protected:
    std::string value;
};

class ASTEscapedString : public ASTNode {
public:
    ASTEscapedString(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value)){};
    virtual std::string emit();
    virtual ~ASTEscapedString() = default;
protected:
    std::string value;
};

class ASTQuotedString : public ASTNode {
public:
    ASTQuotedString(const SQLEmitter& sqlEmitter, std::shared_ptr<ASTDQuote> openQuote,
        std::shared_ptr<ASTEscapedString> escapedString, std::shared_ptr<ASTDQuote> closeQuote)
        : ASTNode(sqlEmitter)
        , openQuote(std::move(openQuote))
        , escapedString(std::move(escapedString))
        , closeQuote(std::move(closeQuote))
    {};
    virtual std::string emit();
    virtual ~ASTQuotedString() = default;
protected:
    std::shared_ptr<ASTDQuote> openQuote;
    std::shared_ptr<ASTEscapedString> escapedString;
    std::shared_ptr<ASTDQuote> closeQuote;
};

/// \brief Represents a comparison operator such as =, >=, <
class ASTCompareOperator : public ASTNode {
public:
    ASTCompareOperator(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value)){};
    virtual std::string emit();
    std::string emit(const std::string& property, const std::string& value);
    std::string getValue() const { return value; }
    virtual ~ASTCompareOperator() = default;
protected:
    std::string value;
};

/// \brief Represents an expression using a comparison operator
class ASTCompareExpression : public ASTNode {
public:
    ASTCompareExpression(const SQLEmitter& sqlEmitter, std::shared_ptr<ASTProperty> lhs,
        std::shared_ptr<ASTCompareOperator> operatr, std::shared_ptr<ASTQuotedString> rhs)
        : ASTNode(sqlEmitter)
        , lhs(std::move(lhs))
        , operatr(std::move(operatr))
        , rhs(std::move(rhs))
    {};
    virtual std::string emit();
    virtual ~ASTCompareExpression() = default;
protected:
    std::shared_ptr<ASTProperty> lhs;
    std::shared_ptr<ASTCompareOperator> operatr;
    std::shared_ptr<ASTQuotedString> rhs;
};

/// \brief Represents an operator defined by a string such as contains, derivedFrom
class ASTStringOperator : public ASTNode {
public:
    ASTStringOperator(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value)){};
    virtual std::string emit();
    std::string emit(const std::string& property, const std::string& value);
    std::string getValue() const { return value; }
    virtual ~ASTStringOperator() = default;
protected:
    std::string value;
};

/// \brief Represents an expression using an operator defined by a string
class ASTStringExpression : public ASTNode {
public:
    ASTStringExpression(const SQLEmitter& sqlEmitter, std::shared_ptr<ASTProperty> lhs,
        std::shared_ptr<ASTStringOperator> operatr, std::shared_ptr<ASTQuotedString> rhs)
        : ASTNode(sqlEmitter)
        , lhs(std::move(lhs))
        , operatr(std::move(operatr))
        , rhs(std::move(rhs))
    {};
    virtual std::string emit();
    virtual ~ASTStringExpression() = default;
protected:
    std::shared_ptr<ASTProperty> lhs;
    std::shared_ptr<ASTStringOperator> operatr;
    std::shared_ptr<ASTQuotedString> rhs;
};

class ASTExistsOperator : public ASTNode {
public:
    ASTExistsOperator(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value)){};
    virtual std::string emit();
    virtual ~ASTExistsOperator() = default;
protected:
    std::string value;
};

/// \brief Represents an expression using the exists operator
class ASTExistsExpression : public ASTNode {
public:
    ASTExistsExpression(const SQLEmitter& sqlEmitter, std::shared_ptr<ASTProperty> lhs,
        std::shared_ptr<ASTExistsOperator> operatr, std::shared_ptr<ASTBoolean> rhs)
        : ASTNode(sqlEmitter)
        , lhs(std::move(lhs))
        , operatr(std::move(operatr))
        , rhs(std::move(rhs))
    {};
    virtual std::string emit();
    virtual ~ASTExistsExpression() = default;
protected:
    std::shared_ptr<ASTProperty> lhs;
    std::shared_ptr<ASTExistsOperator> operatr;
    std::shared_ptr<ASTBoolean> rhs;
};

class ASTAndOperator : public ASTNode {
public:
    ASTAndOperator(const SQLEmitter& sqlEmitter, std::shared_ptr<ASTNode> lhs,
        std::shared_ptr<ASTNode> rhs)
        : ASTNode(sqlEmitter)
        , lhs(std::move(lhs))
        , rhs(std::move(rhs))
    {};
    virtual std::string emit();
    virtual ~ASTAndOperator() = default;
protected:
    std::shared_ptr<ASTNode> lhs;
    std::shared_ptr<ASTNode> rhs;
};

class ASTOrOperator : public ASTNode {
public:
    ASTOrOperator(const SQLEmitter& sqlEmitter, std::shared_ptr<ASTNode> lhs,
        std::shared_ptr<ASTNode> rhs)
        : ASTNode(sqlEmitter)
        , lhs(std::move(lhs))
        , rhs(std::move(rhs))
    {};
    virtual std::string emit();
    virtual ~ASTOrOperator() = default;
protected:
    std::shared_ptr<ASTNode> lhs;
    std::shared_ptr<ASTNode> rhs;
};

class SQLEmitter
{
public:
    virtual std::string emit(const ASTAsterisk& node) const = 0;
    virtual std::string emit(const ASTProperty& node) const = 0;
    virtual std::string emit(const ASTBoolean& node) const = 0;
    virtual std::string emit(const ASTParenthesis& node, const std::string& bracketedNode) const = 0;
    virtual std::string emit(const ASTDQuote& node) const = 0;
    virtual std::string emit(const ASTEscapedString& node) const = 0;
    virtual std::string emit(const ASTQuotedString& node) const = 0;
    virtual std::string emit(const ASTCompareOperator& node,
        const std::string& property, const std::string& value) const = 0;
    virtual std::string emit(const ASTCompareExpression& node) const = 0;
    virtual std::string emit(const ASTStringOperator& node,
        const std::string& property, const std::string& value) const = 0;
    virtual std::string emit(const ASTStringExpression& node) const = 0;
    virtual std::string emit(const ASTExistsOperator& node) const = 0;
    virtual std::string emit(const ASTExistsExpression& node) const = 0;
    virtual std::string emit(const ASTAndOperator& node,
        const std::string& lhs, const std::string& rhs) const = 0;
    virtual std::string emit(const ASTOrOperator& node,
        const std::string& lhs, const std::string& rhs) const = 0;

    virtual ~SQLEmitter() = default;
};

class SqliteEmitter : public SQLEmitter
{
    std::string emit(const ASTAsterisk& node) const override { return "*"; };
    std::string emit(const ASTProperty& node) const override { return "property"; };
    std::string emit(const ASTBoolean& node) const override { return "boolean"; };
    std::string emit(const ASTParenthesis& node, const std::string& bracketedNode) const override;
    std::string emit(const ASTDQuote& node) const override { return ""; };
    std::string emit(const ASTEscapedString& node) const override { return "escaped string"; };
    std::string emit(const ASTQuotedString& node) const override { return "quoted string"; };
    std::string emit(const ASTCompareOperator& node,
        const std::string& property, const std::string& value) const override;
    std::string emit(const ASTCompareExpression& node) const override { return "a >= b"; };
    std::string emit(const ASTStringOperator& node,
        const std::string& property, const std::string& value) const override;
    std::string emit(const ASTStringExpression& node) const override { return "a contains b"; };
    std::string emit(const ASTExistsOperator& node) const override { return "exists"; };
    std::string emit(const ASTExistsExpression& node) const override { return "a exists true"; };
    std::string emit(const ASTAndOperator& node, const std::string& lhs, const std::string& rhs) const override;
    std::string emit(const ASTOrOperator& node, const std::string& lhs, const std::string& rhs) const override;
};

class SearchParser {
public:
    SearchParser(const SQLEmitter& sqlEmitter, const std::string& input);
    std::shared_ptr<ASTNode> parse();

protected:
    void getNextToken();
    std::shared_ptr<ASTNode> parseSearchExpression();
    std::shared_ptr<ASTNode> parseRelationshipExpression();
    std::shared_ptr<ASTNode> parseParenthesis();
    std::shared_ptr<ASTQuotedString> parseQuotedString();
    void checkIsExpected(TokenType tokenType, const std::string& tokenTypeDescription);

private:
    std::shared_ptr<SearchToken> currentToken;
    std::shared_ptr<SearchLexer> lexer;
    const SQLEmitter& sqlEmitter;
};

using uvCdsObject = std::unique_ptr<std::vector<zmm::Ref<CdsObject>>>;
class SearchHandler {
public:
    inline SearchHandler(){};
    inline virtual ~SearchHandler(){};
    uvCdsObject executeSearch(SearchParam& searchParam);
};

#endif // __SEARCH_HANDLER_H__
