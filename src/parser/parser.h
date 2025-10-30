#pragma once

#include "../AST/ast.h"
#include "../lexer/token.h"
#include <stdbool.h>

typedef struct Parser {
    Token* tokens;
    size_t token_count;
    size_t current;
    size_t errors;
    SourceLocation current_location;
} Parser;

Parser* parser_create(Token* tokens, size_t token_count);
void parser_destroy(Parser* parser);
ASTNode* parser_parse(Parser* parser);
