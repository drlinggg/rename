#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include "../system.h"

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
        case OP_MULT: case OP_DIV: case OP_MOD: case KW_IS: return PRECEDENCE_FACTOR;
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
static ASTNode* parser_parse_function_declaration_statement(Parser* parser);
static ASTNode* parser_parse_assignment_statement(Parser* parser);
static ASTNode* parser_parse_if_statement(Parser* parser);
static ASTNode* parser_parse_while_statement(Parser* parser);
static ASTNode* parser_parse_for_statement(Parser* parser);
static ASTNode* parser_parse_return_statement(Parser* parser);
static ASTNode* parser_parse_block_statement(Parser* parser);
static ASTNode* parser_parse_expression_statement(Parser* parser);

static ASTNode* parser_parse_break_statement(Parser* parser);
static ASTNode* parser_parse_continue_statement(Parser* parser);

static ASTNode* parser_parse_precedence(Parser* parser, Precedence min_precedence);
static ASTNode* parser_parse_binary_expression(Parser* parser);
static ASTNode* parser_parse_unary_expression(Parser* parser);
static ASTNode* parser_parse_primary_expression(Parser* parser);
static ASTNode* parser_parse_literal_expression(Parser* parser);
static ASTNode* parser_parse_function_call_expression(Parser* parser);
static ASTNode* parser_parse_variable_expression(Parser* parser);

static ASTNode* parse_array_expression(Parser* parser);
static ASTNode* parse_subscript_expression(Parser* parser, ASTNode* array);


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

static ASTNode* parse_array_expression(Parser* parser) {
    Token* current = parser_peek(parser);
    if (!current) return NULL;
    
    SourceLocation loc = {current->line, current->column};
    parser_consume(parser, LBRACKET, "Expected '[' at start of array expression");
    
    ASTNode** elements = NULL;
    size_t element_count = 0;
    size_t capacity = 4;
    
    elements = malloc(capacity * sizeof(ASTNode*));
    if (!elements) return NULL;
    
    // Parse array elements
    if (parser_peek(parser) && parser_peek(parser)->type != RBRACKET) {
        do {
            if (element_count >= capacity) {
                capacity *= 2;
                ASTNode** new_elements = realloc(elements, capacity * sizeof(ASTNode*));
                if (!new_elements) {
                    for (size_t i = 0; i < element_count; i++) {
                        ast_free(elements[i]);
                    }
                    free(elements);
                    return NULL;
                }
                elements = new_elements;
            }
            
            ASTNode* expr = parser_parse_expression(parser);
            if (!expr) {
                for (size_t i = 0; i < element_count; i++) {
                    ast_free(elements[i]);
                }
                free(elements);
                return NULL;
            }
            
            elements[element_count++] = expr;
            
            if (!parser_peek(parser) || parser_peek(parser)->type != COMMA) {
                break;
            }
            
            parser_consume(parser, COMMA, "Expected ',' between array elements");
        } while (true);
    }
    
    parser_consume(parser, RBRACKET, "Expected ']' at end of array expression");
    
    // Create array node
    ASTNode* array_node = ast_new_array_expression(loc, elements, element_count);
    
    // Free temporary elements array (the array expression owns the copies)
    for (size_t i = 0; i < element_count; i++) {
        ast_free(elements[i]);
    }
    free(elements);
    
    return array_node;
}

static ASTNode* parse_subscript_expression(Parser* parser, ASTNode* array) {
    Token* current = parser_peek(parser);
    if (!current) return NULL;
    
    SourceLocation loc = {current->line, current->column};
    parser_consume(parser, LBRACKET, "Expected '[' after array in subscript expression");
    
    ASTNode* index = parser_parse_expression(parser);
    if (!index) {
        return NULL;
    }
    
    parser_consume(parser, RBRACKET, "Expected ']' after index in subscript expression");
    
    return ast_new_subscript_expression(loc, array, index);
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
        
        // Проверяем, не пустой ли список аргументов или конец
        if (current_arg_token->type == RPAREN) {
            DPRINT("[PARSER] [FUNCTION CALL] Found closing parenthesis, function call has %zu arguments\n", argument_count);
            parser_advance(parser);
            
            // Обрезаем массив до фактического размера
            if (argument_count > 0 && argument_count < capacity) {
                ASTNode** trimmed_args = realloc(function_arguments, argument_count * sizeof(ASTNode*));
                if (trimmed_args) {
                    function_arguments = trimmed_args;
                    DPRINT("[PARSER] [FUNCTION CALL] Trimmed arguments array to size %zu\n", argument_count);
                }
            }

            ASTNode* function_call_expr = ast_new_call_expression(loc, function_identifier, 
                (argument_count > 0) ? function_arguments : NULL, argument_count);
            if (!function_call_expr) {
                DPRINT("[PARSER] [FUNCTION CALL] ERROR: Failed to create function call expression node\n");
                for (size_t i = 0; i < argument_count; i++) {
                    ast_free(function_arguments[i]);
                }
                free(function_arguments);
                ast_free(function_identifier);
                return NULL;
            }

            DPRINT("[PARSER] Successfully created function call expression with %zu arguments\n", argument_count);
            return function_call_expr;
        }
        
        // Парсим аргумент
        ASTNode* argument = parser_parse_expression(parser);
        if (!argument) {
            DPRINT("[PARSER] [FUNCTION CALL] Failed to parse argument %zu\n", argument_count);
            for (size_t i = 0; i < argument_count; i++) {
                ast_free(function_arguments[i]);
            }
            free(function_arguments);
            ast_free(function_identifier);
            return NULL;
        }
        
        // Добавляем аргумент в массив
        if (argument_count >= capacity) {
            capacity *= 2;
            DPRINT("[PARSER] [FUNCTION CALL] Expanding arguments array to capacity %zu\n", capacity);
            ASTNode** new_args = realloc(function_arguments, capacity * sizeof(ASTNode*));
            if (!new_args) {
                DPRINT("[PARSER] [FUNCTION CALL] ERROR: Failed to reallocate arguments array\n");
                ast_free(argument);
                for (size_t i = 0; i < argument_count; i++) {
                    ast_free(function_arguments[i]);
                }
                free(function_arguments);
                ast_free(function_identifier);
                return NULL;
            }
            function_arguments = new_args;
        }
        
        function_arguments[argument_count++] = argument;
        DPRINT("[PARSER] [FUNCTION CALL] Added argument %zu\n", argument_count);
      
        // Проверяем следующий токен
        Token* next_after_arg = parser_peek(parser);
        
        if (next_after_arg->type == COMMA) {
            DPRINT("[PARSER] [FUNCTION CALL] Found comma, continuing to next argument\n");
            parser_advance(parser);
            continue;
        }
        else if (next_after_arg->type == RPAREN) {
            DPRINT("[PARSER] [FUNCTION CALL] Found closing parenthesis, function call has %zu arguments\n", argument_count);
            parser_advance(parser);
            
            // Обрезаем массив до фактического размера
            if (argument_count > 0 && argument_count < capacity) {
                ASTNode** trimmed_args = realloc(function_arguments, argument_count * sizeof(ASTNode*));
                if (trimmed_args) {
                    function_arguments = trimmed_args;
                    DPRINT("[PARSER] [FUNCTION CALL] Trimmed arguments array to size %zu\n", argument_count);
                }
            }

            ASTNode* function_call_expr = ast_new_call_expression(loc, function_identifier, 
                (argument_count > 0) ? function_arguments : NULL, argument_count);
            if (!function_call_expr) {
                DPRINT("[PARSER] [FUNCTION CALL] ERROR: Failed to create function call expression node\n");
                for (size_t i = 0; i < argument_count; i++) {
                    ast_free(function_arguments[i]);
                }
                free(function_arguments);
                ast_free(function_identifier);
                return NULL;
            }

            DPRINT("[PARSER] Successfully created function call expression with %zu arguments\n", argument_count);
            return function_call_expr;
        }
        else {
            DPRINT("[PARSER] [FUNCTION CALL] ERROR: Expected comma or closing parenthesis, got %s\n", 
                   token_type_to_string(next_after_arg->type));
            for (size_t i = 0; i < argument_count; i++) {
                ast_free(function_arguments[i]);
            }
            free(function_arguments);
            ast_free(function_identifier);
            return NULL;
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
            peek_token->type == KW_BOOL) {
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
    DPRINT("[PARSER] [FOR LOOP]   - Condition: %s\n", condition ? ast_node_type_to_string(condition->node_type) : "NULL");
    DPRINT("[PARSER] [FOR LOOP]   - Increment: %s\n", increment ? ast_node_type_to_string(increment->node_type) : "NULL");
    DPRINT("[PARSER] [FOR LOOP]   - Body: %s\n", body ? ast_node_type_to_string(body->node_type) : "NULL");
    
    ASTNode* for_node = ast_new_for_statement(loc, initializer, condition, increment, body);
    
    if (for_node) {
        DPRINT("[PARSER] [FOR LOOP] Successfully created for statement node\n");
    } else {
        DPRINT("[PARSER] [FOR LOOP] ERROR: Failed to create for statement node\n");
    }
    
    return for_node;
}

static ASTNode* parser_parse_return_statement(Parser* parser) {
    DPRINT("[PARSER] parse_return_statement\n");
    
    Token* return_token = parser_consume(parser, KW_RETURN, "return statement should start with return keyword");
    if (!return_token) {
        DPRINT("[PARSER] ERROR: Missing 'return' keyword\n");
        return NULL;
    }
    
    SourceLocation loc = (SourceLocation){return_token->line, return_token->column};
    DPRINT("[PARSER] Return at line %d, column %d\n", loc.line, loc.column);
    
    // Парсим выражение после return
    ASTNode* expr = parser_parse_expression(parser);
    DPRINT("[PARSER] Return expression: %p\n", (void*)expr);
    
    // Если выражение NULL, создаем return с None
    if (!expr) {
        DPRINT("[PARSER] WARNING: Return has no expression, using None\n");
        // Создаем None литерал
        expr = ast_new_literal_expression(loc, TYPE_NONE, 0);
        if (!expr) {
            DPRINT("[PARSER] ERROR: Failed to create None literal\n");
            return NULL;
        }
    }
    
    ASTNode* return_node = ast_new_return_statement(loc, expr);
    DPRINT("[PARSER] Created return statement: %p\n", (void*)return_node);
    
    // Освобождаем expr, так как он теперь принадлежит return_node
    ast_free(expr);
    
    return return_node;
}

static ASTNode* parser_parse_break_statement(Parser* parser) {
    DPRINT("[PARSER] parse_break_statement\n");
    
    Token* break_token = parser_consume(parser, KW_BREAK, "break statement should start with break keyword");
    if (!break_token) {
        DPRINT("[PARSER] ERROR: Missing 'break' keyword\n");
        return NULL;
    }
    
    SourceLocation loc = (SourceLocation){break_token->line, break_token->column};
    DPRINT("[PARSER] break at line %d, column %d\n", loc.line, loc.column);
    
    ASTNode* break_node = ast_new_break_statement(loc);
    DPRINT("[PARSER] Created break statement: %p\n", (void*)break_node);
    
    return break_node;
}

static ASTNode* parser_parse_array_declaration_statement(Parser* parser) {
    DPRINT("[PARSER] Parsing array declaration statement\n");
    DPRINT("[PARSER] Array element type is %s\n", 
           parser_peek(parser) ? token_type_to_string(parser_peek(parser)->type) : "NULL");
    Token* array_token = parser_advance(parser);
    SourceLocation loc = (SourceLocation){array_token->line, array_token->column};
   
    // Parse array size
    parser_consume(parser, LBRACKET, "Expected '[' after array name");
    ASTNode* size = parser_parse_expression(parser);
    if (!size) return NULL;
    parser_consume(parser, RBRACKET, "Expected ']' after array size");

    // Parse array name
    Token* identifier = parser_consume(parser, IDENTIFIER, "Expected array name");
    if (!identifier) return NULL;

    // Parse optional initializer
    ASTNode* initializer = NULL;
    if (parser_peek(parser) && parser_peek(parser)->type == OP_ASSIGN) {
        parser_advance(parser);
        initializer = parser_parse_expression(parser);
    }
    DPRINT("[PARSER] Finished parsing array declaration for '%s'\n", identifier->value);
    return ast_new_array_declaration_statement(loc, token_type_to_type_var(array_token->type), 
                                               identifier->value, size, initializer);
}

static ASTNode* parser_parse_continue_statement(Parser* parser) {
    DPRINT("[PARSER] parse_continue_statement\n");
    
    Token* continue_token = parser_consume(parser, KW_CONTINUE, "continue statement should start with continue keyword");
    if (!continue_token) {
        DPRINT("[PARSER] ERROR: Missing 'continue' keyword\n");
        return NULL;
    }
    
    SourceLocation loc = (SourceLocation){continue_token->line, continue_token->column};
    DPRINT("[PARSER] continue at line %d, column %d\n", loc.line, loc.column);
    
    ASTNode* continue_node = ast_new_continue_statement(loc);
    DPRINT("[PARSER] Created continue statement: %p\n", (void*)continue_node);
    
    return continue_node;
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
    return node;
}


static ASTNode* parser_parse_function_declaration_statement(Parser* parser) {
    DPRINT("[PARSER] Starting to parse function declaration\n");
    Token* return_type_token = parser_advance(parser);
    SourceLocation loc = (SourceLocation){return_type_token->line, return_type_token->column};
    if (return_type_token->type == KW_VOID) {
        return_type_token->type = KW_NONE;
    }

    Token* identifier = parser_consume(parser, IDENTIFIER, "Function declaration should have a name");
    if (!identifier) return NULL;

    if (!parser_consume(parser, LPAREN, "Expected '(' after function name")) {
        return NULL;
    }
    
    Parameter* parameters = NULL;
    size_t param_count = 0;
    size_t param_capacity = 4;
    
    parameters = malloc(sizeof(Parameter) * param_capacity);
    if (!parameters) return NULL;

    // Парсим список параметров (может быть пустым)
    while (parser_peek(parser)->type != RPAREN && !parser_is_at_end(parser)) {
        if (param_count >= param_capacity) {
            param_capacity *= 2;
            Parameter* new_params = realloc(parameters, sizeof(Parameter) * param_capacity);
            if (!new_params) {
                for (size_t i = 0; i < param_count; i++) {
                    free(parameters[i].name);
                }
                free(parameters);
                return NULL;
            }
            parameters = new_params;
        }

        // Парсим тип параметра
        Token* param_type_token = parser_advance(parser);
        if (!param_type_token) {
            for (size_t i = 0; i < param_count; i++) {
                free(parameters[i].name);
            }
            free(parameters);
            return NULL;
        }
        
        TypeVar param_type = token_type_to_type_var(param_type_token->type);
        if (param_type == TYPE_NONE) {
            report_error(parser, "Invalid parameter type");
            for (size_t i = 0; i < param_count; i++) {
                free(parameters[i].name);
            }
            free(parameters);
            return NULL;
        }
        
        // Проверяем, не массив ли это (следующий токен '[')
        bool is_array = false;
        
        if (parser_peek(parser) && parser_peek(parser)->type == LBRACKET) {
            is_array = true;
            parser_advance(parser); // Пропускаем '['
            
            // Пропускаем ']' (размер не важен)
            parser_consume(parser, RBRACKET, "Expected ']' after array type");
        }
        
        // Парсим имя параметра
        Token* param_name = parser_consume(parser, IDENTIFIER, "Expected parameter name after type");
        if (!param_name) {
            for (size_t i = 0; i < param_count; i++) {
                free(parameters[i].name);
            }
            free(parameters);
            return NULL;
        }
        
        // Сохраняем параметр
        parameters[param_count].name = strdup(param_name->value);
        if (!parameters[param_count].name) {
            for (size_t i = 0; i < param_count; i++) {
                free(parameters[i].name);
            }
            free(parameters);
            return NULL;
        }
        parameters[param_count].type = param_type;
        parameters[param_count].is_array = is_array;  // Отмечаем, что это массив
        param_count++;

        // Проверяем, есть ли следующий параметр (через запятую)
        if (parser_peek(parser)->type == COMMA) {
            parser_advance(parser);
        } else {
            // Если нет запятой, выходим из цикла
            break;
        }
    }

    if (!parser_consume(parser, RPAREN, "Expected ')' after parameters")) {
        for (size_t i = 0; i < param_count; i++) {
            free(parameters[i].name);
        }
        free(parameters);
        return NULL;
    }

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
    if ((current->type == KW_INT || current->type == KW_BOOL) && parser_peek(parser)->type == LBRACKET) {
        parser_retreat(parser);
        parser_retreat(parser);
        DPRINT("[PARSER] Parsing array declaration\n");
        DPRINT("[PARSER] Current token after retreat: type=%d, value=%s\n", parser_peek(parser)->type, parser_peek(parser)->value);
        return parser_parse_array_declaration_statement(parser);
    }

    Token* identifier = parser_consume(parser, IDENTIFIER, "declaration should have a naming");
    if (!identifier) {
        DPRINT("[PARSER] ERROR: No identifier in declaration\n");
        return NULL;
    }

    DPRINT("[PARSER] found after identifier: %s\n", token_type_to_string(parser_peek(parser)->type));
    if (parser_peek(parser)->type == OP_ASSIGN) {
        DPRINT("[PARSER] Parsing variable declaration\n");
        parser_retreat(parser);
        parser_retreat(parser);
        return parser_parse_variable_declaration_statement(parser);
    }
    else if (parser_peek(parser)->type == SEMICOLON) {
        DPRINT("[PARSER] Parsing variable declaration (no initialization)\n");
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
    
    // Парсим левую часть - это может быть переменная или индексация массива
    ASTNode* lhs = NULL;
    
    // Парсим идентификатор
    Token* identifier_token = parser_consume(parser, IDENTIFIER, "Expected identifier in assignment");
    if (!identifier_token) return NULL;
    
    SourceLocation loc = (SourceLocation){identifier_token->line, identifier_token->column};
    
    // Проверяем, является ли это индексацией массива
    if (parser_peek(parser) && parser_peek(parser)->type == LBRACKET) {
        // Это индексация массива
        DPRINT("[PARSER] Parsing array subscript in LHS of assignment\n");
        ASTNode* array_expr = ast_new_variable_expression(loc, identifier_token->value);
        lhs = parse_subscript_expression(parser, array_expr);
        ast_free(array_expr);
    } else {
        // Это обычная переменная
        DPRINT("[PARSER] Parsing simple variable in LHS of assignment\n");
        lhs = ast_new_variable_expression(loc, identifier_token->value);
    }
    
    if (!lhs) {
        return NULL;
    }

    // Парсим оператор присваивания
    Token* assign = parser_consume(parser, OP_ASSIGN, "Expected '=' in assignment");
    if (!assign) {
        ast_free(lhs);
        return NULL;
    }

    // Парсим правую часть
    ASTNode* rhs = parser_parse_expression(parser);
    if (!rhs) {
        ast_free(lhs);
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
                case LBRACKET: {
                    ASTNode* array = ast_new_variable_expression(loc, current->value);
                    return parse_subscript_expression(parser, array);
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
        case KW_NONE: {
            DPRINT("[PARSER] Found KW_NONE, creating literal expression\n");
            ASTNode* node = ast_new_literal_expression(loc, TYPE_NONE, 0);
            DPRINT("[PARSER] Created node at %p\n", (void*)node);
            if (!node) {
                DPRINT("[PARSER] ERROR: Failed to create LiteralExpression for none\n");
                return NULL;
            }
            parser_advance(parser);
            DPRINT("[PARSER] Returning node for None\n");
            return node;
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
        case FLOAT_LITERAL: {
            char* value = strdup(current->value);
            DPRINT("[PARSER] Found FLOAT_LITERAL: value=%s\n", value);
            ASTNode* node = ast_new_literal_expression_long_arithmetics(loc, TYPE_FLOAT, value);
            if (!node) {
                DPRINT("[PARSER] ERROR: Failed to create LiteralExpression for float\n");
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
        case LBRACKET: {
            return parse_array_expression(parser);
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

static ASTNode* parser_parse_unary_expression(Parser* parser) {
    Token* operator_ = parser_advance(parser);
    SourceLocation loc = (SourceLocation){operator_->line, operator_->column};
    
    if (operator_->type != OP_PLUS && operator_->type != OP_MINUS && operator_->type != OP_NOT) {
        report_error(parser, "Expected unary operator");
        return NULL;
    }
    
    // Парсим выражение с приоритетом PRECEDENCE_UNARY, чтобы унарный оператор
    // применялся только к первичным выражениям, а не к бинарным операциям
    ASTNode* operand = parser_parse_precedence(parser, PRECEDENCE_UNARY);
    if (!operand) {
        return NULL;
    }
    
    ASTNode *node = ast_new_unary_expression(loc, *operator_, operand);
    
    ast_free(operand);
    
    return node;
}

static ASTNode* parser_parse_precedence(Parser* parser, Precedence min_precedence) {
    Token* current = parser_peek(parser);
    if (!current) return NULL;
    
    SourceLocation loc = (SourceLocation){current->line, current->column};
    
    // Parse left - унарные операторы имеют высший приоритет (PRECEDENCE_UNARY = 8)
    ASTNode* left = NULL;
    switch (current->type) {
        case OP_PLUS:
        case OP_MINUS:
        case OP_NOT:
            // Парсим унарные операторы только если текущий минимальный приоритет позволяет
            if (min_precedence <= PRECEDENCE_UNARY) {
                left = parser_parse_unary_expression(parser);
            } else {
                // Если мы внутри выражения с более высоким приоритетом (чего не может быть, 
                // так как UNARY самый высокий), то это ошибка
                return NULL;
            }
            break;
        case IDENTIFIER:
        case INT_LITERAL:
        case FLOAT_LITERAL:
        case LONG_LITERAL:
        case KW_TRUE:
        case KW_FALSE:
        case LPAREN:
        case KW_NONE:
            left = parser_parse_primary_expression(parser);
            break;
        case LBRACKET:
            left = parse_array_expression(parser);
            break;
        default:
            return NULL;
    }

    if (!left) return NULL;

    // parse operations while their precedence >= min_precedence
    while (true) {
        Token* next = parser_peek(parser);
        if (!next) break;
        
        Precedence precedence = get_precedence(next->type);
        
        if (precedence < min_precedence) break;

        Token operator_token = *next;
        parser_advance(parser);

        // Для правоассоциативных операторов (например, =) нужно precedence вместо precedence + 1
        // Но у нас все операторы левоассоциативные, поэтому используем precedence + 1
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
        case KW_VOID:
            res = parser_parse_function_declaration_statement(parser);
            flag_fun_declaration = true;
            break;
        case KW_INT:
        case KW_LONG:
        case KW_BOOL:
        case KW_FLOAT:
            parser_advance(parser);
            Token* next_token = parser_peek(parser);
            
            if (parser_peek(parser)->type == LBRACKET) {
                parser_retreat(parser);
                res = parser_parse_array_declaration_statement(parser);
                break;
            }
            else if (!parser_consume(parser, IDENTIFIER, "expected identifier after type")) {
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
                if (parser_check(parser, OP_ASSIGN) || parser_check(parser, LBRACKET)) {
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
        case KW_NONE:
        case LONG_LITERAL:
        case OP_PLUS:
        case OP_MINUS:
        case OP_NOT:
            res = parser_parse_expression_statement(parser);
            break;
        case KW_BREAK:
            res = parser_parse_break_statement(parser);
            break;
        case KW_CONTINUE:
            res = parser_parse_continue_statement(parser);
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
        
        if (add_statement_to_block(block, stmt))
            DPRINT("[PARSER] Successfully added statement to block\n");
        else
            DPRINT("[PARSER] Failed to add statement to block\n");

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
            DPRINT("[PARSER] ERROR: Couldn't parse statement at token type=%s, value='%s'\n, ", 
                   token_type_to_string(error_token->type), error_token->value);
            ast_free(block_statements);
            return NULL;
        }
        
        DPRINT("[PARSER] Successfully parsed statement, adding to block\n");
        add_statement_to_block(block_statements, stmt);
        
        ast_free(stmt);
    }
    
    DPRINT("[PARSER] Finished parsing, returning block with statements\n");
    ast_print(block_statements, 0);
    return block_statements;
}
