#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef struct lexer {
    FILE *file;
    int current_char;
    int line;
    int column;
    char *filename;
} lexer;

lexer* lexer_create(const char* filename);
lexer* lexer_create_from_stream(FILE* file, const char* filename);
void lexer_destroy(lexer *l);

void lexer_advance(lexer *l);
void lexer_skip_whitespace(lexer *l);

Token* lexer_parse_file(lexer* lexer_obj, const char* filename);
Token* lexer_parse_identifier(lexer *l);
Token* lexer_next_token(lexer *l);
Token* lexer_parse_number(lexer *l);
Token* lexer_next_token(lexer *l);

Token* token_create(TokenType type, const char* value, int line, int column);
void token_free(Token *token);
