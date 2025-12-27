#pragma once

typedef enum TokenType {

    // 1. Keywords

    // 1.1 TYPES
    KW_INT, KW_FLOAT, KW_BOOL, KW_LONG, KW_VOID, // for functions

    // 1.2 Special constants
    KW_NONE, KW_TRUE, KW_FALSE,

    // 1.3 Control flow
    KW_IF, KW_ELSE, KW_ELIF, 
    KW_WHILE, KW_FOR, KW_BREAK, 
    KW_CONTINUE, KW_RETURN,

    // 1.4 Structs, object.attr
    KW_STRUCT, KW_DOT,
    
    // 2. Identifiers
    IDENTIFIER,
    
    // 3. Literals
    INT_LITERAL, 
    LONG_LITERAL,
    FLOAT_LITERAL,
    
    // 4. Operators

    // 4.1. + - / * %
    OP_PLUS, OP_MINUS, OP_MULT, OP_DIV, OP_MOD,

    // 4.2. = += -= /= *= %=
    OP_ASSIGN, /*OP_ASSIGN_PLUS, OP_ASSIGN_MINUS, OP_ASSIGN_MULT, OP_ASSIGN_DIV, OP_ASSIGN_MOD,*/ // maybe later

    // 4.3. == != <=>
    OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE, KW_IS,

    // 4.4. and or not
    OP_AND, OP_OR, OP_NOT,
    
    // 5. Delimiters

    // 5.1. () {} []
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,

    // 5.2 ; : ,
    SEMICOLON, COLON, COMMA,
    
    // 6. Special
    END_OF_FILE, ERROR, // idk what is it
    
} TokenType;


// This struct represents based indivisible brick of code written by.
// value is used to storage text inside for diagnostics and future trouble-shootings
// examples:
// int x = 5 + 2 => [Token(TokenType(KW_INT), 'int'), Token(TokenType(IDENTIFIER), 'x'), Token(TokenType(OP_ASSIGN), '+') ....]
typedef struct {
    TokenType type;
    char* value;
    int line; // used for showing in lexical exceptions
    int column; // used for showing in lexical exceptions
} Token;

Token* token_create(TokenType type, const char* value, int line, int column);
void token_free(Token *token);
