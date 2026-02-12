#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

/* ===== AST TYPES ===== */

typedef enum {
    // Literals
    AST_NUMBER,
    AST_FLOAT,          // NEW
    AST_STRING,         // NEW
    AST_BOOLEAN,        // NEW
    AST_NULL,           // NEW
    AST_IDENTIFIER,
    
    // Expressions
    AST_BINARY,
    AST_UNARY,          // NEW: -x, !x
    AST_ASSIGN,
    AST_CALL,
    AST_INDEX,          // NEW: array[index]
    
    // Statements
    AST_BLOCK,          // NEW: { ... }
    AST_IF,             // NEW: jika ... maka ... lain
    AST_WHILE,          // NEW: selama ... { ... }
    AST_FOR,            // NEW: untuk ... dalam ...
    AST_FUNCTION,       // NEW: fungsi ... { ... }
    AST_RETURN,         // NEW: kembalikan ...
    AST_VAR_DECL,       // NEW: var x = ...
    AST_EXPR_STMT       // Expression as statement
} ASTType;

/* ===== AST NODE ===== */

typedef struct ASTNode {
    ASTType type;
    int line;           // For error reporting
    
    union {
        // Literals
        int number;
        double float_num;   // NEW
        char* string;       // NEW
        int boolean;        // NEW
        char* name;         // identifier
        
        // Binary expression
        struct {
            struct ASTNode *left;
            struct ASTNode *right;
            TokenType op;
        } binary;
        
        // Unary expression (NEW)
        struct {
            struct ASTNode *operand;
            TokenType op;
        } unary;
        
        // Assignment
        struct {
            char *name;
            struct ASTNode *value;
        } assign;
        
        // Function call
        struct {
            char *name;
            struct ASTNode **args;
            int arg_count;
        } call;
        
        // Array/Dict index (NEW)
        struct {
            struct ASTNode *object;
            struct ASTNode *index;
        } index;
        
        // Block statement (NEW)
        struct {
            struct ASTNode **statements;
            int count;
        } block;
        
        // If statement (NEW)
        struct {
            struct ASTNode *condition;
            struct ASTNode *then_branch;
            struct ASTNode *else_branch;
        } if_stmt;
        
        // While loop (NEW)
        struct {
            struct ASTNode *condition;
            struct ASTNode *body;
        } while_stmt;
        
        // Function definition (NEW)
        struct {
            char *name;
            char **params;
            int param_count;
            struct ASTNode *body;
        } function;
        
        // Return statement (NEW)
        struct {
            struct ASTNode *value;
        } return_stmt;
        
        // Variable declaration (NEW)
        struct {
            char *name;
            struct ASTNode *initializer;
        } var_decl;
    };
} ASTNode;

/* ===== PARSER API ===== */

void parser_init(Token **tokens, int count);
ASTNode* parse(void);

ASTNode* make_node(ASTType type);
void free_ast(ASTNode *node);
void print_ast(ASTNode *node, int indent);

#endif // PARSER_H
