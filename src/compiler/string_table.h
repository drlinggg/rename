#ifndef STRING_TABLE_H
#define STRING_TABLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct string_table {
    char** names;
    size_t count;
    size_t capacity;
} string_table;

string_table* string_table_create();
void string_table_destroy(string_table* table);

size_t string_table_add(string_table* table, const char* name);
const char* string_table_get(const string_table* table, size_t index);
int32_t string_table_find(const string_table* table, const char* name);
bool string_table_contains(const string_table* table, const char* name);

#endif // STRING_TABLE_H
