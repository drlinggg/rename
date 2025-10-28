#include "../../src/lexer/lexer.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

// Функция для создания временного файла из строки
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
        "int", "bool", "long", "array", "none", "true", "false",
        "if", "else", "elif", "while", "for", "break", "continue",
        "return", "struct", "EOF"
    };
    
    for (int i = 0; i < 17; i++) {
        Token* token = lexer_next_token(l);
        assert(token != NULL);
        assert(token->type == expected[i]);
        assert(strcmp(token->value, expected_values[i]) == 0);
        printf("  ✓ %s\n", token->value);
        token_free(token);
    }
    
    lexer_destroy(l);
    printf("Keywords test passed!\n\n");
}

// Тест для идентификаторов
void test_identifiers() {
    printf("Testing identifiers...\n");
    
    const char* code = "variable _var var123 my_var123";
    FILE* temp = create_temp_file(code);
    assert(temp != NULL);
    
    lexer* l = lexer_create_from_stream(temp, "test_identifiers");
    assert(l != NULL);
    
    const char* expected[] = {"variable", "_var", "var123", "my_var123", "EOF"};
    
    for (int i = 0; i < 5; i++) {
        Token* token = lexer_next_token(l);
        assert(token != NULL);
        if (i < 4) {
            assert(token->type == IDENTIFIER);
        } else {
            assert(token->type == END_OF_FILE);
        }
        assert(strcmp(token->value, expected[i]) == 0);
        printf("  ✓ %s\n", token->value);
        token_free(token);
    }
    
    lexer_destroy(l);
    printf("Identifiers test passed!\n\n");
}

// Тест для числовых литералов
void test_numbers() {
    printf("Testing numbers...\n");
    
    const char* code = "123 456 7890";
    FILE* temp = create_temp_file(code);
    assert(temp != NULL);
    
    lexer* l = lexer_create_from_stream(temp, "test_numbers");
    assert(l != NULL);
    
    const char* expected[] = {"123", "456", "7890", "EOF"};
    
    for (int i = 0; i < 4; i++) {
        Token* token = lexer_next_token(l);
        assert(token != NULL);
        if (i < 3) {
            assert(token->type == INT_LITERAL);
        } else {
            assert(token->type == END_OF_FILE);
        }
        assert(strcmp(token->value, expected[i]) == 0);
        printf("  ✓ %s\n", token->value);
        token_free(token);
    }
    
    lexer_destroy(l);
    printf("Numbers test passed!\n\n");
}

// Тест для операторов
void test_operators() {
    printf("Testing operators...\n");
    
    const char* code = "+ - * / % = ( ) { } [ ] ; : , .";
    FILE* temp = create_temp_file(code);
    assert(temp != NULL);
    
    lexer* l = lexer_create_from_stream(temp, "test_operators");
    assert(l != NULL);
    
    TokenType expected[] = {
        OP_PLUS, OP_MINUS, OP_MULT, OP_DIV, OP_MOD, OP_ASSIGN,
        LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
        SEMICOLON, COLON, COMMA, KW_DOT, END_OF_FILE
    };
    
    const char* expected_values[] = {
        "+", "-", "*", "/", "%", "=",
        "(", ")", "{", "}", "[", "]",
        ";", ":", ",", ".", "EOF"
    };
    
    for (int i = 0; i < 17; i++) {
        Token* token = lexer_next_token(l);
        assert(token != NULL);
        assert(token->type == expected[i]);
        assert(strcmp(token->value, expected_values[i]) == 0);
        printf("  ✓ %s\n", token->value);
        token_free(token);
    }
    
    lexer_destroy(l);
    printf("Operators test passed!\n\n");
}

// Тест для простого выражения
void test_expression() {
    printf("Testing expression...\n");
    
    const char* code = "x = 5 + 3 * (2 - 1)";
    FILE* temp = create_temp_file(code);
    assert(temp != NULL);
    
    lexer* l = lexer_create_from_stream(temp, "test_expression");
    assert(l != NULL);
    
    TokenType expected[] = {
        IDENTIFIER, OP_ASSIGN, INT_LITERAL, OP_PLUS, INT_LITERAL,
        OP_MULT, LPAREN, INT_LITERAL, OP_MINUS, INT_LITERAL, RPAREN,
        END_OF_FILE
    };
    
    const char* expected_values[] = {
        "x", "=", "5", "+", "3", "*", "(", "2", "-", "1", ")", "EOF"
    };
    
    for (int i = 0; i < 12; i++) {
        Token* token = lexer_next_token(l);
        assert(token != NULL);
        assert(token->type == expected[i]);
        assert(strcmp(token->value, expected_values[i]) == 0);
        printf("  ✓ %s\n", token->value);
        token_free(token);
    }
    
    lexer_destroy(l);
    printf("Expression test passed!\n\n");
}

// Тест для функции
void test_function() {
    printf("Testing function...\n");
    
    const char* code = "int calculate(a: int, b: int) { return a + b; }";
    FILE* temp = create_temp_file(code);
    assert(temp != NULL);
    
    lexer* l = lexer_create_from_stream(temp, "test_function");
    assert(l != NULL);
    
    TokenType expected[] = {
        KW_INT, IDENTIFIER, LPAREN, IDENTIFIER, COLON, KW_INT, COMMA,
        IDENTIFIER, COLON, KW_INT, RPAREN, LBRACE, KW_RETURN, IDENTIFIER, 
        OP_PLUS, IDENTIFIER, SEMICOLON, RBRACE, END_OF_FILE
    };
    
    const char* expected_values[] = {
        "int", "calculate", "(", "a", ":", "int", ",", "b", ":", "int", 
        ")", "{", "return", "a", "+", "b", ";", "}", "EOF"
    };
    
    for (int i = 0; i < 19; i++) {
        Token* token = lexer_next_token(l);
        assert(token != NULL);
        assert(token->type == expected[i]);
        assert(strcmp(token->value, expected_values[i]) == 0);
        printf("  ✓ %s\n", token->value);
        token_free(token);
    }
    
    lexer_destroy(l);
    printf("Function test passed!\n\n");
}


//gcc tests/lexer/test_lexer.c src/lexer/lexer.c
int main() {
    printf("Starting lexer tests...\n\n");
    
    test_keywords();
    test_identifiers();
    test_numbers();
    test_operators();
    test_expression();
    test_function();
    
    printf("All tests passed! \n");
    return 0;
}
