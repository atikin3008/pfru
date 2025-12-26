#pragma once

#include <cstdint>
#include <string>

enum TOKEN_TYPE {
    LETTER,
    RU_LETTER,
    DIGIT,
    ANY,
    IDENTIFIER,

    INTEGER_LITERAL,
    FLOAT_LITERAL,
    CHAR_LITERAL,
    STRING_LITERAL,
    BOOL_LITERAL,

    SPACE,
    NEWLINE,

    PRIMITIVE_TYPE,
    ARRAY_TYPE,
    TYPE,

    BLOCK,

    STATEMENT,
    VAR_DECL,
    ASSIGNMENT,

    IF_STMT,
    WHILE_STMT,
    DO_WHILE_STMT,
    FOR_STMT,
    RANGE,
    RETURN_STMT,

    EXPR,
    COMMA_EXPR,
    LOGIC_OR,
    LOGIC_AND,
    BIT_OR,
    BIT_XOR,
    BIT_AND,
    EQUALITY,
    REL,
    SHIFT,
    ADD,
    MUL,
    UNARY,

    PRIMARY,
    CALL_EXPR,
    ARG_LIST,
    ARRAY_LITERAL,
    LITERAL,

    PROGRAM,
    TOPLEVEL_DECL,

    REPR_FUNC,
    PARAM_LIST,
    PARAM,
    RETURN_TYPE_LIST,

    ARROW_BLOCK,
    ARROW_LINE,
    ARROW_NODE,
    ARROW_OP,
    LITERAL_LIST
};

struct Token {
    TOKEN_TYPE type;
    uint32_t row, column;
    std::string lexeme;
};
