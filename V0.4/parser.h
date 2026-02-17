#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    // Literals
    AST_NUMBER,
    AST_FLOAT,
    AST_STRING,
    AST_BOOLEAN,
    AST_NULL,
    AST_ARRAY,          // NEW: [1, 2, 3]
    AST_IDENTIFIER,
    
    // Expressions
    AST_BINARY,
    AST_UNARY,
    AST_ASSIGN,
    AST_CALL,
    AST_INDEX,          // NEW: array[i]
    
    // Statements
    AST_BLOCK,
    AST_IF,
    AST_WHILE,
    AST_FOR,            // NEW: untuk i dalam range(10)
    AST_FUNCTION,
    AST_RETURN,
    AST_EXPR_STMT
} ASTType;

typedef struct ASTNode {
    ASTType type;
    int line;
    union {
        // Literals
        int number;
        double float_num;
        char* string;
        int boolean;
        char* name;
        
        // NEW: Array literal
        struct {
            struct ASTNode** elements;
            int count;
        } array;
        
        // Binary expression
        struct {
            struct ASTNode *left;
            struct ASTNode *right;
            TokenType op;
        } binary;
        
        // Unary expression
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
        
        // NEW: Array/Index access
        struct {
            struct ASTNode *object;
            struct ASTNode *index;
        } index;
        
        // Block statement
        struct {
            struct ASTNode **statements;
            int count;
        } block;
        
        // If statement
        struct {
            struct ASTNode *condition;
            struct ASTNode *then_branch;
            struct ASTNode *else_branch;
        } if_stmt;
        
        // While loop
        struct {
            struct ASTNode *condition;
            struct ASTNode *body;
        } while_stmt;
        
        // NEW: For loop
        struct {
            char *var_name;           // loop variable
            struct ASTNode *iterable; // range atau array
            struct ASTNode *body;
        } for_stmt;
        
        // Function definition
        struct {
            char *name;
            char **params;
            int param_count;
            struct ASTNode *body;
        } function;
        
        // Return statement
        struct {
            struct ASTNode *value;
        } return_stmt;
        
        // Expression Statement
        struct {
            struct ASTNode *expr;
        } expr_stmt;
    };
} ASTNode;

void parser_init(Token **tokens, int count);
ASTNode* parse(void);
ASTNode* make_node(ASTType type);
void print_ast(ASTNode *node, int indent);
void free_ast(ASTNode *node);

#endif
