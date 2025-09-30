#include "../../src/parser/parser.h"
#include "../../src/lexer/token.h"
#include "../../src/AST/ast.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>


Token create_token(TokenType type, const char* value, int line, int column) {
    Token token;
    token.type = type;
    token.value = value;
    token.line = line;
    token.column = column;
    return token;
}

void test_simple_expressions() {
    printf("=== TEST 1: Простые выражения ===\n");
    
    Token tokens[] = {
        create_token(IDENTIFIER, "variable", 1, 1),
        create_token(OP_PLUS, "+", 1, 3),
        create_token(INT_LITERAL, "3", 1, 5),
        create_token(OP_MULT, "*", 1, 7),
        create_token(INT_LITERAL, "2", 1, 9),
        create_token(SEMICOLON, ";", 1, 10),
        create_token(END_OF_FILE, "", 1, 11)
    };


    Parser* parser = parser_create(tokens, 7);

    ASTNode* block_statements = parser_parse(parser);
    
    if (block_statements) {
        ast_print_tree(block_statements, 0);
        ast_free(block_statements);
        assert(1 == 1);
    } else {
        assert(0 == "couldnt parse");
    }
    
    parser_destroy(parser);
    printf("\n\n");
}

void test_variable_declaration() {
    printf("=== TEST 2: ===\n");
    
    Token tokens[] = {
        create_token(KW_INT, "int", 1, 11),
        create_token(IDENTIFIER, "var", 1, 13),
        create_token(OP_ASSIGN, "=", 1, 14),
        create_token(INT_LITERAL, "5", 1, 13),
        create_token(SEMICOLON, ";", 23123, 2323),
        create_token(END_OF_FILE, "", 1, 15)
    };
    
    Parser* parser = parser_create(tokens, 6);
    ASTNode* stmt = parser_parse_statement(parser);

    
    if (stmt) {
        ast_print_tree(stmt, 0);
        ast_free(stmt);
        assert(1 == 1);
    } else {
        assert(0 == "couldn parse");
    }
    
    parser_destroy(parser);
    printf("\n\n");
}

void test_unary_expression() {
    printf("=== TEST 3: Унарные выражения ===\n");
    
    Token tokens[] = {
        create_token(OP_MINUS, "-", 1, 1),
        create_token(IDENTIFIER, "x", 1, 2),
        create_token(OP_PLUS, "+", 1, 4),
        create_token(OP_NOT, "!", 1, 6),
        create_token(BOOL_LITERAL, "true", 1, 7),
        create_token(SEMICOLON, ";", 1, 11),
        create_token(END_OF_FILE, "", 1, 12)
    };
    
    Parser* parser = parser_create(tokens, 7);
    ASTNode* stmt = parser_parse_statement(parser);
    
    if (stmt) {
        ast_print_tree(stmt, 0);
        ast_free(stmt);
        assert(1 == 1);
    } else {
        assert(0 == "couldnt parse");
    }
    
    parser_destroy(parser);
    printf("\n\n");
}

void test_complex_expression() {
    printf("=== TEST 4: Сложное выражение с приоритетами ===\n");
    
    Token tokens[] = {
        create_token(IDENTIFIER, "a", 1, 1),
        create_token(OP_PLUS, "+", 1, 3),
        create_token(IDENTIFIER, "b", 1, 5),
        create_token(OP_MULT, "*", 1, 7),
        create_token(IDENTIFIER, "c", 1, 9),
        create_token(OP_MINUS, "-", 1, 11),
        create_token(IDENTIFIER, "d", 1, 13),
        create_token(OP_DIV, "/", 1, 15),
        create_token(INT_LITERAL, "2", 1, 17),
        create_token(SEMICOLON, ";", 1, 18),
        create_token(END_OF_FILE, "", 1, 19)
    };
    
    Parser* parser = parser_create(tokens, 11);
    ASTNode* expr = parser_parse_expression(parser);
    
    if (expr) {
        ast_print_tree(expr, 0);
        ast_free(expr);
        assert(1 == 1);
    } else {
        assert(0 == "couldnt parse");
    }
    
    parser_destroy(parser);
    printf("\n\n");
}

void test_fun_expression() {
    printf("=== TEST 5: ===\n");
    
    Token tokens[] = {
        create_token(IDENTIFIER, "fun", 1, 1),
        create_token(LBRACE, "(", 1, 3),
        create_token(IDENTIFIER, "var", 1, 5),
        create_token(COMMA, ",", 1, 7),
        create_token(INT_LITERAL, "5", 1, 5),
        create_token(OP_MULT, "*", 1, 5), 
        create_token(INT_LITERAL, "5", 1, 5),
        create_token(RBRACE, ")", 1, 3),
        create_token(SEMICOLON, ";", 1, 8),
        create_token(INT_LITERAL, "5", 1, 5),
        create_token(OP_MULT, "*", 1, 5), 
        create_token(INT_LITERAL, "5", 1, 5),
        create_token(SEMICOLON, ";", 1, 8),
        create_token(END_OF_FILE, "", 1, 9),
    };
    
    Parser* parser = parser_create(tokens, 14);
    ASTNode* expr = parser_parse(parser);
    
    if (expr) {
        ast_print_tree(expr, 0);
        ast_free(expr);
        assert(1 == 1);
    }
    else {
        assert(0 == "couldnt parse");
    }
    
    parser_destroy(parser);
    printf("\n\n");
}

// gcc tests/parser/test_parser.c src/lexer/token.h src/AST/ast.c src/parser/parser.c
int main() {
    
    test_simple_expressions();
    test_variable_declaration();
    test_unary_expression();
    test_complex_expression(); // to be fixed prioritet
    test_fun_expression();
    
    return 0;
}
