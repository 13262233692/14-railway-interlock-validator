#include "boolean/BooleanParser.h"
#include <cctype>
#include <sstream>

namespace railway {
namespace boolean {

BooleanParser::BooleanParser() : pos_(0), tokenPos_(0) {
}

std::shared_ptr<BooleanExpr> BooleanParser::parse(const std::string& expression) {
    expr_ = expression;
    pos_ = 0;
    tokenPos_ = 0;
    lastError_.clear();
    tokens_.clear();

    if (!tokenize(expression)) {
        return nullptr;
    }

    auto result = parseEquiv();

    if (!result) {
        if (lastError_.empty()) {
            lastError_ = "Parse error at position " + std::to_string(pos_);
        }
        return nullptr;
    }

    if (tokenPos_ < tokens_.size() && tokens_[tokenPos_].type != TokenType::END) {
        lastError_ = "Unexpected token at position " + std::to_string(tokens_[tokenPos_].position);
        return nullptr;
    }

    return result;
}

std::string BooleanParser::getLastError() const {
    return lastError_;
}

bool BooleanParser::tokenize(const std::string& expression) {
    pos_ = 0;
    tokens_.clear();

    while (pos_ < expression.size()) {
        skipWhitespace();
        if (pos_ >= expression.size()) {
            break;
        }

        if (!readToken()) {
            return false;
        }
    }

    tokens_.push_back({TokenType::END, "", pos_});
    return true;
}

void BooleanParser::skipWhitespace() {
    while (pos_ < expr_.size() && std::isspace(static_cast<unsigned char>(expr_[pos_]))) {
        pos_++;
    }
}

bool BooleanParser::readToken() {
    char c = expr_[pos_];
    size_t startPos = pos_;

    if (c == '(') {
        tokens_.push_back({TokenType::LPAREN, "(", startPos});
        pos_++;
        return true;
    }

    if (c == ')') {
        tokens_.push_back({TokenType::RPAREN, ")", startPos});
        pos_++;
        return true;
    }

    if (c == '!') {
        tokens_.push_back({TokenType::OP_NOT, "!", startPos});
        pos_++;
        return true;
    }

    if (c == '&') {
        tokens_.push_back({TokenType::OP_AND, "&", startPos});
        pos_++;
        return true;
    }

    if (c == '|') {
        tokens_.push_back({TokenType::OP_OR, "|", startPos});
        pos_++;
        return true;
    }

    if (c == '^') {
        tokens_.push_back({TokenType::OP_XOR, "^", startPos});
        pos_++;
        return true;
    }

    if (c == '-' && pos_ + 1 < expr_.size() && expr_[pos_ + 1] == '>') {
        tokens_.push_back({TokenType::OP_IMPLIES, "->", startPos});
        pos_ += 2;
        return true;
    }

    if (c == '<' && pos_ + 2 < expr_.size() && expr_[pos_ + 1] == '-' && expr_[pos_ + 2] == '>') {
        tokens_.push_back({TokenType::OP_EQUIV, "<->", startPos});
        pos_ += 3;
        return true;
    }

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        size_t nameStart = pos_;
        while (pos_ < expr_.size()) {
            char ch = expr_[pos_];
            if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '.') {
                pos_++;
            } else {
                break;
            }
        }

        std::string word = expr_.substr(nameStart, pos_ - nameStart);

        if (word == "true" || word == "false" || word == "TRUE" || word == "FALSE") {
            tokens_.push_back({TokenType::CONSTANT, word, startPos});
        } else if (word == "and" || word == "AND") {
            tokens_.push_back({TokenType::OP_AND, word, startPos});
        } else if (word == "or" || word == "OR") {
            tokens_.push_back({TokenType::OP_OR, word, startPos});
        } else if (word == "not" || word == "NOT") {
            tokens_.push_back({TokenType::OP_NOT, word, startPos});
        } else if (word == "xor" || word == "XOR") {
            tokens_.push_back({TokenType::OP_XOR, word, startPos});
        } else if (word == "implies" || word == "IMPLIES") {
            tokens_.push_back({TokenType::OP_IMPLIES, word, startPos});
        } else if (word == "equiv" || word == "EQUIV" || word == "eq") {
            tokens_.push_back({TokenType::OP_EQUIV, word, startPos});
        } else {
            tokens_.push_back({TokenType::VARIABLE, word, startPos});
        }
        return true;
    }

    lastError_ = "Unexpected character '" + std::string(1, c) + "' at position " + std::to_string(pos_);
    return false;
}

std::shared_ptr<BooleanExpr> BooleanParser::parseEquiv() {
    auto left = parseImplies();
    if (!left) return nullptr;

    while (match(TokenType::OP_EQUIV)) {
        auto right = parseImplies();
        if (!right) return nullptr;
        left = std::make_shared<EquivExpr>(left, right);
    }

    return left;
}

std::shared_ptr<BooleanExpr> BooleanParser::parseImplies() {
    auto left = parseOr();
    if (!left) return nullptr;

    while (match(TokenType::OP_IMPLIES)) {
        auto right = parseImplies();
        if (!right) return nullptr;
        left = std::make_shared<ImpliesExpr>(left, right);
    }

    return left;
}

std::shared_ptr<BooleanExpr> BooleanParser::parseOr() {
    auto left = parseXor();
    if (!left) return nullptr;

    while (match(TokenType::OP_OR)) {
        auto right = parseXor();
        if (!right) return nullptr;
        left = std::make_shared<OrExpr>(left, right);
    }

    return left;
}

std::shared_ptr<BooleanExpr> BooleanParser::parseXor() {
    auto left = parseAnd();
    if (!left) return nullptr;

    while (match(TokenType::OP_XOR)) {
        auto right = parseAnd();
        if (!right) return nullptr;
        left = std::make_shared<XorExpr>(left, right);
    }

    return left;
}

std::shared_ptr<BooleanExpr> BooleanParser::parseAnd() {
    auto left = parseUnary();
    if (!left) return nullptr;

    while (match(TokenType::OP_AND)) {
        auto right = parseUnary();
        if (!right) return nullptr;
        left = std::make_shared<AndExpr>(left, right);
    }

    return left;
}

std::shared_ptr<BooleanExpr> BooleanParser::parseUnary() {
    if (match(TokenType::OP_NOT)) {
        auto operand = parseUnary();
        if (!operand) return nullptr;
        return std::make_shared<NotExpr>(operand);
    }

    return parsePrimary();
}

std::shared_ptr<BooleanExpr> BooleanParser::parsePrimary() {
    const Token& tok = currentToken();

    if (tok.type == TokenType::CONSTANT) {
        bool value = (tok.value == "true" || tok.value == "TRUE");
        advance();
        return std::make_shared<ConstantExpr>(value);
    }

    if (tok.type == TokenType::VARIABLE) {
        std::string name = tok.value;
        advance();
        return std::make_shared<VariableExpr>(name);
    }

    if (tok.type == TokenType::LPAREN) {
        advance();
        auto expr = parseEquiv();
        if (!expr) return nullptr;
        if (!expect(TokenType::RPAREN)) {
            return nullptr;
        }
        return expr;
    }

    lastError_ = "Unexpected token '" + tok.value + "' at position " + std::to_string(tok.position);
    return nullptr;
}

const Token& BooleanParser::currentToken() {
    if (tokenPos_ < tokens_.size()) {
        return tokens_[tokenPos_];
    }
    static Token endToken = {TokenType::END, "", 0};
    return endToken;
}

const Token& BooleanParser::peekToken() {
    if (tokenPos_ + 1 < tokens_.size()) {
        return tokens_[tokenPos_ + 1];
    }
    static Token endToken = {TokenType::END, "", 0};
    return endToken;
}

void BooleanParser::advance() {
    if (tokenPos_ < tokens_.size()) {
        tokenPos_++;
    }
}

bool BooleanParser::match(TokenType type) {
    if (currentToken().type == type) {
        advance();
        return true;
    }
    return false;
}

bool BooleanParser::expect(TokenType type) {
    if (currentToken().type == type) {
        advance();
        return true;
    }
    lastError_ = "Expected token at position " + std::to_string(currentToken().position);
    return false;
}

} // namespace boolean
} // namespace railway
