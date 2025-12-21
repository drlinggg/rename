#ifndef COMPILER_H
#define COMPILER_H

#include "../AST/ast.h"
#include "bytecode.h"
#include "string_table.h"
#include "scope.h"
#include "value.h"

typedef struct compilation_result{
    bytecode_array code_array;

    Value* constants;
    size_t constants_count;
    size_t constants_capacity;
} compilation_result;

typedef struct compiler {
    ASTNode* ast_tree;
    compilation_result* result;

    // compiler tables
    string_table* global_names;
    CompilerScope* current_scope;
} compiler;

compiler* compiler_create(ASTNode* ast_tree);
void compiler_destroy(compiler* compiler);
compilation_result* compiler_compile(compiler* compiler);

#endif // COMPILER_H
