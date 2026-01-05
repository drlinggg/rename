#pragma once

typedef enum TokenType {

    KW_INT, KW_FLOAT, KW_BOOL, KW_LONG, KW_VOID,

    KW_NONE, KW_TRUE, KW_FALSE,

    KW_IF, KW_ELSE, KW_ELIF, 
    KW_WHILE, KW_FOR, KW_BREAK, 
    KW_CONTINUE, KW_RETURN,

    KW_STRUCT, KW_DOT,
    
    IDENTIFIER,
    
    INT_LITERAL, 
    LONG_LITERAL,
    FLOAT_LITERAL,
    
    OP_PLUS, OP_MINUS, OP_MULT, OP_DIV, OP_MOD,

    OP_ASSIGN,

    OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE, KW_IS,

    OP_AND, OP_OR, OP_NOT,
    
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,

    SEMICOLON, COLON, COMMA,
    
    END_OF_FILE, ERROR,
    
} TokenType;


typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

Token* token_create(TokenType type, const char* value, int line, int column);
void token_free(Token *token);
