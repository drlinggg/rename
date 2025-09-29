#include "../../src/parser/parser.h"
#include "../../src/lexer/token.h"
#include "../../src/AST/ast.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


Token create_token(TokenType type, const char* value, int line, int column) {
    Token token;
    token.type = type;
    token.value = value;
    token.line = line;
    token.column = column;
    return token;
}

void test_print_ast() {
    ASTNode** literals = malloc(2 * sizeof(LiteralExpression*));
    literals[0] = ast_new_literal_expression((SourceLocation){.line=5, .column=6}, TYPE_INT, 5);
    literals[1] = ast_new_literal_expression((SourceLocation){.line=8, .column=6}, TYPE_INT, 7);

    ASTNode* bin_expr = ast_new_binary_expression(
        (SourceLocation){.line=1, .column=1}, 
        literals[0], 
        (Token){.type=OP_PLUS}, 
        literals[1]
    );

    ASTNode* expr_stmt = ast_new_expression_statement(
        (SourceLocation){.line=1, .column=1}, 
        bin_expr
    );
    
    ast_tree_print(expr_stmt, 0);
    ast_free(expr_stmt);
    free(literals);
}

void test_memory_leak() {
    for (int i = 0; i < 1000; i++) {
        ASTNode** literals = malloc(2 * sizeof(LiteralExpression*));
        literals[0] = ast_new_literal_expression((SourceLocation){.line=5, .column=6}, TYPE_INT, 5);
        literals[1] = ast_new_literal_expression((SourceLocation){.line=8, .column=6}, TYPE_INT, 7);

        ASTNode* bin_expr = ast_new_binary_expression(
            (SourceLocation){.line=1, .column=1}, 
            literals[0], 
            (Token){.type=OP_PLUS}, 
            literals[1]
        );

        ASTNode* expr_stmt = ast_new_expression_statement(
            (SourceLocation){.line=1, .column=1}, 
            bin_expr
        );
        ast_free(expr_stmt);

        free(literals);
    }

}


// gcc tests/parser/parser_init_test.c src/lexer/token.h src/AST/ast.c src/parser/parser.c
int main() {
    test_memory_leak();
    test_print_ast();
}
