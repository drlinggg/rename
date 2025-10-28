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

void lexer_destroy(lexer *l);
lexer* lexer_create(const char* filename);
lexer* lexer_create_from_stream(FILE* file, const char* filename);
Token* lexer_parse_file(lexer* lexer_obj, const char* filename);
Token* lexer_next_token(lexer *l);
void token_free(Token *token);
