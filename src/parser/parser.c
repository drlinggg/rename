#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Parser* parser_create(Token* tokens, size_t token_count) {
    Parser* parser = malloc(sizeof(Parser));
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    parser->errors = 0;

    parser->current_location.line = 1;
    parser->current_location.column = 1;
    parser->current_location.length = 0;
    return parser;
}

void parser_destroy(Parser* parser) {
    free(parser);
}

bool parser_is_at_end(Parser* parser) {
    return parser->current >= parser->token_count;
}

// current token
Token* parser_peek(Parser* parser) {
    if (parser_is_at_end(parser)) {
        return NULL;
    }
    return &(parser->tokens[parser->current]);
}

// go prev
Token* parser_previous(Parser* parser) {
    if (parser->current == 0) {
        return NULL;
    }
    return &parser->tokens[parser->current - 1];
}

// go next
Token* parser_advance(Parser* parser) {
    if (!parser_is_at_end(parser)) {
        Token* previous = parser_previous(parser);
        Token* current = parser_peek(parser);
        
        if (current) {
            parser->current_location.line = current->line;
            parser->current_location.column = current->column;
        }
        
        parser->current++;
        return current;
    }
    return NULL;
}


bool parser_check(Parser* parser, TokenType type) {
    if (parser_is_at_end(parser)) return false;
    Token* token = parser_peek(parser);
    return token->type == type;
}


bool parser_match(Parser* parser, TokenType type) {
    if (parser_check(parser, type)) {
        parser_advance(parser);
        return true;
    }
    return false;
}
