#include "../include/Lexer.h"


Lexer::Lexer(const std::string &program) : program_(program), symbolIndex_(0), row_(0), lastNewLineIndex_(0) {}

char Lexer::get() {
    return program_[symbolIndex_];
}

bool Lexer::letter() {
    if (isalpha(get())) {

    }
}