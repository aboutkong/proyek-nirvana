#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"

char* my_strdup(const char* s) {
    char* d = malloc(strlen(s) + 1);
    if (d) strcpy(d, s);
    return d;
}

// fungsi untuk mengurai token menjadi AST
static Token** tokens;
static int pos = 0;

static Token* current_token(void) {
    return tokens[pos];
}

static Token* advance(void) {
    if (tokens[pos]->type != TOKEN_EOF) {
        pos++;
    }
    return current_token();
}

static int match(TokenType type) {
    if (current_token()->type == type) {
        advance();
        return 1;
    }
    return 0;
}

static void error(const char* message) {
    fprintf(stderr, "Parsing error: %s\n", message);
    exit(1);
}

static ASTNode* parse_statment(void);
static ASTNode* parse_expression(void);
static ASTNode* parse_term(void);
static ASTNode* parse_factor(void);

ASTNode* parse_program(void) {
    if (!tokens) {
        error("tidak ada token untuk di parse");
    }
    return parse_statment();
}

ASTNode* parse_assignment(void) {
    if (current_token()->type == TOKEN_NAMA) {
        char* nama = current_token()->lexeme;
        advance();
        if (match(TOKEN_EQUAL)) {
            ASTNode* value = parse_expression();
            ASTNode* node = malloc(sizeof(ASTNode));
            node->type = AST_ASSIGN;
            node->assign.name = my_strdup(nama);
            node->assign.value = value;
            return node;
        } else {
            error("Expected '=' after identifier");
        }
    }
    return parse_expression();
}

ASTNode* parse_statment(void) {
    if (current_token()->type == TOKEN_NAMA) {
        if (tokens[pos + 1]->type == TOKEN_EQUAL) {
        return parse_assignment();
    } 
    }
    return parse_expression();
}

ASTNode* parse_expression(void) {
    ASTNode* node = parse_term();
    while (current_token()->type == TOKEN_PLUS || current_token()->type == TOKEN_MINUS) {
        TokenType op = current_token()->type;
        advance();
        ASTNode* right = parse_term();
        ASTNode* new_node = malloc(sizeof(ASTNode));
        new_node->type = AST_BINARY;
        new_node->binary.op = op;
        new_node->binary.left = node;
        new_node->binary.right = right;
        node = new_node;
    }
    return node;
}

ASTNode* parse_term(void) {
    ASTNode* node = parse_factor();
    while (current_token()->type == TOKEN_BINTANG || current_token()->type == TOKEN_GARING) {
        TokenType op = current_token()->type;
        advance();
        ASTNode* right = parse_factor();
        ASTNode* new_node = malloc(sizeof(ASTNode));
        new_node->type = AST_BINARY;
        new_node->binary.op = op;
        new_node->binary.left = node;
        new_node->binary.right = right;
        node = new_node;
    }
    return node;
}

ASTNode* parse_factor(void) {
    if (current_token()->type == TOKEN_NOMER) {
        Token* token = current_token();
        advance();

        ASTNode* node = malloc(sizeof(ASTNode));
        node->type = AST_NUMBER;
        node->number = atoi(token->lexeme);
        return node;
    }

    if (current_token()->type == TOKEN_NAMA) {
        Token* token = current_token();
        advance();

        ASTNode* node = malloc(sizeof(ASTNode));
        node->type = AST_IDENTIFIER;
        node->name = my_strdup(token->lexeme); // PENTING
        return node;
    }

    if (match(TOKEN_BUKA_KURUNG)) {
        ASTNode* node = parse_expression();
        if (!match(TOKEN_TUTUP_KURUNG)) {
            error("Expected ')'");
        }
        return node;
    }

    if (current_token()->type == TOKEN_CETAK) {
        advance();
        if (!match(TOKEN_BUKA_KURUNG)) 
            error("Expected '(' after 'cetak'");
        ASTNode* node = parse_expression();
        if (!match(TOKEN_TUTUP_KURUNG)) {
            error("Expected ')' after expression in 'cetak'");
        }
        ASTNode* new_node = malloc(sizeof(ASTNode));
        new_node->type = AST_CALL;
        new_node->call.name = my_strdup("cetak");
        new_node->call.args = malloc(sizeof(ASTNode*) * 1);
        new_node->call.args[0] = node;
        new_node->call.arg_count = 1;
        return new_node;
    }
    

    error("Unexpected token in factor");
    return NULL;
}


void free_ast(ASTNode* node) {
    if (!node) return;
    switch (node->type) {
        case AST_BINARY:
            free_ast(node->binary.left);
            free_ast(node->binary.right);
            break;
        case AST_ASSIGN:
            free(node->assign.name);
            free_ast(node->assign.value);
            break;
        case AST_IDENTIFIER:
            free(node->name);
            break;
        case AST_NUMBER:
            // tidak ada alokasi dinamis untuk number
            break;
        case AST_CALL:
            for (int i = 0; i < node->call.arg_count; i++) {
                free_ast(node->call.args[i]);
            }
            free(node->call.args);
            free(node->call.name);
    }
    free(node);
}

void print_indent(int level) {
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
}

void print_ast(ASTNode* node, int level) {
    if (!node) return;
    print_indent(level);
    switch (node->type) {
        case AST_NUMBER:
            printf("Number: %d\n", node->number);
            break;
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->name);
            break;
        case AST_BINARY:
            char* op_str;
            switch (node->binary.op) {
                case TOKEN_PLUS: op_str = "+"; break;
                case TOKEN_MINUS: op_str = "-"; break;
                case TOKEN_BINTANG: op_str = "*"; break;
                case TOKEN_GARING: op_str = "/"; break;
                default: op_str = "?";
            }
            printf("Binary: op=%s\n", op_str);
            print_indent(level + 1);
            printf("Left:\n");
            print_ast(node->binary.left, level + 2);
            print_indent(level + 1);
            printf("Right:\n");
            print_ast(node->binary.right, level + 2);
            break;
        case AST_ASSIGN:
            printf("Assign:\n");
            print_indent(level + 1);
            printf("Name: %s\n", node->assign.name);
            print_indent(level + 1);
            printf("Value:\n");
            print_ast(node->assign.value, level + 2);
            break;

        case AST_CALL:
            printf("Call:\n");
            print_indent(level + 1);
            printf("Function: %s\n", node->call.name);
            print_indent(level + 1);
            printf("Arguments:\n");
            for (int i = 0; i < node->call.arg_count; i++)
                print_ast(node->call.args[i], level + 2);
            break;
    }
}

void set_tokens(Token** input_tokens) {
    tokens = input_tokens;
    pos = 0;
}

void parser_init(Token **toks) {
    tokens = toks;
    pos = 0;
}
ASTNode* parse(void) {
    return parse_program();
}