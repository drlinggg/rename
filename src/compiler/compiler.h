#pragma once
#include "../AST/ast.h"
#include "bytecode.h"

typedef struct compilation_result{
    bytecode* code;         
    size_t count;           
    size_t capacity;
} compilation_result;

typedef struct compiler {
    ASTNode* ast_tree;
    size_t ptr;
    compilation_result* result;
} compiler;

compiler* compiler_create(ASTNode* ast_tree);
void compiler_destroy(compiler* compiler);
compilation_result* compiler_compile(compiler* compiler);
