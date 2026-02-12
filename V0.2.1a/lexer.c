#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Stack untuk tracking indentasi
#define MAX_INDENT_STACK 64
static int indent_stack[MAX_INDENT_STACK];
static int indent_top = 0;

static void init_indent(void) {
    indent_top = 0;
    indent_stack[indent_top++] = 0;  // level 0 di bottom
}

static int get_current_indent(void) {
    return indent_stack[indent_top - 1];
}

static void push_indent(int level) {
    if (indent_top < MAX_INDENT_STACK) {
        indent_stack[indent_top++] = level;
    }
}

static int pop_indent(void) {
    if (indent_top > 1) {
        return indent_stack[--indent_top];
    }
    return 0;
}

static int is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}

static TokenType check_keyword(const char* str, int len) {
    if (len == 5 && strncmp(str, "cetak", 5) == 0) return TOKEN_CETAK;
    if (len == 4 && strncmp(str, "jika", 4) == 0) return TOKEN_JIKA;
    if (len == 4 && strncmp(str, "maka", 4) == 0) return TOKEN_MAKA;
    if (len == 4 && strncmp(str, "lain", 4) == 0) return TOKEN_LAIN;
    if (len == 6 && strncmp(str, "selama", 6) == 0) return TOKEN_SELAMA;
    if (len == 7 && strncmp(str, "fungsi", 7) == 0) return TOKEN_FUNGSI;
    if (len == 10 && strncmp(str, "kembalikan", 10) == 0) return TOKEN_KEMBALI;
    if (len == 5 && strncmp(str, "benar", 5) == 0) return TOKEN_BENAR;
    if (len == 5 && strncmp(str, "salah", 5) == 0) return TOKEN_SALAH;
    if (len == 5 && strncmp(str, "kosong", 5) == 0) return TOKEN_NULL;
    return TOKEN_NAMA;
}

Token* make_token(TokenType type, const char* start, int length, int line, int col) {
    Token* t = malloc(sizeof(Token));
    if (!t) {
        fprintf(stderr, "Error: Alokasi memori gagal untuk Token\n");
        exit(1);
    }
    t->type = type;
    t->lexeme = malloc(length + 1);
    if (!t->lexeme) {
        fprintf(stderr, "Error: Alokasi memori gagal untuk lexeme\n");
        free(t);
        exit(1);
    }
    strncpy(t->lexeme, start, length);
    t->lexeme[length] = '\0';
    t->line = line;
    t->column = col;
    t->indent_level = get_current_indent();
    return t;
}

void print_token(Token* token) {
    const char* type_str = "";
    switch (token->type) {
        case TOKEN_STRING: type_str = "STRING"; break;
        case TOKEN_KARAKTER: type_str = "KARAKTER"; break;
        case TOKEN_NOMER: type_str = "NOMER"; break;
        case TOKEN_FLOAT: type_str = "FLOAT"; break;
        case TOKEN_NAMA: type_str = "NAMA"; break;
        case TOKEN_BENAR: type_str = "BENAR"; break;
        case TOKEN_SALAH: type_str = "SALAH"; break;
        case TOKEN_NULL: type_str = "NULL"; break;
        case TOKEN_PLUS: type_str = "PLUS"; break;
        case TOKEN_MINUS: type_str = "MINUS"; break;
        case TOKEN_BINTANG: type_str = "BINTANG"; break;
        case TOKEN_GARING: type_str = "GARING"; break;
        case TOKEN_PERSEN: type_str = "PERSEN"; break;
        case TOKEN_PANGKAT: type_str = "PANGKAT"; break;
        case TOKEN_EQUAL: type_str = "EQUAL"; break;
        case TOKEN_EQ: type_str = "EQ"; break;
        case TOKEN_NEQ: type_str = "NEQ"; break;
        case TOKEN_LT: type_str = "LT"; break;
        case TOKEN_GT: type_str = "GT"; break;
        case TOKEN_LTE: type_str = "LTE"; break;
        case TOKEN_GTE: type_str = "GTE"; break;
        case TOKEN_AND: type_str = "AND"; break;
        case TOKEN_OR: type_str = "OR"; break;
        case TOKEN_NOT: type_str = "NOT"; break;
        case TOKEN_BUKA_KURUNG: type_str = "BUKA_KURUNG"; break;
        case TOKEN_TUTUP_KURUNG: type_str = "TUTUP_KURUNG"; break;
        case TOKEN_BUKA_KURAWAL: type_str = "BUKA_KURAWAL"; break;
        case TOKEN_TUTUP_KURAWAL: type_str = "TUTUP_KURAWAL"; break;
        case TOKEN_BUKA_KOTAK: type_str = "BUKA_KOTAK"; break;
        case TOKEN_TUTUP_KOTAK: type_str = "TUTUP_KOTAK"; break;
        case TOKEN_KOMA: type_str = "KOMA"; break;
        case TOKEN_TITIK_KOMA: type_str = "TITIK_KOMA"; break;
        case TOKEN_TITIK: type_str = "TITIK"; break;
        case TOKEN_TITIK_DUA: type_str = "TITIK_DUA"; break;
        case TOKEN_INDENT: type_str = "INDENT"; break;
        case TOKEN_DEDENT: type_str = "DEDENT"; break;
        case TOKEN_NEWLINE: type_str = "NEWLINE"; break;
        case TOKEN_CETAK: type_str = "CETAK"; break;
        case TOKEN_JIKA: type_str = "JIKA"; break;
        case TOKEN_MAKA: type_str = "MAKA"; break;
        case TOKEN_LAIN: type_str = "LAIN"; break;
        case TOKEN_SELAMA: type_str = "SELAMA"; break;
        case TOKEN_FUNGSI: type_str = "FUNGSI"; break;
        case TOKEN_KEMBALI: type_str = "KEMBALI"; break;
        case TOKEN_EOF: type_str = "EOF"; break;
        case TOKEN_ERROR: type_str = "ERROR"; break;
    }
    printf("[%d:%d] Token: %-12s Lexeme: '%s' (indent:%d)\n", 
           token->line, token->column, type_str, token->lexeme, token->indent_level);
}

void free_token(Token* token) {
    if (token) {
        free(token->lexeme);
        free(token);
    }
}

#define CHECK_CAPACITY() do { \
    if (count + 1 >= capacity) { \
        capacity *= 2; \
        Token** new_buffer = realloc(token_counts, sizeof(Token*) * capacity); \
        if (!new_buffer) { \
            fprintf(stderr, "Error: Alokasi memori gagal\n"); \
            exit(1); \
        } \
        token_counts = new_buffer; \
    } \
} while(0)

#define EMIT_TOKEN(t) do { \
    CHECK_CAPACITY(); \
    token_counts[count++] = (t); \
} while(0)

Token** lex(const char* input, int* token_count) {
    int capacity = 16;
    Token** token_counts = malloc(sizeof(Token*) * capacity);
    if (!token_counts) {
        fprintf(stderr, "Error: Alokasi awal gagal\n");
        exit(1);
    }
    
    int count = 0;
    const char* current = input;
    int line = 1;
    int col = 1;
    int at_line_start = 1;
    
    init_indent();
    
    while (*current != '\0') {
        // Handle indentation at line start
        if (at_line_start) {
            int spaces = 0;
            while (*current == ' ' || *current == '\t') {
                if (*current == ' ') spaces += 1;
                else spaces += 4;  // tab = 4 spaces
                current++;
                col++;
            }
            
            // Skip empty lines and comments
            if (*current == '\n' || *current == '#') {
                if (*current == '\n') {
                    line++;
                    col = 1;
                    current++;
                }
                continue;
            }
            
            // Calculate indent level (4 spaces = 1 level)
            int indent_level = spaces / 4;
            int current_level = get_current_indent();
            
            if (indent_level > current_level) {
                // INDENT
                push_indent(indent_level);
                EMIT_TOKEN(make_token(TOKEN_INDENT, "", 0, line, col));
            } else if (indent_level < current_level) {
                // DEDENT (bisa multiple)
                while (indent_level < get_current_indent()) {
                    pop_indent();
                    EMIT_TOKEN(make_token(TOKEN_DEDENT, "", 0, line, col));
                }
                if (indent_level != get_current_indent()) {
                    fprintf(stderr, "Error [%d:%d]: Indentasi tidak konsisten\n", line, col);
                }
            }
            
            at_line_start = 0;
        }
        
        // Track position
        if (*current == '\n') {
            line++;
            col = 1;
            at_line_start = 1;
            EMIT_TOKEN(make_token(TOKEN_NEWLINE, "\n", 1, line, col));
            current++;
            continue;
        }
        
        // Skip whitespace (non-indent)
        if (isspace(*current)) {
            col++;
            current++;
            continue;
        }
        
        // Comment
        if (*current == '#') {
            while (*current != '\n' && *current != '\0') {
                current++;
                col++;
            }
            continue;
        }
        
        int start_col = col;
        
        // String literal
        if (*current == '"') {
            current++;
            col++;
            const char* start = current;
            int len = 0;
            
            while (*current != '"' && *current != '\0' && *current != '\n') {
                current++;
                len++;
                col++;
            }
            
            if (*current == '"') {
                EMIT_TOKEN(make_token(TOKEN_STRING, start, len, line, start_col));
                current++;
                col++;
            } else {
                fprintf(stderr, "Error [%d:%d]: String tidak ditutup\n", line, start_col);
                EMIT_TOKEN(make_token(TOKEN_ERROR, start, len, line, start_col));
            }
            continue;
        }
        
        // Character literal
        if (*current == '\'') {
            current++;
            col++;
            const char* start = current;
            
            if (*current != '\'' && *current != '\0' && *current != '\n') {
                EMIT_TOKEN(make_token(TOKEN_KARAKTER, start, 1, line, start_col));
                current++;
                col++;
                
                if (*current == '\'') {
                    current++;
                    col++;
                } else {
                    fprintf(stderr, "Error [%d:%d]: Karakter tidak ditutup\n", line, start_col);
                }
            } else {
                fprintf(stderr, "Error [%d:%d]: Karakter kosong\n", line, start_col);
                if (*current == '\'') current++;
            }
            continue;
        }
        
        // Number
        if (isdigit(*current)) {
            const char* start = current;
            int is_float = 0;
            
            while (isdigit(*current)) {
                current++;
                col++;
            }
            
            if (*current == '.' && isdigit(*(current + 1))) {
                is_float = 1;
                current++;
                col++;
                while (isdigit(*current)) {
                    current++;
                    col++;
                }
            }
            
            TokenType type = is_float ? TOKEN_FLOAT : TOKEN_NOMER;
            EMIT_TOKEN(make_token(type, start, current - start, line, start_col));
            continue;
        }
        
        // Identifier or Keyword
        if (isalpha(*current) || *current == '_') {
            const char* start = current;
            
            while (is_identifier_char(*current)) {
                current++;
                col++;
            }
            
            int len = current - start;
            TokenType type = check_keyword(start, len);
            EMIT_TOKEN(make_token(type, start, len, line, start_col));
            continue;
        }
        
        // Two-character operators
        if (*current == '=' && *(current + 1) == '=') {
            EMIT_TOKEN(make_token(TOKEN_EQ, current, 2, line, start_col));
            current += 2;
            col += 2;
            continue;
        }
        
        if (*current == '!' && *(current + 1) == '=') {
            EMIT_TOKEN(make_token(TOKEN_NEQ, current, 2, line, start_col));
            current += 2;
            col += 2;
            continue;
        }
        
        if (*current == '<' && *(current + 1) == '=') {
            EMIT_TOKEN(make_token(TOKEN_LTE, current, 2, line, start_col));
            current += 2;
            col += 2;
            continue;
        }
        
        if (*current == '>' && *(current + 1) == '=') {
            EMIT_TOKEN(make_token(TOKEN_GTE, current, 2, line, start_col));
            current += 2;
            col += 2;
            continue;
        }
        
        if (*current == '&' && *(current + 1) == '&') {
            EMIT_TOKEN(make_token(TOKEN_AND, current, 2, line, start_col));
            current += 2;
            col += 2;
            continue;
        }
        
        if (*current == '|' && *(current + 1) == '|') {
            EMIT_TOKEN(make_token(TOKEN_OR, current, 2, line, start_col));
            current += 2;
            col += 2;
            continue;
        }
        
        // Single-character operators
        char c = *current;
        TokenType type = TOKEN_ERROR;
        
        switch (c) {
            case '+': type = TOKEN_PLUS; break;
            case '-': type = TOKEN_MINUS; break;
            case '*': type = TOKEN_BINTANG; break;
            case '/': type = TOKEN_GARING; break;
            case '%': type = TOKEN_PERSEN; break;
            case '^': type = TOKEN_PANGKAT; break;
            case '=': type = TOKEN_EQUAL; break;
            case '!': type = TOKEN_NOT; break;
            case '<': type = TOKEN_LT; break;
            case '>': type = TOKEN_GT; break;
            case '(': type = TOKEN_BUKA_KURUNG; break;
            case ')': type = TOKEN_TUTUP_KURUNG; break;
            case '{': type = TOKEN_BUKA_KURAWAL; break;
            case '}': type = TOKEN_TUTUP_KURAWAL; break;
            case '[': type = TOKEN_BUKA_KOTAK; break;
            case ']': type = TOKEN_TUTUP_KOTAK; break;
            case ',': type = TOKEN_KOMA; break;
            case ';': type = TOKEN_TITIK_KOMA; break;
            case '.': type = TOKEN_TITIK; break;
            case ':': type = TOKEN_TITIK_DUA; break;
        }
        
        if (type != TOKEN_ERROR) {
            EMIT_TOKEN(make_token(type, current, 1, line, start_col));
        } else {
            fprintf(stderr, "Error [%d:%d]: Karakter tidak dikenal: '%c'\n", line, start_col, c);
            EMIT_TOKEN(make_token(TOKEN_ERROR, current, 1, line, start_col));
        }
        
        current++;
        col++;
    }
    
    // Emit DEDENTs for remaining indent levels
    while (get_current_indent() > 0) {
        pop_indent();
        EMIT_TOKEN(make_token(TOKEN_DEDENT, "", 0, line, col));
    }
    
    // Add EOF
    EMIT_TOKEN(make_token(TOKEN_EOF, "", 0, line, col));
    
    *token_count = count;
    return token_counts;
}
