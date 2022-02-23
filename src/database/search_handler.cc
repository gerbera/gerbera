/*GRB*
  Gerbera - https://gerbera.io/

  search_handler.cc - this file is part of Gerbera.

  Copyright (C) 2018-2022 Gerbera Contributors

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

/// \file search_handler.cc

#include "search_handler.h" // API

#include <algorithm>
#include <stack>

#include "database/sql_database.h"
#include "metadata/metadata_handler.h"
#include "util/tools.h"

static const std::unordered_map<std::string_view, TokenType> tokenTypes {
    { "(", TokenType::LPAREN },
    { ")", TokenType::RPAREN },
    { "*", TokenType::ASTERISK },
    { "\"", TokenType::DQUOTE },
    { "true", TokenType::BOOLVAL },
    { "false", TokenType::BOOLVAL },
    { "exists", TokenType::EXISTS },
    { "contains", TokenType::STRINGOP },
    { "doesnotcontain", TokenType::STRINGOP },
    { "derivedfrom", TokenType::STRINGOP },
    { "startswith", TokenType::STRINGOP },
    { "=", TokenType::COMPAREOP },
    { "!=", TokenType::COMPAREOP },
    { "<", TokenType::COMPAREOP },
    { "<=", TokenType::COMPAREOP },
    { ">", TokenType::COMPAREOP },
    { ">=", TokenType::COMPAREOP },
    { "and", TokenType::AND },
    { "or", TokenType::OR }
};

SearchLexer::SearchLexer(std::string input)
    : input(std::move(input))
{
}

std::unique_ptr<SearchToken> SearchLexer::nextToken()
{
    while (currentPos < input.length()) {
        char ch = input[currentPos];

        switch (ch) {
        case '(':
        case ')':
        case '*':
        case '=': {
            auto token = std::string(&ch, 1);
            TokenType tokenType = tokenTypes.at(token);
            ++currentPos;
            return std::make_unique<SearchToken>(tokenType, std::move(token));
        }
        case '>':
        case '<':
        case '!':
            if (input[currentPos + 1] == '=') {
                auto token = std::string(&ch, 1);
                token.push_back('=');
                TokenType tokenType = tokenTypes.at(token);
                currentPos += 2;
                return std::make_unique<SearchToken>(tokenType, std::move(token));
            } else {
                auto token = std::string(&ch, 1);
                TokenType tokenType = tokenTypes.at(token);
                ++currentPos;
                return std::make_unique<SearchToken>(tokenType, std::move(token));
            }
        case '"':
            if (!inQuotes) {
                auto token = std::string(&ch, 1);
                ++currentPos;
                inQuotes = true;
                return std::make_unique<SearchToken>(TokenType::DQUOTE, std::move(token));
            } else {
                auto token = std::string(&ch, 1);
                ++currentPos;
                inQuotes = false;
                return std::make_unique<SearchToken>(TokenType::DQUOTE, std::move(token));
            }
        default:
            if (inQuotes) {
                auto quotedStr = getQuotedValue(input);
                if (!quotedStr.empty())
                    return std::make_unique<SearchToken>(TokenType::ESCAPEDSTRING, std::move(quotedStr));
            }
            if (std::isspace(ch)) {
                ++currentPos;
            } else {
                auto tokenStr = nextStringToken(input);
                if (!tokenStr.empty()) {
                    auto token = makeToken(tokenStr);
                    if (!token->getValue().empty())
                        return token;
                }
            }
        }
    }
    return nullptr;
}

std::string SearchLexer::getQuotedValue(const std::string& input)
{
    std::string token;
    bool escaping = false;
    while (currentPos < input.length()) {
        auto ch = input[currentPos];
        if (ch == '"' && !escaping) {
            break;
        }
        if (!escaping && ch == '\\') {
            escaping = true;
        } else {
            token.push_back(ch);
            if (ch != '\\')
                escaping = false;
        }
        ++currentPos;
    }
    return token;
}

std::string SearchLexer::nextStringToken(const std::string& input)
{
    auto startPos = currentPos;
    while (currentPos < input.length()) {
        auto ch = input[currentPos];
        if (std::isalnum(ch) || ch == ':' || ch == '@' || ch == '.' || ch == '_')
            ++currentPos;
        else
            break;
    }
    if (startPos == currentPos)
        throw_std_runtime_error("String expected, found '{}' instead", input[currentPos]);
    return input.substr(startPos, currentPos - startPos);
}

std::unique_ptr<SearchToken> SearchLexer::makeToken(std::string tokenStr)
{
    auto itr = tokenTypes.find(toLower(tokenStr));
    if (itr != tokenTypes.end()) {
        return std::make_unique<SearchToken>(itr->second, std::move(tokenStr));
    }
    return std::make_unique<SearchToken>(TokenType::PROPERTY, std::move(tokenStr));
}

SearchParser::SearchParser(const SQLEmitter& sqlEmitter, const std::string& searchCriteria)
    : lexer(std::make_unique<SearchLexer>(searchCriteria))
    , sqlEmitter(sqlEmitter)
{
}

void SearchParser::getNextToken()
{
    currentToken = lexer->nextToken();
}

std::unique_ptr<ASTNode> SearchParser::parse()
{
    getNextToken();
    if (currentToken->getType() == TokenType::ASTERISK)
        return std::make_unique<ASTAsterisk>(sqlEmitter, currentToken->getValue());

    return parseSearchExpression();
}

std::unique_ptr<ASTNode> SearchParser::parseSearchExpression()
{
    std::stack<std::unique_ptr<ASTNode>> nodeStack;
    std::stack<TokenType> operatorStack;
    std::unique_ptr<ASTNode> root;
    TokenType currentOperator = TokenType::INVALID;
    while (currentToken) {
        if (currentToken->getType() == TokenType::PROPERTY) {
            auto expressionNode = parseRelationshipExpression();
            if (currentOperator == TokenType::AND) {
                if (!nodeStack.top())
                    throw_std_runtime_error("Cannot construct ASTAndOperator without lhs");
                if (!expressionNode)
                    throw_std_runtime_error("Cannot construct ASTAndOperator without rhs");
                auto lhs = std::move(nodeStack.top());
                nodeStack.pop();
                nodeStack.push(std::make_unique<ASTAndOperator>(sqlEmitter, std::move(lhs), std::move(expressionNode)));
                operatorStack.pop();
            } else if (currentOperator == TokenType::OR) {
                if (!nodeStack.top())
                    throw_std_runtime_error("Cannot construct ASTOrOperator without lhs");
                if (!expressionNode)
                    throw_std_runtime_error("Cannot construct ASTOrOperator without rhs");
                auto lhs = std::move(nodeStack.top());
                nodeStack.pop();
                nodeStack.push(std::make_unique<ASTOrOperator>(sqlEmitter, std::move(lhs), std::move(expressionNode)));
                operatorStack.pop();
            } else {
                nodeStack.push(std::move(expressionNode));
            }
            currentOperator = TokenType::INVALID;
            getNextToken();
        } else if (currentToken->getType() == TokenType::LPAREN) {
            nodeStack.push(parseParenthesis());
            getNextToken();
        } else if (currentToken->getType() == TokenType::AND || currentToken->getType() == TokenType::OR) {
            currentOperator = currentToken->getType();
            operatorStack.push(currentOperator);
            getNextToken();
        }
    }

    while (!nodeStack.empty()) {
        root = std::move(nodeStack.top());
        nodeStack.pop();
        if (!operatorStack.empty()) {
            currentOperator = operatorStack.top();
            operatorStack.pop();
            if (!nodeStack.empty()) {
                auto lhs = std::move(nodeStack.top());
                nodeStack.pop();
                if (currentOperator == TokenType::AND)
                    root = std::make_unique<ASTAndOperator>(sqlEmitter, std::move(lhs), std::move(root));
                else
                    root = std::make_unique<ASTOrOperator>(sqlEmitter, std::move(lhs), std::move(root));
            } else
                throw_std_runtime_error("Cannot construct ASTOrOperator/ASTAndOperator without rhs");
        }
    }
    return root;
}

std::unique_ptr<ASTNode> SearchParser::parseParenthesis()
{
    if (currentToken->getType() != TokenType::LPAREN)
        throw_std_runtime_error("Failed to parse search criteria - expecting a ')'");

    std::unique_ptr<ASTNode> currentNode;
    getNextToken();
    while (currentToken && currentToken->getType() != TokenType::RPAREN) {
        // just call parseSearchExpression() at this point?
        if (currentToken->getType() == TokenType::PROPERTY) {
            currentNode = parseRelationshipExpression();
            getNextToken();
        } else if (currentToken->getType() == TokenType::AND || currentToken->getType() == TokenType::OR) {
            auto tokenType = currentToken->getType();
            auto lhsNode = std::move(currentNode);

            getNextToken();
            auto rhsNode = (currentToken->getType() == TokenType::LPAREN) ? parseParenthesis() : parseRelationshipExpression();

            if (tokenType == TokenType::AND)
                currentNode = std::make_unique<ASTAndOperator>(sqlEmitter, std::move(lhsNode), std::move(rhsNode));
            else if (tokenType == TokenType::OR)
                currentNode = std::make_unique<ASTOrOperator>(sqlEmitter, std::move(lhsNode), std::move(rhsNode));
            else
                throw_std_runtime_error("Failed to parse search criteria - expected and/or");

            getNextToken();
        } else if (currentToken->getType() == TokenType::LPAREN) {
            currentNode = parseParenthesis();
            getNextToken();
        }
    }
    if (!currentNode)
        throw_std_runtime_error("Failed to parse search criteria - bad expression between parenthesis");

    return std::make_unique<ASTParenthesis>(sqlEmitter, std::move(currentNode));
}

std::unique_ptr<ASTNode> SearchParser::parseRelationshipExpression()
{
    if (currentToken->getType() != TokenType::PROPERTY)
        throw_std_runtime_error("Failed to parse search criteria - expecting a property name");

    auto property = std::make_unique<ASTProperty>(sqlEmitter, currentToken->getValue());

    getNextToken();
    if (currentToken->getType() == TokenType::COMPAREOP) {
        auto operatr = std::make_unique<ASTCompareOperator>(sqlEmitter, currentToken->getValue());
        getNextToken();
        auto quotedString = parseQuotedString();
        return std::make_unique<ASTCompareExpression>(sqlEmitter, std::move(property), std::move(operatr), std::move(quotedString));
    }

    if (currentToken->getType() == TokenType::STRINGOP) {
        auto operatr = std::make_unique<ASTStringOperator>(sqlEmitter, currentToken->getValue());
        getNextToken();
        auto quotedString = parseQuotedString();
        return std::make_unique<ASTStringExpression>(sqlEmitter, std::move(property), std::move(operatr), std::move(quotedString));
    }

    if (currentToken->getType() == TokenType::EXISTS) {
        auto operatr = std::make_unique<ASTExistsOperator>(sqlEmitter, currentToken->getValue());
        getNextToken();
        auto booleanValue = std::make_unique<ASTBoolean>(sqlEmitter, currentToken->getValue());
        return std::make_unique<ASTExistsExpression>(sqlEmitter, std::move(property), std::move(operatr), std::move(booleanValue));
    }

    throw_std_runtime_error("Failed to parse search criteria - expecting a comparison, exists, or string operator");
}

std::unique_ptr<ASTQuotedString> SearchParser::parseQuotedString()
{
    if (currentToken->getType() != TokenType::DQUOTE)
        throw_std_runtime_error("Failed to parse search criteria - expecting a double-quote");
    auto openQuote = std::make_unique<ASTDQuote>(sqlEmitter, currentToken->getValue());
    getNextToken();

    if (currentToken->getType() != TokenType::ESCAPEDSTRING)
        throw_std_runtime_error("Failed to parse search criteria - expecting an escaped string value");

    auto escapedString = std::make_unique<ASTEscapedString>(sqlEmitter, currentToken->getValue());
    getNextToken();

    if (currentToken->getType() != TokenType::DQUOTE)
        throw_std_runtime_error("Failed to parse search criteria - expecting a double-quote");
    auto closeQuote = std::make_unique<ASTDQuote>(sqlEmitter, currentToken->getValue());

    return std::make_unique<ASTQuotedString>(sqlEmitter, std::move(openQuote), std::move(escapedString), std::move(closeQuote));
}

std::string ASTNode::emitSQL() const
{
    return sqlEmitter.emitSQL(this);
}

ASTAsterisk::ASTAsterisk(const SQLEmitter& sqlEmitter, std::string value)
    : ASTNode(sqlEmitter)
    , value(std::move(value))
{
}

std::string ASTAsterisk::emit() const
{
    return sqlEmitter.emit(this);
}

ASTProperty::ASTProperty(const SQLEmitter& sqlEmitter, std::string value)
    : ASTNode(sqlEmitter)
    , value(std::move(value))
{
}

std::string ASTProperty::emit() const
{
    return value;
}

ASTBoolean::ASTBoolean(const SQLEmitter& sqlEmitter, std::string value)
    : ASTNode(sqlEmitter)
    , value(std::move(value))
{
}

std::string ASTBoolean::emit() const
{
    return value;
}

ASTParenthesis::ASTParenthesis(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTNode> node)
    : ASTNode(sqlEmitter)
    , bracketedNode(std::move(node))
{
}

std::string ASTParenthesis::emit() const
{
    return sqlEmitter.emit(this, bracketedNode->emit());
}

ASTDQuote::ASTDQuote(const SQLEmitter& sqlEmitter, std::string value)
    : ASTNode(sqlEmitter)
    , value(std::move(value))
{
}

std::string ASTDQuote::emit() const
{
    return sqlEmitter.emit(this);
}

ASTEscapedString::ASTEscapedString(const SQLEmitter& sqlEmitter, std::string value)
    : ASTNode(sqlEmitter)
    , value(std::move(value))
{
}

std::string ASTEscapedString::emit() const
{
    return value;
}

ASTQuotedString::ASTQuotedString(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTDQuote> openQuote,
    std::unique_ptr<ASTEscapedString> escapedString, std::unique_ptr<ASTDQuote> closeQuote)
    : ASTNode(sqlEmitter)
    , openQuote(std::move(openQuote))
    , escapedString(std::move(escapedString))
    , closeQuote(std::move(closeQuote))
{
}

std::string ASTQuotedString::emit() const
{
    return openQuote->emit() + escapedString->emit() + closeQuote->emit();
}

ASTCompareOperator::ASTCompareOperator(const SQLEmitter& sqlEmitter, std::string value)
    : ASTNode(sqlEmitter)
    , value(std::move(value))
{
}

std::string ASTCompareOperator::emit() const
{
    log_error("Emitting for ASTCompareOperator");
    throw_std_runtime_error("Should not get here");
}

std::string ASTCompareOperator::emit(const std::string& property, const std::string& value) const
{
    return sqlEmitter.emit(this, property, value);
}

ASTCompareExpression::ASTCompareExpression(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTProperty> lhs,
    std::unique_ptr<ASTCompareOperator> operatr, std::unique_ptr<ASTQuotedString> rhs)
    : ASTNode(sqlEmitter)
    , lhs(std::move(lhs))
    , operatr(std::move(operatr))
    , rhs(std::move(rhs))
{
}

std::string ASTCompareExpression::emit() const
{
    return operatr->emit(lhs->emit(), rhs->emit());
}

ASTStringOperator::ASTStringOperator(const SQLEmitter& sqlEmitter, std::string value)
    : ASTNode(sqlEmitter)
    , value(std::move(value))
{
}

std::string ASTStringOperator::emit() const
{
    log_error("Emitting for ASTStringOperator");
    throw_std_runtime_error("Should not get here");
}

std::string ASTStringOperator::emit(const std::string& property, const std::string& value) const
{
    return sqlEmitter.emit(this, property, value);
}

ASTStringExpression::ASTStringExpression(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTProperty> lhs,
    std::unique_ptr<ASTStringOperator> operatr, std::unique_ptr<ASTQuotedString> rhs)
    : ASTNode(sqlEmitter)
    , lhs(std::move(lhs))
    , operatr(std::move(operatr))
    , rhs(std::move(rhs))
{
}

std::string ASTStringExpression::emit() const
{
    return operatr->emit(lhs->emit(), rhs->emit());
}

ASTExistsOperator::ASTExistsOperator(const SQLEmitter& sqlEmitter, std::string value)
    : ASTNode(sqlEmitter)
    , value(std::move(value))
{
}

std::string ASTExistsOperator::emit() const
{
    log_error("Emitting for ASTExistsOperator");
    throw_std_runtime_error("Should not get here");
}

std::string ASTExistsOperator::emit(const std::string& property, const std::string& value) const
{
    return sqlEmitter.emit(this, property, value);
}

ASTExistsExpression::ASTExistsExpression(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTProperty> lhs,
    std::unique_ptr<ASTExistsOperator> operatr, std::unique_ptr<ASTBoolean> rhs)
    : ASTNode(sqlEmitter)
    , lhs(std::move(lhs))
    , operatr(std::move(operatr))
    , rhs(std::move(rhs))
{
}

std::string ASTExistsExpression::emit() const
{
    return operatr->emit(lhs->emit(), rhs->emit());
}

ASTAndOperator::ASTAndOperator(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTNode> lhs, std::unique_ptr<ASTNode> rhs)
    : ASTNode(sqlEmitter)
    , lhs(std::move(lhs))
    , rhs(std::move(rhs))
{
}
std::string ASTAndOperator::emit() const
{
    return sqlEmitter.emit(this, lhs->emit(), rhs->emit());
}

ASTOrOperator::ASTOrOperator(const SQLEmitter& sqlEmitter, std::unique_ptr<ASTNode> lhs, std::unique_ptr<ASTNode> rhs)
    : ASTNode(sqlEmitter)
    , lhs(std::move(lhs))
    , rhs(std::move(rhs))
{
}

std::string ASTOrOperator::emit() const
{
    return sqlEmitter.emit(this, lhs->emit(), rhs->emit());
}

DefaultSQLEmitter::DefaultSQLEmitter(std::shared_ptr<ColumnMapper> colMapper, std::shared_ptr<ColumnMapper> metaMapper, std::shared_ptr<ColumnMapper> resMapper)
    : colMapper(std::move(colMapper))
    , metaMapper(std::move(metaMapper))
    , resMapper(std::move(resMapper))
{
}

std::string DefaultSQLEmitter::emitSQL(const ASTNode* node) const
{
    std::string predicates = node->emit();
    if (!predicates.empty()) {
        return predicates;
    }
    throw_std_runtime_error("No SQL generated from AST");
}

std::string DefaultSQLEmitter::emit(const ASTParenthesis* node, const std::string& bracketedNode) const
{
    return fmt::format("({})", bracketedNode);
}

// format indexes:
//    0: class statement
//    1: property name statement
//    2: property statement lower case
//    3: value
static const std::map<std::string, std::string> logicOperator {
    { "contains", "({2} LIKE LOWER('%{3}%'))" }, // lower
    { "doesnotcontain", "({2} NOT LIKE LOWER('%{3}%'))" }, // lower
    { "startswith", "({2} LIKE LOWER('{3}%'))" }, // lower
    { "derivedfrom", "(LOWER({0}) LIKE LOWER('{3}%'))" },
    //{ "derivedfrom", "{0} LIKE LOWER('{3}%')" },
    { "exists", "({1} IS {3})" },
    { "@exists", "({1} IS {3})" },
    { "newer", "({1} {3})" },
    { "compare", "({2}LOWER('{3}'))" }, // lower
    { "@compare", "({2}LOWER('{3}'))" }, // lower
};

std::pair<std::string, std::string> DefaultSQLEmitter::getPropertyStatement(const std::string& property) const
{
    if (colMapper && colMapper->hasEntry(property)) {
        return { colMapper->mapQuoted(property), colMapper->mapQuotedLower(property) };
    }
    if (resMapper && resMapper->hasEntry(property)) {
        return { resMapper->mapQuoted(property), resMapper->mapQuotedLower(property) };
    }
    if (metaMapper) {
        return {
            fmt::format("{0}='{2}' AND {1}", metaMapper->mapQuoted(META_NAME), metaMapper->mapQuoted(META_VALUE), property),
            fmt::format("{0}='{2}' AND {1}", metaMapper->mapQuoted(META_NAME), metaMapper->mapQuotedLower(META_VALUE), property)
        };
    }
    log_info("Property {} not yet supported. Search may return no result!", property);
    return {};
}

std::string DefaultSQLEmitter::emit(const ASTCompareOperator* node, const std::string& property,
    const std::string& value) const
{
    auto operatr = node->getValue();
    auto [prpUpper, prpLower] = getPropertyStatement(property);

    if ((operatr == ">" || operatr == ">=") && startswith(value, "@last")) {
        auto dateVal = currentTime() - std::chrono::hours(24 * stoiString(value.substr(5)));
        return fmt::format(logicOperator.at("newer"), "", fmt::format("{} {}", prpUpper, operatr), fmt::format("{} {}", prpLower, operatr), dateVal.count());
    }

    if (operatr != "=")
        throw_std_runtime_error("Operator '{}' not yet supported", operatr);

    auto [clsUpper, clcLower] = getPropertyStatement(UPNP_SEARCH_CLASS);
    return fmt::format(logicOperator.at((property[0] == '@') ? "@compare" : "compare"), clsUpper,
        fmt::format("{}{}", prpUpper, operatr), fmt::format("{}{}", prpLower, operatr), value);
}

std::string DefaultSQLEmitter::emit(const ASTStringOperator* node, const std::string& property, const std::string& value) const
{
    auto stringOperator = toLower(node->getValue());
    if (logicOperator.find(stringOperator) == logicOperator.end()) {
        throw_std_runtime_error("Operation '{}' not yet supported", stringOperator);
    }
    auto [prpUpper, prpLower] = getPropertyStatement(property);
    auto [clsUpper, clsLower] = getPropertyStatement(UPNP_SEARCH_CLASS);
    return fmt::format(logicOperator.at(stringOperator), clsUpper, prpUpper, prpLower, value);
}

std::string DefaultSQLEmitter::emit(const ASTExistsOperator* node, const std::string& property, const std::string& value) const
{
    std::string exists;
    if (value == "true") {
        exists = "NOT NULL";
    } else if (value == "false") {
        exists = "NULL";
    } else {
        throw_std_runtime_error("Invalid value '{}' on rhs of 'exists' operator", value);
    }
    auto [prpUpper, prpLower] = getPropertyStatement(property);
    auto [clsUpper, clsLower] = getPropertyStatement(UPNP_SEARCH_CLASS);
    return fmt::format(logicOperator.at((property[0] == '@') ? "@exists" : "exists"), clsUpper, prpUpper, prpLower, exists);
}

std::string DefaultSQLEmitter::emit(const ASTAndOperator* node, const std::string& lhs,
    const std::string& rhs) const
{
    return fmt::format("{} AND {}", lhs, rhs);
}

std::string DefaultSQLEmitter::emit(const ASTOrOperator* node, const std::string& lhs,
    const std::string& rhs) const
{
    return fmt::format("{} OR {}", lhs, rhs);
}

SortParser::SortParser(std::shared_ptr<ColumnMapper> colMapper, std::shared_ptr<ColumnMapper> metaMapper, std::string sortCriteria)
    : colMapper(std::move(colMapper))
    , metaMapper(std::move(metaMapper))
    , sortCrit(std::move(sortCriteria))
{
}

std::string SortParser::parse(std::string& addColumns, std::string& addJoin)
{
    if (sortCrit.empty()) {
        return {};
    }
    std::vector<std::string> sort;
    std::vector<std::string> colBuf;
    std::vector<std::string> joinBuf;
    size_t cnt = 0;
    for (auto&& seg : splitString(sortCrit, ',')) {
        trimStringInPlace(seg);
        bool desc = (seg[0] == '-');
        if (seg[0] == '-' || seg[0] == '+') {
            seg = seg.substr(1);
        } else {
            log_warning("Unknown sort direction '{}' in '{}'", seg, sortCrit);
        }
        auto sortSql = colMapper ? colMapper->mapQuoted(seg) : "";
        if (!sortSql.empty()) {
            sort.push_back(fmt::format("{} {}", sortSql, (desc ? "DESC" : "ASC")));
        } else {
            for (auto&& metaId : MetadataIterator()) {
                auto&& metaName = MetadataHandler::getMetaFieldName(metaId);
                if (metaName == seg) {
                    sortSql = metaName;
                }
            }
            if (!sortSql.empty()) {
                log_debug("Sort by meta data '{}'", sortSql);
                auto metaAlias = fmt::format("meta_prop{}", cnt);
                colBuf.push_back(fmt::format("{}.{}", metaMapper->quote(metaAlias), metaMapper->mapQuoted(META_VALUE, true)));
                joinBuf.push_back(fmt::format("INNER JOIN {0} {1} ON {2} = {1}.{3} AND {1}.{4} = '{5}'",
                    metaMapper->getTableName(), metaMapper->quote(metaAlias),
                    colMapper->mapQuoted(UPNP_SEARCH_ID),
                    metaMapper->mapQuoted(UPNP_SEARCH_ID, true),
                    metaMapper->mapQuoted(META_NAME, true),
                    sortSql));

                sort.push_back(fmt::format("{}.{} {}", metaMapper->quote(metaAlias), metaMapper->mapQuoted(META_VALUE, true), (desc ? "DESC" : "ASC")));
                ++cnt;
            } else {
                log_warning("Unknown sort key '{}' in '{}'", seg, sortCrit);
            }
        }
    }
    if (!colBuf.empty()) {
        colBuf.emplace(colBuf.begin(), ""); // result starts with ,
    }
    addColumns = fmt::format("{}", fmt::join(colBuf, ", "));
    addJoin = fmt::format("{}", fmt::join(joinBuf, " "));
    return fmt::format("{}", fmt::join(sort, ", "));
}
