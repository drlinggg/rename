#include "../../src/parser/parser.h"
#include "../../src/lexer/token.h"
#include "../../src/AST/ast.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../src/debug.h"


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
    ASTNode* stmt = parser_parse(parser);

    
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
        create_token(KW_TRUE, "true", 1, 7),
        create_token(SEMICOLON, ";", 1, 11),
        create_token(END_OF_FILE, "", 1, 12)
    };
    
    Parser* parser = parser_create(tokens, 7);
    ASTNode* stmt = parser_parse(parser);
    
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
    }; // a + b * c - d // 2;
       // bin_expr( bin expr(a, bin_expr(b,c)), bin_expr(d, 2) )
    
    Parser* parser = parser_create(tokens, 11);
    ASTNode* expr = parser_parse(parser);
    
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
        create_token(LPAREN, "(", 1, 3),
        create_token(IDENTIFIER, "var", 1, 5),
        create_token(COMMA, ",", 1, 7),
        create_token(INT_LITERAL, "5", 1, 5),
        create_token(OP_MULT, "*", 1, 5), 
        create_token(INT_LITERAL, "5", 1, 5),
        create_token(RPAREN, ")", 1, 3),
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

void test_function_declaration_statement() {
    printf("=== TEST Function Declaration ===\n");
    
    Token tokens[] = {
        // Тип возвращаемого значения и имя функции
        create_token(KW_INT, "int", 1, 1),
        create_token(IDENTIFIER, "add", 1, 5),
        
        // Параметры функции
        create_token(LPAREN, "(", 1, 8),
        create_token(IDENTIFIER, "a", 1, 9),
        create_token(COLON, ":", 1, 10),
        create_token(KW_INT, "int", 1, 11),
        create_token(COMMA, ",", 1, 14),
        create_token(IDENTIFIER, "b", 1, 15),
        create_token(COLON, ":", 1, 16),
        create_token(KW_INT, "int", 1, 17),
        create_token(RPAREN, ")", 1, 20),
        
        // Тело функции
        create_token(LBRACE, "{", 1, 21),
        create_token(KW_RETURN, "return", 1, 22),
        create_token(IDENTIFIER, "a", 1, 29),
        create_token(OP_PLUS, "+", 1, 31),
        create_token(IDENTIFIER, "b", 1, 33),
        create_token(SEMICOLON, ";", 1, 34),
        create_token(RBRACE, "}", 1, 35),
        
        create_token(END_OF_FILE, "", 1, 36),
        // int add(a: int, b: int) {
        //    return a + b;
        // }
    };
    
    Parser* parser = parser_create(tokens, 18);
    ASTNode* func_decl = parser_parse(parser);
    
    if (func_decl) {
        printf("Successfully parsed function declaration:\n");
        ast_print_tree(func_decl, 0);
        ast_free(func_decl);
        assert(1 == 1);
    } else {
        printf("Failed to parse function declaration\n");
        assert(0 && "couldnt parse function declaration");
    }
    
    parser_destroy(parser);
    printf("\n\n");
}

void test_if_statement() {
    printf("=== TEST 6: If statement ===\n");
    
    Token tokens[] = {
        create_token(KW_IF, "if", 1, 1),
        create_token(LPAREN, "(", 1, 3),
        create_token(IDENTIFIER, "x", 1, 4),
        create_token(OP_GT, ">", 1, 6),
        create_token(INT_LITERAL, "0", 1, 8),
        create_token(RPAREN, ")", 1, 9),
        create_token(LBRACE, "{", 1, 10),
        create_token(KW_RETURN, "return", 1, 12),
        create_token(KW_TRUE, "true", 1, 19),
        create_token(SEMICOLON, ";", 1, 23),
        create_token(RBRACE, "}", 1, 24),
        create_token(END_OF_FILE, "", 1, 25),
    };
    
    Parser* parser = parser_create(tokens, 12);
    ASTNode* stmt = parser_parse(parser);
    
    if (stmt) {
        printf("Successfully parsed if statement:\n");
        ast_print_tree(stmt, 0);
        ast_free(stmt);
        assert(1 == 1);
    } else {
        printf("Failed to parse if statement\n");
        assert(0 && "couldn't parse if statement");
    }
    
    parser_destroy(parser);
    printf("\n\n");
}

void test_if_else_statement() {
    printf("=== TEST 7: If-else statement ===\n");
    
    Token tokens[] = {
        create_token(KW_IF, "if", 1, 1),
        create_token(LPAREN, "(", 1, 3),
        create_token(IDENTIFIER, "x", 1, 4),
        create_token(OP_GT, ">", 1, 6),
        create_token(INT_LITERAL, "0", 1, 8),
        create_token(RPAREN, ")", 1, 9),
        create_token(LBRACE, "{", 1, 10),
        create_token(KW_RETURN, "return", 1, 12),
        create_token(KW_TRUE, "true", 1, 19),
        create_token(SEMICOLON, ";", 1, 23),
        create_token(RBRACE, "}", 1, 24),
        create_token(KW_ELSE, "else", 1, 26),
        create_token(LBRACE, "{", 1, 30),
        create_token(KW_RETURN, "return", 1, 32),
        create_token(KW_FALSE, "false", 1, 39),
        create_token(SEMICOLON, ";", 1, 44),
        create_token(RBRACE, "}", 1, 45),
        create_token(END_OF_FILE, "", 1, 46),
    };
    
    Parser* parser = parser_create(tokens, 18);
    ASTNode* stmt = parser_parse(parser);
    
    if (stmt) {
        printf("Successfully parsed if-else statement:\n");
        ast_print_tree(stmt, 0);
        ast_free(stmt);
        assert(1 == 1);
    } else {
        printf("Failed to parse if-else statement\n");
        assert(0 && "couldn't parse if-else statement");
    }
    
    parser_destroy(parser);
    printf("\n\n");
}

void test_if_elif_else_statement() {
    printf("=== TEST 8: If-elif-else statement ===\n");
    
    Token tokens[] = {
        create_token(KW_IF, "if", 1, 1),
        create_token(LPAREN, "(", 1, 3),
        create_token(IDENTIFIER, "x", 1, 4),
        create_token(OP_GT, ">", 1, 6),
        create_token(INT_LITERAL, "10", 1, 8),
        create_token(RPAREN, ")", 1, 10),
        create_token(LBRACE, "{", 1, 11),
        create_token(IDENTIFIER, "print", 1, 13),
        create_token(LPAREN, "(", 1, 18),
        create_token(INT_LITERAL, "1", 1, 19),
        create_token(RPAREN, ")", 1, 20),
        create_token(SEMICOLON, ";", 1, 21),
        create_token(RBRACE, "}", 1, 22),
        
        create_token(KW_ELIF, "elif", 1, 24),
        create_token(LPAREN, "(", 1, 28),
        create_token(IDENTIFIER, "x", 1, 29),
        create_token(OP_GT, ">", 1, 31),
        create_token(INT_LITERAL, "0", 1, 33),
        create_token(RPAREN, ")", 1, 34),
        create_token(LBRACE, "{", 1, 35),
        create_token(IDENTIFIER, "print", 1, 37),
        create_token(LPAREN, "(", 1, 42),
        create_token(INT_LITERAL, "2", 1, 43),
        create_token(RPAREN, ")", 1, 44),
        create_token(SEMICOLON, ";", 1, 45),
        create_token(RBRACE, "}", 1, 46),
        
        create_token(KW_ELSE, "else", 1, 48),
        create_token(LBRACE, "{", 1, 52),
        create_token(IDENTIFIER, "print", 1, 54),
        create_token(LPAREN, "(", 1, 59),
        create_token(INT_LITERAL, "3", 1, 60),
        create_token(RPAREN, ")", 1, 61),
        create_token(SEMICOLON, ";", 1, 62),
        create_token(RBRACE, "}", 1, 63),
        
        create_token(END_OF_FILE, "", 1, 64),
    };
    
    Parser* parser = parser_create(tokens, 34);
    ASTNode* stmt = parser_parse(parser);
    
    if (stmt) {
        printf("Successfully parsed if-elif-else statement:\n");
        ast_print_tree(stmt, 0);
        ast_free(stmt);
        assert(1 == 1);
    } else {
        printf("Failed to parse if-elif-else statement\n");
        assert(0 && "couldn't parse if-elif-else statement");
    }
    
    parser_destroy(parser);
    printf("\n\n");
}

void test_while_statement() {
    printf("=== TEST 9: While statement ===\n");
    
    Token tokens[] = {
        create_token(KW_WHILE, "while", 1, 1),
        create_token(LPAREN, "(", 1, 7),
        create_token(IDENTIFIER, "i", 1, 8),
        create_token(OP_LT, "<", 1, 10),
        create_token(INT_LITERAL, "10", 1, 12),
        create_token(RPAREN, ")", 1, 14),
        create_token(LBRACE, "{", 1, 15),
        create_token(IDENTIFIER, "print", 1, 17),
        create_token(LPAREN, "(", 1, 22),
        create_token(IDENTIFIER, "i", 1, 23),
        create_token(RPAREN, ")", 1, 24),
        create_token(SEMICOLON, ";", 1, 25),
        create_token(IDENTIFIER, "i", 1, 27),
        create_token(OP_ASSIGN, "=", 1, 29),
        create_token(IDENTIFIER, "i", 1, 31),
        create_token(OP_PLUS, "+", 1, 33),
        create_token(INT_LITERAL, "1", 1, 35),
        create_token(SEMICOLON, ";", 1, 36),
        create_token(RBRACE, "}", 1, 37),
        create_token(END_OF_FILE, "", 1, 38),
    };
    
    Parser* parser = parser_create(tokens, 20);
    ASTNode* stmt = parser_parse(parser);
    
    if (stmt) {
        printf("Successfully parsed while statement:\n");
        ast_print_tree(stmt, 0);
        ast_free(stmt);
        assert(1 == 1);
    } else {
        printf("Failed to parse while statement\n");
        assert(0 && "couldn't parse while statement");
    }
    
    parser_destroy(parser);
    printf("\n\n");
}

void test_for_statement_simple() {
    printf("=== TEST 10: For statement (simple) ===\n");
    
    // for (int i = 0; i < 10; i = i + 1) { print(i); }
    Token tokens[] = {
        create_token(KW_FOR, "for", 1, 1), 
        create_token(LPAREN, "(", 1, 5),
        create_token(KW_INT, "int", 1, 6),
        create_token(IDENTIFIER, "i", 1, 10),
        create_token(OP_ASSIGN, "=", 1, 12),
        create_token(INT_LITERAL, "0", 1, 14),
        create_token(SEMICOLON, ";", 1, 15),
        create_token(IDENTIFIER, "i", 1, 17),
        create_token(OP_LT, "<", 1, 19),
        create_token(INT_LITERAL, "10", 1, 21),
        create_token(SEMICOLON, ";", 1, 23),
        create_token(IDENTIFIER, "i", 1, 25),
        create_token(OP_ASSIGN, "=", 1, 27),
        create_token(IDENTIFIER, "i", 1, 29),
        create_token(OP_PLUS, "+", 1, 31),
        create_token(INT_LITERAL, "1", 1, 33),
        create_token(RPAREN, ")", 1, 34),
        create_token(LBRACE, "{", 1, 35),
        create_token(IDENTIFIER, "print", 1, 37),
        create_token(LPAREN, "(", 1, 42),
        create_token(IDENTIFIER, "i", 1, 43),
        create_token(RPAREN, ")", 1, 44),
        create_token(SEMICOLON, ";", 1, 45),
        create_token(RBRACE, "}", 1, 46),
        create_token(END_OF_FILE, "", 1, 47),
    };
    
    Parser* parser = parser_create(tokens, 25);
    ASTNode* stmt = parser_parse(parser);
    
    if (stmt) {
        printf("Successfully parsed for statement:\n");
        ast_print_tree(stmt, 0);
        ast_free(stmt);
        assert(1 == 1);
    } else {
        printf("Failed to parse for statement\n");
        assert(0 && "couldn't parse for statement");
    }
    
    parser_destroy(parser);
    printf("\n\n");
}

void test_for_statement_no_initializer() {
    printf("=== TEST 11: For statement (no initializer) ===\n");
    
    // for (; i < 10; i = i + 1) { print(i) }
    Token tokens[] = {
        create_token(KW_FOR, "for", 1, 1),
        create_token(LPAREN, "(", 1, 5),
        create_token(SEMICOLON, ";", 1, 6),
        create_token(IDENTIFIER, "i", 1, 8),
        create_token(OP_LT, "<", 1, 10),
        create_token(INT_LITERAL, "10", 1, 12),
        create_token(SEMICOLON, ";", 1, 14),
        create_token(IDENTIFIER, "i", 1, 16),
        create_token(OP_ASSIGN, "=", 1, 18),
        create_token(IDENTIFIER, "i", 1, 20),
        create_token(OP_PLUS, "+", 1, 22),
        create_token(INT_LITERAL, "1", 1, 24),
        create_token(RPAREN, ")", 1, 20),
        create_token(LBRACE, "{", 1, 21),
        create_token(IDENTIFIER, "print", 1, 23),
        create_token(LPAREN, "(", 1, 28),
        create_token(INT_LITERAL, "i", 1, 29),
        create_token(RPAREN, ")", 1, 30),
        create_token(SEMICOLON, ";", 1, 31),
        create_token(RBRACE, "}", 1, 32),
        create_token(END_OF_FILE, "", 1, 33),
    };
    
    Parser* parser = parser_create(tokens, 21);
    ASTNode* stmt = parser_parse(parser);
    
    if (stmt) {
        printf("Successfully parsed for statement without initializer:\n");
        ast_print_tree(stmt, 0);
        ast_free(stmt);
        assert(1 == 1);
    } else {
        printf("Failed to parse for statement without initializer\n");
        assert(0 && "couldn't parse for statement without initializer");
    }
    
    parser_destroy(parser);
    printf("\n\n");
}

void test_nested_control_flow() {
    printf("=== TEST 12: Nested control flow ===\n");
    
    Token tokens[] = {
        // for (int i = 0; i < 10; i = i + 1) {
        create_token(KW_FOR, "for", 1, 1),
        create_token(LPAREN, "(", 1, 5),
        create_token(KW_INT, "int", 1, 6),
        create_token(IDENTIFIER, "i", 1, 10),
        create_token(OP_ASSIGN, "=", 1, 12),
        create_token(INT_LITERAL, "0", 1, 14),
        create_token(SEMICOLON, ";", 1, 15),
        create_token(IDENTIFIER, "i", 1, 17),
        create_token(OP_LT, "<", 1, 19),
        create_token(INT_LITERAL, "10", 1, 21),
        create_token(SEMICOLON, ";", 1, 23),
        create_token(IDENTIFIER, "i", 1, 25),
        create_token(OP_ASSIGN, "=", 1, 27),
        create_token(IDENTIFIER, "i", 1, 29),
        create_token(OP_PLUS, "+", 1, 31),
        create_token(INT_LITERAL, "1", 1, 33),
        create_token(RPAREN, ")", 1, 34),
        create_token(LBRACE, "{", 1, 35),
        
        // if (i % 2 == 0) {
        create_token(KW_IF, "if", 1, 37),
        create_token(LPAREN, "(", 1, 40),
        create_token(IDENTIFIER, "i", 1, 41),
        create_token(OP_MOD, "%", 1, 43),
        create_token(INT_LITERAL, "2", 1, 45),
        create_token(OP_EQ, "==", 1, 47),
        create_token(INT_LITERAL, "0", 1, 50),
        create_token(RPAREN, ")", 1, 51),
        create_token(LBRACE, "{", 1, 52),
        create_token(IDENTIFIER, "print", 1, 54),
        create_token(LPAREN, "(", 1, 59),
        create_token(IDENTIFIER, "i", 1, 60),
        create_token(RPAREN, ")", 1, 61),
        create_token(SEMICOLON, ";", 1, 62),
        create_token(RBRACE, "}", 1, 63),
        
        create_token(RBRACE, "}", 1, 65),
        create_token(END_OF_FILE, "", 1, 66),
    };
    
    Parser* parser = parser_create(tokens, 37);
    ASTNode* stmt = parser_parse(parser);
    
    if (stmt) {
        printf("Successfully parsed nested control flow:\n");
        ast_print_tree(stmt, 0);
        ast_free(stmt);
        assert(1 == 1);
    } else {
        printf("Failed to parse nested control flow\n");
        assert(0 && "couldn't parse nested control flow");
    }
    
    parser_destroy(parser);
    printf("\n\n");
}

int main() {
    debug_enabled = 1;
    test_simple_expressions();
    test_variable_declaration();
    test_unary_expression();
    test_complex_expression();
    test_fun_expression();
    //test_function_declaration_statement(); fix
    test_if_statement(); // v
    test_if_else_statement(); // v
    test_if_elif_else_statement(); // v+- TODO check fun arg count
    test_while_statement(); // v
    test_for_statement_simple(); // v
    test_for_statement_no_initializer(); // v
    test_nested_control_flow(); // v
    
    printf("All tests passed!\n");
    return 0;
}
