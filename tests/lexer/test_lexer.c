#include "../../src/lexer/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../../src/debug.h"


FILE* create_temp_file(const char* content) {
    FILE* temp = tmpfile();
    if (!temp) return NULL;
    
    fputs(content, temp);
    rewind(temp);
    return temp;
}

void test_keywords() {
    printf("Testing keywords...\n");
    
    const char* code = "int bool long array none true false if else elif while for break continue return struct";
    FILE* temp = create_temp_file(code);
    assert(temp != NULL);
    
    lexer* l = lexer_create_from_stream(temp, "test_keywords");
    assert(l != NULL);
    
    TokenType expected[] = {
        KW_INT, KW_BOOL, KW_LONG, KW_ARRAY, KW_NONE, KW_TRUE, KW_FALSE,
        KW_IF, KW_ELSE, KW_ELIF, KW_WHILE, KW_FOR, KW_BREAK, KW_CONTINUE,
        KW_RETURN, KW_STRUCT, END_OF_FILE
    };
    
    const char* expected_values[] = {
        "int", "bool", "long", "array", "None", "true", "false",
        "if", "else", "elif", "while", "for", "break", "continue",
        "return", "struct", "EOF"
    };

    // Используем lexer_parse_file вместо lexer_next_token
    Token* tokens = lexer_parse_file(l, "test_keywords");
    assert(tokens != NULL);
    
    for (int i = 0; i < 17; i++) {
        assert(tokens[i].type == expected[i]);
        assert(strcmp(tokens[i].value, expected_values[i]) == 0);
        printf("  ✓ %s\n", tokens[i].value);
    }
    
    // Освобождаем память
    for (int i = 0; tokens[i].type != END_OF_FILE || tokens[i].value != NULL; i++) {
        free(tokens[i].value);
    }
    free(tokens);
    
    lexer_destroy(l);
    printf("Keywords test passed!\n\n");
}

void test_identifiers() {
    printf("Testing identifiers...\n");
    
    const char* code = "variable _var var123 my_var123";
    FILE* temp = create_temp_file(code);
    assert(temp != NULL);
    
    lexer* l = lexer_create_from_stream(temp, "test_identifiers");
    assert(l != NULL);
    
    const char* expected_values[] = {"variable", "_var", "var123", "my_var123", "EOF"};
    TokenType expected_types[] = {IDENTIFIER, IDENTIFIER, IDENTIFIER, IDENTIFIER, END_OF_FILE};

    Token* tokens = lexer_parse_file(l, "test_identifiers");
    assert(tokens != NULL);
    
    for (int i = 0; i < 5; i++) {
        assert(tokens[i].type == expected_types[i]);
        assert(strcmp(tokens[i].value, expected_values[i]) == 0);
        printf("  ✓ %s\n", tokens[i].value);
    }
    
    for (int i = 0; tokens[i].type != END_OF_FILE || tokens[i].value != NULL; i++) {
        free(tokens[i].value);
    }
    free(tokens);
    
    lexer_destroy(l);
    printf("Identifiers test passed!\n\n");
}

void test_numbers() {
    printf("Testing numbers...\n");
    
    const char* code = "123 456 7890";
    FILE* temp = create_temp_file(code);
    assert(temp != NULL);
    
    lexer* l = lexer_create_from_stream(temp, "test_numbers");
    assert(l != NULL);
    
    const char* expected_values[] = {"123", "456", "7890", "EOF"};
    TokenType expected_types[] = {INT_LITERAL, INT_LITERAL, INT_LITERAL, END_OF_FILE};

    Token* tokens = lexer_parse_file(l, "test_numbers");
    assert(tokens != NULL);
    
    for (int i = 0; i < 4; i++) {
        assert(tokens[i].type == expected_types[i]);
        assert(strcmp(tokens[i].value, expected_values[i]) == 0);
        printf("  ✓ %s\n", tokens[i].value);
    }
    
    for (int i = 0; tokens[i].type != END_OF_FILE || tokens[i].value != NULL; i++) {
        free(tokens[i].value);
    }
    free(tokens);
    
    lexer_destroy(l);
    printf("Numbers test passed!\n\n");
}

void test_operators() {
    printf("Testing operators...\n");
    
    const char* code = "+ - * / % = ( ) { } [ ] ; : , .";
    FILE* temp = create_temp_file(code);
    assert(temp != NULL);
    
    lexer* l = lexer_create_from_stream(temp, "test_operators");
    assert(l != NULL);
    
    TokenType expected_types[] = {
        OP_PLUS, OP_MINUS, OP_MULT, OP_DIV, OP_MOD, OP_ASSIGN,
        LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
        SEMICOLON, COLON, COMMA, KW_DOT, END_OF_FILE
    };
    
    const char* expected_values[] = {
        "+", "-", "*", "/", "%", "=",
        "(", ")", "{", "}", "[", "]",
        ";", ":", ",", ".", "EOF"
    };

    Token* tokens = lexer_parse_file(l, "test_operators");
    assert(tokens != NULL);
    
    for (int i = 0; i < 17; i++) {
        assert(tokens[i].type == expected_types[i]);
        assert(strcmp(tokens[i].value, expected_values[i]) == 0);
        printf("  ✓ %s\n", tokens[i].value);
    }
    
    for (int i = 0; tokens[i].type != END_OF_FILE || tokens[i].value != NULL; i++) {
        free(tokens[i].value);
    }
    free(tokens);
    
    lexer_destroy(l);
    printf("Operators test passed!\n\n");
}

void test_expression() {
    printf("Testing expression...\n");
    
    const char* code = "x = 5 + 3 * (2 - 1)";
    FILE* temp = create_temp_file(code);
    assert(temp != NULL);
    
    lexer* l = lexer_create_from_stream(temp, "test_expression");
    assert(l != NULL);
    
    TokenType expected_types[] = {
        IDENTIFIER, OP_ASSIGN, INT_LITERAL, OP_PLUS, INT_LITERAL,
        OP_MULT, LPAREN, INT_LITERAL, OP_MINUS, INT_LITERAL, RPAREN,
        END_OF_FILE
    };
    
    const char* expected_values[] = {
        "x", "=", "5", "+", "3", "*", "(", "2", "-", "1", ")", "EOF"
    };

    Token* tokens = lexer_parse_file(l, "test_expression");
    assert(tokens != NULL);
    
    for (int i = 0; i < 12; i++) {
        assert(tokens[i].type == expected_types[i]);
        assert(strcmp(tokens[i].value, expected_values[i]) == 0);
        printf("  ✓ %s\n", tokens[i].value);
    }
    
    for (int i = 0; tokens[i].type != END_OF_FILE || tokens[i].value != NULL; i++) {
        free(tokens[i].value);
    }
    free(tokens);
    
    lexer_destroy(l);
    printf("Expression test passed!\n\n");
}

void test_function() {
    printf("Testing function...\n");
    
    const char* code = "int calculate(a: int, b: int) { return a + b; }";
    FILE* temp = create_temp_file(code);
    assert(temp != NULL);
    
    lexer* l = lexer_create_from_stream(temp, "test_function");
    assert(l != NULL);
    
    TokenType expected_types[] = {
        KW_INT, IDENTIFIER, LPAREN, IDENTIFIER, COLON, KW_INT, COMMA,
        IDENTIFIER, COLON, KW_INT, RPAREN, LBRACE, KW_RETURN, IDENTIFIER, 
        OP_PLUS, IDENTIFIER, SEMICOLON, RBRACE, END_OF_FILE
    };
    
    const char* expected_values[] = {
        "int", "calculate", "(", "a", ":", "int", ",", "b", ":", "int", 
        ")", "{", "return", "a", "+", "b", ";", "}", "EOF"
    };

    Token* tokens = lexer_parse_file(l, "test_function");
    assert(tokens != NULL);
    
    for (int i = 0; i < 19; i++) {
        assert(tokens[i].type == expected_types[i]);
        assert(strcmp(tokens[i].value, expected_values[i]) == 0);
        printf("  ✓ %s\n", tokens[i].value);
    }
    
    for (int i = 0; tokens[i].type != END_OF_FILE || tokens[i].value != NULL; i++) {
        free(tokens[i].value);
    }
    free(tokens);
    
    lexer_destroy(l);
    printf("Function test passed!\n\n");
}

// gcc tests/lexer/test_lexer.c src/lexer/lexer.c src/lexer/token.c
int main() {
    debug_enabled = 1;
    printf("Starting lexer tests...\n\n");
    
    //test_keywords(); kw_none
    test_identifiers();
    test_numbers();
    test_operators();
    test_expression();
    test_function();
    
    printf("All tests passed! ✅\n");
    return 0;
}
