#include "../include/Lexer.h"
#include "../include/Token.h"

#include <iostream>
#include <string>

std::string tokenName(TOKEN_TYPE type) {
    switch (type) {
        case LETTER: return "LETTER";
        case RU_LETTER: return "RU_LETTER";
        case DIGIT: return "DIGIT";
        case ANY: return "ANY";
        case IDENTIFIER: return "IDENTIFIER";
        case INTEGER_LITERAL: return "INTEGER_LITERAL";
        case FLOAT_LITERAL: return "FLOAT_LITERAL";
        case CHAR_LITERAL: return "CHAR_LITERAL";
        case STRING_LITERAL: return "STRING_LITERAL";
        case BOOL_LITERAL: return "BOOL_LITERAL";
        case SPACE: return "SPACE";
        case NEWLINE: return "NEWLINE";
        case PRIMITIVE_TYPE: return "PRIMITIVE_TYPE";
        case ARRAY_TYPE: return "ARRAY_TYPE";
        case TYPE: return "TYPE";
        case BLOCK: return "BLOCK";
        case STATEMENT: return "STATEMENT";
        case VAR_DECL: return "VAR_DECL";
        case ASSIGNMENT: return "ASSIGNMENT";
        case IF_STMT: return "IF_STMT";
        case WHILE_STMT: return "WHILE_STMT";
        case DO_WHILE_STMT: return "DO_WHILE_STMT";
        case FOR_STMT: return "FOR_STMT";
        case RANGE: return "RANGE";
        case RETURN_STMT: return "RETURN_STMT";
        case EXPR: return "EXPR";
        case COMMA_EXPR: return "COMMA_EXPR";
        case LOGIC_OR: return "LOGIC_OR";
        case LOGIC_AND: return "LOGIC_AND";
        case BIT_OR: return "BIT_OR";
        case BIT_XOR: return "BIT_XOR";
        case BIT_AND: return "BIT_AND";
        case EQUALITY: return "EQUALITY";
        case REL: return "REL";
        case SHIFT: return "SHIFT";
        case ADD: return "ADD";
        case MUL: return "MUL";
        case UNARY: return "UNARY";
        case PRIMARY: return "PRIMARY";
        case CALL_EXPR: return "CALL_EXPR";
        case ARG_LIST: return "ARG_LIST";
        case ARRAY_LITERAL: return "ARRAY_LITERAL";
        case LITERAL: return "LITERAL";
        case PROGRAM: return "PROGRAM";
        case TOPLEVEL_DECL: return "TOPLEVEL_DECL";
        case REPR_FUNC: return "REPR_FUNC";
        case PARAM_LIST: return "PARAM_LIST";
        case PARAM: return "PARAM";
        case RETURN_TYPE_LIST: return "RETURN_TYPE_LIST";
        case ARROW_BLOCK: return "ARROW_BLOCK";
        case ARROW_LINE: return "ARROW_LINE";
        case ARROW_NODE: return "ARROW_NODE";
        case ARROW_OP: return "ARROW_OP";
        case LITERAL_LIST: return "LITERAL_LIST";
    }
    return "UNKNOWN";
}

int main() {
    const std::string sample = R"(repr sum(x:i32, y:i32) -> i32 {
  total: i32 = x + y;
  return total;
}

repr loop(n:i32) {
  i: i32 = 0;
  while i < n {
    i = i + 1;
  }
  return i;
}

#strelki {
  start -> sum;
  sum -(1, 2, 3)> end;
}
)";

    Lexer lexer(sample);
    if (!lexer.parseProgram()) {
        std::cerr << "Lexer failed to parse sample program\n";
        return 1;
    }

    const auto &tokens = lexer.tokens();
    std::cout << "Parsed tokens: " << tokens.size() << "\n";
    for (const auto &t : tokens) {
        std::cout << tokenName(t.type) << " @" << t.row << ":" << t.column
                  << " '" << t.lexeme << "'\n";
    }

    return 0;
}
