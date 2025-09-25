#pragma once

struct BinaryExpression;
struct VariableDeclaration;
// ...

struct ASTVisitor {
    // todo
};

void ASTvisitor_destroy(ASTVisitor* visitor);
void visit(BinaryExpression* expr);
void visit(VariableDeclaration* stmt);

// todo
