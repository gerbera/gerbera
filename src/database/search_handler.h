/*GRB*
  Gerbera - https://gerbera.io/

  search_handler.h - this file is part of Gerbera.

  Copyright (C) 2018-2025 Gerbera Contributors

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
/// Implements functionality defined in "standardizeddcps/MediaServer_4 and MediaRenderer_3/UPnP-av-ContentDirectory-v4-Service-20150630.pdf"
/// Document available in upnpresources.zip, download from here:
/// https://openconnectivity.org/developer/specifications/upnp-resources/upnp
#ifndef __SEARCH_HANDLER_H__
#define __SEARCH_HANDLER_H__

#include "upnp/upnp_common.h"
#include "util/tools.h"

#include <algorithm>
#include <fmt/core.h>
#include <map>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

#define UPNP_SEARCH_ID "@id"
#define UPNP_SEARCH_REFID "@refID"
#define UPNP_SEARCH_PARENTID "@parentID"
#define UPNP_SEARCH_LAST_UPDATED "last_updated"
#define UPNP_SEARCH_LAST_MODIFIED "last_modified"
#define UPNP_SEARCH_PLAY_GROUP "play_group"
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
    SearchToken(TokenType type, std::string&& value)
        : type(type)
        , value(std::move(value))
    {
    }

    const TokenType& getType() const { return type; }
    std::string getValue() const { return value; }

    friend bool operator==(const SearchToken& lhs, const SearchToken& rhs)
    {
        return lhs.type == rhs.type && lhs.value == rhs.value;
    }

protected:
    TokenType type;
    std::string value;
};

class SearchLexer {
public:
    explicit SearchLexer(std::string input);

    std::unique_ptr<SearchToken> nextToken();

protected:
    std::string nextStringToken(const std::string& input);
    static std::unique_ptr<SearchToken> makeToken(std::string tokenStr);
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
    virtual ~ASTNode() = default;

    std::string emitSQL() const;
    virtual std::string emit() const = 0;

protected:
    explicit ASTNode(const SQLEmitter& emitter)
        : sqlEmitter(emitter)
    {
    }

    const SQLEmitter& sqlEmitter;
};

class ASTAsterisk : public ASTNode {
public:
    ASTAsterisk(const SQLEmitter& sqlEmitter, std::string value);
    std::string emit() const override;

protected:
    std::string value;
};

class ASTProperty : public ASTNode {
public:
    ASTProperty(const SQLEmitter& sqlEmitter, std::string value);
    std::string emit() const override;

protected:
    std::string value;
};

class ASTBoolean : public ASTNode {
public:
    ASTBoolean(const SQLEmitter& sqlEmitter, std::string value);
    std::string emit() const override;

protected:
    std::string value;
};

class ASTParenthesis : public ASTNode {
public:
    ASTParenthesis(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTNode> node);
    std::string emit() const override;

protected:
    std::unique_ptr<ASTNode> bracketedNode;
};

class ASTDQuote : public ASTNode {
public:
    ASTDQuote(const SQLEmitter& sqlEmitter, std::string value);
    std::string emit() const override;

protected:
    std::string value;
};

class ASTEscapedString : public ASTNode {
public:
    ASTEscapedString(const SQLEmitter& sqlEmitter, std::string value);
    std::string emit() const override;

protected:
    std::string value;
};

class ASTQuotedString : public ASTNode {
public:
    ASTQuotedString(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTDQuote> openQuote,
        std::unique_ptr<ASTEscapedString> escapedString, std::unique_ptr<ASTDQuote> closeQuote);
    std::string emit() const override;

protected:
    std::unique_ptr<ASTDQuote> openQuote;
    std::unique_ptr<ASTEscapedString> escapedString;
    std::unique_ptr<ASTDQuote> closeQuote;
};

/// \brief Represents a comparison operator such as =, >=, <
class ASTCompareOperator : public ASTNode {
public:
    ASTCompareOperator(const SQLEmitter& sqlEmitter, std::string value);
    std::string emit() const override;
    std::string emit(const std::string& property, const std::string& value) const;
    std::string getValue() const { return value; }

protected:
    std::string value;
};

/// \brief Represents an expression using a comparison operator
class ASTCompareExpression : public ASTNode {
public:
    ASTCompareExpression(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTProperty> lhs,
        std::unique_ptr<ASTCompareOperator> operatr, std::unique_ptr<ASTQuotedString> rhs);
    std::string emit() const override;

protected:
    std::unique_ptr<ASTProperty> lhs;
    std::unique_ptr<ASTCompareOperator> operatr;
    std::unique_ptr<ASTQuotedString> rhs;
};

/// \brief Represents an operator defined by a string such as contains, derivedFrom
class ASTStringOperator : public ASTNode {
public:
    ASTStringOperator(const SQLEmitter& sqlEmitter, std::string value);
    std::string emit() const override;
    std::string emit(const std::string& property, const std::string& value) const;
    std::string getValue() const { return value; }

protected:
    std::string value;
};

/// \brief Represents an expression using an operator defined by a string
class ASTStringExpression : public ASTNode {
public:
    ASTStringExpression(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTProperty> lhs,
        std::unique_ptr<ASTStringOperator> operatr, std::unique_ptr<ASTQuotedString> rhs);
    std::string emit() const override;

protected:
    std::unique_ptr<ASTProperty> lhs;
    std::unique_ptr<ASTStringOperator> operatr;
    std::unique_ptr<ASTQuotedString> rhs;
};

class ASTExistsOperator : public ASTNode {
public:
    ASTExistsOperator(const SQLEmitter& sqlEmitter, std::string value);
    std::string emit() const override;
    std::string emit(const std::string& property, const std::string& value) const;

protected:
    std::string value;
};

/// \brief Represents an expression using the exists operator
class ASTExistsExpression : public ASTNode {
public:
    ASTExistsExpression(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTProperty> lhs,
        std::unique_ptr<ASTExistsOperator> operatr, std::unique_ptr<ASTBoolean> rhs);
    std::string emit() const override;

protected:
    std::unique_ptr<ASTProperty> lhs;
    std::unique_ptr<ASTExistsOperator> operatr;
    std::unique_ptr<ASTBoolean> rhs;
};

class ASTAndOperator : public ASTNode {
public:
    ASTAndOperator(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTNode> lhs, std::unique_ptr<ASTNode> rhs);
    std::string emit() const override;

protected:
    std::unique_ptr<ASTNode> lhs;
    std::unique_ptr<ASTNode> rhs;
};

class ASTOrOperator : public ASTNode {
public:
    ASTOrOperator(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTNode> lhs, std::unique_ptr<ASTNode> rhs);
    std::string emit() const override;

protected:
    std::unique_ptr<ASTNode> lhs;
    std::unique_ptr<ASTNode> rhs;
};

class SQLEmitter {
public:
    virtual ~SQLEmitter() = default;
    virtual std::string emitSQL(const ASTNode* node) const = 0;
    virtual std::string emit(const ASTAsterisk* node) const = 0;
    virtual std::string emit(const ASTParenthesis* node,
        const std::string& bracketedNode) const
        = 0;
    virtual std::string emit(const ASTDQuote* node) const = 0;
    virtual std::string emit(const ASTCompareOperator* node,
        const std::string& property, const std::string& value) const
        = 0;
    virtual std::string emit(const ASTStringOperator* node,
        const std::string& property, const std::string& value) const
        = 0;
    virtual std::string emit(const ASTExistsOperator* node,
        const std::string& property, const std::string& value) const
        = 0;
    virtual std::string emit(const ASTAndOperator* node,
        const std::string& lhs, const std::string& rhs) const
        = 0;
    virtual std::string emit(const ASTOrOperator* node,
        const std::string& lhs, const std::string& rhs) const
        = 0;
};

enum class FieldType {
    String,
    Integer,
    Bool,
    Date,
};

class ColumnMapper {
public:
    virtual ~ColumnMapper() = default;
    virtual bool hasEntry(const std::string& tag) const = 0;
    virtual std::string getTableName() const = 0;
    virtual std::string tableQuoted() const = 0;
    virtual std::string mapQuoted(const std::string& tag, bool noAlias = false) const = 0;
    virtual bool mapQuotedList(std::vector<std::string>& sort, const std::string& tag, const std::string& desc) const = 0;
    virtual std::string mapQuotedLower(const std::string& tag) const = 0;
    virtual FieldType getFieldType(const std::string& tag) const = 0;
    virtual std::string quote(const std::string& tag) const = 0;
};

struct SearchProperty {
    std::string alias;
    std::string field;
    FieldType type;

    std::string print(char table_quote_begin, char table_quote_end, bool noAlias) const
    {
        if (alias.empty() || noAlias) // no column alias
            return fmt::format("{}{}{}", table_quote_begin, field, table_quote_end);
        return fmt::format("{0}{1}{3}.{0}{2}{3}", table_quote_begin, alias, field, table_quote_end);
    }

    std::string convert(char table_quote_begin, char table_quote_end) const
    {
        if (alias.empty()) // no column
            return fmt::format("LOWER({}{}{})", table_quote_begin, field, table_quote_end);
        return fmt::format("LOWER({0}{1}{3}.{0}{2}{3})", table_quote_begin, alias, field, table_quote_end);
    }
};

class DefaultSQLEmitter : public SQLEmitter {
public:
    DefaultSQLEmitter(std::shared_ptr<ColumnMapper> colMapper,
        std::shared_ptr<ColumnMapper> metaMapper,
        std::shared_ptr<ColumnMapper> resMapper,
        std::shared_ptr<ColumnMapper> plyMapper);

    std::string emitSQL(const ASTNode* node) const override;
    std::string emit(const ASTAsterisk* node) const override { return {}; }
    std::string emit(const ASTParenthesis* node, const std::string& bracketedNode) const override;
    std::string emit(const ASTDQuote* node) const override { return {}; }
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
    std::shared_ptr<ColumnMapper> resMapper;
    std::shared_ptr<ColumnMapper> plyMapper;

    std::tuple<std::string, std::string, FieldType> getPropertyStatement(const std::string& property) const;
};

class SearchParser {
public:
    SearchParser(const SQLEmitter& sqlEmitter, const std::string& searchCriteria);
    std::unique_ptr<ASTNode> parse();

protected:
    void getNextToken();
    std::unique_ptr<ASTNode> parseSearchExpression();
    std::unique_ptr<ASTNode> parseRelationshipExpression();
    std::unique_ptr<ASTNode> parseParenthesis();
    std::unique_ptr<ASTQuotedString> parseQuotedString();

private:
    std::unique_ptr<SearchToken> currentToken;
    std::unique_ptr<SearchLexer> lexer;
    const SQLEmitter& sqlEmitter;
};

template <class En>
class EnumColumnMapper : public ColumnMapper {
public:
    explicit EnumColumnMapper(const char tabQuoteBegin, const char tabQuoteEnd, std::string tableAlias, std::string tableName,
        std::vector<std::pair<std::string, En>> keyMap, std::map<En, SearchProperty> colMap)
        : table_quote_begin(tabQuoteBegin)
        , table_quote_end(tabQuoteEnd)
        , tableAlias(std::move(tableAlias))
        , tableName(std::move(tableName))
        , keyMap(std::move(keyMap))
        , colMap(std::move(colMap))
    {
    }
    bool hasEntry(const std::string& tag) const override
    {
        return std::any_of(keyMap.begin(), keyMap.end(), [=](auto&& map) { return map.first == tag; });
    }
    std::string getTableName() const override
    {
        return fmt::format("{}{}{}", table_quote_begin, tableName, table_quote_end);
    }

    std::string getAlias() const
    {
        return fmt::format("{}{}{}", table_quote_begin, tableAlias, table_quote_end);
    }

    std::string mapQuoted(En tag) const
    {
        auto it = std::find_if(colMap.begin(), colMap.end(), [=](auto&& map) { return map.first == tag; });
        if (it != colMap.end()) {
            return colMap.at(tag).print(table_quote_begin, table_quote_end, false);
        }
        return {};
    }

    std::string tableQuoted() const override
    {
        return fmt::format("{0}{1}{3} {0}{2}{3}", table_quote_begin, tableName, tableAlias, table_quote_end);
    }

    bool mapQuotedList(std::vector<std::string>& sort, const std::string& tag, const std::string& desc) const override
    {
        bool result = false;
        for (auto&& [key, value] : keyMap) {
            if (key == tag) {
                result = true;
                auto sortCol = colMap.at(value).print(table_quote_begin, table_quote_end, false);
                auto it = std::find_if(sort.begin(), sort.end(), [=](auto&& col) { return startswith(col, sortCol); });
                if (it == sort.end())
                    sort.push_back(fmt::format("{} {}", sortCol, desc));
            }
        }
        return result;
    }

    std::string mapQuoted(const std::string& tag, bool noAlias = false) const override
    {
        auto it = std::find_if(keyMap.begin(), keyMap.end(), [=](auto&& map) { return map.first == tag; });
        if (it != keyMap.end()) {
            return colMap.at(it->second).print(table_quote_begin, table_quote_end, noAlias);
        }
        return {};
    }

    std::string quote(const std::string& tag) const override
    {
        if (!tag.empty()) {
            return fmt::format("{0}{1}{2}", table_quote_begin, tag, table_quote_end);
        }
        return {};
    }

    std::string mapQuotedLower(const std::string& tag) const override
    {
        auto it = std::find_if(keyMap.begin(), keyMap.end(), [=](auto&& map) { return map.first == tag; });
        if (it != keyMap.end()) {
            return colMap.at(it->second).convert(table_quote_begin, table_quote_end);
        }
        return {};
    }

    FieldType getFieldType(const std::string& tag) const override
    {
        auto it = std::find_if(keyMap.begin(), keyMap.end(), [=](auto&& map) { return map.first == tag; });
        if (it != keyMap.end()) {
            return colMap.at(it->second).type;
        }
        return FieldType::String;
    }

private:
    const char table_quote_begin;
    const char table_quote_end;
    std::string tableAlias;
    std::string tableName;
    std::vector<std::pair<std::string, En>> keyMap;
    std::map<En, SearchProperty> colMap;
};

class SortParser {
public:
    SortParser(std::shared_ptr<ColumnMapper> colMapper, std::shared_ptr<ColumnMapper> plyMapper, std::shared_ptr<ColumnMapper> metaMapper, std::string sortCriteria);
    std::string parse(std::string& addColumns, std::string& addJoin);

private:
    std::shared_ptr<ColumnMapper> colMapper;
    std::shared_ptr<ColumnMapper> plyMapper;
    std::shared_ptr<ColumnMapper> metaMapper;
    std::string sortCrit;
};
#endif // __SEARCH_HANDLER_H__
