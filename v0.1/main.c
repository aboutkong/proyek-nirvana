#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[]) {
    const char *input = "umur = 20 + (5 - 3)";
    char buffer[256];
    if (argc > 1) {
        FILE *file = fopen(argv[1], "r");
        if (!file) {
            fprintf(stderr, "Gagal membuka file: %s\n", argv[1]);
            return 1;
        }
        size_t len = fread(buffer, 1, sizeof(buffer) - 1, file);
        buffer[len] = '\0';
        input = buffer;
        fclose(file);
    }
    int token_count = 0;
    printf("Lexingan kode: %s\n", input);
    Token** tokens = lex(input, &token_count);
    for (int i = 0; i < token_count; i++) {
        print_token(tokens[i]);
        free_token(tokens[i]);
}
free(tokens);
    return 0;
    printf("Lexer test selesai.\n");
}
