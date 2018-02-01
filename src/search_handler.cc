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

std::shared_ptr<ASTNode> SearchParser::parse()
{
    getNextToken();
    if (currentToken->getType()==TokenType::ASTERISK)
        return std::make_unique<ASTAsterisk>(sqlEmitter, currentToken->getValue());
    else
        return parseSearchExpression();
}

std::shared_ptr<ASTNode> SearchParser::parseSearchExpression()
{
    std::cout << "entered SearchParser::parseExpression()" << std::endl;
    
    std::stack<std::shared_ptr<ASTNode>> nodeStack;
    std::stack<TokenType> operatorStack;
    std::shared_ptr<ASTNode> root = nullptr;
    std::shared_ptr<ASTNode> expressionNode = nullptr;
    TokenType currentOperator = TokenType::INVALID;
    while (currentToken) {
        if (currentToken->getType() == TokenType::PROPERTY) {
            expressionNode = parseRelationshipExpression();
            if (currentOperator == TokenType::AND) {
                if (nodeStack.top() == nullptr)
                    throw _Exception(_("Cannot construct ASTAndOperator without lhs"));
                if (expressionNode == nullptr)
                    throw _Exception(_("Cannot construct ASTAndOperator without rhs"));
                std::shared_ptr<ASTNode> lhs(nodeStack.top());
                nodeStack.pop();
                nodeStack.push(std::make_shared<ASTAndOperator>(sqlEmitter, lhs, expressionNode));
                operatorStack.pop();
            } else if (currentOperator == TokenType::OR) {
                if (nodeStack.top() == nullptr)
                    throw _Exception(_("Cannot construct ASTOrOperator without lhs"));
                if (expressionNode == nullptr)
                     throw _Exception(_("Cannot construct ASTOrOperator without rhs"));
                std::shared_ptr<ASTNode> lhs(nodeStack.top());
                nodeStack.pop();
                nodeStack.push(std::make_shared<ASTOrOperator>(sqlEmitter, lhs, expressionNode));
                operatorStack.pop();
            } else {
                nodeStack.push(expressionNode);
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
        root = nodeStack.top();
        nodeStack.pop();
        if (!operatorStack.empty()) {
            currentOperator = operatorStack.top();
            operatorStack.pop();
            if (!nodeStack.empty()) {
                std::shared_ptr<ASTNode> lhs = nodeStack.top();
                nodeStack.pop();
                if (currentOperator == TokenType::AND)
                    root = std::make_shared<ASTAndOperator>(sqlEmitter, lhs, root);
                else
                    root = std::make_shared<ASTOrOperator>(sqlEmitter, lhs, root);
            } else
                throw _Exception(_("Cannot construct ASTOrOperator/ASTAndOperator without rhs"));
        }
    }
    return root;
}

std::shared_ptr<ASTNode> SearchParser::parseParenthesis()
{
    if (currentToken->getType() != TokenType::LPAREN)
        throw _Exception(_("Failed to parse search criteria - expecting a ')'"));

    std::shared_ptr<ASTNode> currentNode = nullptr;
    std::shared_ptr<ASTNode> lhsNode = nullptr;
    std::shared_ptr<ASTNode> rhsNode = nullptr;
    getNextToken();
    while (currentToken != nullptr && currentToken->getType() != TokenType::RPAREN) {
        // just call parseSearchExpression() at this point?
        if (currentToken->getType() == TokenType::PROPERTY) {
            currentNode = parseRelationshipExpression();
            getNextToken();
        } else if (currentToken->getType() == TokenType::AND || currentToken->getType() == TokenType::OR) {
            auto tokenType = currentToken->getType();
            lhsNode = currentNode;

            getNextToken();
            if (currentToken->getType() == TokenType::LPAREN)
                rhsNode = parseParenthesis();
            else
                rhsNode = parseRelationshipExpression();

            if (tokenType == TokenType::AND)
                currentNode = std::make_shared<ASTAndOperator>(sqlEmitter, lhsNode, rhsNode);
            else if (tokenType == TokenType::OR)
                currentNode = std::make_shared<ASTOrOperator>(sqlEmitter, lhsNode, rhsNode);
            else
                throw _Exception(_("Failed to parse search criteria - expected and/or"));
            
            getNextToken();
        } else if (currentToken->getType() == TokenType::LPAREN) {
            currentNode = parseParenthesis();
            getNextToken();
        }
    }
    if (currentNode == nullptr)
        throw _Exception(_("Failed to parse search criteria - bad expression between parenthesis"));
        
    return std::make_shared<ASTParenthesis>(sqlEmitter, currentNode);
}

std::shared_ptr<ASTNode> SearchParser::parseRelationshipExpression()
{
    if (currentToken->getType() != TokenType::PROPERTY)
        throw _Exception(_("Failed to parse search criteria - expecting a property name"));

    std::shared_ptr<ASTNode> relationshipExpr = nullptr;
    std::shared_ptr<ASTProperty> property = std::make_shared<ASTProperty>(sqlEmitter, currentToken->getValue());

    getNextToken();
    if (currentToken->getType() == TokenType::COMPAREOP) {
        std::shared_ptr<ASTCompareOperator> operatr = std::make_shared<ASTCompareOperator>(sqlEmitter,
            currentToken->getValue());
        getNextToken();
        std::shared_ptr<ASTQuotedString> quotedString = parseQuotedString();
        relationshipExpr = std::make_shared<ASTCompareExpression>(sqlEmitter, property, operatr, quotedString);
    } else if (currentToken->getType() == TokenType::STRINGOP) {
        std::shared_ptr<ASTStringOperator> operatr = std::make_shared<ASTStringOperator>(sqlEmitter,
            currentToken->getValue());
        getNextToken();
        std::shared_ptr<ASTQuotedString> quotedString = parseQuotedString();
        relationshipExpr = std::make_shared<ASTStringExpression>(sqlEmitter, property, operatr, quotedString);
    } else if (currentToken->getType() == TokenType::EXISTS) {
        std::shared_ptr<ASTExistsOperator> operatr = std::make_shared<ASTExistsOperator>(sqlEmitter,
            currentToken->getValue());
        getNextToken();
        std::shared_ptr<ASTBoolean> booleanValue = std::make_shared<ASTBoolean>(sqlEmitter, currentToken->getValue());
        relationshipExpr = std::make_shared<ASTExistsExpression>(sqlEmitter, property, operatr, booleanValue);
    } else
        throw _Exception(_("Failed to parse search criteria - expecting a comparison, exists, or string operator"));

    return relationshipExpr;
}

std::shared_ptr<ASTQuotedString> SearchParser::parseQuotedString()
{
    if (currentToken->getType() != TokenType::DQUOTE)
        throw _Exception(_("Failed to parse search criteria - expecting a double-quote"));
    std::shared_ptr<ASTDQuote> openQuote = std::make_shared<ASTDQuote>(sqlEmitter, currentToken->getValue());
    getNextToken();

    if (currentToken->getType() != TokenType::ESCAPEDSTRING)
        throw _Exception(_("Failed to parse search criteria - expecting an escaped string value"));

    std::shared_ptr<ASTEscapedString> escapedString = std::make_shared<ASTEscapedString>(sqlEmitter,
        currentToken->getValue());
    getNextToken();

    if (currentToken->getType() != TokenType::DQUOTE)
        throw _Exception(_("Failed to parse search criteria - expecting a double-quote"));
    std::shared_ptr<ASTDQuote> closeQuote = std::make_shared<ASTDQuote>(sqlEmitter, currentToken->getValue());
    // getNextToken();

    return std::make_shared<ASTQuotedString>(sqlEmitter, openQuote, escapedString, closeQuote);
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
    throw _Exception(_("Should not get here"));
}

std::string ASTStringOperator::emit(const std::string& property, const std::string& value)
{
    std::cout << "Emitting for ASTContainsOperator " << std::endl;
    return sqlEmitter.emit(*this, property, value);
}

std::string ASTStringExpression::emit()
{
    std::cout << "Emitting for ASTStringExpression "    << std::endl;
    std::string property = lhs->emit();
    std::string value = rhs->emit();
    return operatr->emit(property, value);
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

std::string SqliteEmitter::emit(const ASTStringOperator& node, const std::string& property,
    const std::string& value) const
{
    auto operatr = node.getValue();
    if (operatr != "contains")
        throw _Exception(_("operator not yet supported"));

    std::stringstream sqlFragment;
    if (*aslowercase(operatr)=="contains") {
        sqlFragment << "(lower(metadata) contains '"
                    << *aslowercase(value)
                    << "' and upnp_class is not null)";
    }
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

