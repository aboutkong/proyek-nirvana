#ifndef LEXER_H
#define LEXER_H

typedef enum {
    // Literals
    TOKEN_STRING,       
    TOKEN_KARAKTER,     
    TOKEN_NOMER,        
    TOKEN_FLOAT,        
    TOKEN_NAMA,         
    TOKEN_BENAR,        
    TOKEN_SALAH,        
    TOKEN_NULL,         
    
    // Arithmetic Operators
    TOKEN_PLUS,         
    TOKEN_MINUS,        
    TOKEN_BINTANG,      
    TOKEN_GARING,       
    TOKEN_PERSEN,       
    TOKEN_PANGKAT,      
    
    // Comparison Operators
    TOKEN_EQUAL,        
    TOKEN_EQ,           
    TOKEN_NEQ,          
    TOKEN_LT,           
    TOKEN_GT,           
    TOKEN_LTE,          
    TOKEN_GTE,          
    
    // Logical Operators
    TOKEN_AND,          
    TOKEN_OR,           
    TOKEN_NOT,          
    
    // Delimiters
    TOKEN_BUKA_KURUNG,  
    TOKEN_TUTUP_KURUNG, 
    TOKEN_BUKA_KURAWAL, // { - brace style
    TOKEN_TUTUP_KURAWAL,// } - brace style
    TOKEN_BUKA_KOTAK,   
    TOKEN_TUTUP_KOTAK,  
    TOKEN_KOMA,         
    TOKEN_TITIK_KOMA,   
    TOKEN_TITIK,        
    TOKEN_TITIK_DUA,    // : - indentation style (NEW)
    
    // Indentation (NEW)
    TOKEN_INDENT,       // Increase indent level
    TOKEN_DEDENT,       // Decrease indent level
    TOKEN_NEWLINE,      // Explicit newline
    
    // Keywords
    TOKEN_CETAK,        
    TOKEN_JIKA,         
    TOKEN_MAKA,         
    TOKEN_LAIN,         
    TOKEN_SELAMA,       
    TOKEN_FUNGSI,       
    TOKEN_KEMBALI,      
    
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char* lexeme;
    int line;
    int column;
    int indent_level;   // NEW: track indent level
} Token;

// API Functions
Token** lex(const char* input, int* token_count);
void print_token(Token* token);
void free_token(Token* token);
Token* make_token(TokenType type, const char* start, int length, int line, int col);

#endif // LEXER_H
