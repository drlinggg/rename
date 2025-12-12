#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/lexer/lexer.h"
#include "../src/parser/parser.h"
#include "../src/compiler/compiler.h"
#include "../src/runtime/vm/vm.h"
#include "../src/runtime/vm/heap.h"
#include "../src/compiler/string_table.h"
#include "../src/runtime/vm/object.h"
#include "../src/debug.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s [--debug|-d] <source_file.lang>\n", argv[0]);
        return 1;
    }

    // --- DEBUG FLAG CHECK ---
    int argi = 1;
    if (strcmp(argv[argi], "--debug") == 0 || strcmp(argv[argi], "-d") == 0) {
        debug_enabled = 1;
        argi++;
        DPRINT("[RUNNER] Debug mode enabled\n");
    }

    if (argi >= argc) {
        printf("Usage: %s [--debug|-d] <source_file.lang>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[argi];

    lexer* l = lexer_create(filename);
    if (!l) {
        fprintf(stderr, "Failed to create lexer for file: %s\n", filename);
        return 1;
    }

    Token* tokens = lexer_parse_file(l, filename);
    if (!tokens) {
        fprintf(stderr, "Failed to parse file: %s\n", filename);
        lexer_destroy(l);
        return 1;
    }

    // Count tokens (including END_OF_FILE sentinel)
    size_t token_count = 0;
    while (tokens[token_count].type != END_OF_FILE) token_count++;
    token_count++; // include EOF token

    DPRINT("[RUNNER] token_count=%zu\n", token_count);
    for (size_t ti = 0; ti < token_count; ti++) {
        DPRINT("[RUNNER] token[%zu] type=%d value='%s'\n", ti, tokens[ti].type, tokens[ti].value ? tokens[ti].value : "NULL");
    }
    Parser* parser = parser_create(tokens, token_count);
    if (!parser) {
        fprintf(stderr, "Failed to create parser\n");
        lexer_destroy(l);
        return 1;
    }

    ASTNode* ast = parser_parse(parser);
    parser_destroy(parser);
    lexer_destroy(l);
    DPRINT("[RUNNER] Parsing completed\n");

    if (!ast) {
        fprintf(stderr, "Parsing failed\n");
        return 1;
    }

    compiler* comp = compiler_create(ast);
    if (!comp) {
        fprintf(stderr, "Failed to create compiler\n");
        return 1;
    }

    compilation_result* result = compiler_compile(comp);
    if (!result) {
        fprintf(stderr, "Compilation failed\n");
        compiler_destroy(comp);
        return 1;
    }
    DPRINT("[RUNNER] Compilation completed: bytecodes=%u constants=%zu\n", result->code_array.count, result->constants_count);

    // Optional: dump bytecode
    DPRINT("[RUNNER] Dumping module bytecode:\n");
    bytecode_array_print(&result->code_array);

    // Free token memory (values) - lexer_parse_file allocated strings
    for (size_t i = 0; i < token_count; i++) {
        if (tokens[i].value) free((void*)tokens[i].value);
    }
    free(tokens);

    // Create heap and VM
    size_t global_count = comp->global_names ? comp->global_names->count : 0;
    Heap* heap = heap_create();
    VM* vm = vm_create(heap, global_count);

    // Wrap top-level code into a CodeObj
    CodeObj module_code;
    module_code.code = result->code_array; // transfer pointer
    module_code.name = strdup("<module>");
    module_code.arg_count = 0;
    module_code.local_count = comp->current_scope && comp->current_scope->locals ? comp->current_scope->locals->count : 0;
    module_code.constants = result->constants;
    module_code.constants_count = result->constants_count;

    // Execute module top-level code (defines functions and globals)
    Object* module_res = vm_execute(vm, &module_code);
    if (module_res) {
        // ignore, likely None
    }

    // Find main global
    size_t main_index = SIZE_MAX;
    if (comp->global_names) {
        int32_t gi = string_table_find(comp->global_names, "main");
        if (gi >= 0) main_index = (size_t)gi;
    }

    if (main_index == SIZE_MAX) {
        fprintf(stderr, "No main function found\n");
        vm_destroy(vm);
        heap_destroy(heap);
        compiler_destroy(comp);
        return 1;
    }

    Object* main_obj = vm_get_global(vm, main_index);
    if (!main_obj) {
        fprintf(stderr, "Global 'main' is not set.\n");
        vm_destroy(vm);
        heap_destroy(heap);
        compiler_destroy(comp);
        return 1;
    }
    DPRINT("[RUNNER] main global type: %d\n", main_obj->type);

    // Build call-main code: LOAD_GLOBAL main_index, PUSH_NULL, CALL_FUNCTION 0, RETURN_VALUE
    bytecode* codes = malloc(4 * sizeof(bytecode));
    codes[0] = bytecode_create_with_number(LOAD_GLOBAL, (uint32_t)(main_index << 1)); // << 1 because VM decodes >> 1
    codes[1] = bytecode_create(PUSH_NULL, 0, 0, 0);
    codes[2] = bytecode_create_with_number(CALL_FUNCTION, 0);
    codes[3] = bytecode_create(RETURN_VALUE, 0, 0, 0);

    CodeObj call_main;
    call_main.code = create_bytecode_array(codes, 4);
    call_main.name = strdup("<call_main>");
    call_main.arg_count = 0;
    call_main.local_count = 0;
    call_main.constants = NULL;
    call_main.constants_count = 0;

    Object* ret = vm_execute(vm, &call_main);
    char* s = object_to_string(ret);
    if (s) {
        printf("Proccess finished with status: %s\n", s);
        free(s);
    } else {
        printf("Proccess finished with status: 0\n");
    }

    // Cleanup
    if (ret) object_decref(ret);
    if (module_res) object_decref(module_res);
    free(call_main.name);
    free(module_code.name);

    compiler_destroy(comp); // frees result / AST etc
    vm_destroy(vm);
    heap_destroy(heap);

    return 0;
}
