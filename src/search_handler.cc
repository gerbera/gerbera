#include "search_handler.h"
#include <cctype>
#include <algorithm>

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

SearchParser::SearchParser(const std::string& searchCriteria)
    : lexer(std::make_unique<SearchLexer>(searchCriteria))
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
        return std::make_unique<ASTAsterisk>(currentToken->getValue());
    else
        return parseSearchExpression();
}

std::unique_ptr<ASTNode> SearchParser::parseSearchExpression()
{
    std::unique_ptr<ASTNode> root = nullptr;
    std::unique_ptr<ASTNode> currentNode = nullptr;
    std::unique_ptr<ASTNode> expressionNode = nullptr;
    std::unique_ptr<ASTNode> lhsNode = nullptr;
    std::unique_ptr<ASTNode> rhsNode = nullptr;
    TokenType currentOperator = TokenType::INVALID;
    while (currentToken) {
        if (currentToken->getType() == TokenType::PROPERTY) {
            expressionNode = parseRelationshipExpression();
            if (currentOperator == TokenType::AND) {
                currentNode = std::make_unique<ASTAndOperator>(std::move(lhsNode), std::move(expressionNode));
            } else if (currentOperator == TokenType::OR) {
                currentNode = std::make_unique<ASTOrOperator>(std::move(lhsNode), std::move(expressionNode));
            } else {
                currentNode = std::move(expressionNode);
            }
            currentOperator = TokenType::INVALID;
            getNextToken();
        } else if (currentToken->getType() == TokenType::LPAREN) {
            currentNode = parseParenthesis();
            getNextToken();
        } else if (currentToken->getType() == TokenType::AND || currentToken->getType() == TokenType::OR) {
            currentOperator = currentToken->getType();
            lhsNode = std::move(expressionNode);
            getNextToken();
        }
    }

    if (root == nullptr)
        root = std::move(currentNode);
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
                currentNode = std::make_unique<ASTAndOperator>(std::move(lhsNode), std::move(rhsNode));
            else
                currentNode = std::make_unique<ASTOrOperator>(std::move(lhsNode), std::move(rhsNode));

            getNextToken();
        } else if (currentToken->getType() == TokenType::LPAREN) {
            currentNode = parseParenthesis();
            getNextToken();
        }
    }
    if (currentNode == nullptr)
        throw _Exception(_("Failed to parse search criteria - bad expression between parenthesis"));
        
    return std::make_unique<ASTParenthesis>(std::move(currentNode));
}

std::unique_ptr<ASTNode> SearchParser::parseRelationshipExpression()
{
    if (currentToken->getType() != TokenType::PROPERTY)
        throw _Exception(_("Failed to parse search criteria - expecting a property name"));

    std::unique_ptr<ASTNode> relationshipExpr = nullptr;
    std::unique_ptr<ASTProperty> property = std::make_unique<ASTProperty>(currentToken->getValue());

    getNextToken();
    if (currentToken->getType() == TokenType::COMPAREOP) {
        std::unique_ptr<ASTCompareOperator> operatr = std::make_unique<ASTCompareOperator>(currentToken->getValue());
        getNextToken();
        std::unique_ptr<ASTQuotedString> quotedString = parseQuotedString();
        relationshipExpr = std::make_unique<ASTCompareExpression>(std::move(property), std::move(operatr),
            std::move(quotedString));
    } else if (currentToken->getType() == TokenType::STRINGOP) {
        std::unique_ptr<ASTStringOperator> operatr = std::make_unique<ASTStringOperator>(currentToken->getValue());
        getNextToken();
        std::unique_ptr<ASTQuotedString> quotedString = parseQuotedString();
        relationshipExpr = std::make_unique<ASTStringExpression>(std::move(property), std::move(operatr),
            std::move(quotedString));
    } else if (currentToken->getType() == TokenType::EXISTS) {
        std::unique_ptr<ASTExistsOperator> operatr = std::make_unique<ASTExistsOperator>(currentToken->getValue());
        getNextToken();
        std::unique_ptr<ASTBoolean> booleanValue = std::make_unique<ASTBoolean>(currentToken->getValue());
        relationshipExpr = std::make_unique<ASTExistsExpression>(std::move(property), std::move(operatr),
            std::move(booleanValue));
    } else
        throw _Exception(_("Failed to parse search criteria - expecting a comparison, exists, or string operator"));

    return relationshipExpr;
}

std::unique_ptr<ASTQuotedString> SearchParser::parseQuotedString()
{
    if (currentToken->getType() != TokenType::DQUOTE)
        throw _Exception(_("Failed to parse search criteria - expecting a double-quote"));
    std::unique_ptr<ASTDQuote> openQuote = std::make_unique<ASTDQuote>(currentToken->getValue());
    getNextToken();

    if (currentToken->getType() != TokenType::ESCAPEDSTRING)
        throw _Exception(_("Failed to parse search criteria - expecting an escaped string value")); 
    std::unique_ptr<ASTEscapedString> escapedString = std::make_unique<ASTEscapedString>(currentToken->getValue());
    getNextToken();
    
    if (currentToken->getType() != TokenType::DQUOTE)
        throw _Exception(_("Failed to parse search criteria - expecting a double-quote"));
    std::unique_ptr<ASTDQuote> closeQuote = std::make_unique<ASTDQuote>(currentToken->getValue());
    getNextToken();
    
    return std::make_unique<ASTQuotedString>(std::move(openQuote), std::move(escapedString), std::move(closeQuote));
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
