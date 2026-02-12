#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"

static Token** tokens;
static int token_count;
static int pos = 0;

static void error_at_token(const char* message, Token* token) {
    fprintf(stderr, "Parse Error [%d:%d]: %s\n", token->line, token->column, message);
    fprintf(stderr, "  Near: '%s' (type: %d)\n", token->lexeme, token->type);
    exit(1);
}

static void error(const char* message) {
    error_at_token(message, tokens[pos]);
}

char* my_strdup(const char* s) {
    char* d = malloc(strlen(s) + 1);
    if (d) strcpy(d, s);
    return d;
}

ASTNode* make_node(ASTType type) {
    ASTNode* node = calloc(1, sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Failed to allocate AST node\n");
        exit(1);
    }
    node->type = type;
    node->line = tokens[pos]->line;
    return node;
}

static Token* current(void) { return tokens[pos]; }

static Token* peek(int offset) {
    int idx = pos + offset;
    if (idx < 0) idx = 0;
    if (idx >= token_count) idx = token_count - 1;
    return tokens[idx];
}

static Token* advance(void) {
    Token* t = current();
    if (t->type != TOKEN_EOF) pos++;
    return t;
}

static int check(TokenType type) {
    return current()->type == type;
}

static int match(TokenType type) {
    if (check(type)) {
        advance();
        return 1;
    }
    return 0;
}

static Token* consume(TokenType type, const char* message) {
    if (check(type)) return advance();
    error(message);
    return NULL;
}

// Skip newlines, indent, dedent
static void skip_whitespace(void) {
    while (check(TOKEN_NEWLINE) || check(TOKEN_INDENT) || check(TOKEN_DEDENT)) {
        advance();
    }
}

// Forward declarations
static ASTNode* parse_statement(void);
static ASTNode* parse_expression(void);
static ASTNode* parse_block(void);

// === EXPRESSION PARSING ===

static ASTNode* parse_primary(void) {
    Token* t = current();
    
    if (match(TOKEN_NOMER)) {
        ASTNode* node = make_node(AST_NUMBER);
        node->number = atoi(t->lexeme);
        return node;
    }
    
    if (match(TOKEN_FLOAT)) {
        ASTNode* node = make_node(AST_FLOAT);
        node->float_num = atof(t->lexeme);
        return node;
    }
    
    if (match(TOKEN_STRING)) {
        ASTNode* node = make_node(AST_STRING);
        node->string = my_strdup(t->lexeme);
        return node;
    }
    
    if (match(TOKEN_BENAR)) {
        ASTNode* node = make_node(AST_BOOLEAN);
        node->boolean = 1;
        return node;
    }
    
    if (match(TOKEN_SALAH)) {
        ASTNode* node = make_node(AST_BOOLEAN);
        node->boolean = 0;
        return node;
    }
    
    if (match(TOKEN_NULL)) {
        return make_node(AST_NULL);
    }
    
    // Identifier atau function call (termasuk 'cetak')
    if (check(TOKEN_NAMA) || check(TOKEN_CETAK)) {
        advance();
        char* name = my_strdup(t->lexeme);
        
        // Function call
        if (match(TOKEN_BUKA_KURUNG)) {
            ASTNode* node = make_node(AST_CALL);
            node->call.name = name;
            node->call.arg_count = 0;
            node->call.args = NULL;
            
            if (!check(TOKEN_TUTUP_KURUNG)) {
                int capacity = 4;
                node->call.args = malloc(sizeof(ASTNode*) * capacity);
                do {
                    if (node->call.arg_count >= capacity) {
                        capacity *= 2;
                        node->call.args = realloc(node->call.args, sizeof(ASTNode*) * capacity);
                    }
                    node->call.args[node->call.arg_count++] = parse_expression();
                } while (match(TOKEN_KOMA));
            }
            
            if (!match(TOKEN_TUTUP_KURUNG)) {
                error("Expected ')' after arguments");
            }
            return node;
        }
        
        // Just identifier
        ASTNode* node = make_node(AST_IDENTIFIER);
        node->name = name;
        return node;
    }
    
    // Parenthesized expression
    if (match(TOKEN_BUKA_KURUNG)) {
        ASTNode* expr = parse_expression();
        if (!match(TOKEN_TUTUP_KURUNG)) {
            error("Expected ')' after expression");
        }
        return expr;
    }
    
    return NULL;
}

static ASTNode* parse_unary(void) {
    Token* t = current();
    
    if (match(TOKEN_MINUS) || match(TOKEN_NOT)) {
        ASTNode* node = make_node(AST_UNARY);
        node->unary.op = t->type;
        node->unary.operand = parse_unary();
        return node;
    }
    
    ASTNode* primary = parse_primary();
    if (!primary) {
        error("Unexpected token in expression");
    }
    return primary;
}

static ASTNode* parse_power(void) {
    ASTNode* node = parse_unary();
    
    while (match(TOKEN_PANGKAT)) {
        ASTNode* right = parse_unary();
        ASTNode* new_node = make_node(AST_BINARY);
        new_node->binary.op = TOKEN_PANGKAT;
        new_node->binary.left = node;
        new_node->binary.right = right;
        node = new_node;
    }
    
    return node;
}

static ASTNode* parse_multiplicative(void) {
    ASTNode* node = parse_power();
    
    while (check(TOKEN_BINTANG) || check(TOKEN_GARING) || check(TOKEN_PERSEN)) {
        Token* op = current();
        advance();
        ASTNode* right = parse_power();
        ASTNode* new_node = make_node(AST_BINARY);
        new_node->binary.op = op->type;
        new_node->binary.left = node;
        new_node->binary.right = right;
        node = new_node;
    }
    
    return node;
}

static ASTNode* parse_additive(void) {
    ASTNode* node = parse_multiplicative();
    
    while (check(TOKEN_PLUS) || check(TOKEN_MINUS)) {
        Token* op = current();
        advance();
        ASTNode* right = parse_multiplicative();
        ASTNode* new_node = make_node(AST_BINARY);
        new_node->binary.op = op->type;
        new_node->binary.left = node;
        new_node->binary.right = right;
        node = new_node;
    }
    
    return node;
}

static ASTNode* parse_comparison(void) {
    ASTNode* node = parse_additive();
    
    while (check(TOKEN_LT) || check(TOKEN_GT) || check(TOKEN_LTE) || 
           check(TOKEN_GTE) || check(TOKEN_EQ) || check(TOKEN_NEQ)) {
        Token* op = current();
        advance();
        ASTNode* right = parse_additive();
        ASTNode* new_node = make_node(AST_BINARY);
        new_node->binary.op = op->type;
        new_node->binary.left = node;
        new_node->binary.right = right;
        node = new_node;
    }
    
    return node;
}

static ASTNode* parse_logical_and(void) {
    ASTNode* node = parse_comparison();
    
    while (match(TOKEN_AND)) {
        ASTNode* right = parse_comparison();
        ASTNode* new_node = make_node(AST_BINARY);
        new_node->binary.op = TOKEN_AND;
        new_node->binary.left = node;
        new_node->binary.right = right;
        node = new_node;
    }
    
    return node;
}

static ASTNode* parse_logical_or(void) {
    ASTNode* node = parse_logical_and();
    
    while (match(TOKEN_OR)) {
        ASTNode* right = parse_logical_and();
        ASTNode* new_node = make_node(AST_BINARY);
        new_node->binary.op = TOKEN_OR;
        new_node->binary.left = node;
        new_node->binary.right = right;
        node = new_node;
    }
    
    return node;
}

static ASTNode* parse_expression(void) {
    return parse_logical_or();
}

// === BLOCK PARSING ===

// Parse block dengan braces: { ... }
static ASTNode* parse_brace_block(void) {
    if (!match(TOKEN_BUKA_KURAWAL)) {
        error("Expected '{' to start block");
    }
    
    skip_whitespace();
    
    ASTNode* node = make_node(AST_BLOCK);
    int capacity = 8;
    node->block.statements = malloc(sizeof(ASTNode*) * capacity);
    node->block.count = 0;
    
    while (!check(TOKEN_TUTUP_KURAWAL) && !check(TOKEN_EOF)) {
        skip_whitespace();
        if (check(TOKEN_TUTUP_KURAWAL) || check(TOKEN_EOF)) break;
        
        if (node->block.count >= capacity) {
            capacity *= 2;
            node->block.statements = realloc(node->block.statements, sizeof(ASTNode*) * capacity);
        }
        
        ASTNode* stmt = parse_statement();
        if (stmt) {
            node->block.statements[node->block.count++] = stmt;
        }
        skip_whitespace();
    }
    
    if (!match(TOKEN_TUTUP_KURAWAL)) {
        error("Expected '}' to end block");
    }
    
    return node;
}

// Parse block dengan indentation setelah ':'
static ASTNode* parse_indent_block(void) {
    // Expect ':'
    if (!match(TOKEN_TITIK_DUA)) {
        error("Expected ':' or '{' to start block");
    }
    
    skip_whitespace();
    
    // Expect INDENT
    if (!match(TOKEN_INDENT)) {
        // Single statement after ':'
        ASTNode* node = make_node(AST_BLOCK);
        node->block.statements = malloc(sizeof(ASTNode*));
        node->block.count = 1;
        node->block.statements[0] = parse_statement();
        return node;
    }
    
    ASTNode* node = make_node(AST_BLOCK);
    int capacity = 8;
    node->block.statements = malloc(sizeof(ASTNode*) * capacity);
    node->block.count = 0;
    
    // Parse statements sampai DEDENT atau EOF
    while (!check(TOKEN_DEDENT) && !check(TOKEN_EOF)) {
        skip_whitespace();
        if (check(TOKEN_DEDENT) || check(TOKEN_EOF)) break;
        
        if (node->block.count >= capacity) {
            capacity *= 2;
            node->block.statements = realloc(node->block.statements, sizeof(ASTNode*) * capacity);
        }
        
        ASTNode* stmt = parse_statement();
        if (stmt) {
            node->block.statements[node->block.count++] = stmt;
        }
        skip_whitespace();
    }
    
    // Consume DEDENT
    if (check(TOKEN_DEDENT)) {
        advance();
    }
    
    return node;
}

// Parse block - auto detect style
static ASTNode* parse_block(void) {
    skip_whitespace();
    
    if (check(TOKEN_BUKA_KURAWAL)) {
        return parse_brace_block();
    } else if (check(TOKEN_TITIK_DUA)) {
        return parse_indent_block();
    } else {
        // Single statement as block
        ASTNode* node = make_node(AST_BLOCK);
        node->block.statements = malloc(sizeof(ASTNode*));
        node->block.count = 1;
        node->block.statements[0] = parse_statement();
        return node;
    }
}

// === STATEMENT PARSING ===

static ASTNode* parse_if_statement(void) {
    consume(TOKEN_JIKA, "Expected 'jika'");
    consume(TOKEN_BUKA_KURUNG, "Expected '(' after 'jika'");
    
    ASTNode* node = make_node(AST_IF);
    node->if_stmt.condition = parse_expression();
    
    consume(TOKEN_TUTUP_KURUNG, "Expected ')' after condition");
    
    // Optional 'maka' keyword (untuk brace style)
    if (check(TOKEN_MAKA)) advance();
    
    skip_whitespace();
    
    // Parse then branch
    node->if_stmt.then_branch = parse_block();
    
    node->if_stmt.else_branch = NULL;
    
    skip_whitespace();
    
    // Parse else branch
    if (match(TOKEN_LAIN)) {
        skip_whitespace();
        node->if_stmt.else_branch = parse_block();
    }
    
    return node;
}

static ASTNode* parse_while_statement(void) {
    consume(TOKEN_SELAMA, "Expected 'selama'");
    consume(TOKEN_BUKA_KURUNG, "Expected '(' after 'selama'");
    
    ASTNode* node = make_node(AST_WHILE);
    node->while_stmt.condition = parse_expression();
    
    consume(TOKEN_TUTUP_KURUNG, "Expected ')' after condition");
    
    skip_whitespace();
    node->while_stmt.body = parse_block();
    
    return node;
}

static ASTNode* parse_function(void) {
    consume(TOKEN_FUNGSI, "Expected 'fungsi'");
    
    Token* name = consume(TOKEN_NAMA, "Expected function name");
    consume(TOKEN_BUKA_KURUNG, "Expected '(' after function name");
    
    ASTNode* node = make_node(AST_FUNCTION);
    node->function.name = my_strdup(name->lexeme);
    node->function.param_count = 0;
    node->function.params = NULL;
    
    if (!check(TOKEN_TUTUP_KURUNG)) {
        int capacity = 4;
        node->function.params = malloc(sizeof(char*) * capacity);
        do {
            Token* param = consume(TOKEN_NAMA, "Expected parameter name");
            if (node->function.param_count >= capacity) {
                capacity *= 2;
                node->function.params = realloc(node->function.params, sizeof(char*) * capacity);
            }
            node->function.params[node->function.param_count++] = my_strdup(param->lexeme);
        } while (match(TOKEN_KOMA));
    }
    
    consume(TOKEN_TUTUP_KURUNG, "Expected ')' after parameters");
    
    skip_whitespace();
    node->function.body = parse_block();
    
    return node;
}

static ASTNode* parse_return(void) {
    consume(TOKEN_KEMBALI, "Expected 'kembalikan'");
    
    ASTNode* node = make_node(AST_RETURN);
    
    if (!check(TOKEN_NEWLINE) && !check(TOKEN_EOF) && !check(TOKEN_TUTUP_KURAWAL) && 
        !check(TOKEN_DEDENT) && !check(TOKEN_TITIK_DUA)) {
        node->return_stmt.value = parse_expression();
    } else {
        node->return_stmt.value = NULL;
    }
    
    return node;
}

static ASTNode* parse_statement(void) {
    skip_whitespace();
    
    if (check(TOKEN_EOF)) return NULL;
    
    // Block dengan braces
    if (check(TOKEN_BUKA_KURAWAL)) {
        return parse_brace_block();
    }
    
    // If statement
    if (check(TOKEN_JIKA)) {
        return parse_if_statement();
    }
    
    // While loop
    if (check(TOKEN_SELAMA)) {
        return parse_while_statement();
    }
    
    // Function definition
    if (check(TOKEN_FUNGSI)) {
        return parse_function();
    }
    
    // Return statement
    if (check(TOKEN_KEMBALI)) {
        return parse_return();
    }
    
    // Assignment: identifier = expression
    if (check(TOKEN_NAMA) && peek(1)->type == TOKEN_EQUAL) {
        Token* name = current();
        advance(); // consume name
        advance(); // consume =
        
        ASTNode* node = make_node(AST_ASSIGN);
        node->assign.name = my_strdup(name->lexeme);
        node->assign.value = parse_expression();
        return node;
    }
    
    // Expression statement
    return parse_expression();
}

// === PUBLIC API ===

void parser_init(Token **input_tokens, int count) {
    tokens = input_tokens;
    token_count = count;
    pos = 0;
}

ASTNode* parse(void) {
    ASTNode* node = make_node(AST_BLOCK);
    int capacity = 16;
    node->block.statements = malloc(sizeof(ASTNode*) * capacity);
    node->block.count = 0;
    
    while (!check(TOKEN_EOF)) {
        skip_whitespace();
        if (check(TOKEN_EOF)) break;
        
        if (node->block.count >= capacity) {
            capacity *= 2;
            node->block.statements = realloc(node->block.statements, sizeof(ASTNode*) * capacity);
        }
        
        ASTNode* stmt = parse_statement();
        if (stmt) {
            node->block.statements[node->block.count++] = stmt;
        }
        
        skip_whitespace();
    }
    
    return node;
}

// === PRINT & FREE ===

void print_indent(int level) {
    for (int i = 0; i < level; i++) printf("  ");
}

void print_ast(ASTNode* node, int level) {
    if (!node) return;
    print_indent(level);
    
    switch (node->type) {
        case AST_NUMBER:
            printf("Number: %d\n", node->number);
            break;
        case AST_FLOAT:
            printf("Float: %f\n", node->float_num);
            break;
        case AST_STRING:
            printf("String: \"%s\"\n", node->string);
            break;
        case AST_BOOLEAN:
            printf("Boolean: %s\n", node->boolean ? "true" : "false");
            break;
        case AST_NULL:
            printf("Null\n");
            break;
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->name);
            break;
        case AST_BINARY: {
            const char* op = "?";
            switch (node->binary.op) {
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
            print_ast(node->binary.left, level + 1);
            print_ast(node->binary.right, level + 1);
            break;
        }
        case AST_ASSIGN:
            printf("Assign: %s =\n", node->assign.name);
            print_ast(node->assign.value, level + 1);
            break;
        case AST_CALL:
            printf("Call: %s()\n", node->call.name);
            for (int i = 0; i < node->call.arg_count; i++) {
                print_ast(node->call.args[i], level + 1);
            }
            break;
        case AST_BLOCK:
            printf("Block (%d stmts):\n", node->block.count);
            for (int i = 0; i < node->block.count; i++) {
                print_ast(node->block.statements[i], level + 1);
            }
            break;
        case AST_IF:
            printf("If:\n");
            print_ast(node->if_stmt.condition, level + 1);
            print_ast(node->if_stmt.then_branch, level + 1);
            if (node->if_stmt.else_branch) {
                print_indent(level);
                printf("Else:\n");
                print_ast(node->if_stmt.else_branch, level + 1);
            }
            break;
        case AST_WHILE:
            printf("While:\n");
            print_ast(node->while_stmt.condition, level + 1);
            print_ast(node->while_stmt.body, level + 1);
            break;
        default:
            printf("Unknown node\n");
    }
}

void free_ast(ASTNode* node) {
    if (!node) return;
    switch (node->type) {
        case AST_STRING: free(node->string); break;
        case AST_IDENTIFIER: free(node->name); break;
        case AST_BINARY:
            free_ast(node->binary.left);
            free_ast(node->binary.right);
            break;
        case AST_UNARY:
            free_ast(node->unary.operand);
            break;
        case AST_ASSIGN:
            free(node->assign.name);
            free_ast(node->assign.value);
            break;
        case AST_CALL:
            free(node->call.name);
            for (int i = 0; i < node->call.arg_count; i++) free_ast(node->call.args[i]);
            free(node->call.args);
            break;
        case AST_BLOCK:
            for (int i = 0; i < node->block.count; i++) free_ast(node->block.statements[i]);
            free(node->block.statements);
            break;
        case AST_IF:
            free_ast(node->if_stmt.condition);
            free_ast(node->if_stmt.then_branch);
            free_ast(node->if_stmt.else_branch);
            break;
        case AST_WHILE:
            free_ast(node->while_stmt.condition);
            free_ast(node->while_stmt.body);
            break;
        default: break;
    }
    free(node);
}
