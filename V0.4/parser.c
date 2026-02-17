#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"

static Token** tokens;
static int token_count;
static int pos = 0;

static void error(const char* msg) {
    fprintf(stderr, "Parse Error [%d:%d]: %s\n", tokens[pos]->line, 
            tokens[pos]->column, msg);
    fprintf(stderr, "  Near: '%s'\n", tokens[pos]->lexeme);
    exit(1);
}

char* my_strdup(const char* s) {
    char* d = malloc(strlen(s) + 1);
    if (d) strcpy(d, s);
    return d;
}

ASTNode* make_node(ASTType type) {
    ASTNode* n = calloc(1, sizeof(ASTNode));
    n->type = type;
    n->line = tokens[pos]->line;
    return n;
}

static Token* cur() { return tokens[pos]; }
static Token* adv() { Token* t = cur(); if (t->type != TOKEN_EOF) pos++; return t; }
static int chk(TokenType t) { return cur()->type == t; }
static int mat(TokenType t) { if (chk(t)) { adv(); return 1; } return 0; }

static Token* consume(TokenType type) {
    if (chk(type)) return adv();
    fprintf(stderr, "Expected token type %d\n", type);
    exit(1);
    return NULL;
}

static void skip_ws() {
    while (chk(TOKEN_NEWLINE) || chk(TOKEN_INDENT) || chk(TOKEN_DEDENT)) adv();
}

// Forward declarations
static ASTNode* parse_stmt();
static ASTNode* parse_expr();
static ASTNode* parse_block();

// === EXPRESSION PARSING ===

// NEW: Parse array literal [1, 2, 3]
static ASTNode* parse_array() {
    mat(TOKEN_BUKA_KOTAK);  // consume [
    
    ASTNode* node = make_node(AST_ARRAY);
    int cap = 4;
    node->array.elements = malloc(sizeof(ASTNode*) * cap);
    node->array.count = 0;
    
    // Empty array []
    if (chk(TOKEN_TUTUP_KOTAK)) {
        adv();
        return node;
    }
    
    // Parse elements
    do {
        if (node->array.count >= cap) {
            cap *= 2;
            node->array.elements = realloc(node->array.elements, sizeof(ASTNode*) * cap);
        }
        node->array.elements[node->array.count++] = parse_expr();
    } while (mat(TOKEN_KOMA));
    
    if (!mat(TOKEN_TUTUP_KOTAK)) {
        error("Expected ']' to close array");
    }
    
    return node;
}

static ASTNode* parse_primary() {
    Token* t = cur();
    
    // NEW: Array literal
    if (chk(TOKEN_BUKA_KOTAK)) {
        return parse_array();
    }
    
    if (mat(TOKEN_NOMER)) {
        ASTNode* n = make_node(AST_NUMBER);
        n->number = atoi(t->lexeme);
        return n;
    }
    
    if (mat(TOKEN_FLOAT)) {
        ASTNode* n = make_node(AST_FLOAT);
        n->float_num = atof(t->lexeme);
        return n;
    }
    
    if (mat(TOKEN_STRING)) {
        ASTNode* n = make_node(AST_STRING);
        n->string = my_strdup(t->lexeme);
        return n;
    }
    
    // NEW: Boolean literals
    if (mat(TOKEN_BENAR)) {
        ASTNode* n = make_node(AST_BOOLEAN);
        n->boolean = 1;
        return n;
    }
    
    if (mat(TOKEN_SALAH)) {
        ASTNode* n = make_node(AST_BOOLEAN);
        n->boolean = 0;
        return n;
    }
    
    if (mat(TOKEN_NULL)) return make_node(AST_NULL);
    
    // Identifier atau call
    if (chk(TOKEN_NAMA) || chk(TOKEN_CETAK) || chk(TOKEN_RANGE)) {
        adv();
        char* name = my_strdup(t->lexeme);
        
        // Function call
        if (mat(TOKEN_BUKA_KURUNG)) {
            ASTNode* n = make_node(AST_CALL);
            n->call.name = name;
            n->call.args = NULL;
            n->call.arg_count = 0;
            
            if (!chk(TOKEN_TUTUP_KURUNG)) {
                int cap = 4;
                n->call.args = malloc(sizeof(ASTNode*) * cap);
                do {
                    if (n->call.arg_count >= cap) {
                        cap *= 2;
                        n->call.args = realloc(n->call.args, sizeof(ASTNode*) * cap);
                    }
                    n->call.args[n->call.arg_count++] = parse_expr();
                } while (mat(TOKEN_KOMA));
            }
            if (!mat(TOKEN_TUTUP_KURUNG)) error("Expected ')'");
            return n;
        }
        
        // NEW: Index access arr[i]
        if (mat(TOKEN_BUKA_KOTAK)) {
            ASTNode* n = make_node(AST_INDEX);
            n->index.object = make_node(AST_IDENTIFIER);
            n->index.object->name = name;
            n->index.index = parse_expr();
            if (!mat(TOKEN_TUTUP_KOTAK)) error("Expected ']'");
            return n;
        }
        
        ASTNode* n = make_node(AST_IDENTIFIER);
        n->name = name;
        return n;
    }
    
    if (mat(TOKEN_BUKA_KURUNG)) {
        ASTNode* e = parse_expr();
        if (!mat(TOKEN_TUTUP_KURUNG)) error("Expected ')'");
        return e;
    }
    
    error("Unexpected token in expression");
    return NULL;
}

static ASTNode* parse_unary() {
    Token* t = cur();
    if (mat(TOKEN_MINUS) || mat(TOKEN_NOT)) {
        ASTNode* n = make_node(AST_UNARY);
        n->unary.op = t->type;
        n->unary.operand = parse_unary();
        return n;
    }
    return parse_primary();
}

static ASTNode* parse_mul() {
    ASTNode* n = parse_unary();
    while (chk(TOKEN_BINTANG) || chk(TOKEN_GARING) || chk(TOKEN_PERSEN)) {
        Token* op = cur(); adv();
        ASTNode* r = parse_unary();
        ASTNode* nn = make_node(AST_BINARY);
        nn->binary.op = op->type;
        nn->binary.left = n;
        nn->binary.right = r;
        n = nn;
    }
    return n;
}

static ASTNode* parse_add() {
    ASTNode* n = parse_mul();
    while (chk(TOKEN_PLUS) || chk(TOKEN_MINUS)) {
        Token* op = cur(); adv();
        ASTNode* r = parse_mul();
        ASTNode* nn = make_node(AST_BINARY);
        nn->binary.op = op->type;
        nn->binary.left = n;
        nn->binary.right = r;
        n = nn;
    }
    return n;
}

static ASTNode* parse_cmp() {
    ASTNode* n = parse_add();
    while (chk(TOKEN_LT) || chk(TOKEN_GT) || chk(TOKEN_LTE) || 
           chk(TOKEN_GTE) || chk(TOKEN_EQ) || chk(TOKEN_NEQ)) {
        Token* op = cur(); adv();
        ASTNode* r = parse_add();
        ASTNode* nn = make_node(AST_BINARY);
        nn->binary.op = op->type;
        nn->binary.left = n;
        nn->binary.right = r;
        n = nn;
    }
    return n;
}

static ASTNode* parse_and() {
    ASTNode* n = parse_cmp();
    while (mat(TOKEN_AND)) {
        ASTNode* r = parse_cmp();
        ASTNode* nn = make_node(AST_BINARY);
        nn->binary.op = TOKEN_AND;
        nn->binary.left = n;
        nn->binary.right = r;
        n = nn;
    }
    return n;
}

static ASTNode* parse_or() {
    ASTNode* n = parse_and();
    while (mat(TOKEN_OR)) {
        ASTNode* r = parse_and();
        ASTNode* nn = make_node(AST_BINARY);
        nn->binary.op = TOKEN_OR;
        nn->binary.left = n;
        nn->binary.right = r;
        n = nn;
    }
    return n;
}

static ASTNode* parse_expr() { return parse_or(); }

// === STATEMENT PARSING ===

static ASTNode* parse_brace_block() {
    mat(TOKEN_BUKA_KURAWAL);
    skip_ws();
    
    ASTNode* n = make_node(AST_BLOCK);
    int cap = 8;
    n->block.statements = malloc(sizeof(ASTNode*) * cap);
    n->block.count = 0;
    
    while (!chk(TOKEN_TUTUP_KURAWAL) && !chk(TOKEN_EOF)) {
        skip_ws();
        if (chk(TOKEN_TUTUP_KURAWAL) || chk(TOKEN_EOF)) break;
        if (n->block.count >= cap) {
            cap *= 2;
            n->block.statements = realloc(n->block.statements, sizeof(ASTNode*) * cap);
        }
        ASTNode* s = parse_stmt();
        if (s) n->block.statements[n->block.count++] = s;
        skip_ws();
    }
    mat(TOKEN_TUTUP_KURAWAL);
    return n;
}

static ASTNode* parse_indent_block() {
    if (!mat(TOKEN_TITIK_DUA)) error("Expected ':' or '{'");
    skip_ws();
    
    // Single statement
    if (!chk(TOKEN_INDENT)) {
        ASTNode* n = make_node(AST_BLOCK);
        n->block.statements = malloc(sizeof(ASTNode*));
        n->block.count = 1;
        n->block.statements[0] = parse_stmt();
        return n;
    }
    
    mat(TOKEN_INDENT);
    ASTNode* n = make_node(AST_BLOCK);
    int cap = 8;
    n->block.statements = malloc(sizeof(ASTNode*) * cap);
    n->block.count = 0;
    
    while (!chk(TOKEN_DEDENT) && !chk(TOKEN_EOF)) {
        skip_ws();
        if (chk(TOKEN_DEDENT) || chk(TOKEN_EOF)) break;
        if (n->block.count >= cap) {
            cap *= 2;
            n->block.statements = realloc(n->block.statements, sizeof(ASTNode*) * cap);
        }
        ASTNode* s = parse_stmt();
        if (s) n->block.statements[n->block.count++] = s;
        skip_ws();
    }
    mat(TOKEN_DEDENT);
    return n;
}

static ASTNode* parse_block() {
    skip_ws();
    if (chk(TOKEN_BUKA_KURAWAL)) return parse_brace_block();
    if (chk(TOKEN_TITIK_DUA)) return parse_indent_block();
    
    // Single statement
    ASTNode* n = make_node(AST_BLOCK);
    n->block.statements = malloc(sizeof(ASTNode*));
    n->block.count = 1;
    n->block.statements[0] = parse_stmt();
    return n;
}

// NEW: Parse for loop
static ASTNode* parse_for() {
    mat(TOKEN_UNTUK);
    
    Token* var = cur();
    if (!chk(TOKEN_NAMA)) error("Expected variable name after 'untuk'");
    adv();
    
    if (!mat(TOKEN_DALAM)) error("Expected 'dalam' after variable");
    
    ASTNode* n = make_node(AST_FOR);
    n->for_stmt.var_name = my_strdup(var->lexeme);
    n->for_stmt.iterable = parse_expr();
    n->for_stmt.body = parse_block();
    
    return n;
}

static ASTNode* parse_function_definition() {
    mat(TOKEN_FUNGSI); // consume 'fungsi'

    ASTNode* n = make_node(AST_FUNCTION);

    // Function name
    Token* name_token = consume(TOKEN_NAMA);
    n->function.name = my_strdup(name_token->lexeme);

    // Parameters
    if (!mat(TOKEN_BUKA_KURUNG)) error("Expected '(' for function parameters");
    
    n->function.params = NULL;
    n->function.param_count = 0;
    int cap = 4;

    if (!chk(TOKEN_TUTUP_KURUNG)) {
        n->function.params = malloc(sizeof(char*) * cap);
        do {
            if (n->function.param_count >= cap) {
                cap *= 2;
                n->function.params = realloc(n->function.params, sizeof(char*) * cap);
            }
            Token* param_token = consume(TOKEN_NAMA);
            n->function.params[n->function.param_count++] = my_strdup(param_token->lexeme);
        } while (mat(TOKEN_KOMA));
    }
    if (!mat(TOKEN_TUTUP_KURUNG)) error("Expected ')' after function parameters");
    
    // Function body
    n->function.body = parse_block();
    
    return n;
}

static ASTNode* parse_if() {
    mat(TOKEN_JIKA);
    if (!mat(TOKEN_BUKA_KURUNG)) error("Expected '('");
    ASTNode* n = make_node(AST_IF);
    n->if_stmt.condition = parse_expr();
    if (!mat(TOKEN_TUTUP_KURUNG)) error("Expected ')'");
    if (chk(TOKEN_MAKA)) adv();
    skip_ws();
    n->if_stmt.then_branch = parse_block();
    n->if_stmt.else_branch = NULL;
    skip_ws();
    if (mat(TOKEN_LAIN)) {
        skip_ws();
        n->if_stmt.else_branch = parse_block();
    }
    return n;
}

static ASTNode* parse_while() {
    mat(TOKEN_SELAMA);
    if (!mat(TOKEN_BUKA_KURUNG)) error("Expected '('");
    ASTNode* n = make_node(AST_WHILE);
    n->while_stmt.condition = parse_expr();
    if (!mat(TOKEN_TUTUP_KURUNG)) error("Expected ')'");
    skip_ws();
    n->while_stmt.body = parse_block();
    return n;
}

// NEW: Parse return statement
static ASTNode* parse_return_statement() {
    mat(TOKEN_KEMBALI); // consume kembali
    
    ASTNode* n = make_node(AST_RETURN);
    // Return value is optional
    if (!chk(TOKEN_NEWLINE) && !chk(TOKEN_TITIK_KOMA) && !chk(TOKEN_EOF)) {
        n->return_stmt.value = parse_expr();
    } else {
        n->return_stmt.value = NULL; // No explicit return value
    }
    
    mat(TOKEN_TITIK_KOMA); // optional semicolon
    return n;
}

// ... other parsing functions ...

static ASTNode* parse_stmt() {
    skip_ws();
    fprintf(stderr, "DEBUG: parse_stmt - current token: %d ('%s')\n", cur()->type, cur()->lexeme);
    if (chk(TOKEN_EOF)) {
        fprintf(stderr, "DEBUG: parse_stmt - returning NULL (EOF)\n");
        return NULL;
    }
    if (chk(TOKEN_BUKA_KURAWAL)) {
        fprintf(stderr, "DEBUG: parse_stmt - parsing brace block\n");
        return parse_brace_block();
    }
    if (chk(TOKEN_JIKA)) {
        fprintf(stderr, "DEBUG: parse_stmt - parsing if statement\n");
        return parse_if();
    }
    if (chk(TOKEN_UNTUK)) {
        fprintf(stderr, "DEBUG: parse_stmt - parsing for loop\n");
        return parse_for();
    }
    if (chk(TOKEN_SELAMA)) {
        fprintf(stderr, "DEBUG: parse_stmt - parsing while loop\n");
        return parse_while();
    }
    if (chk(TOKEN_FUNGSI)) {
        fprintf(stderr, "DEBUG: parse_stmt - parsing function definition\n");
        return parse_function_definition();
    }
    if (chk(TOKEN_KEMBALI)) {
        fprintf(stderr, "DEBUG: parse_stmt - parsing return statement\n");
        return parse_return_statement();
    }
    
    // Assignment
    if (chk(TOKEN_NAMA) && tokens[pos+1]->type == TOKEN_EQUAL) {
        fprintf(stderr, "DEBUG: parse_stmt - parsing assignment\n");
        Token* name = cur();
        adv(); adv(); // consume name and '='
        ASTNode* n = make_node(AST_ASSIGN);
        n->assign.name = my_strdup(name->lexeme);
        n->assign.value = parse_expr();
        mat(TOKEN_TITIK_KOMA); // Consume optional semicolon
        return n;
    }
    
    // Fallback to expression statement
    fprintf(stderr, "DEBUG: parse_stmt - falling back to expression statement\n");
    ASTNode* expr_node = parse_expr();
    if (expr_node) {
        fprintf(stderr, "DEBUG: parse_stmt - created AST_EXPR_STMT\n");
        ASTNode* n = make_node(AST_EXPR_STMT);
        n->expr_stmt.expr = expr_node;
        mat(TOKEN_TITIK_KOMA); // optional semicolon
        return n;
    }
    
    fprintf(stderr, "DEBUG: parse_stmt - returning NULL (no statement matched)\n");
    return NULL;
}

// === PUBLIC API ===

void parser_init(Token** t, int c) { tokens = t; token_count = c; pos = 0; }

ASTNode* parse() {
    ASTNode* n = make_node(AST_BLOCK);
    int cap = 16;
    n->block.statements = malloc(sizeof(ASTNode*) * cap);
    n->block.count = 0;
    
    while (!chk(TOKEN_EOF)) {
        skip_ws();
        if (chk(TOKEN_EOF)) break;
        if (n->block.count >= cap) {
            cap *= 2;
            n->block.statements = realloc(n->block.statements, sizeof(ASTNode*) * cap);
        }
        ASTNode* s = parse_stmt();
        if (s) n->block.statements[n->block.count++] = s;
        skip_ws();
    }
    return n;
}

// === PRINT & FREE ===

void print_indent(int l) { for (int i = 0; i < l; i++) printf("  "); }

void print_ast(ASTNode* n, int l) {
    if (!n) return;
    print_indent(l);
    
    switch (n->type) {
        case AST_NUMBER: printf("Number: %d\n", n->number); break;
        case AST_FLOAT: printf("Float: %f\n", n->float_num); break;
        case AST_STRING: printf("String: \"%s\"\n", n->string); break;
        case AST_BOOLEAN: printf("Boolean: %s\n", n->boolean ? "true" : "false"); break;
        case AST_NULL: printf("Null\n"); break;
        
        // NEW: Array
        case AST_ARRAY:
            printf("Array [%d items]:\n", n->array.count);
            for (int i = 0; i < n->array.count; i++) {
                print_indent(l + 1);
                printf("[%d]:\n", i);
                print_ast(n->array.elements[i], l + 2);
            }
            break;
            
        case AST_IDENTIFIER: printf("Id: %s\n", n->name); break;
        
        case AST_BINARY: {
            const char* op = "?";
            switch (n->binary.op) {
                case TOKEN_PLUS: op = "+"; break;
                case TOKEN_MINUS: op = "-"; break;
                case TOKEN_BINTANG: op = "*"; break;
                case TOKEN_GARING: op = "/"; break;
                case TOKEN_EQ: op = "=="; break;
                case TOKEN_LT: op = "<"; break;
                case TOKEN_GT: op = ">"; break;
                default: break;
            }
            printf("Binary(%s):\n", op);
            print_ast(n->binary.left, l + 1);
            print_ast(n->binary.right, l + 1);
            break;
        }
        
        case AST_ASSIGN:
            printf("Assign %s =\n", n->assign.name);
            print_ast(n->assign.value, l + 1);
            break;
            
        case AST_CALL:
            printf("Call %s()\n", n->call.name);
            for (int i = 0; i < n->call.arg_count; i++)
                print_ast(n->call.args[i], l + 1);
            break;
            
        // NEW: Index access
        case AST_INDEX:
            printf("IndexAccess:\n");
            print_indent(l + 1); printf("Object:\n");
            print_ast(n->index.object, l + 2);
            print_indent(l + 1); printf("Index:\n");
            print_ast(n->index.index, l + 2);
            break;
            
        case AST_BLOCK:
            printf("Block (%d stmts):\n", n->block.count);
            for (int i = 0; i < n->block.count; i++)
                print_ast(n->block.statements[i], l + 1);
            break;
            
        case AST_IF:
            printf("If:\n");
            print_ast(n->if_stmt.condition, l + 1);
            print_ast(n->if_stmt.then_branch, l + 1);
            if (n->if_stmt.else_branch) {
                print_indent(l);
                printf("Else:\n");
                print_ast(n->if_stmt.else_branch, l + 1);
            }
            break;
            
        case AST_WHILE:
            printf("While:\n");
            print_ast(n->while_stmt.condition, l + 1);
            print_ast(n->while_stmt.body, l + 1);
            break;
            
        // NEW: For loop
        case AST_FOR:
            printf("For %s dalam:\n", n->for_stmt.var_name);
            print_ast(n->for_stmt.iterable, l + 1);
            print_ast(n->for_stmt.body, l + 1);
            break;
            
        // NEW: Function
        case AST_FUNCTION:
            printf("Function: %s (", n->function.name);
            for (int i = 0; i < n->function.param_count; i++) {
                printf("%s%s", n->function.params[i], (i == n->function.param_count - 1) ? "" : ", ");
            }
            printf(")\n");
            print_ast(n->function.body, l + 1);
            break;
            
        // NEW: Return
        case AST_RETURN:
            printf("Return:\n");
            if (n->return_stmt.value) {
                print_ast(n->return_stmt.value, l + 1);
            } else {
                print_indent(l + 1);
                printf("No value\n");
            }
            break;
            
        // NEW: Expression Statement
        case AST_EXPR_STMT:
            printf("Expression Statement:\n");
            print_ast(n->expr_stmt.expr, l + 1);
            break;
            
        default: printf("Unknown AST type %d\n", n->type);
    }
}

void free_ast(ASTNode* n) {
    if (!n) return;
    switch (n->type) {
        case AST_STRING: free(n->string); break;
        case AST_IDENTIFIER: free(n->name); break;
        
        // NEW: Free array elements
        case AST_ARRAY:
            for (int i = 0; i < n->array.count; i++) free_ast(n->array.elements[i]);
            free(n->array.elements);
            break;
            
        case AST_BINARY: free_ast(n->binary.left); free_ast(n->binary.right); break;
        case AST_UNARY: free_ast(n->unary.operand); break;
        case AST_ASSIGN: free(n->assign.name); free_ast(n->assign.value); break;
        case AST_CALL:
            free(n->call.name);
            for (int i = 0; i < n->call.arg_count; i++) free_ast(n->call.args[i]);
            free(n->call.args);
            break;
            
        // NEW: Free index
        case AST_INDEX:
            free_ast(n->index.object);
            free_ast(n->index.index);
            break;
            
        case AST_BLOCK:
            for (int i = 0; i < n->block.count; i++) free_ast(n->block.statements[i]);
            free(n->block.statements);
            break;
        case AST_IF:
            free_ast(n->if_stmt.condition);
            free_ast(n->if_stmt.then_branch);
            free_ast(n->if_stmt.else_branch);
            break;
        case AST_WHILE:
            free_ast(n->while_stmt.condition);
            free_ast(n->while_stmt.body);
            break;
            
        // NEW: Free for loop
        case AST_FOR:
            free(n->for_stmt.var_name);
            free_ast(n->for_stmt.iterable);
            free_ast(n->for_stmt.body);
            break;
            
        // NEW: Free function
        case AST_FUNCTION:
            free(n->function.name);
            for (int i = 0; i < n->function.param_count; i++) free(n->function.params[i]);
            free(n->function.params);
            free_ast(n->function.body);
            break;
            
        // NEW: Free return
        case AST_RETURN:
            free_ast(n->return_stmt.value);
            break;
            
        // NEW: Free Expression Statement
        case AST_EXPR_STMT:
            free_ast(n->expr_stmt.expr);
            break;
            
        default: break;
    }
    free(n);
}
