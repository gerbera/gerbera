#include "search_handler.h"
#include "tools.h"
#include <cctype>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stack>

static std::unordered_map<std::string, TokenType> tokenTypes {
    {"(", TokenType::LPAREN},
    {")", TokenType::RPAREN},
    {"*", TokenType::ASTERISK},
    {"\"", TokenType::DQUOTE},
    {"true", TokenType::BOOLVAL},
    {"false", TokenType::BOOLVAL},
    {"exists", TokenType::EXISTS},
    {"contains", TokenType::STRINGOP},
    {"doesnotcontain", TokenType::STRINGOP},
    {"derivedfrom", TokenType::STRINGOP},
    {"startswith", TokenType::STRINGOP},
    {"=", TokenType::COMPAREOP},
    {"!=", TokenType::COMPAREOP},
    {"<", TokenType::COMPAREOP},
    {"<=", TokenType::COMPAREOP},
    {">", TokenType::COMPAREOP},
    {">=", TokenType::COMPAREOP},
    {"and", TokenType::AND},
    {"or", TokenType::OR}
};

std::unique_ptr<std::string> aslowercase(const std::string& src)
{
    auto copy = std::make_unique<std::string>(src);
    std::transform(copy->begin(), copy->end(), copy->begin(), ::tolower);
    return copy;
}

std::unique_ptr<SearchToken> SearchLexer::nextToken()
{
    for (; currentPos < input.length(); ) {
        char ch = input[currentPos];

        switch (ch) {
        case '(':
        case ')':
        case '*':
        case '=':
	    {
                std::string token = std::string(&ch, 1);
                TokenType tokenType = tokenTypes.at(token);
                currentPos++;
                return std::make_unique<SearchToken>(tokenType, token);
            }
            break;
        case '>':
        case '<':
        case '!':
            if (input[currentPos+1]=='=') {
                std::string token = std::string(&ch, 1);
                token.push_back('=');
                TokenType tokenType = tokenTypes.at(token);
                currentPos += 2;
                return std::make_unique<SearchToken>(tokenType, token);
            } else {
                std::string token = std::string(&ch, 1);
                TokenType tokenType = tokenTypes.at(token);
                currentPos++;
                return std::make_unique<SearchToken>(tokenType, token);
            }
            break;
        case '"':
            if (!inQuotes) {
                std::string token = std::string(&ch, 1);
                currentPos++;
                inQuotes = true;
                return std::make_unique<SearchToken>(TokenType::DQUOTE, token);
            } else {
                std::string token = std::string(&ch, 1);
                currentPos++;
                inQuotes = false;
                return std::make_unique<SearchToken>(TokenType::DQUOTE, token);
            }
            break;
        default:
            if (inQuotes) {
                std::string quotedStr = getQuotedValue(input);
                if (quotedStr.length())
                    return std::make_unique<SearchToken>(TokenType::ESCAPEDSTRING, quotedStr);
            }
            if (std::isspace(ch)) {
                currentPos++;
            } else {
                auto tokenStr = nextStringToken(input);
                if (tokenStr.length()) {
                    std::unique_ptr<SearchToken> token = makeToken(tokenStr);
                    if (token->getValue().length())
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
    for (; currentPos < input.length(); ) {
        auto ch = input[currentPos];
        if (ch=='"' && !escaping) {
            break;
        }
        if (!escaping && ch=='\\') {
            escaping = true;
        } else {
            token.push_back(ch);
            if (ch!='\\')
                escaping = false;
        }
        currentPos++;
    }
    return token;
}

std::string SearchLexer::nextStringToken(const std::string& input)
{
    auto startPos = currentPos;
    for (; currentPos < input.length(); ) {
        auto ch = input[currentPos];
        if (std::isalnum(ch) || ch==':' || ch=='@' || ch=='.')
            currentPos++;
        else
            break;
    }
    return input.substr(startPos, currentPos - startPos);
}

std::unique_ptr<SearchToken> SearchLexer::makeToken(const std::string& tokenStr)
{
    auto itr = tokenTypes.find(*(aslowercase(tokenStr)));
    if (itr!=tokenTypes.end()) {
        return std::make_unique<SearchToken>(itr->second, tokenStr);
    } else {
        return std::make_unique<SearchToken>(TokenType::PROPERTY, tokenStr);
    }
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
    if (currentToken->getType()==TokenType::ASTERISK)
        return std::make_unique<ASTAsterisk>(sqlEmitter, currentToken->getValue());
    else
        return parseSearchExpression();
}

std::unique_ptr<ASTNode> SearchParser::parseSearchExpression()
{
    std::stack<std::unique_ptr<ASTNode>> nodeStack;
    std::unique_ptr<ASTNode> root = nullptr;
    std::unique_ptr<ASTNode> expressionNode = nullptr;
    TokenType currentOperator = TokenType::INVALID;
    while (currentToken) {
        if (currentToken->getType() == TokenType::PROPERTY) {
            expressionNode = parseRelationshipExpression();
            if (currentOperator == TokenType::AND) {
                if (nodeStack.top() == nullptr)
                    throw _Exception(_("Cannot construct ASTAndOperator without lhs"));
                if (expressionNode == nullptr)
                    throw _Exception(_("Cannot construct ASTAndOperator without rhs"));
                nodeStack.push(std::make_unique<ASTAndOperator>(sqlEmitter, std::move(nodeStack.top()), std::move(expressionNode)));
            } else if (currentOperator == TokenType::OR) {
                if (nodeStack.top() == nullptr)
                    throw _Exception(_("Cannot construct ASTOrOperator without lhs"));
                if (expressionNode == nullptr)
                     throw _Exception(_("Cannot construct ASTOrOperator without rhs"));
                nodeStack.push(std::make_unique<ASTOrOperator>(sqlEmitter, std::move(nodeStack.top()), std::move(expressionNode)));
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
            getNextToken();
        }
    }

    root = std::move(nodeStack.top());
    return root;
}

std::unique_ptr<ASTNode> SearchParser::parseParenthesis()
{
    if (currentToken->getType() != TokenType::LPAREN)
        throw _Exception(_("Failed to parse search criteria - expecting a ')'"));

    std::unique_ptr<ASTNode> currentNode = nullptr;
    std::unique_ptr<ASTNode> lhsNode = nullptr;
    std::unique_ptr<ASTNode> rhsNode = nullptr;
    getNextToken();
    while (currentToken != nullptr && currentToken->getType() != TokenType::RPAREN) {
        // just call parseSearchExpression() at this point?
        if (currentToken->getType() == TokenType::PROPERTY) {
            currentNode = parseRelationshipExpression();
            getNextToken();
        } else if (currentToken->getType() == TokenType::AND || currentToken->getType() == TokenType::OR) {
            auto tokenType = currentToken->getType();
            lhsNode = std::move(currentNode);

            getNextToken();
            if (currentToken->getType() == TokenType::LPAREN)
                rhsNode = parseParenthesis();
            else
                rhsNode = parseRelationshipExpression();

            if (tokenType == TokenType::AND)
                currentNode = std::make_unique<ASTAndOperator>(sqlEmitter, std::move(lhsNode), std::move(rhsNode));
            else
                currentNode = std::make_unique<ASTOrOperator>(sqlEmitter, std::move(lhsNode), std::move(rhsNode));

            getNextToken();
        } else if (currentToken->getType() == TokenType::LPAREN) {
            currentNode = parseParenthesis();
            getNextToken();
        }
    }
    if (currentNode == nullptr)
        throw _Exception(_("Failed to parse search criteria - bad expression between parenthesis"));
        
    return std::make_unique<ASTParenthesis>(sqlEmitter, std::move(currentNode));
}

std::unique_ptr<ASTNode> SearchParser::parseRelationshipExpression()
{
    if (currentToken->getType() != TokenType::PROPERTY)
        throw _Exception(_("Failed to parse search criteria - expecting a property name"));

    std::unique_ptr<ASTNode> relationshipExpr = nullptr;
    std::unique_ptr<ASTProperty> property = std::make_unique<ASTProperty>(sqlEmitter, currentToken->getValue());

    getNextToken();
    if (currentToken->getType() == TokenType::COMPAREOP) {
        std::unique_ptr<ASTCompareOperator> operatr = std::make_unique<ASTCompareOperator>(sqlEmitter,
            currentToken->getValue());
        getNextToken();
        std::unique_ptr<ASTQuotedString> quotedString = parseQuotedString();
        relationshipExpr = std::make_unique<ASTCompareExpression>(sqlEmitter, std::move(property),
            std::move(operatr), std::move(quotedString));
    } else if (currentToken->getType() == TokenType::STRINGOP) {
        std::unique_ptr<ASTStringOperator> operatr = std::make_unique<ASTStringOperator>(sqlEmitter,
            currentToken->getValue());
        getNextToken();
        std::unique_ptr<ASTQuotedString> quotedString = parseQuotedString();
        relationshipExpr = std::make_unique<ASTStringExpression>(sqlEmitter, std::move(property),
            std::move(operatr), std::move(quotedString));
    } else if (currentToken->getType() == TokenType::EXISTS) {
        std::unique_ptr<ASTExistsOperator> operatr = std::make_unique<ASTExistsOperator>(sqlEmitter,
            currentToken->getValue());
        getNextToken();
        std::unique_ptr<ASTBoolean> booleanValue = std::make_unique<ASTBoolean>(sqlEmitter, currentToken->getValue());
        relationshipExpr = std::make_unique<ASTExistsExpression>(sqlEmitter, std::move(property), std::move(operatr),
            std::move(booleanValue));
    } else
        throw _Exception(_("Failed to parse search criteria - expecting a comparison, exists, or string operator"));

    return relationshipExpr;
}

std::unique_ptr<ASTQuotedString> SearchParser::parseQuotedString()
{
    if (currentToken->getType() != TokenType::DQUOTE)
        throw _Exception(_("Failed to parse search criteria - expecting a double-quote"));
    std::unique_ptr<ASTDQuote> openQuote = std::make_unique<ASTDQuote>(sqlEmitter, currentToken->getValue());
    getNextToken();

    if (currentToken->getType() != TokenType::ESCAPEDSTRING)
        throw _Exception(_("Failed to parse search criteria - expecting an escaped string value"));

    std::unique_ptr<ASTEscapedString> escapedString = std::make_unique<ASTEscapedString>(sqlEmitter,
        currentToken->getValue());
    getNextToken();

    if (currentToken->getType() != TokenType::DQUOTE)
        throw _Exception(_("Failed to parse search criteria - expecting a double-quote"));
    std::unique_ptr<ASTDQuote> closeQuote = std::make_unique<ASTDQuote>(sqlEmitter, currentToken->getValue());
    // getNextToken();

    return std::make_unique<ASTQuotedString>(sqlEmitter, std::move(openQuote), std::move(escapedString),
        std::move(closeQuote));
}

void SearchParser::checkIsExpected(TokenType tokenType, const std::string& tokenTypeDescription)
{
    if (currentToken->getType() != tokenType) {
        std::string errorMsg(std::string("Failed to parse search criteria - expecting ") + tokenTypeDescription);
        throw _Exception(_(errorMsg.c_str()));
    }
}

uvCdsObject SearchHandler::executeSearch(SearchParam& searchParam)
{
    return nullptr;
}

std::string ASTAsterisk::emit()
{
    std::cout << "Emitting for ASTAsterisk "    << std::endl;
    return sqlEmitter.emit(*this);
}

std::string ASTProperty::emit()
{
    std::cout << "Emitting for ASTProperty "    << std::endl;
    return value;
}

std::string ASTBoolean::emit()
{
    std::cout << "Emitting for ASTBoolean "    << std::endl;
    return sqlEmitter.emit(*this);
}

std::string ASTParenthesis::emit()
{
    std::cout << "Emitting for ASTParenthesis "    << std::endl;
    return sqlEmitter.emit(*this, bracketedNode->emit());
}

std::string ASTDQuote::emit()
{
    std::cout << "Emitting for ASTDQuote "    << std::endl;
    return sqlEmitter.emit(*this);
}

std::string ASTEscapedString::emit()
{
    std::cout << "Emitting for ASTEscapedString "    << std::endl;
    return value;
}

std::string ASTQuotedString::emit()
{
    std::cout << "Emitting for ASTQuotedString "    << std::endl;
    return openQuote->emit() + escapedString->emit() + closeQuote->emit();
}

std::string ASTCompareOperator::emit()
{
    std::cout << "Emitting for ASTCompareOperator "    << std::endl;
    throw _Exception(_("Should not get here"));
}

std::string ASTCompareOperator::emit(const std::string& property, const std::string& value)
{
    std::cout << "Emitting for ASTCompareOperator " << std::endl;
    return sqlEmitter.emit(*this, property, value);
}

std::string ASTCompareExpression::emit()
{
    std::cout << "Emitting for ASTCompareExpression "    << std::endl;
    std::string property = lhs->emit();
    std::string value = rhs->emit();
    return operatr->emit(property, value);
}

std::string ASTStringOperator::emit()
{
    std::cout << "Emitting for ASTStringOperator "    << std::endl;
    return sqlEmitter.emit(*this);
}

std::string ASTStringExpression::emit()
{
    std::cout << "Emitting for ASTStringExpression "    << std::endl;
    return sqlEmitter.emit(*this);
}

std::string ASTExistsOperator::emit()
{
    std::cout << "Emitting for ASTExistsOperator "    << std::endl;
    return sqlEmitter.emit(*this);
}

std::string ASTExistsExpression::emit()
{
    std::cout << "Emitting for ASTExistsExpression "    << std::endl;
    return sqlEmitter.emit(*this);
}

std::string ASTAndOperator::emit()
{
    std::cout << "Emitting for ASTAndOperator "    << std::endl;
    return sqlEmitter.emit(*this, lhs->emit(), rhs->emit());
}

std::string ASTOrOperator::emit()
{
    std::cout << "Emitting for ASTOrOperator "    << std::endl;
    return sqlEmitter.emit(*this, lhs->emit(), rhs->emit());
}

std::string SqliteEmitter::emit(const ASTParenthesis& node, const std::string& bracketedNode) const {
    std::stringstream sqlFragment;
    sqlFragment << "(" << bracketedNode << ")";
    return sqlFragment.str();
}

std::string SqliteEmitter::emit(const ASTCompareOperator& node, const std::string& property,
    const std::string& value) const
{
    auto operatr = node.getValue();
    if (operatr != "=")
        throw _Exception(_("operator not yet supported"));

    std::stringstream sqlFragment;
    sqlFragment << "(instr(lower(metadata), lower('"
                << url_escape(property.c_str()).c_str()
                << operatr
                << url_escape(value.c_str()).c_str()
                << "')) > 0 and upnp_class is not null)";
    return sqlFragment.str();
}

std::string SqliteEmitter::emit(const ASTAndOperator& node, const std::string& lhs,
    const std::string& rhs) const
{
    std::stringstream sqlFragment;
    sqlFragment << lhs << " and " << rhs;
    return sqlFragment.str();
}

std::string SqliteEmitter::emit(const ASTOrOperator& node, const std::string& lhs,
    const std::string& rhs) const
{
    std::stringstream sqlFragment;
    sqlFragment << lhs << " or " << rhs;
    return sqlFragment.str();
}

