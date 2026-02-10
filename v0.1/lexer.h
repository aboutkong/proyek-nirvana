#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_STRING,
    TOKEN_KARAKTER,
    TOKEN_NOMER,
    TOKEN_NAMA,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_BINTANG,
    TOKEN_GARING,
    TOKEN_EQUAL,
    TOKEN_BUKA_KURUNG, // '('
    TOKEN_TUTUP_KURUNG, // ')'
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char* lexeme;
} Token;

Token** lex(const char* input, int* token_count);
void print_token(Token* token);
void free_token(Token* token);
Token* make_token(TokenType type, const char* start, int length);
#endif // LEXER_H