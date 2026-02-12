/*
 * NIRVANA LANG v0.2.1
 * Register-Based VM inspired by LuaJIT
 * Supports both Brace { } and Indentation-based syntax
 */

#include "lexer.h"
#include "parser.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "0.2.1a"

void print_banner(void) {
    printf("╔════════════════════════════════════════╗\n");
    printf("║     NIRVANA LANG v%s                ║\n", VERSION);
    printf("║     Register-Based VM (LuaJIT-style)   ║\n");
    printf("║     Brace { } or Indentation-based     ║\n");
    printf("╚════════════════════════════════════════╝\n");
}

void print_help(void) {
    printf("Usage: nirvana [options] [file]\n\n");
    printf("Options:\n");
    printf("  -r, --repl       Start interactive REPL\n");
    printf("  -d, --debug      Show bytecode and debug info\n");
    printf("  -h, --help       Show this help\n");
    printf("  -v, --version    Show version\n");
    printf("\nSyntax Styles:\n");
    printf("  Brace style:      jika (x < y) maka { cetak(x) }\n");
    printf("  Indent style:     jika (x < y):\n");
    printf("                      cetak(x)\n");
}

char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Cannot allocate memory\n");
        fclose(file);
        return NULL;
    }
    
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    
    return buffer;
}

void run_code(const char* code, int debug) {
    // Lexing
    if (debug) printf("\n[LEXING]\n");
    int token_count = 0;
    Token** tokens = lex(code, &token_count);
    
    if (debug) {
        for (int i = 0; i < token_count; i++) {
            print_token(tokens[i]);
        }
    }
    
    // Parsing
    if (debug) printf("\n[PARSING]\n");
    parser_init(tokens, token_count);
    ASTNode* ast = parse();
    
    if (debug) {
        print_ast(ast, 0);
    }
    
    // Compilation & Execution
    if (debug) printf("\n[COMPILATION]\n");
    VM* vm = vm_create();
    vm_compile(vm, ast);
    
    if (debug) {
        vm_print_bytecode(vm);
    }
    
    if (debug) printf("\n[EXECUTION]\n");
    vm_run(vm);
    
    if (debug) {
        vm_print_registers(vm);
        vm_print_globals(vm);
    }
    
    // Cleanup
    vm_destroy(vm);
    free_ast(ast);
    for (int i = 0; i < token_count; i++) {
        free_token(tokens[i]);
    }
    free(tokens);
}

void repl_mode(int debug) {
    char line[1024];
    
    printf("\nEnter 'exit' or press Ctrl+D to quit\n");
    printf("Type 'debug' to toggle debug mode\n");
    printf("Supports both brace { } and indentation (4 spaces)\n\n");
    
    while (1) {
        printf(">>> ");
        fflush(stdout);
        
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
        
        if (strcmp(line, "exit") == 0) break;
        if (strcmp(line, "quit") == 0) break;
        if (strcmp(line, "debug") == 0) {
            debug = !debug;
            printf("Debug mode: %s\n", debug ? "ON" : "OFF");
            continue;
        }
        if (strlen(line) == 0) continue;
        
        run_code(line, debug);
    }
    
    printf("Goodbye!\n");
}

int main(int argc, char* argv[]) {
    int debug = 0;
    int repl = 0;
    char* filename = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_banner();
            print_help();
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("Nirvana Lang v%s\n", VERSION);
            return 0;
        }
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            debug = 1;
        }
        else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--repl") == 0) {
            repl = 1;
        }
        else if (argv[i][0] != '-') {
            filename = argv[i];
        }
    }
    
    print_banner();
    
    if (repl) {
        repl_mode(debug);
    }
    else if (filename) {
        char* code = read_file(filename);
        if (code) {
            run_code(code, debug);
            free(code);
        }
    }
    else {
        // Demo: Indentation-based style (Python-like)
        const char* example_indent = 
            "x = 10\n"
            "y = 20\n"
            "cetak(x + y)\n"
            "jika (x < y):\n"
            "    cetak(999)\n";
        
        // Demo: Brace-based style (C-like)
        const char* example_brace = 
            "x = 10\n"
            "y = 20\n"
            "cetak(x + y)\n"
            "jika (x < y) maka {\n"
            "    cetak(999)\n"
            "}\n";
        
        printf("\n=== Demo 1: Indentation-based (Python-style) ===\n");
        printf("Code:\n%s\n", example_indent);
        run_code(example_indent, debug);
        
        printf("\n=== Demo 2: Brace-based (C-style) ===\n");
        printf("Code:\n%s\n", example_brace);
        run_code(example_brace, debug);
        
        printf("\nUse -r for REPL or provide filename\n");
    }
    
    return 0;
}
