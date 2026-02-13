/*
    * NIRVANA LANG v0.2.2 - Optimized
    * Array, Boolean, For Loop (Optimized)
    */

    #include "lexer.h"
    #include "parser.h"
    #include "vm.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #define VERSION "0.3.0-OPT"

    void print_banner() {
        printf("╔════════════════════════════════════════╗\n");
        printf("║     NIRVANA LANG v%s              ║\n", VERSION);
        printf("║     Optimized For Loop                 ║\n");
        printf("╚════════════════════════════════════════╝\n");
    }

    void run_code(const char* code, int debug) {
        int tc;
        Token** tokens = lex(code, &tc);
        if (debug) {
            printf("\n[TOKENS]\n");
            for (int i = 0; i < tc; i++) print_token(tokens[i]);
        }
        
        parser_init(tokens, tc);
        ASTNode* ast = parse();
        if (debug) {
            printf("\n[AST]\n");
            print_ast(ast, 0);
        }
        
        VM* vm = vm_create();
        vm_compile(vm, ast);
        if (debug) vm_print_bytecode(vm);
        
        printf("\n[OUTPUT]\n");
        vm_run(vm);
        
        vm_destroy(vm);
        free_ast(ast);
        for (int i = 0; i < tc; i++) free_token(tokens[i]);
        free(tokens);
    }

    int main(int argc, char* argv[]) {
        print_banner();
        
        int debug = (argc > 1 && strcmp(argv[1], "-d") == 0);
        
        // Test 1: For loop kecil
        printf("\n=== Test 1: For loop range(5) ===\n");
        const char* code1 = 
            "untuk i dalam range(5):\n"
            "    cetak(i)\n";
        printf("Code:\n%s\n", code1);
        run_code(code1, debug);
        
        // Test 2: For loop BESAR (1000 iterasi)
        printf("\n=== Test 2: For loop range(1000) - Optimized! ===\n");
        const char* code2 = 
            "sum = 0\n"
            "untuk i dalam range(1000):\n"
            "    sum = sum + i\n"
            "cetak(sum)\n";
        printf("Code: (sum 0..999)\n");
        run_code(code2, debug);
        
        // Test 3: For dengan array
        printf("\n=== Test 3: For dengan array ===\n");
        const char* code3 = 
            "arr = [10, 20, 30, 40, 50]\n"
            "untuk x dalam arr:\n"
            "    cetak(x)\n";
        printf("Code:\n%s\n", code3);
        run_code(code3, debug);
        
        // Test 4: Nested loop
        printf("\n=== Test 4: Nested loops ===\n");
        const char* code4 = 
            "untuk i dalam range(3):\n"
            "    untuk j dalam range(3):\n"
            "        cetak(i * 10 + j)\n";
        printf("Code:\n%s\n", code4);
        run_code(code4, debug);
        
        return 0;
    }
    
