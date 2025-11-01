#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

static void lexer_advance(lexer *l);
static void lexer_skip_whitespace(lexer *l);
static Token* lexer_parse_identifier(lexer *l);
static Token* lexer_next_token(lexer *l);
static Token* lexer_parse_number(lexer *l);

static void lexer_advance(lexer *l) {
    if (l->current_char == '\n') {
        l->line++;
        l->column = 1;
    } else {
        l->column++;
    }
    l->current_char = fgetc(l->file);
}

static void lexer_skip_whitespace(lexer *l) {
    while (l->current_char != EOF && isspace(l->current_char)) {
        lexer_advance(l);
    }
}

static Token* lexer_parse_identifier(lexer *l) {
    char buffer[256] = {0};
    int i = 0;
    int start_line = l->line;
    int start_column = l->column;
    
    while ((isalnum(l->current_char) || l->current_char == '_') && i < 255) {
        buffer[i++] = l->current_char;
        lexer_advance(l);
    }
    buffer[i] = '\0';
    
    // Check for keywords
    if (strcmp(buffer, "int") == 0) return token_create(KW_INT, buffer, start_line, start_column);
    if (strcmp(buffer, "bool") == 0) return token_create(KW_BOOL, buffer, start_line, start_column);
    if (strcmp(buffer, "long") == 0) return token_create(KW_LONG, buffer, start_line, start_column);
    if (strcmp(buffer, "array") == 0) return token_create(KW_ARRAY, buffer, start_line, start_column);
    if (strcmp(buffer, "none") == 0) return token_create(KW_NONE, buffer, start_line, start_column);
    if (strcmp(buffer, "true") == 0) return token_create(KW_TRUE, buffer, start_line, start_column);
    if (strcmp(buffer, "false") == 0) return token_create(KW_FALSE, buffer, start_line, start_column);
    if (strcmp(buffer, "if") == 0) return token_create(KW_IF, buffer, start_line, start_column);
    if (strcmp(buffer, "else") == 0) return token_create(KW_ELSE, buffer, start_line, start_column);
    if (strcmp(buffer, "elif") == 0) return token_create(KW_ELIF, buffer, start_line, start_column);
    if (strcmp(buffer, "while") == 0) return token_create(KW_WHILE, buffer, start_line, start_column);
    if (strcmp(buffer, "for") == 0) return token_create(KW_FOR, buffer, start_line, start_column);
    if (strcmp(buffer, "break") == 0) return token_create(KW_BREAK, buffer, start_line, start_column);
    if (strcmp(buffer, "continue") == 0) return token_create(KW_CONTINUE, buffer, start_line, start_column);
    if (strcmp(buffer, "return") == 0) return token_create(KW_RETURN, buffer, start_line, start_column);
    if (strcmp(buffer, "struct") == 0) return token_create(KW_STRUCT, buffer, start_line, start_column);
    
    return token_create(IDENTIFIER, buffer, start_line, start_column);
}

static Token* lexer_parse_number(lexer *l) {
    char buffer[256] = {0};
    int i = 0;
    int start_line = l->line;
    int start_column = l->column;
    
    while (isdigit(l->current_char) && i < 255) {
        buffer[i++] = l->current_char;
        lexer_advance(l);
    }
    buffer[i] = '\0';
    
    return token_create(INT_LITERAL, buffer, start_line, start_column);
}

static Token* lexer_next_token(lexer *l) {
    if (!l || !l->file) return NULL;
    
    lexer_skip_whitespace(l);
    
    if (l->current_char == EOF) {
        return token_create(END_OF_FILE, "EOF", l->line, l->column);
    }
    
    int start_line = l->line;
    int start_column = l->column;
    
    // Identifiers and keywords
    if (isalpha(l->current_char) || l->current_char == '_') {
        return lexer_parse_identifier(l);
    }
    
    // Numbers
    if (isdigit(l->current_char)) {
        return lexer_parse_number(l);
    }
    
    // Single character tokens
    char single_char[2] = {l->current_char, '\0'};
    Token* token = NULL;
    
    switch (l->current_char) {
        case '+': token = token_create(OP_PLUS, single_char, start_line, start_column); break;
        case '-': token = token_create(OP_MINUS, single_char, start_line, start_column); break;
        case '*': token = token_create(OP_MULT, single_char, start_line, start_column); break;
        case '/': token = token_create(OP_DIV, single_char, start_line, start_column); break;
        case '%': token = token_create(OP_MOD, single_char, start_line, start_column); break;
        case '=': token = token_create(OP_ASSIGN, single_char, start_line, start_column); break;
        case '(': token = token_create(LPAREN, single_char, start_line, start_column); break;
        case ')': token = token_create(RPAREN, single_char, start_line, start_column); break;
        case '{': token = token_create(LBRACE, single_char, start_line, start_column); break;
        case '}': token = token_create(RBRACE, single_char, start_line, start_column); break;
        case '[': token = token_create(LBRACKET, single_char, start_line, start_column); break;
        case ']': token = token_create(RBRACKET, single_char, start_line, start_column); break;
        case ';': token = token_create(SEMICOLON, single_char, start_line, start_column); break;
        case ':': token = token_create(COLON, single_char, start_line, start_column); break;
        case ',': token = token_create(COMMA, single_char, start_line, start_column); break;
        case '.': token = token_create(KW_DOT, single_char, start_line, start_column); break;
        default: 
            token = token_create(ERROR, single_char, start_line, start_column);
            break;
    }
    
    if (token) {
        lexer_advance(l);
    }
    
    return token;
}

lexer* lexer_create(const char* filename) {
    // initialize lexer from file function
    lexer *l = malloc(sizeof(lexer));
    if (!l) return NULL;
    
    l->file = fopen(filename, "r");
    if (!l->file) {
        free(l);
        return NULL;
    }
    
    l->current_char = fgetc(l->file);
    l->line = 1;
    l->column = 1;
    l->filename = strdup(filename);
    
    return l;
}

lexer* lexer_create_from_stream(FILE* file, const char* filename) {
    // todo info
    lexer *l = malloc(sizeof(lexer));
    if (!l) return NULL;
    
    l->file = file;
    l->current_char = fgetc(l->file);
    l->line = 1;
    l->column = 1;
    l->filename = strdup(filename);
    
    return l;
}

void lexer_destroy(lexer *l) {
    if (l) {
        if (l->file) fclose(l->file);
        if (l->filename) free(l->filename);
        free(l);
    }
}

Token* lexer_parse_file(lexer* lexer, const char* filename) {
    // if lexer is NULL creates new one from filename
    // if filename is NULL uses given lexer with opened file inside
    if (!lexer) {
        lexer = lexer_create(filename);
        if (!lexer) return NULL;
    }
    
    // Count tokens first to allocate array
    int capacity = 100;
    int count = 0;
    Token** tokens = malloc(capacity * sizeof(Token*));
    
    Token* token;
    while ((token = lexer_next_token(lexer)) != NULL && token->type != END_OF_FILE) {
        if (count >= capacity) {
            capacity *= 2;
            tokens = realloc(tokens, capacity * sizeof(Token*));
        }
        tokens[count++] = token;
        
        if (token->type == ERROR) {
            printf("Lexical error at %d:%d: unexpected character '%s'\n", 
                   token->line, token->column, token->value);
        }
    }
    
    // Add EOF token
    if (count >= capacity) {
        capacity += 1;
        tokens = realloc(tokens, capacity * sizeof(Token*));
    }
    tokens[count++] = token_create(END_OF_FILE, "EOF", lexer->line, lexer->column);
    
    // Create final array
    Token* result = malloc((count + 1) * sizeof(Token));
    for (int i = 0; i < count; i++) {
        result[i] = *tokens[i];
        free(tokens[i]);
    }
    result[count].type = END_OF_FILE; // Sentinel value
    result[count].value = NULL;
    
    free(tokens);
    return result;
}
