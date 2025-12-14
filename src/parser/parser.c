#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include "../debug.h"

typedef enum {
    PRECEDENCE_NONE = 0,
    PRECEDENCE_ASSIGNMENT = 1,
    PRECEDENCE_OR = 2,
    PRECEDENCE_AND = 3,
    PRECEDENCE_EQUALITY = 4,
    PRECEDENCE_COMPARISON = 5,
    PRECEDENCE_TERM = 6,
    PRECEDENCE_FACTOR = 7,
    PRECEDENCE_UNARY = 8,
    PRECEDENCE_CALL = 9,
} Precedence;

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

static Precedence get_precedence(TokenType type);
static bool parser_is_at_end(Parser* parser);
static void report_error(Parser* parser, const char* error_message);
static Token* parser_consume(Parser* parser, TokenType expected, const char* error_message);
static Token* parser_peek(Parser* parser);
static Token* parser_previous(Parser* parser);
static Token* parser_retreat(Parser* parser);
static Token* parser_advance(Parser* parser);
static bool parser_check(Parser* parser, TokenType type);

static ASTNode* parser_parse_block(Parser* parser);
static ASTNode* parser_parse_statement(Parser* parser);
static ASTNode* parser_parse_expression(Parser* parser);
static ASTNode* parser_parse_declaration_statement(Parser* parser);
static ASTNode* parser_parse_variable_declaration_statement(Parser* parser);
static Parameter* parser_parse_parameter(Parser* parser);
static ASTNode* parser_parse_function_declaration_statement(Parser* parser);
static ASTNode* parser_parse_assignment_statement(Parser* parser);
static ASTNode* parser_parse_if_statement(Parser* parser);
static ASTNode* parser_parse_while_statement(Parser* parser);
static ASTNode* parser_parse_for_statement(Parser* parser);
static ASTNode* parser_parse_return_statement(Parser* parser);
static ASTNode* parser_parse_block_statement(Parser* parser);
static ASTNode* parser_parse_expression_statement(Parser* parser);
static ASTNode* parser_parse_precedence(Parser* parser, Precedence min_precedence);
static ASTNode* parser_parse_binary_expression(Parser* parser);
static ASTNode* parser_parse_unary_expression(Parser* parser);
static ASTNode* parser_parse_primary_expression(Parser* parser);
static ASTNode* parser_parse_literal_expression(Parser* parser);
static ASTNode* parser_parse_function_call_expression(Parser* parser);
static ASTNode* parser_parse_variable_expression(Parser* parser);


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

static bool parser_is_at_end(Parser* parser) {
    return parser->current >= parser->token_count;
}

void report_error(Parser* parser, const char* error_message) {
    // Diagnostic function used to print errors in stdout
    if (error_message == NULL); return;
    DPRINT("%s\n", error_message);
    DPRINT("%s", "current token is ");
    DPRINT(" %s\n", parser->tokens[parser->current].value);
}

// if current token is the same as expected -> 
static Token* parser_consume(Parser* parser, TokenType expected, const char* error_message) {
    if (parser_check(parser, expected)) {
        return parser_advance(parser);
    }
    
    report_error(parser, error_message);
    return NULL;
}

// returns current token without moving forward
static Token* parser_peek(Parser* parser) {
    if (parser_is_at_end(parser)) {
        return NULL;
    }
    return &(parser->tokens[parser->current]);
}

// return previous token without moving
static Token* parser_previous(Parser* parser) {
    if (parser->current == 0) {
        return NULL;
    }
    return &(parser->tokens[parser->current - 1]);
}

static Token* parser_retreat(Parser* parser) {
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
static Token* parser_advance(Parser* parser) {
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

static ASTNode* parser_parse_function_call_expression(Parser* parser) {
    DPRINT("[PARSER] [FUNCTION CALL] parse_function_call_expression started\n");
    
    Token* current = parser_advance(parser);
    SourceLocation loc = (SourceLocation){current->line, current->column};
    DPRINT("[PARSER] [FUNCTION CALL] Function name: '%s' at line %d, col %d\n", current->value, loc.line, loc.column);
    
    parser_consume(parser, LPAREN, "call expression should have opening parenthesis");
    DPRINT("[PARSER] [FUNCTION CALL] Found opening parenthesis\n");

    ASTNode* function_identifier = ast_new_variable_expression(loc, current->value);
    DPRINT("[PARSER] [FUNCTION CALL] Created function identifier node\n");

    size_t argument_count = 0;
    size_t capacity = 4;
    ASTNode** function_arguments = malloc(capacity * sizeof(ASTNode*));
    if (!function_arguments) {
        DPRINT("[PARSER] [FUNCTION CALL] ERROR: Failed to allocate arguments array\n");
        ast_free(function_identifier);
        return NULL;
    }
    DPRINT("[PARSER] [FUNCTION CALL] Initialized arguments array with capacity %zu\n", capacity);

    while (true) {
        parser_peek(parser);
        Token* current_arg_token = parser_peek(parser);
        
        if (argument_count >= capacity) {
            capacity *= 2;
            DPRINT("[PARSER] [FUNCTION CALL] Expanding arguments array to capacity %zu\n", capacity);
            ASTNode** new_args = realloc(function_arguments, capacity * sizeof(ASTNode*));
            if (!new_args) {
                DPRINT("[PARSER] [FUNCTION CALL] ERROR: Failed to reallocate arguments array\n");
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
            DPRINT("[PARSER] [FUNCTION CALL] Failed to parse argument %zu\n", argument_count);

            if (parser_peek(parser)->type != RPAREN) {
                for (size_t i = 0; i < argument_count; i++) {
                    ast_free(function_arguments[i]);
                }
                free(function_arguments);
                ast_free(function_identifier);
                return NULL;
            }
        }
        
      
        Token* next_after_arg = parser_peek(parser);
        
        if (next_after_arg->type != COMMA) {
            DPRINT("[PARSER] [FUNCTION CALL] No comma found, expecting closing parenthesis\n");
            
            if (!parser_consume(parser, RPAREN, "call expression should have closing parenthesis")) {
                DPRINT("[PARSER] [FUNCTION CALL] ERROR: Missing closing parenthesis\n");
                for (size_t i = 0; i < argument_count; i++) {
                    ast_free(function_arguments[i]);
                }
                free(function_arguments);
                ast_free(function_identifier);
                return NULL;
            }
            
            DPRINT("[PARSER] [FUNCTION CALL] Found closing parenthesis, function call has %zu arguments\n", argument_count);
            
            if (argument_count == 0) {
                free(function_arguments);
                function_arguments = NULL; 

            } else if (argument_count < capacity) {
                ASTNode** trimmed_args = realloc(function_arguments, argument_count * sizeof(ASTNode*));
                if (trimmed_args) {
                    function_arguments = trimmed_args;
                    DPRINT("[PARSER] [FUNCTION CALL] Trimmed arguments array to size %zu\n", argument_count);
                }
            }

            ASTNode* function_call_expr = ast_new_call_expression(loc, function_identifier, function_arguments, argument_count);
            if (!function_call_expr) {
                DPRINT("[PARSER] [FUNCTION CALL] ERROR: Failed to create function call expression node\n");
                for (size_t i = 0; i < argument_count; i++) {
                    ast_free(function_arguments[i]);
                }
                free(function_arguments);
                ast_free(function_identifier);
                return NULL;
            }

            //ast_free(function_identifier);
            //for (size_t i = 0; i < argument_count; i++) {
            //    ast_free(function_arguments[i]);
            //}
            //free(function_arguments);
            DPRINT("[PARSER] Successfully created function call expression\n");
            ast_print(function_call_expr, 0);
            return function_call_expr;
        } else {
            function_arguments[argument_count++] = argument;
            DPRINT("[PARSER] Found comma, continuing to next argument\n");
            parser_advance(parser);
        }
    }
}

static ASTNode* parser_parse_if_statement(Parser* parser) {
    Token* if_token = parser_consume(parser, KW_IF, "if statement should start with 'if' keyword");
    if (!if_token) return NULL;
    
    SourceLocation loc = (SourceLocation){if_token->line, if_token->column};
    
    parser_consume(parser, LPAREN, "Expected '(' after 'if'");
    
    ASTNode* condition = parser_parse_expression(parser);
    if (!condition) {
        report_error(parser, "Expected condition expression in if statement");
        return NULL;
    }
    
    parser_consume(parser, RPAREN, "Expected ')' after if condition");
    
    ASTNode* then_branch = parser_parse_block(parser);
    if (!then_branch) {
        ast_free(condition);
        return NULL;
    }
    
    ASTNode* if_node = ast_new_if_statement(loc, condition, then_branch);
    IfStatement* if_stmt = (IfStatement*)if_node;
    
    // Parse elif branches
    while (parser_peek(parser) && parser_peek(parser)->type == KW_ELIF) {
        Token* elif_token = parser_advance(parser);
        
        parser_consume(parser, LPAREN, "Expected '(' after 'elif'");
        
        ASTNode* elif_condition = parser_parse_expression(parser);
        if (!elif_condition) {
            ast_free(if_node);
            return NULL;
        }
        
        parser_consume(parser, RPAREN, "Expected ')' after elif condition");
        
        ASTNode* elif_branch = parser_parse_block(parser);
        if (!elif_branch) {
            ast_free(elif_condition);
            ast_free(if_node);
            return NULL;
        }
        
        // Reallocate memory for elif arrays
        size_t new_elif_count = if_stmt->elif_count + 1;
        ASTNode** new_elif_conditions = realloc(if_stmt->elif_conditions, 
                                               new_elif_count * sizeof(ASTNode*));
        ASTNode** new_elif_branches = realloc(if_stmt->elif_branches,
                                             new_elif_count * sizeof(ASTNode*));
        
        if (!new_elif_conditions || !new_elif_branches) {
            free(new_elif_conditions);
            free(new_elif_branches);
            ast_free(elif_condition);
            ast_free(elif_branch);
            ast_free(if_node);
            return NULL;
        }
        
        if_stmt->elif_conditions = new_elif_conditions;
        if_stmt->elif_branches = new_elif_branches;
        
        if_stmt->elif_conditions[if_stmt->elif_count] = elif_condition;
        if_stmt->elif_branches[if_stmt->elif_count] = elif_branch;
        if_stmt->elif_count = new_elif_count;
    }
    
    // Parse else branch if present
    if (parser_peek(parser) && parser_peek(parser)->type == KW_ELSE) {
        parser_advance(parser); // Consume 'else'
        
        ASTNode* else_branch = parser_parse_block(parser);
        if (!else_branch) {
            ast_free(if_node);
            return NULL;
        }
        
        if_stmt->else_branch = else_branch;
    }
    
    return if_node;
}

static ASTNode* parser_parse_while_statement(Parser* parser) {
    Token* while_token = parser_consume(parser, KW_WHILE, "while statement should start with 'while' keyword");
    if (!while_token) return NULL;
    
    SourceLocation loc = (SourceLocation){while_token->line, while_token->column};
    
    parser_consume(parser, LPAREN, "Expected '(' after 'while'");
    
    ASTNode* condition = parser_parse_expression(parser);
    if (!condition) {
        report_error(parser, "Expected condition expression in while statement");
        return NULL;
    }
    
    parser_consume(parser, RPAREN, "Expected ')' after while condition");
    
    ASTNode* body = parser_parse_block(parser);
    if (!body) {
        ast_free(condition);
        return NULL;
    }
    
    ASTNode* while_node = ast_new_while_statement(loc, condition, body);
    
    return while_node;
}

static ASTNode* parser_parse_for_statement(Parser* parser) {
    Token* for_token = parser_consume(parser, KW_FOR, "for statement should start with 'for' keyword");
    if (!for_token) {
        DPRINT("[PARSER] [FOR LOOP] ERROR: No 'for' keyword found\n");
        return NULL;
    }
    
    SourceLocation loc = (SourceLocation){for_token->line, for_token->column};
    DPRINT("[PARSER] [FOR LOOP] Starting for loop at line %d, column %d\n", loc.line, loc.column);
    
    parser_consume(parser, LPAREN, "Expected '(' after 'for'");
    DPRINT("[PARSER] [FOR LOOP] Found opening parenthesis\n");
    
    // Parse initializer (optional)
    ASTNode* initializer = NULL;
    Token* peek_token = parser_peek(parser);
    
    DPRINT("[PARSER] [FOR LOOP] Peek token for initializer: type=%s, value='%s'\n", 
           token_type_to_string(peek_token ? peek_token->type : -1), 
           peek_token ? peek_token->value : "NULL");
    
    if (peek_token && peek_token->type != SEMICOLON) {
        // Check if it's a variable declaration or expression
        if (peek_token->type == KW_INT || peek_token->type == KW_LONG || 
            peek_token->type == KW_BOOL || peek_token->type == KW_ARRAY) {
            DPRINT("[PARSER] [FOR LOOP] Parsing variable declaration as initializer\n");
            initializer = parser_parse_variable_declaration_statement(parser);
            if (initializer) {
                DPRINT("[PARSER] [FOR LOOP] Successfully parsed variable declaration initializer\n");
            } else {
                DPRINT("[PARSER] [FOR LOOP] ERROR: Failed to parse variable declaration initializer\n");
            }
        } else {
            DPRINT("[PARSER] [FOR LOOP] Parsing expression statement as initializer\n");
            initializer = parser_parse_expression_statement(parser);
            if (initializer) {
                DPRINT("[PARSER] [FOR LOOP] Successfully parsed expression statement initializer\n");
            } else {
                DPRINT("[PARSER] [FOR LOOP] ERROR: Failed to parse expression statement initializer\n");
            }
        }
        parser_consume(parser, SEMICOLON, "Expected ';' after initializer");
    } else {
        DPRINT("[PARSER] [FOR LOOP] No initializer, consuming semicolon\n");
        parser_consume(parser, SEMICOLON, "Expected ';' after for initializer (even if empty)");
    }

    // Parse condition (optional)
    ASTNode* condition = NULL;
    peek_token = parser_peek(parser);
    
    DPRINT("[PARSER] [FOR LOOP] Peek token for condition: type=%s, value='%s'\n", 
           token_type_to_string(peek_token ? peek_token->type : -1), 
           peek_token ? peek_token->value : "NULL");
    
    if (peek_token && peek_token->type != SEMICOLON) {
        DPRINT("[PARSER] [FOR LOOP] Parsing condition expression\n");
        ASTNode* condition_expr = parser_parse_expression(parser);
        if (condition_expr) {
            condition = ast_new_expression_statement(condition_expr->location, condition_expr);
            DPRINT("[PARSER] [FOR LOOP] Successfully parsed condition\n");
            ast_free(condition_expr);
        } else {
            DPRINT("[PARSER] [FOR LOOP] ERROR: Failed to parse condition expression\n");
        }
        parser_consume(parser, SEMICOLON, "Expected ';' after condition");
    } else {
        DPRINT("[PARSER] [FOR LOOP] No condition, consuming semicolon\n");
        parser_consume(parser, SEMICOLON, "Expected ';' after for condition");
    }

    // Parse increment (optional)
    ASTNode* increment = NULL;
    peek_token = parser_peek(parser);
    
    DPRINT("[PARSER] [FOR LOOP] Peek token for increment: type=%s, value='%s'\n", 
           token_type_to_string(peek_token ? peek_token->type : -1), 
           peek_token ? peek_token->value : "NULL");
    
    if (peek_token && peek_token->type != RPAREN) {
        DPRINT("[PARSER] [FOR LOOP] Parsing increment\n");
        increment = parser_parse_assignment_statement(parser);
        if (increment) {
            DPRINT("[PARSER] [FOR LOOP] Successfully parsed increment\n");
        } else {
            DPRINT("[PARSER] [FOR LOOP] ERROR: Failed to parse increment\n");
        }
    } else {
        DPRINT("[PARSER] [FOR LOOP] No increment expression\n");
    }
    
    parser_consume(parser, RPAREN, "Expected ')' after for clause");
    DPRINT("[PARSER] [FOR LOOP] Found closing parenthesis\n");
    Token* cur_token = parser_peek(parser);
    
    // Parse body
    DPRINT("[PARSER] [FOR LOOP] Parsing body block\n");
    ASTNode* body = parser_parse_block(parser);
    if (!body) {
        DPRINT("[PARSER] [FOR LOOP] ERROR: Failed to parse body block\n");
        if (initializer) ast_free(initializer);
        if (condition) ast_free(condition);
        if (increment) ast_free(increment);
        return NULL;
    }
   
    DPRINT("[PARSER] [FOR LOOP] Successfully parsed body block\n");
    
    DPRINT("[PARSER] [FOR LOOP] Creating for statement node:\n");
    DPRINT("[PARSER] [FOR LOOP]   - Initializer: %s\n", initializer ? ast_node_type_to_string(initializer->node_type) : "NULL");
    ast_print(initializer, 0);
    DPRINT("[PARSER] [FOR LOOP]   - Condition: %s\n", condition ? ast_node_type_to_string(condition->node_type) : "NULL");
    ast_print(condition, 0);
    DPRINT("[PARSER] [FOR LOOP]   - Increment: %s\n", increment ? ast_node_type_to_string(increment->node_type) : "NULL");
    ast_print(increment, 0);
    DPRINT("[PARSER] [FOR LOOP]   - Body: %s\n", body ? ast_node_type_to_string(body->node_type) : "NULL");
    ast_print(body, 0);
    
    ASTNode* for_node = ast_new_for_statement(loc, initializer, condition, increment, body);
    
    if (for_node) {
        DPRINT("[PARSER] [FOR LOOP] Successfully created for statement node\n");
    } else {
        DPRINT("[PARSER] [FOR LOOP] ERROR: Failed to create for statement node\n");
    }
    
    return for_node;
}

static ASTNode* parser_parse_return_statement(Parser* parser) {
    parser_consume(parser, KW_RETURN, "return statement should start with return keyword");
    ASTNode* expr = parser_parse_expression_statement(parser);
    return ast_new_return_statement(expr->location, expr);
}

static ASTNode* parser_parse_variable_declaration_statement(Parser* parser){
    Token* token = parser_advance(parser);
    SourceLocation loc = (SourceLocation){token->line, token->column};
    ASTNode* initializer = NULL;
    DPRINT("[PARSER] parse_variable_declaration start: type=%d value=%s\n", token->type, token->value);

    Token* identifier = parser_consume(parser, IDENTIFIER, "declaration should have a naming");
    if (!identifier) return NULL;

    if (parser_consume(parser, OP_ASSIGN, NULL)){
        initializer = parser_parse_expression(parser);
    }

    ASTNode* node = ast_new_variable_declaration_statement(loc, token_type_to_type_var(token->type), identifier->value, initializer);
    
    //if (initializer) {
    //    ast_free(initializer);
    //}
    DPRINT("[PARSER] parse_variable_declaration finished for name=%s\n", identifier->value);
    ast_print(initializer, 0);
    return node;
}

static Parameter* parser_parse_parameter(Parser* parser) {
    Token* name = parser_consume(parser, IDENTIFIER, "Expected parameter name");
    if (!name) return NULL;
    
    parser_consume(parser, COLON, "Expected ':' after parameter name");
    
    TypeVar type = token_type_to_type_var(parser_advance(parser)->type);
    
    return ast_new_parameter(name->value, type);
}


static ASTNode* parser_parse_function_declaration_statement(Parser* parser) {
    DPRINT("[PARSER] Starting to parse function declaration\n");
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


static ASTNode* parser_parse_declaration_statement(Parser* parser) {
    DPRINT("[PARSER] Starting to parse declaration statement\n");
    Token* current = parser_advance(parser);
    if (!current) {
        DPRINT("[PARSER] ERROR: No current token\n");
        return NULL;
    }
    
    SourceLocation loc = (SourceLocation){current->line, current->column};
    DPRINT("[PARSER] Declaration type: %d, value: %s\n", current->type, current->value);

    Token* identifier = parser_consume(parser, IDENTIFIER, "declaration should have a naming");
    if (!identifier) {
        DPRINT("[PARSER] ERROR: No identifier in declaration\n");
        return NULL;
    }

    if (parser_peek(parser)->type == OP_ASSIGN) {
        DPRINT("[PARSER] Parsing variable declaration\n");
        parser_retreat(parser);
        parser_retreat(parser);
        return parser_parse_variable_declaration_statement(parser);
    }
    
    DPRINT("[PARSER] Parsing function declaration\n");
    parser_retreat(parser);
    parser_retreat(parser);
    return parser_parse_function_declaration_statement(parser);
}

static ASTNode* parser_parse_assignment_statement(Parser* parser) {
    DPRINT("[PARSER] Starting to parse assignment statement\n");
    Token* identifier = parser_consume(parser, IDENTIFIER, "Expected identifier in assignment");
    if (!identifier) return NULL;

    Token* assign = parser_consume(parser, OP_ASSIGN, "Expected '=' in assignment");
    if (!assign) {
        return NULL;
    }

    ASTNode* rhs = parser_parse_expression(parser);
    if (!rhs) return NULL;

    SourceLocation loc = (SourceLocation){identifier->line, identifier->column};
    ASTNode* lhs = ast_new_variable_expression(loc, identifier->value);
    if (!lhs) {
        ast_free(rhs);
        return NULL;
    }

    ASTNode* assign_node = ast_new_assignment_statement(loc, lhs, rhs);
    ast_free(lhs);
    ast_free(rhs);
    return assign_node;
}

static ASTNode* parser_parse_primary_expression(Parser* parser) {
    Token* current = parser_peek(parser);
    SourceLocation loc = (SourceLocation){current->line, current->column};
    
    DPRINT("[PARSER] parse_primary_expression: token_type=%d, value='%s' at line %d, col %d\n", 
           current->type, current->value, loc.line, loc.column);
    
    switch (current->type) {
        case IDENTIFIER: {
            DPRINT("[PARSER] Found IDENTIFIER: '%s'\n", current->value);
            parser_advance(parser);
            Token* next = parser_peek(parser);
            DPRINT("[PARSER] Next token after identifier: type=%d, value='%s'\n", next->type, next->value);
            
            switch (next->type) {
                case LPAREN: {
                    DPRINT("[PARSER] Identifier followed by LPAREN -> parsing function call\n");
                    parser_retreat(parser);
                    return parser_parse_function_call_expression(parser);
                }
                default: {
                    DPRINT("[PARSER] Creating VariableExpression for '%s'\n", current->value);
                    ASTNode* node = ast_new_variable_expression(loc, current->value);
                    if (!node) {
                        DPRINT("[PARSER] ERROR: Failed to create VariableExpression\n");
                        return NULL;
                    }
                    return node;
                }
            }
        }
        case INT_LITERAL: {
            int value = atoi(current->value);
            DPRINT("[PARSER] Found INT_LITERAL: value=%d\n", value);
            ASTNode* node = ast_new_literal_expression(loc, TYPE_INT, value);
            if (!node) {
                DPRINT("[PARSER] ERROR: Failed to create LiteralExpression for int\n");
                return NULL;
            }
            parser_advance(parser);
            return node;
        }
        case KW_TRUE:
        case KW_FALSE: {
            int value = (strcmp(current->value, "true") == 0) ? 1 : 0;
            DPRINT("[PARSER] Found BOOL_LITERAL: value='%s' -> %d\n", current->value, value);
            ASTNode* node = ast_new_literal_expression(loc, TYPE_BOOL, value);
            if (!node) {
                DPRINT("[PARSER] ERROR: Failed to create LiteralExpression for bool\n");
                return NULL;
            }
            parser_advance(parser);
            return node;
        }
        case LONG_LITERAL: {
            long value = atol(current->value);
            DPRINT("[PARSER] Found LONG_LITERAL: value=%ld\n", value);
            ASTNode* node = ast_new_literal_expression(loc, TYPE_LONG, value);
            if (!node) {
                DPRINT("[PARSER] ERROR: Failed to create LiteralExpression for long\n");
                return NULL;
            }
            parser_advance(parser);
            return node;
        }
        case LPAREN: {
            // Parenthesized expression
            parser_advance(parser); // consume LPAREN
            ASTNode* inner = parser_parse_expression(parser);
            if (!parser_consume(parser, RPAREN, "Expected ')' after expression")) {
                ast_free(inner);
                return NULL;
            }
            return inner;
        }
        default: {
            DPRINT("[PARSER] ERROR: Unexpected token type %d, value='%s'\n", current->type, current->value);
            report_error(parser, "couldn't parse token");
            return NULL;
        }
    }
}

static ASTNode* parser_parse_expression(Parser* parser) {
    return parser_parse_precedence(parser, PRECEDENCE_ASSIGNMENT);
}

static ASTNode* parser_parse_precedence(Parser* parser, Precedence min_precedence) {
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
        case KW_TRUE:
        case KW_FALSE:
        case LONG_LITERAL:
        case LPAREN:
            left = parser_parse_primary_expression(parser);
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

static ASTNode* parser_parse_unary_expression(Parser* parser) {
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

static ASTNode* parser_parse_expression_statement(Parser* parser) {
    ASTNode* expression = parser_parse_expression(parser);
    if (!expression) return NULL;
    
    ASTNode* node = ast_new_expression_statement(expression->location, expression);
    ast_free(expression);
    
    return node;
}

static ASTNode* parser_parse_statement(Parser* parser) {
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
            res = parser_parse_block(parser);
            break;

        case END_OF_FILE:
            res = NULL;
            break;

        case INT_LITERAL:
        case KW_TRUE:
        case KW_FALSE:
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

static ASTNode* parser_parse_block(Parser* parser) {
    DPRINT("[PARSER] Starting to parse block\n");
    
    Token* lbrace = parser_consume(parser, LBRACE, "Expected '{' to start block");
    if (!lbrace) {
        DPRINT("[PARSER] ERROR: Missing opening bracket for block\n");
        return NULL;
    }
    
    SourceLocation loc = (SourceLocation){lbrace->line, lbrace->column};
    DPRINT("[PARSER] Block starts at line %d, column %d\n", loc.line, loc.column);
    
    ASTNode* block = ast_new_block_statement(loc, NULL, 0);
    if (!block) {
        DPRINT("[PARSER] ERROR: Failed to create block statement\n");
        return NULL;
    }

    while (!parser_is_at_end(parser) && parser_peek(parser) != NULL && parser_peek(parser)->type != RBRACE) {
        Token* current = parser_peek(parser);
        DPRINT("[PARSER] Parsing statement in block: type=%s, value='%s'\n", 
               token_type_to_string(current->type), current->value);
        
        ASTNode* stmt = parser_parse_statement(parser);
        if (!stmt) {
            DPRINT("[PARSER] ERROR: Failed to parse statement in block\n");
            Token* cur = parser_peek(parser);
            if (cur) DPRINT("[PARSER] Current token at fail: type=%s value='%s'\n", token_type_to_string(cur->type), cur->value);
            ast_free(block);
            return NULL;
        }
        
        add_statement_to_block(block, stmt);
        DPRINT("[PARSER] Successfully added statement to block\n");
    }

    if (!parser_consume(parser, RBRACE, "Expected '}' after block")) {
        DPRINT("[PARSER] ERROR: Missing closing bracket for block\n");
        ast_free(block);
        return NULL;
    }
    
    DPRINT("[PARSER] Successfully parsed block\n");
    //printf(token_type_to_string(parser->tokens[parser->current].type));
    return block;
}

ASTNode* parser_parse(Parser* parser) {
    // Main function for starting parsing, use it with initialized parser to get ast_tree
    DPRINT("[PARSER] Starting parsing\n");
    
    ASTNode* block_statements = ast_new_block_statement((SourceLocation){.line=0, .column=0}, NULL, 0);
    if (!block_statements) {
        DPRINT("[PARSER] ERROR: Failed to create block statement\n");
        return NULL;
    }

    while (!parser_is_at_end(parser)) {
        Token* current = parser_peek(parser);
        
        if (current->type == END_OF_FILE) {
            DPRINT("[PARSER] Reached end of file\n");
            break;
        }
        
        DPRINT("[PARSER] Parsing statement, current token: type=%d, value='%s'\n", 
               current->type, current->value);
        
        ASTNode* stmt = parser_parse_statement(parser);
        if (!stmt) {
            Token* error_token = parser_peek(parser);
            DPRINT("[PARSER] ERROR: Couldn't parse statement at token type=%s, value='%s'\n", 
                   token_type_to_string(error_token->type), error_token->value);
            ast_free(block_statements);
            return NULL;
        }
        
        DPRINT("[PARSER] Successfully parsed statement, adding to block\n");
        ast_print(stmt,0);
        add_statement_to_block(block_statements, stmt);
        
        ast_free(stmt);
    }
    
    DPRINT("[PARSER] Finished parsing, returning block with statements\n");
    return block_statements;
}
