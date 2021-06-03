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

#include <algorithm>
#include <fmt/core.h>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define UPNP_SEARCH_CLASS "upnp:class"
#define UPNP_SEARCH_ID "@id"
#define UPNP_SEARCH_REFID "@refID"
#define UPNP_SEARCH_PARENTID "@parentID"
#define UPNP_SEARCH_PATH "path"
#define META_NAME "name"
#define META_VALUE "value"

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
    explicit SearchLexer(std::string input)
        : input(std::move(input))
    {
    }
    virtual ~SearchLexer() = default;

    std::unique_ptr<SearchToken> nextToken();

    SearchLexer& operator=(const SearchLexer&) = delete;
    SearchLexer(const SearchLexer&) = delete;

protected:
    std::string nextStringToken(const std::string& input);
    static std::unique_ptr<SearchToken> makeToken(const std::string& tokenStr);
    std::string getQuotedValue(const std::string& input);

    std::string input;
    unsigned currentPos {};
    bool inQuotes {};
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
};

class ColumnMapper {
public:
    virtual ~ColumnMapper() = default;
    virtual bool hasEntry(const std::string& tag) const = 0;
    virtual std::string tableQuoted() const = 0;
    virtual std::string mapQuoted(const std::string& tag) const = 0;
    virtual std::string mapQuotedLower(const std::string& tag) const = 0;
};

class DefaultSQLEmitter : public SQLEmitter {
public:
    DefaultSQLEmitter(std::shared_ptr<ColumnMapper> colMapper, std::shared_ptr<ColumnMapper> metaMapper)
        : colMapper(colMapper)
        , metaMapper(metaMapper)
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

private:
    std::shared_ptr<ColumnMapper> colMapper;
    std::shared_ptr<ColumnMapper> metaMapper;

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

template <class En>
class EnumColumnMapper : public ColumnMapper {
public:
    explicit EnumColumnMapper(const char tabQuoteBegin, const char tabQuoteEnd, std::string tableAlias, std::string tableName,
        const std::vector<std::pair<std::string, En>>& keyMap, const std::map<En, std::pair<std::string, std::string>>& colMap)
        : table_quote_begin(tabQuoteBegin)
        , table_quote_end(tabQuoteEnd)
        , tableAlias(std::move(tableAlias))
        , tableName(std::move(tableName))
        , keyMap(keyMap)
        , colMap(colMap)
    {
    }
    bool hasEntry(const std::string& tag) const override
    {
        return std::find_if(keyMap.begin(), keyMap.end(), [=](auto&& map) { return map.first == tag; }) != keyMap.end();
    }
    std::string mapQuoted(En tag) const
    {
        auto it = std::find_if(colMap.begin(), colMap.end(), [=](auto&& map) { return map.first == tag; });
        if (it != colMap.end()) {
            return fmt::format("{0}{1}{3}.{0}{2}{3}", table_quote_begin, colMap.at(tag).first, colMap.at(tag).second, table_quote_end);
        }
        return "";
    }

    std::string tableQuoted() const override
    {
        return fmt::format("{0}{1}{3} {0}{2}{3}", table_quote_begin, tableName, tableAlias, table_quote_end);
    }

    std::string mapQuoted(const std::string& tag) const override
    {
        auto it = std::find_if(keyMap.begin(), keyMap.end(), [=](auto&& map) { return map.first == tag; });
        if (it != keyMap.end()) {
            return fmt::format("{0}{1}{3}.{0}{2}{3}", table_quote_begin, colMap.at(it->second).first, colMap.at(it->second).second, table_quote_end);
        }
        return "";
    }
    std::string mapQuotedLower(const std::string& tag) const override
    {
        auto it = std::find_if(keyMap.begin(), keyMap.end(), [=](auto&& map) { return map.first == tag; });
        if (it != keyMap.end()) {
            return fmt::format("LOWER({0}{1}{3}.{0}{2}{3})", table_quote_begin, colMap.at(it->second).first, colMap.at(it->second).second, table_quote_end);
        }
        return "";
    }

private:
    const char table_quote_begin;
    const char table_quote_end;
    std::string tableAlias;
    std::string tableName;
    const std::vector<std::pair<std::string, En>>& keyMap;
    const std::map<En, std::pair<std::string, std::string>>& colMap;
};

class SortParser {
public:
    SortParser(std::shared_ptr<ColumnMapper> colMapper, const std::string& sortCriteria)
        : colMapper(std::move(colMapper))
        , sortCrit(sortCriteria)
    {
    }
    std::string parse();

private:
    std::shared_ptr<ColumnMapper> colMapper;
    std::string sortCrit;
};
#endif // __SEARCH_HANDLER_H__
