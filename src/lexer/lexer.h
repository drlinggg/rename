#pragma once

#include "token.h"
#include <stdio.h>

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
Token* lexer_parse_file(lexer* lexer_obj, const char* filename);
