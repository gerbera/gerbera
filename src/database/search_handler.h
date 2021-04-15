/*GRB*
  Gerbera - https://gerbera.io/

  search_handler.h - this file is part of Gerbera.

  Copyright (C) 2018-2021 Gerbera Contributors

  Gerbera is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2
  as published by the Free Software Foundation.

  Gerbera is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

  $Id$
*/

/// \file search_handler.h
/// \brief Definitions of classes supporting implementation of UPnP search
/// Implements functionality defined in standardizeddcps\MediaServer_4 and MediaRenderer_3\UPnP-av-ContentDirectory-v4-Service-20150630.pdf
/// Document available in upnpresources.zip, download from here:
/// https://openconnectivity.org/developer/specifications/upnp-resources/upnp
#ifndef __SEARCH_HANDLER_H__
#define __SEARCH_HANDLER_H__

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class SearchParam;

enum class TokenType {
    ASTERISK,
    DQUOTE,
    ESCAPEDSTRING,
    PROPERTY,
    BOOLVAL,
    EXISTS,
    STRINGOP,
    COMPAREOP,
    AND,
    OR,
    LPAREN,
    RPAREN,
    INVALID
};

class SearchToken {
public:
    SearchToken(TokenType type, std::string value)
        : type(type)
        , value(std::move(value))
    {
    }
    virtual ~SearchToken() = default;

    const TokenType& getType() const { return type; }
    std::string getValue() const { return value; }

    friend bool operator==(const SearchToken& lhs, const SearchToken& rhs)
    {
        return lhs.type == rhs.type && lhs.value == rhs.value;
    }

protected:
    const TokenType type;
    std::string value;
};

class SearchLexer {
public:
    explicit SearchLexer(const std::string& input)
        : input(input)
        , currentPos(0)
        , inQuotes(false)
    {
    }
    virtual ~SearchLexer() = default;

    std::unique_ptr<SearchToken> nextToken();

    SearchLexer& operator=(const SearchLexer&) = delete;
    SearchLexer(const SearchLexer&) = delete;
    SearchLexer& operator=(const SearchLexer&&) = delete;
    SearchLexer(const SearchLexer&&) = delete;

protected:
    std::string nextStringToken(const std::string& input);
    static std::unique_ptr<SearchToken> makeToken(const std::string& tokenStr);
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
    std::string emitSQL() const;
    virtual std::string emit() const = 0;

    virtual ~ASTNode() = default;

protected:
    explicit ASTNode(const SQLEmitter& emitter)
        : sqlEmitter(emitter)
    {
    }

    const SQLEmitter& sqlEmitter;
};

class ASTAsterisk : public ASTNode {
public:
    ASTAsterisk(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value))
    {
    }
    std::string emit() const override;

protected:
    std::string value;
};

class ASTProperty : public ASTNode {
public:
    ASTProperty(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value))
    {
    }
    std::string emit() const override;

protected:
    std::string value;
};

class ASTBoolean : public ASTNode {
public:
    ASTBoolean(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value))
    {
    }
    std::string emit() const override;

protected:
    std::string value;
};

class ASTParenthesis : public ASTNode {
public:
    ASTParenthesis(const SQLEmitter& sqlEmitter, std::shared_ptr<ASTNode> node)
        : ASTNode(sqlEmitter)
        , bracketedNode(std::move(node))
    {
    }
    std::string emit() const override;

protected:
    std::shared_ptr<ASTNode> bracketedNode;
};

class ASTDQuote : public ASTNode {
public:
    ASTDQuote(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value))
    {
    }
    std::string emit() const override;

protected:
    std::string value;
};

class ASTEscapedString : public ASTNode {
public:
    ASTEscapedString(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value))
    {
    }
    std::string emit() const override;

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
    {
    }
    std::string emit() const override;

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
        , value(std::move(value))
    {
    }
    std::string emit() const override;
    std::string emit(const std::string& property, const std::string& value) const;
    std::string getValue() const { return value; }

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
    {
    }
    std::string emit() const override;

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
        , value(std::move(value))
    {
    }
    std::string emit() const override;
    std::string emit(const std::string& property, const std::string& value) const;
    std::string getValue() const { return value; }

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
    {
    }
    std::string emit() const override;

protected:
    std::shared_ptr<ASTProperty> lhs;
    std::shared_ptr<ASTStringOperator> operatr;
    std::shared_ptr<ASTQuotedString> rhs;
};

class ASTExistsOperator : public ASTNode {
public:
    ASTExistsOperator(const SQLEmitter& sqlEmitter, std::string value)
        : ASTNode(sqlEmitter)
        , value(std::move(value))
    {
    }
    std::string emit() const override;
    std::string emit(const std::string& property, const std::string& value) const;

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
    {
    }
    std::string emit() const override;

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
    {
    }
    std::string emit() const override;

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
    {
    }
    std::string emit() const override;

protected:
    std::shared_ptr<ASTNode> lhs;
    std::shared_ptr<ASTNode> rhs;
};

class SQLEmitter {
public:
    virtual std::string emitSQL(const ASTNode* node) const = 0;
    virtual std::string emit(const ASTAsterisk* node) const = 0;
    virtual std::string emit(const ASTParenthesis* node, const std::string& bracketedNode) const = 0;
    virtual std::string emit(const ASTDQuote* node) const = 0;
    virtual std::string emit(const ASTCompareOperator* node,
        const std::string& property, const std::string& value) const = 0;
    virtual std::string emit(const ASTStringOperator* node,
        const std::string& property, const std::string& value) const = 0;
    virtual std::string emit(const ASTExistsOperator* node,
        const std::string& property, const std::string& value) const = 0;
    virtual std::string emit(const ASTAndOperator* node,
        const std::string& lhs, const std::string& rhs) const = 0;
    virtual std::string emit(const ASTOrOperator* node,
        const std::string& lhs, const std::string& rhs) const = 0;

    virtual ~SQLEmitter() = default;
    virtual std::string tableQuote() const = 0;
};

class DefaultSQLEmitter : public SQLEmitter {
public:
    DefaultSQLEmitter(const std::string& tabQuote, const std::string& tableAlias, const std::string& metaAlias)
        : tabQuote(tabQuote)
        , tableAlias(tableAlias)
        , metaAlias(metaAlias)
    {
    }

    std::string emitSQL(const ASTNode* node) const override;
    std::string emit(const ASTAsterisk* node) const override { return "*"; }
    std::string emit(const ASTParenthesis* node, const std::string& bracketedNode) const override;
    std::string emit(const ASTDQuote* node) const override { return ""; }
    std::string emit(const ASTCompareOperator* node,
        const std::string& property, const std::string& value) const override;
    std::string emit(const ASTStringOperator* node,
        const std::string& property, const std::string& value) const override;
    std::string emit(const ASTExistsOperator* node,
        const std::string& property, const std::string& value) const override;
    std::string emit(const ASTAndOperator* node, const std::string& lhs, const std::string& rhs) const override;
    std::string emit(const ASTOrOperator* node, const std::string& lhs, const std::string& rhs) const override;

    std::string tableQuote() const override { return tabQuote; }

private:
    std::string tabQuote;
    std::string tableAlias;
    std::string metaAlias;

    std::pair<std::string, std::string> getPropertyStatement(const std::string& property) const;
};

class SearchParser {
public:
    SearchParser(const SQLEmitter& sqlEmitter, const std::string& searchCriteria)
        : lexer(std::make_unique<SearchLexer>(searchCriteria))
        , sqlEmitter(sqlEmitter)
    {
    }
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
#endif // __SEARCH_HANDLER_H__
