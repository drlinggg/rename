#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>


static Precedence get_precedence(TokenType type) {
    // Function for getting precedence for each binary token operation
    switch (type) {
        case OP_OR: return PRECEDENCE_OR;
        case OP_AND: return PRECEDENCE_AND;
        case OP_EQ: case OP_NE: return PRECEDENCE_EQUALITY;
        case OP_LT: case OP_GT: case OP_LE: case OP_GE: return PRECEDENCE_COMPARISON;
        case OP_PLUS: case OP_MINUS: return PRECEDENCE_TERM;
        case OP_MULT: case OP_DIV: case OP_MOD: return PRECEDENCE_FACTOR;
        default: return PRECEDENCE_NONE;
    }
}


Parser* parser_create(Token* tokens, size_t token_count) {
    // Parser create fuction
    // Args: c-style Token array and size
    // Returns: initialized parser
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
    // Diagnostic function used to print errors in stdout
    if (error_message == NULL); return;
    printf("%s\n", error_message);
    printf("%s", "current token is ");
    printf(" %s\n", parser->tokens[parser->current].value);
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
    // Goes back and return previous token
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
    // returns comparesment beetwen given and current token type
    if (parser_is_at_end(parser)) return false;
    Token* token = parser_peek(parser);
    return token->type == type;
}

ASTNode* parser_parse_function_call_expression(Parser* parser) {
    printf("[PARSER] [FUNCTION CALL] parse_function_call_expression started\n");
    
    Token* current = parser_advance(parser);
    SourceLocation loc = (SourceLocation){current->line, current->column};
    printf("[PARSER] [FUNCTION CALL] Function name: '%s' at line %d, col %d\n", current->value, loc.line, loc.column);
    
    parser_consume(parser, LPAREN, "call expression should have opening parenthesis");
    printf("[PARSER] [FUNCTION CALL] Found opening parenthesis\n");

    ASTNode* function_identifier = ast_new_variable_expression(loc, current->value);
    printf("[PARSER] [FUNCTION CALL] Created function identifier node\n");

    size_t argument_count = 0;
    size_t capacity = 4;
    ASTNode** function_arguments = malloc(capacity * sizeof(ASTNode*));
    if (!function_arguments) {
        printf("[PARSER] [FUNCTION CALL] ERROR: Failed to allocate arguments array\n");
        ast_free(function_identifier);
        return NULL;
    }
    printf("[PARSER] [FUNCTION CALL] Initialized arguments array with capacity %zu\n", capacity);

    while (true) {
        parser_peek(parser);
        Token* current_arg_token = parser_peek(parser);
        
        if (argument_count >= capacity) {
            capacity *= 2;
            printf("[PARSER] [FUNCTION CALL] Expanding arguments array to capacity %zu\n", capacity);
            ASTNode** new_args = realloc(function_arguments, capacity * sizeof(ASTNode*));
            if (!new_args) {
                printf("[PARSER] [FUNCTION CALL] ERROR: Failed to reallocate arguments array\n");
                for (size_t i = 0; i < argument_count; i++) {
                    ast_free(function_arguments[i]);
                }
                free(function_arguments);
                ast_free(function_identifier);
                return NULL;
            }
            function_arguments = new_args;
        }

        ASTNode* argument = parser_parse_expression(parser);
        if (!argument) {
            printf("[PARSER] [FUNCTION CALL] ERROR: Failed to parse argument %zu\n", argument_count);
            for (size_t i = 0; i < argument_count; i++) {
                ast_free(function_arguments[i]);
            }
            free(function_arguments);
            ast_free(function_identifier);
            return NULL;
        }
        
        function_arguments[argument_count++] = argument;
        
        Token* next_after_arg = parser_peek(parser);
        
        if (next_after_arg->type != COMMA) {
            printf("[PARSER] [FUNCTION CALL] No comma found, expecting closing parenthesis\n");
            
            if (!parser_consume(parser, RPAREN, "call expression should have closing parenthesis")) {
                printf("[PARSER] [FUNCTION CALL] ERROR: Missing closing parenthesis\n");
                for (size_t i = 0; i < argument_count; i++) {
                    ast_free(function_arguments[i]);
                }
                free(function_arguments);
                ast_free(function_identifier);
                return NULL;
            }
            
            printf("[PARSER] [FUNCTION CALL] Found closing parenthesis, function call has %zu arguments\n", argument_count);
            
            if (argument_count < capacity) {
                ASTNode** trimmed_args = realloc(function_arguments, argument_count * sizeof(ASTNode*));
                if (trimmed_args) {
                    function_arguments = trimmed_args;
                    printf("[PARSER] [FUNCTION CALL] Trimmed arguments array to size %zu\n", argument_count);
                }
            }

            ASTNode* function_call_expr = ast_new_call_expression(loc, function_identifier, function_arguments, argument_count);
            if (!function_call_expr) {
                printf("[PARSER] [FUNCTION CALL] ERROR: Failed to create function call expression node\n");
                for (size_t i = 0; i < argument_count; i++) {
                    ast_free(function_arguments[i]);
                }
                free(function_arguments);
                ast_free(function_identifier);
                return NULL;
            }
            
            ast_free(function_identifier);
            for (size_t i = 0; i < argument_count; i++) {
                ast_free(function_arguments[i]);
            }
            free(function_arguments);
            
            printf("[PARSER] Successfully created function call expression\n");
            return function_call_expr;
        } else {
            printf("[PARSER] Found comma, continuing to next argument\n");
            parser_advance(parser);
        }
    }
}

ASTNode* parser_parse_if_statement(Parser* parser) {
    // TODO: реализовать
    return NULL;
}

ASTNode* parser_parse_while_statement(Parser* parser) {
    // TODO: реализовать
    return NULL;
}

ASTNode* parser_parse_for_statement(Parser* parser) {
    // TODO: реализовать
    return NULL;
}

ASTNode* parser_parse_return_statement(Parser* parser) {
    parser_consume(parser, KW_RETURN, "return statement should start with return keyword");
    ASTNode* expr = parser_parse_expression_statement(parser);
    return ast_new_return_statement(expr->location, expr);
}

ASTNode* parser_parse_variable_declaration_statement(Parser* parser){
    Token* token = parser_advance(parser);
    SourceLocation loc = (SourceLocation){token->line, token->column};
    ASTNode* initializer = NULL;

    Token* identifier = parser_consume(parser, IDENTIFIER, "declaration should have a naming");
    if (!identifier) return NULL;

    if (parser_consume(parser, OP_ASSIGN, NULL)){
        initializer = parser_parse_expression(parser);
    }

    ASTNode* node = ast_new_variable_declaration_statement(loc, token_type_to_type_var(token->type), identifier->value, initializer);
    
    if (initializer) {
        ast_free(initializer);
    }
    
    return node;
}

Parameter* parser_parse_parameter(Parser* parser) {
    Token* name = parser_consume(parser, IDENTIFIER, "Expected parameter name");
    if (!name) return NULL;
    
    parser_consume(parser, COLON, "Expected ':' after parameter name");
    
    TypeVar type = token_type_to_type_var(parser_advance(parser)->type);
    
    return ast_new_parameter(name->value, type);
}


ASTNode* parser_parse_function_declaration_statement(Parser* parser) {
    printf("[PARSER] Starting to parse function declaration\n");
    Token* return_type_token = parser_advance(parser);
    SourceLocation loc = (SourceLocation){return_type_token->line, return_type_token->column};

    Token* identifier = parser_consume(parser, IDENTIFIER, "Function declaration should have a name");
    if (!identifier) return NULL;

    parser_consume(parser, LPAREN, "Expected '(' after function name");
    
    Parameter* parameters = NULL;
    size_t param_count = 0;
    size_t param_capacity = 4;
    
    parameters = malloc(sizeof(Parameter) * param_capacity);
    if (!parameters) return NULL;

    while (parser_peek(parser)->type != RPAREN && !parser_is_at_end(parser)) {
        if (param_count >= param_capacity) {
            param_capacity *= 2;
            Parameter* new_params = realloc(parameters, sizeof(Parameter) * param_capacity);
            if (!new_params) {
                free(parameters);
                return NULL;
            }
            parameters = new_params;
        }

        Parameter param;
        Token* param_name = parser_consume(parser, IDENTIFIER, "Expected parameter name");
        if (!param_name) {
            free(parameters);
            return NULL;
        }

        parser_consume(parser, COLON, "Expected ':' after parameter name");
        
        Token* param_type_token = parser_advance(parser);
        TypeVar param_type = token_type_to_type_var(param_type_token->type);
        
        param.name = strdup(param_name->value);
        param.type = param_type;
        
        parameters[param_count++] = param;

        if (parser_peek(parser)->type != COMMA) break;
        parser_advance(parser);
    }

    parser_consume(parser, RPAREN, "Expected ')' after parameters");

    ASTNode* function_body = parser_parse_block(parser);

    if (!function_body) {
        for (size_t i = 0; i < param_count; i++) {
            free(parameters[i].name);
        }
        free(parameters);
        return NULL;
    }

    TypeVar return_type = token_type_to_type_var(return_type_token->type);
    ASTNode* func_node = ast_new_function_declaration_statement(loc, identifier->value, return_type, parameters, param_count, function_body);
    
    //for (size_t i = 0; i < param_count; i++) {
    //    free(parameters[i].name);
    //}
    //free(parameters);
    ast_free(function_body);

    return func_node;
}


ASTNode* parser_parse_declaration_statement(Parser* parser) {
    printf("[PARSER] Starting to parse declaration statement\n");
    Token* current = parser_advance(parser);
    if (!current) {
        printf("[PARSER] ERROR: No current token\n");
        return NULL;
    }
    
    SourceLocation loc = (SourceLocation){current->line, current->column};
    printf("[PARSER] Declaration type: %d, value: %s\n", current->type, current->value);

    Token* identifier = parser_consume(parser, IDENTIFIER, "declaration should have a naming");
    if (!identifier) {
        printf("[PARSER] ERROR: No identifier in declaration\n");
        return NULL;
    }

    if (parser_peek(parser)->type == OP_ASSIGN) {
        printf("[PARSER] Parsing variable declaration\n");
        parser_retreat(parser);
        parser_retreat(parser);
        return parser_parse_variable_declaration_statement(parser);
    }
    
    printf("[PARSER] Parsing function declaration\n");
    parser_retreat(parser);
    parser_retreat(parser);
    return parser_parse_function_declaration_statement(parser);
}

ASTNode* parser_parse_assignment_statement(Parser* parser) {
    // TODO: реализовать
    return NULL;
}

ASTNode* parser_parse_primary_expression(Parser* parser) {
    Token* current = parser_peek(parser);
    SourceLocation loc = (SourceLocation){current->line, current->column};
    
    printf("[PARSER] parse_primary_expression: token_type=%d, value='%s' at line %d, col %d\n", 
           current->type, current->value, loc.line, loc.column);
    
    switch (current->type) {
        case IDENTIFIER: {
            printf("[PARSER] Found IDENTIFIER: '%s'\n", current->value);
            parser_advance(parser);
            Token* next = parser_peek(parser);
            printf("[PARSER] Next token after identifier: type=%d, value='%s'\n", next->type, next->value);
            
            switch (next->type) {
                case LPAREN: {
                    printf("[PARSER] Identifier followed by LPAREN -> parsing function call\n");
                    parser_retreat(parser);
                    return parser_parse_function_call_expression(parser);
                }
                default: {
                    printf("[PARSER] Creating VariableExpression for '%s'\n", current->value);
                    ASTNode* node = ast_new_variable_expression(loc, current->value);
                    if (!node) {
                        printf("[PARSER] ERROR: Failed to create VariableExpression\n");
                        return NULL;
                    }
                    return node;
                }
            }
        }
        case INT_LITERAL: {
            int value = atoi(current->value);
            printf("[PARSER] Found INT_LITERAL: value=%d\n", value);
            ASTNode* node = ast_new_literal_expression(loc, TYPE_INT, value);
            if (!node) {
                printf("[PARSER] ERROR: Failed to create LiteralExpression for int\n");
                return NULL;
            }
            parser_advance(parser);
            return node;
        }
        case BOOL_LITERAL: {
            int value = (strcmp(current->value, "true") == 0) ? 1 : 0;
            printf("[PARSER] Found BOOL_LITERAL: value='%s' -> %d\n", current->value, value);
            ASTNode* node = ast_new_literal_expression(loc, TYPE_BOOL, value);
            if (!node) {
                printf("[PARSER] ERROR: Failed to create LiteralExpression for bool\n");
                return NULL;
            }
            parser_advance(parser);
            return node;
        }
        case LONG_LITERAL: {
            long value = atol(current->value);
            printf("[PARSER] Found LONG_LITERAL: value=%ld\n", value);
            ASTNode* node = ast_new_literal_expression(loc, TYPE_LONG, value);
            if (!node) {
                printf("[PARSER] ERROR: Failed to create LiteralExpression for long\n");
                return NULL;
            }
            parser_advance(parser);
            return node;
        }
        default: {
            printf("[PARSER] ERROR: Unexpected token type %d, value='%s'\n", current->type, current->value);
            report_error(parser, "couldn't parse token");
            return NULL;
        }
    }
}

ASTNode* parser_parse_expression(Parser* parser) {
    return parser_parse_precedence(parser, PRECEDENCE_ASSIGNMENT);
}

ASTNode* parser_parse_precedence(Parser* parser, Precedence min_precedence) {
    Token* current = parser_peek(parser);
    SourceLocation loc = (SourceLocation){current->line, current->column};
    
    // Parse left
    ASTNode* left = NULL;
    switch (current->type) {
        case OP_PLUS:
        case OP_MINUS:
        case OP_NOT:
            left = parser_parse_unary_expression(parser);
            break;
        case IDENTIFIER:
        case INT_LITERAL:
        case BOOL_LITERAL:
        case LONG_LITERAL:
            left = parser_parse_primary_expression(parser);
            break;
        case LPAREN:
            left = parser_parse_expression(parser);
            break;
        default:
            return NULL;
    }

    if (!left) return NULL;

    // parse operations while their precedence >= min_precedence
    while (true) {
        Token* next = parser_peek(parser);
        Precedence precedence = get_precedence(next->type);
        
        if (precedence < min_precedence) break;

        Token operator_token = *next;
        parser_advance(parser);

        ASTNode* right = parser_parse_precedence(parser, precedence + 1);
        if (!right) {
            ast_free(left);
            return NULL;
        }

        ASTNode* binary_expr = ast_new_binary_expression(
            (SourceLocation){operator_token.line, operator_token.column},
            left, operator_token, right
        );

        ast_free(left);
        ast_free(right);

        left = binary_expr;
    }

    return left;
}

ASTNode* parser_parse_unary_expression(Parser* parser) {
    Token* operator_ = parser_advance(parser);
    SourceLocation loc = (SourceLocation){operator_->line, operator_->column};
    
    if (operator_->type != OP_PLUS && operator_->type != OP_MINUS && operator_->type != OP_NOT) {
        report_error(parser, "Expected unary operator");
        return NULL;
    }
    
    ASTNode* operand = parser_parse_expression(parser);
    if (!operand) {
        return NULL;
    }
    
    ASTNode *node = ast_new_unary_expression(loc, *operator_, operand);
    
    ast_free(operand);
    
    return node;
}

ASTNode* parser_parse_expression_statement(Parser* parser) {
    ASTNode* expression = parser_parse_expression(parser);
    if (!expression) return NULL;
    
    ASTNode* node = ast_new_expression_statement(expression->location, expression);
    ast_free(expression);
    
    return node;
}

ASTNode* parser_parse_statement(Parser* parser) {
    Token* current = parser_peek(parser);
    ASTNode* res = NULL;
    bool flag_fun_declaration = false;
    
    switch (current->type) {
        case KW_IF:
            res = parser_parse_if_statement(parser);
            break;
            
        case KW_WHILE:
            res = parser_parse_while_statement(parser);
            break;
            
        case KW_FOR:
            res = parser_parse_for_statement(parser);
            break;
            
        case KW_RETURN:
            res = parser_parse_return_statement(parser);
            break;
            
        case KW_INT:
        case KW_LONG:
        case KW_BOOL:
        case KW_ARRAY:
            parser_advance(parser);
            Token* next_token = parser_peek(parser);
            
            if (!parser_consume(parser, IDENTIFIER, "expected identifier after type")) {
                res = NULL;
                parser_retreat(parser);
                break;
            }

            parser_retreat(parser);
            parser_retreat(parser);
            res = parser_parse_declaration_statement(parser);
            if (res->node_type == NODE_FUNCTION_DECLARATION_STATEMENT) {
                flag_fun_declaration = true;
            }
            break;

        case IDENTIFIER:
            {
                Token* current_identifier = parser_advance(parser);
                if (parser_check(parser, OP_ASSIGN)) {
                    parser_retreat(parser);
                    res = parser_parse_assignment_statement(parser);
                } else {
                    parser_retreat(parser);
                    res = parser_parse_expression_statement(parser);
                }
            }
            break;

        case LBRACE:
            res = parser_parse(parser);
            break;

        case END_OF_FILE:
            res = NULL;
            break;

        case INT_LITERAL:
        case BOOL_LITERAL:
        case LONG_LITERAL:
        case OP_PLUS:
        case OP_MINUS:
        case OP_NOT:
            res = parser_parse_expression_statement(parser);
            break;
            
        default:
            report_error(parser, "couldn't parse token");
            res = NULL;
            break;
    }

    if (res != NULL && 
        current->type != KW_IF && 
        current->type != KW_WHILE && 
        current->type != KW_FOR && 
        current->type != LBRACE &&
        !flag_fun_declaration) {

        
        if (!parser_consume(parser, SEMICOLON, "';' expected")) {
            ast_free(res);
            return NULL;
        }
    }
    return res;
}

ASTNode* parser_parse(Parser* parser) {
    // Main function for starting parsing, use it with initialized parser to get ast_tree
    printf("[PARSER] Starting parsing\n");
    
    ASTNode* block_statements = ast_new_block_statement((SourceLocation){.line=0, .column=0}, NULL, 0);
    if (!block_statements) {
        printf("[PARSER] ERROR: Failed to create block statement\n");
        return NULL;
    }

    while (!parser_is_at_end(parser)) {
        Token* current = parser_peek(parser);
        
        if (current->type == END_OF_FILE) {
            printf("[PARSER] Reached end of file\n");
            break;
        }
        
        printf("[PARSER] Parsing statement, current token: type=%d, value='%s'\n", 
               current->type, current->value);
        
        ASTNode* stmt = parser_parse_statement(parser);
        if (!stmt) {
            Token* error_token = parser_peek(parser);
            printf("[PARSER] ERROR: Couldn't parse statement at token type=%s, value='%s'\n", 
                   token_type_to_string(error_token->type), error_token->value);
            ast_free(block_statements);
            return NULL;
        }
        
        printf("[PARSER] Successfully parsed statement, adding to block\n");
        add_statement_to_block(block_statements, stmt);
        
        ast_free(stmt);
    }
    
    printf("[PARSER] Finished parsing, returning block with statements\n");
    return block_statements;
}


ASTNode* parser_parse_block(Parser* parser) {
    printf("[PARSER] Starting to parse block\n");
    
    Token* lbrace = parser_consume(parser, LBRACE, "Expected '{' to start block");
    if (!lbrace) {
        printf("[PARSER] ERROR: Missing opening bracket for block\n");
        return NULL;
    }
    
    SourceLocation loc = (SourceLocation){lbrace->line, lbrace->column};
    printf("[PARSER] Block starts at line %d, column %d\n", loc.line, loc.column);
    
    ASTNode* block = ast_new_block_statement(loc, NULL, 0);
    if (!block) {
        printf("[PARSER] ERROR: Failed to create block statement\n");
        return NULL;
    }

    while (!parser_is_at_end(parser) && parser_peek(parser) != NULL && parser_peek(parser)->type != RBRACE) {
        Token* current = parser_peek(parser);
        printf("[PARSER] Parsing statement in block: type=%s, value='%s'\n", 
               token_type_to_string(current->type), current->value);
        
        ASTNode* stmt = parser_parse_statement(parser);
        if (!stmt) {
            printf("[PARSER] ERROR: Failed to parse statement in block\n");
            ast_free(block);
            return NULL;
        }
        
        add_statement_to_block(block, stmt);
        printf("[PARSER] Successfully added statement to block\n");
    }

    if (!parser_consume(parser, RBRACE, "Expected '}' after block")) {
        printf("[PARSER] ERROR: Missing closing bracket for block\n");
        ast_free(block);
        return NULL;
    }
    
    printf("[PARSER] Successfully parsed block\n");
    //printf(token_type_to_string(parser->tokens[parser->current].type));
    return block;
}
