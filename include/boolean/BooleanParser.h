#ifndef RAILWAY_BOOLEAN_PARSER_H
#define RAILWAY_BOOLEAN_PARSER_H

#include "boolean/BooleanExpr.h"
#include <memory>
#include <string>
#include <vector>

namespace railway {
namespace boolean {

enum class TokenType {
    CONSTANT,
    VARIABLE,
    OP_NOT,
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_IMPLIES,
    OP_EQUIV,
    LPAREN,
    RPAREN,
    END
};

struct Token {
    TokenType type;
    std::string value;
    size_t position;
};

class BooleanParser {
public:
    BooleanParser();

    std::shared_ptr<BooleanExpr> parse(const std::string& expression);

    std::string getLastError() const;

private:
    bool tokenize(const std::string& expression);
    void skipWhitespace();
    bool readToken();

    std::shared_ptr<BooleanExpr> parseEquiv();
    std::shared_ptr<BooleanExpr> parseImplies();
    std::shared_ptr<BooleanExpr> parseOr();
    std::shared_ptr<BooleanExpr> parseXor();
    std::shared_ptr<BooleanExpr> parseAnd();
    std::shared_ptr<BooleanExpr> parseUnary();
    std::shared_ptr<BooleanExpr> parsePrimary();

    const Token& currentToken();
    const Token& peekToken();
    void advance();
    bool match(TokenType type);
    bool expect(TokenType type);

    std::string expr_;
    size_t pos_;
    std::vector<Token> tokens_;
    size_t tokenPos_;
    std::string lastError_;
};

} // namespace boolean
} // namespace railway

#endif // RAILWAY_BOOLEAN_PARSER_H
