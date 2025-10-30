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


ASTNode* ast_node_allocate(NodeType node_type, SourceLocation loc);
void ast_free(ASTNode* node);

Parameter* ast_new_parameter(const char* name, TypeVar type);
void parameter_free(Parameter* param);

bool add_statement_to_block(ASTNode* block_stmt, ASTNode* stmt);

const char* ast_node_type_to_string(int node_type);
const char* type_var_to_string(int node_type);
const char* token_type_to_string(int token_type);
const TypeVar token_type_to_type_var(TokenType type);

void ast_print(ASTNode* node, int indent);
void ast_print_tree(ASTNode* node, int indent);
