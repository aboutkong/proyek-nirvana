#ifndef LEXER_H
#define LEXER_H

typedef enum {
    // Literals
    TOKEN_STRING,
    TOKEN_KARAKTER,
    TOKEN_NOMER,
    TOKEN_FLOAT,
    TOKEN_NAMA,
    TOKEN_BENAR,        // true/benar
    TOKEN_SALAH,        // false/salah
    TOKEN_NULL,
    
    // Array tokens (NEW)
    TOKEN_BUKA_KOTAK,   // [
    TOKEN_TUTUP_KOTAK,  // ]
    
    // Arithmetic
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_BINTANG,
    TOKEN_GARING,
    TOKEN_PERSEN,
    TOKEN_PANGKAT,
    
    // Comparison
    TOKEN_EQUAL,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LTE,
    TOKEN_GTE,
    
    // Logical
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    
    // Delimiters
    TOKEN_BUKA_KURUNG,
    TOKEN_TUTUP_KURUNG,
    TOKEN_BUKA_KURAWAL,
    TOKEN_TUTUP_KURAWAL,
    TOKEN_KOMA,
    TOKEN_TITIK_KOMA,
    TOKEN_TITIK,
    TOKEN_TITIK_DUA,
    
    // Indentation
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_NEWLINE,
    
    // Keywords
    TOKEN_CETAK,
    TOKEN_JIKA,
    TOKEN_MAKA,
    TOKEN_LAIN,
    TOKEN_SELAMA,
    TOKEN_FUNGSI,
    TOKEN_DEFI,         // NEW: for Python-like function definition
    TOKEN_KEMBALI,
    TOKEN_VAR,          // NEW: for variable declaration
    TOKEN_UNTUK,        // NEW: untuk (for loop)
    TOKEN_DALAM,        // NEW: dalam (in)
    TOKEN_RANGE,        // NEW: range
    
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char* lexeme;
    int line;
    int column;
    int indent_level;
} Token;

Token** lex(const char* input, int* token_count);
void print_token(Token* token);
void free_token(Token* token);
Token* make_token(TokenType type, const char* start, int length, int line, int col);

#endif
