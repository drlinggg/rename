#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "../lexer/token.h"

typedef struct { // debug info
    int line;
    int column;
} SourceLocation;

typedef enum {
     TYPE_INT, TYPE_LONG, TYPE_BOOL, TYPE_ARRAY, TYPE_STRUCT,   
} TypeVar;

typedef enum NodeType { // ASTNode could have one of the statement/expression type
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
    
    // Expressions
    NODE_BINARY_EXPRESSION,
    NODE_UNARY_EXPRESSION,
    NODE_LITERAL_EXPRESSION,
    NODE_VARIABLE_EXPRESSION,
    NODE_FUNCTION_CALL_EXPRESSION
} NodeType;

typedef struct ASTNode { // Base version for implementing others
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
    int argument_count;
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

//typedef struct ProgramStatement TODO

typedef struct  {
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
    ASTNode* body; // BlockStatement
} FunctionDeclarationStatement;

typedef struct {
    ASTNode base;
    ASTNode* expression; // ExpressionStatement
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
    
    ASTNode* condition;   // ExpressionStatement
    ASTNode* then_branch; // BlockStatement
    
    ASTNode** elif_conditions;    // ExpressionStatement
    ASTNode** elif_branches;      // BlockStatement
    size_t elif_count;
    
    ASTNode* else_branch; //BlockStatement
} IfStatement;

typedef struct {
    ASTNode base;
    ASTNode* condition;    // ExpressionStatement
    ASTNode* body;         // BlockStatement
} WhileStatement;

// Структура для ForStatement
typedef struct {
    ASTNode base;
    ASTNode* initializer;  // VariableDeclaration or ExpressionStatement or NULL
    ASTNode* condition;    // ExpressionStatement or NULL
    ASTNode* increment;    // ExpressionStatement or NULL
    ASTNode* body;         // BlockStatement
} ForStatement;


static ASTNode* ast_node_allocate(NodeType node_type, SourceLocation loc);

static BinaryExpression* copy_binary_expression(ASTNode* original);
static UnaryExpression* copy_unary_expression(ASTNode* original);
static VariableExpression* copy_variable_expression(ASTNode* original);
static FunctionCallExpression* copy_call_expression(ASTNode* original);
static AssignmentStatement* copy_assignment_statement(ASTNode* original);
static VariableDeclarationStatement* copy_variable_declaration_statement(ASTNode* original);
static ReturnStatement* copy_return_statement(ASTNode* original);
static ExpressionStatement* copy_expression_statement(ASTNode* original);
static BlockStatement* copy_block_statement(ASTNode* original);
static IfStatement* copy_if_statement(ASTNode* original);
static WhileStatement* copy_while_statement(ASTNode* original);
static ForStatement* copy_for_statement(ASTNode* original);
static FunctionDeclarationStatement* copy_function_declaration_statement(ASTNode* original);
static ASTNode* ast_node_copy(ASTNode* node);

void ast_free(ASTNode* node);

ASTNode* ast_new_binary_expression(SourceLocation loc, ASTNode* left, Token op, ASTNode* right);
ASTNode* ast_new_unary_expression(SourceLocation loc, Token op, ASTNode* operand);
ASTNode* ast_new_literal_expression(SourceLocation loc, TypeVar type, int64_t value);
ASTNode* ast_new_variable_expression(SourceLocation loc, const char* name);

ASTNode* ast_new_assignment_statement(SourceLocation loc, ASTNode* left, ASTNode* right);
ASTNode* ast_new_variable_declaration_statement(SourceLocation loc, TypeVar var_type, const char* name, ASTNode* initializer);
ASTNode* ast_new_return_statement(SourceLocation loc, ASTNode* expression);
ASTNode* ast_new_if_statement(SourceLocation loc, ASTNode* condition, ASTNode* then_branch);
ASTNode* ast_new_while_statement(SourceLocation loc, ASTNode* condition, ASTNode* body);
ASTNode* ast_new_function_declaration_statement(SourceLocation loc, const char* name, TypeVar return_type, Parameter* parameters, size_t parameter_count, ASTNode* body);
ASTNode* ast_new_call_expression(SourceLocation loc, ASTNode* callee, ASTNode** args, int arg_count);
ASTNode* ast_new_for_statement(SourceLocation loc, ASTNode* initializer, ASTNode* condition, ASTNode* increment, ASTNode* body);
ASTNode* ast_new_expression_statement(SourceLocation loc, ASTNode* expression);
ASTNode* ast_new_block_statement(SourceLocation loc, ASTNode** statements, size_t count);

Parameter* ast_new_parameter(const char* name, TypeVar type);
void parameter_free(Parameter* param);

bool add_statement_to_block(ASTNode* block_stmt, ASTNode* stmt);

const char* ast_node_type_to_string(int node_type);
const char* type_var_to_string(int node_type);
const char* token_type_to_string(int token_type);
const TypeVar token_type_to_type_var(TokenType type);

void ast_print(ASTNode* node, int indent);
void ast_print_tree(ASTNode* node, int indent);
