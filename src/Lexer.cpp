#include "../include/Lexer.h"

#include <cctype>
#include <tuple>
#include <unordered_set>

namespace {
bool isAsciiLetter(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool isDigitChar(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

const std::unordered_set<std::string> kKeywords = {
    "if",   "elif",  "while",  "do",    "for",   "in",    "return", "repr",
    "true", "false", "start",  "end",   "i8",    "i16",   "i32",    "i64",
    "f32",  "f64",   "char",   "stringa", "bool"};
}  // namespace

Lexer::Lexer(const std::string &program)
    : program_(program), symbolIndex_(0), row_(1), lastNewLineIndex_(-1) {}

void Lexer::reset() {
    symbolIndex_ = 0;
    row_ = 1;
    lastNewLineIndex_ = -1;
    tokens_.clear();
}

bool Lexer::parseProgram() {
    reset();
    return program();
}

const std::vector<Token> &Lexer::tokens() const { return tokens_; }

bool Lexer::decodeUtf8(size_t index, Utf8Char &out) const {
    if (index >= program_.size()) return false;
    unsigned char c = static_cast<unsigned char>(program_[index]);
    if ((c & 0x80) == 0) {
        out.codepoint = c;
        out.length = 1;
        return true;
    }
    if ((c & 0xE0) == 0xC0) {
        if (index + 1 >= program_.size()) return false;
        unsigned char c1 = static_cast<unsigned char>(program_[index + 1]);
        if ((c1 & 0xC0) != 0x80) return false;
        out.codepoint = ((c & 0x1F) << 6) | (c1 & 0x3F);
        out.length = 2;
        return true;
    }
    if ((c & 0xF0) == 0xE0) {
        if (index + 2 >= program_.size()) return false;
        unsigned char c1 = static_cast<unsigned char>(program_[index + 1]);
        unsigned char c2 = static_cast<unsigned char>(program_[index + 2]);
        if (((c1 & 0xC0) != 0x80) || ((c2 & 0xC0) != 0x80)) return false;
        out.codepoint = ((c & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
        out.length = 3;
        return true;
    }
    if ((c & 0xF8) == 0xF0) {
        if (index + 3 >= program_.size()) return false;
        unsigned char c1 = static_cast<unsigned char>(program_[index + 1]);
        unsigned char c2 = static_cast<unsigned char>(program_[index + 2]);
        unsigned char c3 = static_cast<unsigned char>(program_[index + 3]);
        if (((c1 & 0xC0) != 0x80) || ((c2 & 0xC0) != 0x80) ||
            ((c3 & 0xC0) != 0x80)) {
            return false;
        }
        out.codepoint = ((c & 0x07) << 18) | ((c1 & 0x3F) << 12) |
                        ((c2 & 0x3F) << 6) | (c3 & 0x3F);
        out.length = 4;
        return true;
    }
    return false;
}

void Lexer::skipWhitespace() {
    while (symbolIndex_ < static_cast<int>(program_.size())) {
        char c = program_[symbolIndex_];
        if (c == ' ' || c == '\t') {
            next();
            continue;
        }
        if (c == '\r') {
            next();
            if (symbolIndex_ < static_cast<int>(program_.size()) &&
                program_[symbolIndex_] == '\n') {
                next();
            }
            continue;
        }
        if (c == '\n') {
            next();
            continue;
        }
        break;
    }
}

bool Lexer::matchLiteral(const std::string &literal, bool skipSpace) {
    int originalIndex = symbolIndex_;
    int originalRow = row_;
    int originalLast = lastNewLineIndex_;
    if (skipSpace) {
        skipWhitespace();
    }
    if (program_.compare(symbolIndex_, literal.size(), literal) != 0) {
        symbolIndex_ = originalIndex;
        row_ = originalRow;
        lastNewLineIndex_ = originalLast;
        return false;
    }
    for (size_t i = 0; i < literal.size(); ++i) {
        next();
    }
    return true;
}

bool Lexer::matchKeyword(const std::string &keyword) {
    int originalIndex = symbolIndex_;
    int originalRow = row_;
    int originalLast = lastNewLineIndex_;
    if (!matchLiteral(keyword, true)) return false;
    size_t nextIndex = symbolIndex_;
    size_t adv = 0;
    if (isIdentifierChar(nextIndex, adv)) {
        symbolIndex_ = originalIndex;
        row_ = originalRow;
        lastNewLineIndex_ = originalLast;
        return false;
    }
    return true;
}

bool Lexer::isKeyword(const std::string &word) const {
    return kKeywords.count(word) > 0;
}

bool Lexer::isIdentifierChar(size_t index, size_t &advance) {
    Utf8Char ch{};
    if (!decodeUtf8(index, ch)) return false;
    uint32_t cp = ch.codepoint;
    if (cp < 128) {
        char small = static_cast<char>(cp);
        if (isAsciiLetter(small) || isDigitChar(small)) {
            advance = ch.length;
            return true;
        }
    }
    if ((cp >= 0x410 && cp <= 0x42F) || (cp >= 0x430 && cp <= 0x44F) ||
        cp == 0x5F) {  // А-Я, а-я, "_"
        advance = ch.length;
        return true;
    }
    return false;
}

void Lexer::emitToken(TOKEN_TYPE type, int startIndex, int startRow, int startCol,
                      int endIndex) {
    if (endIndex < 0) endIndex = symbolIndex_;
    if (endIndex < startIndex) endIndex = startIndex;
    tokens_.push_back({type,
                       static_cast<uint32_t>(startRow),
                       static_cast<uint32_t>(startCol),
                       program_.substr(startIndex, endIndex - startIndex)});
}

bool Lexer::peek(const std::string &symbols) {
    if (symbolIndex_ >= static_cast<int>(program_.size())) return false;
    return symbols.find(program_[symbolIndex_]) != std::string::npos;
}

char Lexer::get() {
    if (symbolIndex_ >= static_cast<int>(program_.size())) return '\0';
    return program_[symbolIndex_];
}

bool Lexer::next() {
    if (symbolIndex_ >= static_cast<int>(program_.size())) return false;
    char c = program_[symbolIndex_];
    ++symbolIndex_;
    if (c == '\n' || c == '\r') {
        ++row_;
        lastNewLineIndex_ = symbolIndex_ - 1;
    }
    return true;
}

bool Lexer::letter() {
    if (symbolIndex_ >= static_cast<int>(program_.size())) return false;
    char c = get();
    if (isAsciiLetter(c)) {
        int startIndex = symbolIndex_;
        int startRow = row_;
        int startCol = startIndex - lastNewLineIndex_;
        next();
        emitToken(LETTER, startIndex, startRow, startCol);
        return true;
    }
    return false;
}

bool Lexer::ruLetter() {
    Utf8Char ch{};
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!decodeUtf8(symbolIndex_, ch)) return false;
    uint32_t cp = ch.codepoint;
    if ((cp >= 0x410 && cp <= 0x42F) || (cp >= 0x430 && cp <= 0x44F) ||
        cp == 0x5F) {
        for (size_t i = 0; i < ch.length; ++i) next();
        emitToken(RU_LETTER, startIndex, startRow, startCol);
        return true;
    }
    return false;
}

bool Lexer::digit() {
    if (symbolIndex_ >= static_cast<int>(program_.size())) return false;
    char c = get();
    if (isDigitChar(c)) {
        int startIndex = symbolIndex_;
        int startRow = row_;
        int startCol = startIndex - lastNewLineIndex_;
        next();
        emitToken(DIGIT, startIndex, startRow, startCol);
        return true;
    }
    return false;
}

bool Lexer::any() {
    if (symbolIndex_ >= static_cast<int>(program_.size())) return false;
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    next();
    emitToken(ANY, startIndex, startRow, startCol);
    return true;
}

bool Lexer::identifier() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;

    size_t adv = 0;
    if (!isIdentifierChar(symbolIndex_, adv)) return false;
    Utf8Char first{};
    decodeUtf8(symbolIndex_, first);
    if (first.codepoint >= '0' && first.codepoint <= '9') return false;

    auto advanceBytes = [&](size_t count) {
        for (size_t i = 0; i < count; ++i) next();
    };
    advanceBytes(adv);
    while (isIdentifierChar(symbolIndex_, adv)) {
        advanceBytes(adv);
    }

    std::string word = program_.substr(startIndex, symbolIndex_ - startIndex);
    if (isKeyword(word)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }

    emitToken(IDENTIFIER, startIndex, startRow, startCol);
    return true;
}

bool Lexer::integerLiteral() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!digit()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    while (digit()) {
    }
    emitToken(INTEGER_LITERAL, startIndex, startRow, startCol);
    return true;
}

bool Lexer::floatLiteral() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;

    if (!digit()) return false;
    while (digit()) {
    }
    if (!matchLiteral(".", false)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    if (!digit()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    while (digit()) {
    }
    emitToken(FLOAT_LITERAL, startIndex, startRow, startCol);
    return true;
}

bool Lexer::charLiteral() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!matchLiteral("'", false)) return false;
    size_t adv = 0;
    if (isIdentifierChar(symbolIndex_, adv)) {
        for (size_t i = 0; i < adv; ++i) next();
    }
    if (!matchLiteral("'", false)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(CHAR_LITERAL, startIndex, startRow, startCol);
    return true;
}

bool Lexer::stringLiteral() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!matchLiteral("\"", false)) return false;
    size_t adv = 0;
    while (isIdentifierChar(symbolIndex_, adv)) {
        for (size_t i = 0; i < adv; ++i) next();
    }
    if (!matchLiteral("\"", false)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(STRING_LITERAL, startIndex, startRow, startCol);
    return true;
}

bool Lexer::boolLiteral() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (matchKeyword("true") || matchKeyword("false")) {
        emitToken(BOOL_LITERAL, startIndex, startRow, startCol);
        return true;
    }
    symbolIndex_ = startIndex;
    row_ = startRow;
    lastNewLineIndex_ = startLast;
    return false;
}

bool Lexer::space() {
    if (symbolIndex_ >= static_cast<int>(program_.size())) return false;
    if (get() == ' ' || get() == '\t') {
        int startIndex = symbolIndex_;
        int startRow = row_;
        int startCol = startIndex - lastNewLineIndex_;
        next();
        emitToken(SPACE, startIndex, startRow, startCol);
        return true;
    }
    return false;
}

bool Lexer::newline() {
    if (symbolIndex_ >= static_cast<int>(program_.size())) return false;
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (matchLiteral("\r\n", false) || matchLiteral("\n", false) ||
        matchLiteral("\r", false)) {
        emitToken(NEWLINE, startIndex, startRow, startCol);
        return true;
    }
    return false;
}

bool Lexer::primitiveType() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    static const std::vector<std::string> types = {"i8",      "i16",   "i32",
                                                   "i64",     "f32",   "f64",
                                                   "char",    "stringa", "bool"};
    for (const auto &t : types) {
        if (matchKeyword(t)) {
            emitToken(PRIMITIVE_TYPE, startIndex, startRow, startCol);
            return true;
        }
    }
    symbolIndex_ = startIndex;
    row_ = startRow;
    lastNewLineIndex_ = startLast;
    return false;
}

bool Lexer::arrayType() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!primitiveType()) return false;
    if (!matchLiteral("[", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    if (!integerLiteral()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    if (!matchLiteral("]", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(ARRAY_TYPE, startIndex, startRow, startCol);
    return true;
}

bool Lexer::type() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (arrayType() || primitiveType()) {
        emitToken(TYPE, startIndex, startRow, startCol);
        return true;
    }
    symbolIndex_ = startIndex;
    row_ = startRow;
    lastNewLineIndex_ = startLast;
    return false;
}

bool Lexer::block() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!matchLiteral("{", false)) return false;
    while (true) {
        skipWhitespace();
        auto saveIndex = symbolIndex_;
        auto saveRow = row_;
        auto saveLast = lastNewLineIndex_;
        if (!statement()) {
            symbolIndex_ = saveIndex;
            row_ = saveRow;
            lastNewLineIndex_ = saveLast;
            break;
        }
    }
    skipWhitespace();
    if (!matchLiteral("}", false)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(BLOCK, startIndex, startRow, startCol);
    return true;
}

bool Lexer::statement() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;

    if (ifStmt() || whileStmt() || doWhileStmt() || forStmt()) {
        emitToken(STATEMENT, startIndex, startRow, startCol);
        return true;
    }

    if (returnStmt()) {
        if (!matchLiteral(";", true)) {
            symbolIndex_ = startIndex;
            row_ = startRow;
            lastNewLineIndex_ = startLast;
            return false;
        }
        emitToken(STATEMENT, startIndex, startRow, startCol);
        return true;
    }

    if (varDecl()) {
        if (!matchLiteral(";", true)) {
            symbolIndex_ = startIndex;
            row_ = startRow;
            lastNewLineIndex_ = startLast;
            return false;
        }
        emitToken(STATEMENT, startIndex, startRow, startCol);
        return true;
    }

    if (assignment()) {
        if (!matchLiteral(";", true)) {
            symbolIndex_ = startIndex;
            row_ = startRow;
            lastNewLineIndex_ = startLast;
            return false;
        }
        emitToken(STATEMENT, startIndex, startRow, startCol);
        return true;
    }

    if (expr()) {
        if (!matchLiteral(";", true)) {
            symbolIndex_ = startIndex;
            row_ = startRow;
            lastNewLineIndex_ = startLast;
            return false;
        }
        emitToken(STATEMENT, startIndex, startRow, startCol);
        return true;
    }

    symbolIndex_ = startIndex;
    row_ = startRow;
    lastNewLineIndex_ = startLast;
    return false;
}

bool Lexer::varDecl() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;

    if (!identifier()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
    if (matchLiteral(":", true)) {
        if (!type()) {
            std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
        }
    }
    save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
    if (matchLiteral("=", true)) {
        if (!expr()) {
            std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
        }
    }
    emitToken(VAR_DECL, startIndex, startRow, startCol);
    return true;
}

bool Lexer::assignment() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;

    if (!identifier()) return false;
    if (!matchLiteral("=", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    if (!commaExpr()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(ASSIGNMENT, startIndex, startRow, startCol);
    return true;
}

bool Lexer::ifStmt() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!matchKeyword("if")) return false;
    if (!expr() || !block()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchKeyword("elif")) {
            if (!expr() || !block()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(IF_STMT, startIndex, startRow, startCol);
    return true;
}

bool Lexer::whileStmt() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!matchKeyword("while")) return false;
    if (!expr() || !block()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(WHILE_STMT, startIndex, startRow, startCol);
    return true;
}

bool Lexer::doWhileStmt() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!matchKeyword("do")) return false;
    if (!block()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    if (!matchKeyword("while") || !expr() || !matchLiteral(";", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(DO_WHILE_STMT, startIndex, startRow, startCol);
    return true;
}

bool Lexer::range() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!matchLiteral("[", false)) return false;
    if (!expr() || !matchLiteral(";", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
    if (expr()) {
    } else {
        std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
    }
    if (!matchLiteral(";", true) || !expr() || !matchLiteral("]", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(RANGE, startIndex, startRow, startCol);
    return true;
}

bool Lexer::forStmt() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!matchKeyword("for")) return false;
    if (!identifier() || !matchKeyword("in") || !range() || !block()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(FOR_STMT, startIndex, startRow, startCol);
    return true;
}

bool Lexer::returnStmt() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!matchKeyword("return")) return false;
    if (!expr()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(RETURN_STMT, startIndex, startRow, startCol);
    return true;
}

bool Lexer::expr() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (commaExpr()) {
        emitToken(EXPR, startIndex, startRow, startCol);
        return true;
    }
    return false;
}

bool Lexer::commaExpr() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!logicOr()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral(",", true)) {
            if (!logicOr()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(COMMA_EXPR, startIndex, startRow, startCol);
    return true;
}

bool Lexer::logicOr() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!logicAnd()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral("||", true)) {
            if (!logicAnd()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(LOGIC_OR, startIndex, startRow, startCol);
    return true;
}

bool Lexer::logicAnd() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!bitOr()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral("&&", true)) {
            if (!bitOr()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(LOGIC_AND, startIndex, startRow, startCol);
    return true;
}

bool Lexer::bitOr() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!bitXor()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral("|", true)) {
            if (!bitXor()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(BIT_OR, startIndex, startRow, startCol);
    return true;
}

bool Lexer::bitXor() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!bitAnd()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral("^", true)) {
            if (!bitAnd()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(BIT_XOR, startIndex, startRow, startCol);
    return true;
}

bool Lexer::bitAnd() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!equality()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral("&", true)) {
            if (!equality()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(BIT_AND, startIndex, startRow, startCol);
    return true;
}

bool Lexer::equality() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!rel()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral("==", true) || matchLiteral("!=", true)) {
            if (!rel()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(EQUALITY, startIndex, startRow, startCol);
    return true;
}

bool Lexer::rel() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!shift()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral("<=", true) || matchLiteral(">=", true) ||
            matchLiteral("<", true) || matchLiteral(">", true)) {
            if (!shift()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(REL, startIndex, startRow, startCol);
    return true;
}

bool Lexer::shift() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!add()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral("<<", true) || matchLiteral(">>", true)) {
            if (!add()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(SHIFT, startIndex, startRow, startCol);
    return true;
}

bool Lexer::add() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!mul()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral("+", true) || matchLiteral("-", true)) {
            if (!mul()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(ADD, startIndex, startRow, startCol);
    return true;
}

bool Lexer::mul() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!unary()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral("*", true) || matchLiteral("/", true) ||
            matchLiteral("%", true)) {
            if (!unary()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(MUL, startIndex, startRow, startCol);
    return true;
}

bool Lexer::unary() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;

    auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
    if (matchLiteral("+", true) || matchLiteral("-", true) ||
        matchLiteral("!", true)) {
    } else {
        std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
    }
    if (!primary()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(UNARY, startIndex, startRow, startCol);
    return true;
}

bool Lexer::primary() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;

    if (literal() || callExpr() || identifier() || arrayLiteral()) {
        emitToken(PRIMARY, startIndex, startRow, startCol);
        return true;
    }

    if (matchLiteral("(", false)) {
        if (expr() && matchLiteral(")", true)) {
            emitToken(PRIMARY, startIndex, startRow, startCol);
            return true;
        }
    }

    symbolIndex_ = startIndex;
    row_ = startRow;
    lastNewLineIndex_ = startLast;
    return false;
}

bool Lexer::callExpr() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!identifier()) return false;
    if (!matchLiteral("(", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
    if (!argList()) {
        std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
    }
    if (!matchLiteral(")", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(CALL_EXPR, startIndex, startRow, startCol);
    return true;
}

bool Lexer::argList() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!expr()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral(",", true)) {
            if (!expr()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(ARG_LIST, startIndex, startRow, startCol);
    return true;
}

bool Lexer::arrayLiteral() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!matchLiteral("{", false)) return false;
    if (!expr()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral(",", true)) {
            if (!expr()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    if (!matchLiteral("}", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(ARRAY_LITERAL, startIndex, startRow, startCol);
    return true;
}

bool Lexer::literal() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (floatLiteral() || integerLiteral() || stringLiteral() || charLiteral() ||
        boolLiteral()) {
        emitToken(LITERAL, startIndex, startRow, startCol);
        return true;
    }
    symbolIndex_ = startIndex;
    row_ = startRow;
    lastNewLineIndex_ = startLast;
    return false;
}

bool Lexer::program() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    while (topLevelDecl()) {
        skipWhitespace();
    }
    skipWhitespace();
    if (symbolIndex_ != static_cast<int>(program_.size())) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(PROGRAM, startIndex, startRow, startCol);
    return true;
}

bool Lexer::topLevelDecl() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (reprFunc() || arrowBlock()) {
        emitToken(TOPLEVEL_DECL, startIndex, startRow, startCol);
        return true;
    }
    symbolIndex_ = startIndex;
    row_ = startRow;
    lastNewLineIndex_ = startLast;
    return false;
}

bool Lexer::reprFunc() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!matchKeyword("repr")) return false;
    if (!identifier() || !matchLiteral("(", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
    if (!paramList()) {
        std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
    }
    if (!matchLiteral(")", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
    if (matchLiteral("->", true)) {
        if (!returnTypeList()) {
            std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
        }
    }
    if (!block()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(REPR_FUNC, startIndex, startRow, startCol);
    return true;
}

bool Lexer::paramList() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!param()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral(",", true)) {
            if (!param()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(PARAM_LIST, startIndex, startRow, startCol);
    return true;
}

bool Lexer::param() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!identifier() || !matchLiteral(":", true) || !type()) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(PARAM, startIndex, startRow, startCol);
    return true;
}

bool Lexer::returnTypeList() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!type()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral(",", true)) {
            if (!type()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(RETURN_TYPE_LIST, startIndex, startRow, startCol);
    return true;
}

bool Lexer::arrowBlock() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!matchLiteral("#", false)) return false;
    auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
    if (!identifier()) {
        std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
    }
    if (!matchLiteral("{", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    while (arrowLine()) {
    }
    if (!matchLiteral("}", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(ARROW_BLOCK, startIndex, startRow, startCol);
    return true;
}

bool Lexer::arrowLine() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (!arrowNode()) return false;
    if (!arrowOp() || !arrowNode() || !matchLiteral(";", true)) {
        symbolIndex_ = startIndex;
        row_ = startRow;
        lastNewLineIndex_ = startLast;
        return false;
    }
    emitToken(ARROW_LINE, startIndex, startRow, startCol);
    return true;
}

bool Lexer::arrowNode() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (matchKeyword("start") || matchKeyword("end") || identifier()) {
        emitToken(ARROW_NODE, startIndex, startRow, startCol);
        return true;
    }
    symbolIndex_ = startIndex;
    row_ = startRow;
    lastNewLineIndex_ = startLast;
    return false;
}

bool Lexer::arrowOp() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    int startLast = lastNewLineIndex_;
    if (matchLiteral("->", false)) {
        emitToken(ARROW_OP, startIndex, startRow, startCol);
        return true;
    }
    if (matchLiteral("-(", false)) {
        if (!literalList() || !matchLiteral(")>", true)) {
            symbolIndex_ = startIndex;
            row_ = startRow;
            lastNewLineIndex_ = startLast;
            return false;
        }
        emitToken(ARROW_OP, startIndex, startRow, startCol);
        return true;
    }
    return false;
}

bool Lexer::literalList() {
    skipWhitespace();
    int startIndex = symbolIndex_;
    int startRow = row_;
    int startCol = startIndex - lastNewLineIndex_;
    if (!literal()) return false;
    while (true) {
        auto save = std::tuple<int, int, int>(symbolIndex_, row_, lastNewLineIndex_);
        if (matchLiteral(",", true)) {
            if (!literal()) {
                std::tie(symbolIndex_, row_, lastNewLineIndex_) = save;
                break;
            }
        } else {
            break;
        }
    }
    emitToken(LITERAL_LIST, startIndex, startRow, startCol);
    return true;
}
