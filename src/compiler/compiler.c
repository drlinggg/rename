#include "compiler.h"
#include <stdlib.h>

static void emit_bytecode(compilation_result* result, bytecode bc) {
    if (result->count >= result->capacity) {
        result->capacity = result->capacity == 0 ? 10 : result->capacity * 2;
        result->code = realloc(result->code, 
                                     result->capacity * sizeof(bytecode));
    }
    result->code[result->count++] = bc;
}

compiler* compiler_create(ASTNode* ast_tree) {
    compiler* comp = malloc(sizeof(compiler));
    comp->ast_tree = ast_tree;
    comp->result = malloc(sizeof(compilation_result));
    comp->result->code = NULL;
    comp->result->count = 0;
    comp->result->capacity = 0;
    return comp;
}

void compiler_destroy(compiler* compiler) {
    if (!compiler) {
        return;
    }

    if (compiler->result) {
        free(compiler->result->code);
        free(compiler->result);
    }

    if (compiler->ast_tree) {
        ast_free(compiler->ast_tree);
    }
    free(compiler);
}

compilation_result* compiler_compile(compiler* compiler) {
    if (!compiler || !compiler->ast_tree) return NULL;
    //
    return compiler->result;
}
