#pragma once
#include <string>
#include <vector>

enum class TokenType {
    // Keywords
    
    // TYPES
    KW_INT, KW_BOOL, KW_LONG, KW_ARRAY,
    // Special constants
    KW_NONE, KW_TRUE, KW_FALSE
    // Control flow
    KW_IF, KW_ELSE, KW_ELIF, 
    KW_WHILE, KW_FOR, KW_BREAK, 
    KW_CONTINUE, KW_RETURN,

    // structs, object.attr
    KW_STRUCT, KW_DOT,
    
    // Identifiers
    IDENTIFIER,
    
    // Literals
    INT_LITERAL, BOOL_LITERAL, LONG_LITERAL
    
    // Operators
    // +-/*%
    OP_PLUS, OP_MINUS, OP_MULT, OP_DIV, OP_MOD,
    // =
    OP_ASSIGN, 
    // == != <=>
    OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE,
    // and or not
    OP_AND, OP_OR, OP_NOT,
    
    // Delimiters
    // { } ( ) [ ]
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    // ; ,
    SEMICOLON, COMMA,
    
    // Special
    END_OF_FILE, ERROR, // idk what is it
    
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};
