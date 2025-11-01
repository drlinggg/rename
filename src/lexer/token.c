#include "token.h"
#include <stdlib.h>
#include <string.h>

Token* token_create(TokenType type, const char* value, int line, int column) {
    Token *token = malloc(sizeof(Token));
    token->type = type;
    token->value = strdup(value);
    token->line = line;
    token->column = column;
    return token;
}

void token_free(Token *token) {
    if (token) {
        if (token->value) free(token->value);
        free(token);
    }
}
