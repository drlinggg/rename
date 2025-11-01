#ifndef SCOPE_H
#define SCOPE_H

#include "string_table.h"

typedef struct CompilerScope {
    string_table* locals;
    struct CompilerScope* parent;
} CompilerScope;

CompilerScope* scope_create(CompilerScope* parent);
void scope_destroy(CompilerScope* scope);

size_t scope_add_local(CompilerScope* scope, const char* name);
int32_t scope_find_local(CompilerScope* scope, const char* name);
bool scope_contains_local(CompilerScope* scope, const char* name);

#endif // SCOPE_H
