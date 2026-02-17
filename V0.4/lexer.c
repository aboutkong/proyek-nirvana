#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_INDENT_STACK 64
static int indent_stack[MAX_INDENT_STACK];
static int indent_top = 0;

static void init_indent(void) {
    indent_top = 0;
    indent_stack[indent_top++] = 0;
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
    TokenType result = TOKEN_NAMA; // Default
    
    // Keywords existing
    if (len == 5 && strncmp(str, "cetak", 5) == 0) result = TOKEN_CETAK;
    else if (len == 4 && strncmp(str, "jika", 4) == 0) result = TOKEN_JIKA;
    else if (len == 4 && strncmp(str, "maka", 4) == 0) result = TOKEN_MAKA;
    else if (len == 4 && strncmp(str, "lain", 4) == 0) result = TOKEN_LAIN;
    else if (len == 6 && strncmp(str, "selama", 6) == 0) result = TOKEN_SELAMA;
    else if (len == 6 && strncmp(str, "fungsi", 6) == 0) result = TOKEN_FUNGSI;
    else if (len == 4 && strncmp(str, "defi", 4) == 0) result = TOKEN_DEFI;
    else if (len == 7 && strncmp(str, "kembali", 7) == 0) result = TOKEN_KEMBALI;
    else if (len == 3 && strncmp(str, "var", 3) == 0) result = TOKEN_VAR;
    
    // Boolean keywords
    else if (len == 5 && strncmp(str, "benar", 5) == 0) result = TOKEN_BENAR;
    else if (len == 4 && strncmp(str, "true", 4) == 0) result = TOKEN_BENAR;
    else if (len == 5 && strncmp(str, "salah", 5) == 0) result = TOKEN_SALAH;
    else if (len == 5 && strncmp(str, "false", 5) == 0) result = TOKEN_SALAH;
    else if (len == 5 && strncmp(str, "kosong", 5) == 0) result = TOKEN_NULL;
    else if (len == 4 && strncmp(str, "null", 4) == 0) result = TOKEN_NULL;
    
    // NEW: For loop keywords
    else if (len == 5 && strncmp(str, "untuk", 5) == 0) result = TOKEN_UNTUK;
    else if (len == 3 && strncmp(str, "for", 3) == 0) result = TOKEN_UNTUK;
    else if (len == 5 && strncmp(str, "dalam", 5) == 0) result = TOKEN_DALAM;
    else if (len == 2 && strncmp(str, "in", 2) == 0) result = TOKEN_DALAM;
    else if (len == 5 && strncmp(str, "range", 5) == 0) result = TOKEN_RANGE;
    
    // DEBUG PRINT
    fprintf(stderr, "DEBUG: check_keyword - str: '%.*s', len: %d, returns TokenType: %d\n", len, str, len, result);
    
    return result;
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
    const char* type_str = "UNKNOWN";
    switch (token->type) {
        case TOKEN_STRING: type_str = "STRING"; break;
        case TOKEN_KARAKTER: type_str = "KARAKTER"; break;
        case TOKEN_NOMER: type_str = "NOMER"; break;
        case TOKEN_FLOAT: type_str = "FLOAT"; break;
        case TOKEN_NAMA: type_str = "NAMA"; break;
        case TOKEN_BENAR: type_str = "BENAR"; break;
        case TOKEN_SALAH: type_str = "SALAH"; break;
        case TOKEN_NULL: type_str = "NULL"; break;
        case TOKEN_BUKA_KOTAK: type_str = "BUKA_KOTAK"; break;
        case TOKEN_TUTUP_KOTAK: type_str = "TUTUP_KOTAK"; break;
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
        case TOKEN_DEFI: type_str = "DEFI"; break;
        case TOKEN_KEMBALI: type_str = "KEMBALI"; break;
        case TOKEN_VAR: type_str = "VAR"; break;
        case TOKEN_UNTUK: type_str = "UNTUK"; break;
        case TOKEN_DALAM: type_str = "DALAM"; break;
        case TOKEN_RANGE: type_str = "RANGE"; break;
        case TOKEN_EOF: type_str = "EOF"; break;
        case TOKEN_ERROR: type_str = "ERROR"; break;
    }
    printf("[%d:%d] %-12s '%s' (indent:%d)\n", 
           token->line, token->column, type_str, token->lexeme, token->indent_level);
}

void free_token(Token* token) {
    if (token) {
        free(token->lexeme);
        free(token);
    }
}

#define CHECK_CAP if (count + 1 >= capacity) { \
    capacity *= 2; \
    token_counts = realloc(token_counts, sizeof(Token*) * capacity); \
}

#define EMIT_TOKEN(t) do { CHECK_CAP; token_counts[count++] = (t); } while(0)

Token** lex(const char* input, int* token_count) {
    int capacity = 16;
    Token** token_counts = malloc(sizeof(Token*) * capacity);
    int count = 0;
    const char* current = input;
    int line = 1, col = 1;
    int at_line_start = 1;
    
    init_indent();
    
    while (*current != '\0') {
        // Handle indentation at line start
        if (at_line_start) {
            int spaces = 0;
            while (*current == ' ' || *current == '\t') {
                if (*current == ' ') spaces += 1;
                else spaces += 4;
                current++;
                col++;
            }
            
            // Skip empty lines and comments
            if (*current == '\n' || *current == '#') {
                if (*current == '\n') { line++; col = 1; current++; }
                continue;
            }
            
            int indent_level = spaces / 4;
            int current_level = get_current_indent();
            
            if (indent_level > current_level) {
                push_indent(indent_level);
                EMIT_TOKEN(make_token(TOKEN_INDENT, "", 0, line, col));
            } else if (indent_level < current_level) {
                while (indent_level < get_current_indent()) {
                    pop_indent();
                    EMIT_TOKEN(make_token(TOKEN_DEDENT, "", 0, line, col));
                }
            }
            
            at_line_start = 0;
        }
        
        // Newline
        if (*current == '\n') {
            line++;
            col = 1;
            at_line_start = 1;
            EMIT_TOKEN(make_token(TOKEN_NEWLINE, "\n", 1, line, col));
            current++;
            continue;
        }
        
        // Whitespace
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
            current++; col++;
            const char* start = current;
            int len = 0;
            while (*current != '"' && *current != '\0' && *current != '\n') {
                current++; len++; col++;
            }
            EMIT_TOKEN(make_token(TOKEN_STRING, start, len, line, start_col));
            if (*current == '"') { current++; col++; }
            continue;
        }
        
        // Number
        if (isdigit(*current)) {
            const char* start = current;
            int is_float = 0;
            while (isdigit(*current)) { current++; col++; }
            if (*current == '.' && isdigit(*(current+1))) {
                is_float = 1;
                current++; col++;
                while (isdigit(*current)) { current++; col++; }
            }
            EMIT_TOKEN(make_token(is_float ? TOKEN_FLOAT : TOKEN_NOMER, 
                                   start, current - start, line, start_col));
            continue;
        }
        
        // Identifier/Keyword
        if (isalpha(*current) || *current == '_') {
            const char* start = current;
            while (is_identifier_char(*current)) { current++; col++; }
            int len = current - start;
            EMIT_TOKEN(make_token(check_keyword(start, len), start, len, line, start_col));
            continue;
        }
        
        // Two-char operators
        if (*current == '=' && *(current+1) == '=') {
            EMIT_TOKEN(make_token(TOKEN_EQ, current, 2, line, start_col));
            current += 2; col += 2; continue;
        }
        if (*current == '!' && *(current+1) == '=') {
            EMIT_TOKEN(make_token(TOKEN_NEQ, current, 2, line, start_col));
            current += 2; col += 2; continue;
        }
        if (*current == '<' && *(current+1) == '=') {
            EMIT_TOKEN(make_token(TOKEN_LTE, current, 2, line, start_col));
            current += 2; col += 2; continue;
        }
        if (*current == '>' && *(current+1) == '=') {
            EMIT_TOKEN(make_token(TOKEN_GTE, current, 2, line, start_col));
            current += 2; col += 2; continue;
        }
        if (*current == '&' && *(current+1) == '&') {
            EMIT_TOKEN(make_token(TOKEN_AND, current, 2, line, start_col));
            current += 2; col += 2; continue;
        }
        if (*current == '|' && *(current+1) == '|') {
            EMIT_TOKEN(make_token(TOKEN_OR, current, 2, line, start_col));
            current += 2; col += 2; continue;
        }
        
        // Single char tokens
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
            case '[': type = TOKEN_BUKA_KOTAK; break;   // NEW
            case ']': type = TOKEN_TUTUP_KOTAK; break;  // NEW
            case ',': type = TOKEN_KOMA; break;
            case ':': type = TOKEN_TITIK_DUA; break;
            case ';': type = TOKEN_TITIK_KOMA; break;
            case '.': type = TOKEN_TITIK; break;
        }
        
        EMIT_TOKEN(make_token(type, current, 1, line, start_col));
        current++; col++;
    }
    
    // Emit remaining DEDENTs
    while (get_current_indent() > 0) {
        pop_indent();
        EMIT_TOKEN(make_token(TOKEN_DEDENT, "", 0, line, col));
    }
    
    EMIT_TOKEN(make_token(TOKEN_EOF, "", 0, line, col));
    *token_count = count;
    return token_counts;
}
