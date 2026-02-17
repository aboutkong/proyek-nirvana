#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "vm.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Penggunaan: %s <nama_file>\n", argv[0]);
        return 1;
    }

    // Baca file
    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror("fopen");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* input = malloc(len + 1);
    fread(input, 1, len, f);
    input[len] = '\0';
    fclose(f);

    // Lexing
    int token_count;
    Token** tokens = lex(input, &token_count);
    printf("Token-token:\n");
    for (int i = 0; i < token_count; i++) {
        print_token(tokens[i]);
    }

    // Parsing
    parser_init(tokens, token_count);
    ASTNode* ast = parse();
    printf("\nAST:\n");
    print_ast(ast, 0);

    // Environment global
    Environment* global = env_new(NULL);
    // Daftarkan fungsi bawaan
    env_set(global, "cetak", value_native("cetak", native_print));
    env_set(global, "range", value_native("range", native_range));
    env_set(global, "print", value_native("print", native_print)); // English version

    // Eksekusi
    printf("\nHasil Eksekusi:\n");
    bool returned_flag = false;
    Value result = eval(ast, global, &returned_flag);
    printf("Nilai kembali: ");
    print_value(result);
    printf("\n");

    // Pembersihan
    value_free(result);
    free_ast(ast);
    for (int i = 0; i < token_count; i++) free_token(tokens[i]);
    free(tokens);
    free(input);
    env_free(global);

    return 0;
}