#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

Parser* parser_create(Token* tokens, size_t token_count) {
    Parser* parser = malloc(sizeof(Parser));
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    parser->errors = 0;

    parser->current_location.line = 1;
    parser->current_location.column = 1;
    return parser;
}

void parser_destroy(Parser* parser) {
    free(parser);
}

bool parser_is_at_end(Parser* parser) {
    return parser->current >= parser->token_count;
}

void report_error(Parser* parser, const char* error_message) {
    printf("%s\n", error_message);
    printf("%s", "current token is");
    printf("%s", parser->tokens[parser->current].value);
}

// if current token is the same as expected -> 
Token* parser_consume(Parser* parser, TokenType expected, const char* error_message) {
    if (parser_check(parser, expected)) {
        return parser_advance(parser);
    }
    
    report_error(parser, error_message);
    return NULL;
}

// returns current token without moving forward
Token* parser_peek(Parser* parser) {
    if (parser_is_at_end(parser)) {
        return NULL;
    }
    return &(parser->tokens[parser->current]);
}

// return previous token without moving
Token* parser_previous(Parser* parser) {
    if (parser->current == 0) {
        return NULL;
    }
    return &(parser->tokens[parser->current - 1]);
}

Token* parser_retreat(Parser* parser) {
    if (parser->current == 0) {
        return NULL;
    }
    parser->current--;

    Token* current = parser_peek(parser);

    parser->current_location.line = current->line;
    parser->current_location.column = current->column;

    return current;
}

// returns current token and moving forward
Token* parser_advance(Parser* parser) {
    if (parser_is_at_end(parser)) return NULL;

    Token* current = parser_peek(parser);
        
    if (!current) return NULL;

    parser->current_location.line = current->line;
    parser->current_location.column = current->column;
    parser->current++;

    return current;
}


bool parser_check(Parser* parser, TokenType type) {
    if (parser_is_at_end(parser)) return false;
    Token* token = parser_peek(parser);
    return token->type == type;
}

ASTNode* parser_parse_function_call_expression(Parser* parser) {
}

ASTNode* parser_parse_if_statement(Parser* parser) {
}

ASTNode* parser_parse_while_statement(Parser* parser) {
}

ASTNode* parser_parse_for_statement(Parser* parser) {
}

ASTNode* parser_parse_return_statement(Parser* parser) {
}

ASTNode* parser_parse_declaration(Parser* parser) {
}

ASTNode* parser_parse_assignment_statement(Parser* parser) {
}

ASTNode* parser_parse_primary_expression(Parser* parser) {
    Token* current = parser_peek(parser);
    SourceLocation loc = (SourceLocation) {current->line, current->column};

    switch (current->type) {
        case IDENTIFIER: {
            Token* next = parser_peek(parser);
            switch (next->type) {
                case LBRACE: {
                    return parser_parse_function_call_expression(parser);
                }
                default: {
                    VariableExpression* node = malloc(sizeof(VariableExpression));
                    if (!node) return NULL;
                    *node = (VariableExpression) {NODE_VARIABLE_EXPRESSION, loc, current->value};
                    return (ASTNode*) node;
                }
            }
        }
        case INT_LITERAL: {
            LiteralExpression* node = malloc(sizeof(LiteralExpression));
            if (!node) return NULL;
            *node = (LiteralExpression) {NODE_LITERAL_EXPRESSION, loc, TYPE_INT, atoi(current->value)};
            return (ASTNode*) node;
        }
        case BOOL_LITERAL: {
            LiteralExpression* node = malloc(sizeof(LiteralExpression));
            if (!node) return NULL;
            *node = (LiteralExpression) {NODE_LITERAL_EXPRESSION, loc, TYPE_BOOL, atoi(current->value)};
            return (ASTNode*) node;
        }
        case LONG_LITERAL: {
            LiteralExpression* node = malloc(sizeof(LiteralExpression));
            if (!node) return NULL;
            *node = (LiteralExpression) {NODE_LITERAL_EXPRESSION, loc, TYPE_LONG, atoi(current->value)};
            return (ASTNode*) node;
        }
        default: {
            report_error(parser, "couldn't parse token");
            return NULL;
        }
    }
}

ASTNode* parser_parse_expression(Parser* parser) { // ReturnType: Expression
    Token* current = parser_peek(parser);
    SourceLocation loc = (SourceLocation) {current->line, current->column};
    ASTNode* left = NULL;
    
    switch (current->type) {
        case IDENTIFIER:
        case INT_LITERAL:
        case BOOL_LITERAL:
        case LONG_LITERAL:

            //parser_retreat(parser);
            left = parser_parse_primary_expression(parser);
            break;
        case OP_PLUS:
        case OP_MINUS:
        case OP_NE:
            //parser_retreat(parser);
            left = parser_parse_unary_expression(parser, 0); //TODO
            break;
        case LBRACE:
            left = parser_parse_expression(parser);
            break;
        default:
            report_error(parser, "couldn't parse token");
            free(left);
            return NULL;
    }

    parser_advance(parser);
    Token* next = parser_peek(parser);

    switch (next->type) {
        case OP_EQ:
        case OP_NE:
        case OP_LT:
        case OP_GT:
        case OP_LE:
        case OP_GE:
        case OP_AND:
        case OP_OR:
        case OP_PLUS:
        case OP_MINUS:
        case OP_MULT:
        case OP_DIV:
        case OP_MOD:
            parser_advance(parser);
            ASTNode* right = parser_parse_expression(parser);
            BinaryExpression* node = malloc(sizeof(BinaryExpression));
            *node = (BinaryExpression) {NODE_BINARY_EXPRESSION, loc, left, right, *next};
            return (ASTNode*) node;

        case SEMICOLON:
            parser_advance(parser);
            return left;

        default:
            report_error(parser, "couldn't parse token");
            return NULL;
    }
}

ASTNode* parser_parse_unary_expression(Parser* parser, int min_precedence) {
    UnaryExpression* node = malloc(sizeof(UnaryExpression));
    Token* operator_ = parser_advance(parser);
    SourceLocation loc = (SourceLocation) {operator_->line, operator_->column};
    ASTNode* operand = parser_parse_expression(parser);
    *node = (UnaryExpression) {NODE_UNARY_EXPRESSION, loc, *operator_, operand};
    return (ASTNode*) node;
}

ASTNode* parser_parse_expression_statement(Parser* parser) {
    ASTNode* expression = parser_parse_expression(parser);
    parser_consume(parser, SEMICOLON, "';'?");
    return ast_new_expression_statement(expression->location, expression);
}

ASTNode* parser_parse_statement(Parser* parser) {
    Token* current = parser_peek(parser);
    
    switch (current->type) {
        case KW_IF:
            return parser_parse_if_statement(parser);
            
        case KW_WHILE:
            return parser_parse_while_statement(parser);
            
        case KW_FOR:
            return parser_parse_for_statement(parser);
            
        case KW_RETURN:
            return parser_parse_return_statement(parser);
            
        case KW_INT:
        case KW_LONG:
        case KW_BOOL:
            parser_advance(parser);
            Token* next_token = parser_peek(parser);
            if (next_token->type == IDENTIFIER) {
                parser_retreat(parser);
                return parser_parse_declaration(parser);
            }
            parser_retreat(parser);

        case IDENTIFIER:
            Token* current_identifier = parser_advance(parser);
            if (!parser_check(parser, OP_ASSIGN)) {
                parser_retreat(parser);
                return parser_parse_assignment_statement(parser);
            }
            parser_retreat(parser);
            break;

        case LBRACE:
            return parser_parse(parser);

        case END_OF_FILE:
            return NULL;
            
        default: // x = 5 ?? fun()??
            return parser_parse_expression_statement(parser);
    }
}

ASTNode* parser_parse(Parser* parser) {

    ASTNode* block_statements = ast_new_block_statement((SourceLocation){.line=0, .column=0}, NULL, 0);

    while (!parser_is_at_end(parser)) {
        ASTNode* stmt = parser_parse_statement(parser);

        if (!stmt) return block_statements;
        add_statement_to_block(block_statements, stmt);
    }
    return block_statements;
}
