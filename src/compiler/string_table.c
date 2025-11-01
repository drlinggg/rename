#include "string_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

string_table* string_table_create() {
    string_table* table = malloc(sizeof(string_table));
    table->names = NULL;
    table->count = 0;
    table->capacity = 0;
    return table;
}

void string_table_destroy(string_table* table) {
    if (!table) return;
    
    for (size_t i = 0; i < table->count; i++) {
        free(table->names[i]);
    }
    free(table->names);
    free(table);
}

size_t string_table_add(string_table* table, const char* name) {
    if (!table || !name) return SIZE_MAX;
    
    int32_t existing = string_table_find(table, name);
    if (existing >= 0) {
        return (size_t)existing;
    }
    
    if (table->count >= table->capacity) {
        size_t new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
        char** new_names = realloc(table->names, new_capacity * sizeof(char*));
        if (!new_names) return SIZE_MAX;
        
        table->names = new_names;
        table->capacity = new_capacity;
    }
    
    char* name_copy = strdup(name);
    if (!name_copy) return SIZE_MAX;
    
    table->names[table->count] = name_copy;
    return table->count++;
}

const char* string_table_get(const string_table* table, size_t index) {
    if (!table || index >= table->count) return NULL;
    return table->names[index];
}

int32_t string_table_find(const string_table* table, const char* name) {
    if (!table || !name) return -1;
    
    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->names[i], name) == 0) {
            return (int32_t)i;
        }
    }
    return -1;
}

bool string_table_contains(const string_table* table, const char* name) {
    return string_table_find(table, name) >= 0;
}
