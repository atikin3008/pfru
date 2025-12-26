#pragma once

#include "Token.h"
#include <string>
#include <vector>

class Lexer {
 public:
    Lexer(const std::string &program);
    bool parseProgram();
    const std::vector<Token> &tokens() const;

 private:
    struct Utf8Char {
        uint32_t codepoint{};
        size_t length{};
    };

    bool decodeUtf8(size_t index, Utf8Char &out) const;
    void reset();
    void skipWhitespace();
    bool matchLiteral(const std::string &literal, bool skipSpace = true);
    bool matchKeyword(const std::string &keyword);
    bool isKeyword(const std::string &word) const;
    bool isIdentifierChar(size_t index, size_t &advance);
    void emitToken(TOKEN_TYPE type, int startIndex, int startRow, int startCol,
                   int endIndex = -1);

    bool peek(const std::string &symbols);

    char get();

    bool next();

    bool letter();
    bool ruLetter();
    bool digit();
    bool any();
    bool identifier();

    bool integerLiteral();
    bool floatLiteral();
    bool charLiteral();
    bool stringLiteral();
    bool boolLiteral();

    bool space();
    bool newline();

    bool primitiveType();
    bool arrayType();
    bool type();

    bool block();
    bool statement();
    bool varDecl();
    bool assignment();

    bool ifStmt();
    bool whileStmt();
    bool doWhileStmt();
    bool forStmt();
    bool range();
    bool returnStmt();

    bool expr();
    bool commaExpr();
    bool logicOr();
    bool logicAnd();
    bool bitOr();
    bool bitXor();
    bool bitAnd();
    bool equality();
    bool rel();
    bool shift();
    bool add();
    bool mul();
    bool unary();

    bool primary();
    bool callExpr();
    bool argList();
    bool arrayLiteral();
    bool literal();

    bool program();
    bool topLevelDecl();

    bool reprFunc();
    bool paramList();
    bool param();
    bool returnTypeList();

    bool arrowBlock();
    bool arrowLine();
    bool arrowNode();
    bool arrowOp();
    bool literalList();

    std::string program_;
    std::vector<Token> tokens_;
    int symbolIndex_, row_, lastNewLineIndex_;
};
