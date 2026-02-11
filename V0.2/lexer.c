#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// fungsi untuk membuat lexer baru
Token* make_token(TokenType type, const char* start, int length) {
    Token* t = malloc(sizeof(Token));
    if (!t) {
        fprintf(stderr, "Alokasi memori gagal untuk Token\n");
        exit(1);
    }
    t->type = type;
    t->lexeme = malloc(length + 1);
    if (!t->lexeme) {
        fprintf(stderr, "Alokasi memori gagal untuk lexeme\n");
        free(t);
        exit(1);
    }
    strncpy(t->lexeme, start, length);
    t->lexeme[length] = '\0';
    //*result = t;
    return t;
}

// fungsi untuk mencetak token
void print_token(Token* token) {
    const char* type = "";
    switch (token->type) {
        case TOKEN_STRING: type = "STRING"; break;
        case TOKEN_KARAKTER: type = "KARAKTER"; break;
        case TOKEN_NOMER: type = "NOMER"; break;
        case TOKEN_NAMA: type = "NAMA"; break;
        case TOKEN_PLUS: type = "PLUS"; break;
        case TOKEN_MINUS: type = "MINUS"; break;
        case TOKEN_EQUAL: type = "EQUAL"; break;
        case TOKEN_BUKA_KURUNG: type = "BUKA_KURUNG"; break;
        case TOKEN_TUTUP_KURUNG: type = "TUTUP_KURUNG"; break;
        case TOKEN_BINTANG: type = "BINTANG"; break;
        case TOKEN_GARING: type = "GARING"; break;
        case TOKEN_CETAK: type = "CETAK"; break;
        case TOKEN_EOF: type = "EOF"; break;
    }
    printf("Token: %s, Lexeme: %s\n", type, token->lexeme);
}

// fungsi untuk menghapus token
void free_token(Token* token) {
    if (token) {
        free(token->lexeme);
        free(token);
    }
}

// fungsi untuk melakukan lexing pada input string
Token** lex(const char* input, int* token_count) {
    int capacity = 16;
    Token** token_counts = malloc(sizeof(Token*) * capacity); // alokasi awal untuk
    int count = 0;
    const char* current = input;
    while (*current != '\0') {
        if (isspace(*current)) {
            current++;
            continue;
        } if (count + 1 >= capacity) {
            capacity *= 2;
            Token** new_buffer = realloc(token_counts, sizeof(Token*) * capacity);
            if (!new_buffer) {
                fprintf(stderr, "Alokasi memori gagal untuk token_counts\n");
                exit(1); 
            }
            token_counts = new_buffer;
        }
        else if (*current >= '0' && *current <= '9') {
            const char* start = current;
            while (*current >= '0' && *current <= '9') current++;
            if (count >= capacity) {
                capacity *= 2;
                token_counts = realloc(token_counts, sizeof(Token*) * capacity);
            }
            token_counts[count++] = make_token(TOKEN_NOMER, start, current - start);
            continue;
        } else if ((*current >= 'a' && *current <= 'z') || (*current >= 'A' && *current <= 'Z')) {
            const char* start = current;
            while ((*current >= 'a' && *current <= 'z') || (*current >= 'A' && *current <= 'Z')) current++;
            if (count >= capacity) {
                capacity *= 2;
                token_counts = realloc(token_counts, sizeof(Token*) * capacity);
            }
            token_counts[count++] = make_token(TOKEN_NAMA, start, current - start);
            continue;
        } else if (*current == '+') {
            if (count >= capacity) {
                capacity *= 2;
                token_counts = realloc(token_counts, sizeof(Token*) * capacity);
            }
            token_counts[count++] = make_token(TOKEN_PLUS, current, 1);
        } else if (*current == '-') {
            if (count >= capacity) {
                capacity *= 2;
                token_counts = realloc(token_counts, sizeof(Token*) * capacity);
            }
            token_counts[count++] = make_token(TOKEN_MINUS, current, 1);
        } else if (*current == '=') {
            if (count >= capacity) {
                capacity *= 2;
                token_counts = realloc(token_counts, sizeof(Token*) * capacity);
            }
            token_counts[count++] = make_token(TOKEN_EQUAL, current, 1);
        } else if (*current == '(') {
            if (count >= capacity) {
                capacity *= 2;
                token_counts = realloc(token_counts, sizeof(Token*) * capacity);
            }
            token_counts[count++] = make_token(TOKEN_BUKA_KURUNG, current, 1);
        } else if (*current == ')') {
            if (count >= capacity) {
                capacity *= 2;
                token_counts = realloc(token_counts, sizeof(Token*) * capacity);
            }
            token_counts[count++] = make_token(TOKEN_TUTUP_KURUNG, current, 1);
        } else if (*current == '*') {
            if (count >= capacity) {
                capacity *= 2;
                token_counts = realloc(token_counts, sizeof(Token*) * capacity);
            }
            token_counts[count++] = make_token(TOKEN_BINTANG, current, 1);
        } else if (*current == '/') {
            if (count >= capacity) {
                capacity *= 2;
                token_counts = realloc(token_counts, sizeof(Token*) * capacity);
            }
            token_counts[count++] = make_token(TOKEN_GARING, current, 1);

        } else if (isalpha(*current)) {
            const char* start = current;
            while (isalpha(*current)) current++;
            int length = current - start;
            if (length == 5 && strncmp(start, "cetak", 5) == 0) {
                if (count >= capacity) {
                    capacity *= 2;
                    token_counts[count++] = make_token(TOKEN_CETAK, start, length);
                } else {
                    if (count >= capacity) {
                        capacity *= 2;
                        token_counts = realloc(token_counts, sizeof(Token*) * capacity);
                    }
                    token_counts[count++] = make_token(TOKEN_NAMA, start, length);
                }
                continue;
            } else {
                if (count >= capacity) {
                    capacity *= 2;
                    token_counts = realloc(token_counts, sizeof(Token*) * capacity);
                }
                token_counts[count++] = make_token(TOKEN_CETAK, start, length);
            }
        } else{
            fprintf(stderr, "Karakter tidak dikenal: %c\n", *current);
        }
        current++;

    }
    // tambahkan EOF
    if (count >= capacity) {
        capacity *= 2;
        token_counts = realloc(token_counts, sizeof(Token*) * capacity);
    }
    token_counts[count++] = make_token(TOKEN_EOF, "", 0);

    *token_count = count;
    return token_counts;
}