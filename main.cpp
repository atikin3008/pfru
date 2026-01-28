#include <string>
#include <vector>
#include <iostream>

enum class TokenType{
    IDENTIFIER, INTEGER_LITERAL, FLOAT_LITERAL, 
    CHAR_LITERAL, STRING_LITERAL, BOOL_LITERAL,

    TYPE,
    
    IF, ELIF, WHILE,
    FOR, RETURN,

    OR, AND, BIT_OR, BIT_XOR,
    BIT_AND, EQ, NEQ,
};

struct Token{
    TokenType type;
    std::string text;
    int pos;
};

class Lexer {
public:

    Lexer(const std::string& code) : source(code) {
        pos = 0;
        if (!source.empty()) {
            c = source[pos];
        } else {
            c = '\0';
        }
    }

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        c = source[0];
        while (pos < source.size()) {
            if (isspace(c)) {
                next();
            } else if (isalpha(c)) {
                tokens.push_back(readIdentifier());
            } else if (isdigit(c)) {
                tokens.push_back(readNumber());
            } else {
                std::cerr << "Неизвестный символ: " << c << " в позиции " << pos << std::endl;
                next();
            }
        }
        return tokens;
    }

private:
    std::string source;
    int pos;
    char c; 

    void next() {
        pos++;
        if (pos < source.size()) {
            c = source[pos];
        } else {
            c = '\0';
        }
    }

    Token readIdentifier() {
        int start_pos = pos;
        std::string cur_token;
        
        while (isalnum(c) || c == '_') {
            cur_token += c;
            next();
        }

        if (cur_token == "if") return Token{TokenType::IF, cur_token, start_pos};
        if (cur_token == "elif") return Token{TokenType::ELIF, cur_token, start_pos};
        if (cur_token == "while") return Token{TokenType::WHILE, cur_token, start_pos};
        if (cur_token == "for") return Token{TokenType::FOR, cur_token, start_pos};
        if (cur_token == "return") return Token{TokenType::RETURN, cur_token, start_pos};

        if (cur_token == "true" || cur_token == "false") return Token{TokenType::BOOL_LITERAL, cur_token, start_pos};
        if (cur_token == "i8" || cur_token == "i16" || cur_token == "i32" ||
            cur_token == "i64" || cur_token == "f32" || cur_token == "f64" ||
            cur_token == "char" || cur_token == "stringa" || cur_token == "bool") {
                return Token{TokenType::TYPE, cur_token, start_pos};
        }
        
        return Token{TokenType::IDENTIFIER, cur_token, start_pos};
    }

    Token readNumber() {
        int start_pos = pos;
        std::string cur_token;
        bool has_point = false;

        while (isdigit(c)) {
            cur_token += c;
            next();
        }

        if (c == '.') {
            has_point = true;
            cur_token += c;
            next();

            while (isdigit(c)) {
                cur_token += c;
                next();
            }

            if (cur_token.back() == '.') {
                throw "Error: nothing digits after point";
            }
        }

        if (has_point) {
            return Token{TokenType::FLOAT_LITERAL, cur_token, start_pos};
        }
        return Token{TokenType::INTEGER_LITERAL, cur_token, start_pos};
}
};

int main() {
    Lexer lexer("i64 identifier2 true");
    auto ans = lexer.tokenize();
    for (auto x : ans) {
        std::cout << x.pos << ' ' << (int)x.type << ' ' << x.text << '\n';
    }
}