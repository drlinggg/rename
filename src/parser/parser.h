#pragma once

#include "../AST/ast.h"
#include "../lexer/token.h"
#include <stdbool.h>
//#include "lexer.h"
//#include <stddef.h>

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

ASTNode* parser_parse_statement(Parser* parser);
ASTNode* parser_parse_expression(Parser* parser);

ASTNode* parser_parse_declaration(Parser* parser);
ASTNode* parser_parse_variable_declaration_statement(Parser* parser);
ASTNode* parser_parse_function_declaration_statement(Parser* parser);

ASTNode* parser_parse_assignment_statement(Parser* parser);
ASTNode* parser_parse_if_statement(Parser* parser);
ASTNode* parser_parse_while_statement(Parser* parser);
ASTNode* parser_parse_for_statement(Parser* parser);
ASTNode* parser_parse_return_statement(Parser* parser);
ASTNode* parser_parse_block_statement(Parser* parser);
ASTNode* parser_parse_expression_statement(Parser* parser);

ASTNode* parser_parse_expression(Parser* parser);
ASTNode* parser_parse_binary_expression(Parser* parser, int min_precedence); // TODO IMPLEMENT PRECEDENCE 
ASTNode* parser_parse_unary_expression(Parser* parser, int min_precedence); 
ASTNode* parser_parse_primary_expression(Parser* parser);
ASTNode* parser_parse_literal_expression(Parser* parser);
ASTNode* parser_parse_function_call_expression(Parser* parser);
ASTNode* parser_parse_variable_expression(Parser* parser);

void report_error(Parser* parser, const char* error_message);

Token* parser_consume(Parser* parser, TokenType expected, const char* error_message); // if current 
Token* parser_peek(Parser* parser); // returns current token without moving forward
Token* parser_advance(Parser* parser); // returns current token and moving forward
Token* parser_previous(Parser* parser); // return previous token without moving
Token* parser_retreat(Parser* parser); // return previous token and moving back;

bool parser_check(Parser* parser, TokenType type);
bool parser_is_at_end(Parser* parser);
