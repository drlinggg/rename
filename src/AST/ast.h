#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../lexer/token.h"

typedef struct { // debug info
    int line;
    int column;
} SourceLocation;

typedef enum {
     TYPE_INT, TYPE_LONG, TYPE_BOOL, TYPE_ARRAY, TYPE_STRUCT, TYPE_NONE,
} TypeVar;

typedef enum NodeType {
    // Statements
    NODE_PROGRAM,
    NODE_FUNCTION_DECLARATION_STATEMENT,
    NODE_VARIABLE_DECLARATION_STATEMENT,
    NODE_EXPRESSION_STATEMENT,
    NODE_RETURN_STATEMENT,
    NODE_BLOCK_STATEMENT,
    NODE_IF_STATEMENT,
    NODE_WHILE_STATEMENT,
    NODE_FOR_STATEMENT,
    NODE_ASSIGNMENT_STATEMENT,

    // Loops control flow
    NODE_BREAK_STATEMENT,
    NODE_CONTINUE_STATEMENT,
    
    // Expressions
    NODE_BINARY_EXPRESSION,
    NODE_UNARY_EXPRESSION,
    NODE_LITERAL_EXPRESSION,
    NODE_VARIABLE_EXPRESSION,
    NODE_FUNCTION_CALL_EXPRESSION,

    // array
    NODE_ARRAY_EXPRESSION,
    NODE_SUBSCRIPT_EXPRESSION,
    NODE_ARRAY_DECLARATION_STATEMENT
} NodeType;

typedef struct ASTNode {
    NodeType node_type;
    SourceLocation location;
} ASTNode;

// Expressions
typedef struct {
    ASTNode base;
    ASTNode* left;
    ASTNode* right;
    Token operator_;
} BinaryExpression;

typedef struct {
    ASTNode base;
    ASTNode* callee;
    ASTNode** arguments;
    int8_t argument_count;
} FunctionCallExpression;

typedef struct {
    ASTNode base;
    Token operator_;
    ASTNode* operand;
} UnaryExpression;

typedef struct {
    ASTNode base;
    TypeVar type;
    int64_t value;
} LiteralExpression;

typedef struct {
    ASTNode base;
    char* name;
} VariableExpression;

typedef struct {
    TypeVar type;
    char* name;
} Parameter;

// Statements
typedef struct {
    ASTNode base;
    ASTNode* left;
    ASTNode* right;
} AssignmentStatement;

typedef struct {
    ASTNode base;
    TypeVar var_type;
    char* name;
    ASTNode* initializer;
} VariableDeclarationStatement;

typedef struct {
    ASTNode base;
    TypeVar return_type;
    char* name;
    Parameter* parameters;
    size_t parameter_count;
    ASTNode* body;
} FunctionDeclarationStatement;

typedef struct {
    ASTNode base;
    ASTNode* expression;
} ReturnStatement;

typedef struct {
    ASTNode base;
    ASTNode* expression;
} ExpressionStatement;

typedef struct {
    ASTNode base;
    ASTNode** statements;
    size_t statement_count;
} BlockStatement;

typedef struct {
    ASTNode base;
    ASTNode* condition;
    ASTNode* then_branch;
    ASTNode** elif_conditions;
    ASTNode** elif_branches;
    size_t elif_count;
    ASTNode* else_branch;
} IfStatement;

typedef struct {
    ASTNode base;
    ASTNode* condition;
    ASTNode* body;
} WhileStatement;

typedef struct {
    ASTNode base;
    ASTNode* initializer;
    ASTNode* condition;
    ASTNode* increment;
    ASTNode* body;
} ForStatement;

typedef struct {
    ASTNode base;
} BreakStatement;

typedef struct {
    ASTNode base;
} ContinueStatement;

typedef struct {
    ASTNode base;
    TypeVar element_type;
    char* name;
    ASTNode* size;
    ASTNode* initializer;
} ArrayDeclarationStatement;

typedef struct {
    ASTNode base;
    ASTNode** elements;
    size_t element_count;
} ArrayExpression;

typedef struct {
    ASTNode base;
    ASTNode* array;
    ASTNode* index;
} SubscriptExpression;

// Function declarations
void ast_free(ASTNode* node);
ASTNode* ast_new_binary_expression(SourceLocation loc, ASTNode* left, Token op, ASTNode* right);
ASTNode* ast_new_unary_expression(SourceLocation loc, Token op, ASTNode* operand);
ASTNode* ast_new_literal_expression(SourceLocation loc, TypeVar type, int64_t value);
ASTNode* ast_new_variable_expression(SourceLocation loc, const char* name);
ASTNode* ast_new_call_expression(SourceLocation loc, ASTNode* callee, ASTNode** args, int arg_count);
ASTNode* ast_new_assignment_statement(SourceLocation loc, ASTNode* left, ASTNode* right);
ASTNode* ast_new_variable_declaration_statement(SourceLocation loc, TypeVar var_type, const char* name, ASTNode* initializer);
ASTNode* ast_new_return_statement(SourceLocation loc, ASTNode* expression);
ASTNode* ast_new_expression_statement(SourceLocation loc, ASTNode* expression);
ASTNode* ast_new_block_statement(SourceLocation loc, ASTNode** statements, size_t count);
ASTNode* ast_new_if_statement(SourceLocation loc, ASTNode* condition, ASTNode* then_branch);
ASTNode* ast_new_while_statement(SourceLocation loc, ASTNode* condition, ASTNode* body);
ASTNode* ast_new_for_statement(SourceLocation loc, ASTNode* initializer, ASTNode* condition, ASTNode* increment, ASTNode* body);
ASTNode* ast_new_function_declaration_statement(SourceLocation loc, const char* name, TypeVar return_type, Parameter* parameters, size_t parameter_count, ASTNode* body);
ASTNode* ast_new_break_statement(SourceLocation loc);
ASTNode* ast_new_continue_statement(SourceLocation loc);
ASTNode* ast_new_array_expression(SourceLocation loc, ASTNode** elements, size_t element_count);
ASTNode* ast_new_subscript_expression(SourceLocation loc, ASTNode* array, ASTNode* index);
ASTNode* ast_new_array_declaration_statement(SourceLocation loc, TypeVar element_type, const char* name, ASTNode* size, ASTNode* initializer);
Parameter* ast_new_parameter(const char* name, TypeVar type);
void parameter_free(Parameter* param);
bool add_statement_to_block(ASTNode* block_stmt, ASTNode* stmt);
const char* ast_node_type_to_string(int node_type);
const char* type_var_to_string(int node_type);
const char* token_type_to_string(int token_type);
const TypeVar token_type_to_type_var(TokenType type);
void ast_print(ASTNode* node, int indent);
void ast_print_tree(ASTNode* node, int indent);
