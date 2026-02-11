#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

/* ===== AST TYPES ===== */

typedef enum {
    AST_NUMBER,
    AST_IDENTIFIER,
    AST_BINARY,
    AST_ASSIGN,
    AST_CALL
} ASTType;

/* ===== AST NODE ===== */

typedef struct ASTNode {
    ASTType type;

    union {
        /* literal number */
        int number;

        /* identifier */
        char *name;

        /* binary expression */
        struct {
            struct ASTNode *left;
            struct ASTNode *right;
            TokenType op;
        } binary;

        /* assignment */
        struct {
            char *name;
            struct ASTNode *value;
        } assign;
        /* function call */
        struct {
            char *name;
            struct ASTNode **args;
            int arg_count;
        } call;
    };
} ASTNode;

/* ===== PARSER API ===== */

void parser_init(Token **tokens);
ASTNode* parse(void);

void print_ast(ASTNode *node, int indent);
void free_ast(ASTNode *node);

#endif
// PARSER_H