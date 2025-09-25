#pragma once

typedef struct Type {
    enum {
        INT, BOOL, LONG, ARRAY, STRUCT, NONETYPE, // ?
    } TypeKing; 

    char* name;  // "int", "bool", etc
    
    // for arrays
    struct Type* element_type;
    size_t array_size;    
    // for arrays

} Type;
