#include "lexer.h"
#include "parser.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void set_tokens(Token** input_tokens);

int main(int argc, char* argv[]) {
    const char code[] = "cetak(1 + 2 * 3)";
    char buffer[256];
    if (argc > 1) {
        FILE *file = fopen(argv[1], "r");
        if (!file) {
            fprintf(stderr, "Gagal membuka file: %s\n", argv[1]);
            return 1;
        }
        size_t len = fread(buffer, 1, sizeof(buffer) - 1, file);
        buffer[len] = '\0';
        strcpy((char*)code, buffer);
        fclose(file);
    }
    int token_count = 0;
    printf("Lexingan kode: %s\n", code);
    Token** tokens = lex(code, &token_count);
    for (int i = 0; i < token_count; i++) {
        print_token(tokens[i]);
    }
    printf("\n[PARSING]\n");
    parser_init(tokens);
    ASTNode* ast = parse();
    printf("\n[VM EXECUTION]\n");
    VM* vm = vm_create();
    vm_compile(vm, ast);
    vm_print_bytecode(vm);
    vm_run(vm);
    vm_print_variables(vm);
    
    // Cleanup
    vm_destroy(vm);
    free_ast(ast);
    for (int i = 0; i < token_count; i++) {
        free_token(tokens[i]);
    }
    free(tokens);
    
    return 0;
}
