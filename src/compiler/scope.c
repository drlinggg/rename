#include "scope.h"
#include <stdlib.h>

CompilerScope* scope_create(CompilerScope* parent) {
    CompilerScope* scope = malloc(sizeof(CompilerScope));
    scope->locals = string_table_create();
    scope->parent = parent;
    return scope;
}

void scope_destroy(CompilerScope* scope) {
    if (!scope) return;
    
    if (scope->locals) {
        string_table_destroy(scope->locals);
    }
    free(scope);
}

size_t scope_add_local(CompilerScope* scope, const char* name) {
    if (!scope || !scope->locals || !name) return SIZE_MAX;
    return string_table_add(scope->locals, name);
}

int32_t scope_find_local(CompilerScope* scope, const char* name) {
    if (!scope || !scope->locals || !name) return -1;
    return string_table_find(scope->locals, name);
}

bool scope_contains_local(CompilerScope* scope, const char* name) {
    return scope_find_local(scope, name) >= 0;
}
