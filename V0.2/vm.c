// vm.c
#include "vm.h"
#include "parser.h" // Untuk AST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Helper untuk menambahkan instruksi ke bytecode ---
static void add_instruction(VM* vm, OpCode op, int value) {
    if (vm->code_size >= vm->code_capacity) {
        vm->code_capacity *= 2;
        vm->code = realloc(vm->code, sizeof(BytecodeInstruction) * vm->code_capacity);
    }

    vm->code[vm->code_size].op = op;
    vm->code[vm->code_size].operand.value = value;
    vm->code_size++;
}

static void add_instruction_name(VM* vm, OpCode op, const char* name) {
    if (vm->code_size >= vm->code_capacity) {
        vm->code_capacity *= 2;
        vm->code = realloc(vm->code, sizeof(BytecodeInstruction) * vm->code_capacity);
    }

    vm->code[vm->code_size].op = op;
    strncpy(vm->code[vm->code_size].operand.name, name, 31);
    vm->code[vm->code_size].operand.name[31] = '\0'; // Pastikan null-terminated
    vm->code_size++;
}

// --- Fungsi untuk mencari variabel ---
static int find_variable_index(VM* vm, const char* name) {
    for (int i = 0; i < vm->var_count; i++) {
        if (strcmp(vm->variables[i].name, name) == 0) {
            return i;
        }
    }
    return -1; // Tidak ditemukan
}

static void set_variable(VM* vm, const char* name, int value) {
    int index = find_variable_index(vm, name);
    if (index != -1) {
        vm->variables[index].value = value;
    } else {
        if (vm->var_count < 64) {
            strncpy(vm->variables[vm->var_count].name, name, 31);
            vm->variables[vm->var_count].name[31] = '\0';
            vm->variables[vm->var_count].value = value;
            vm->var_count++;
        }
    }
}

static int get_variable(VM* vm, const char* name) {
    int index = find_variable_index(vm, name);
    if (index != -1) {
        return vm->variables[index].value;
    }
    fprintf(stderr, "Error: Variabel '%s' tidak didefinisikan.\n", name);
    exit(1);
}

// --- Compiler: Ubah AST ke Bytecode (Recursive Descent Compilation) ---
static void compile_node(VM* vm, ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case AST_NUMBER:
            add_instruction(vm, OP_PUSH, node->number);
            break;
        case AST_IDENTIFIER:
            add_instruction_name(vm, OP_LOAD, node->name);
            break;
        case AST_BINARY:
            compile_node(vm, node->binary.left);
            compile_node(vm, node->binary.right);

            switch (node->binary.op) {
                case TOKEN_PLUS: add_instruction(vm, OP_ADD, 0); break;
                case TOKEN_MINUS: add_instruction(vm, OP_SUB, 0); break;
                case TOKEN_BINTANG: add_instruction(vm, OP_MUL, 0); break;
                case TOKEN_GARING: add_instruction(vm, OP_DIV, 0); break;
                default: break; // Harusnya tidak terjadi
            }
            break;
        case AST_ASSIGN:
            compile_node(vm, node->assign.value); // Eval ekspresi dulu
            add_instruction_name(vm, OP_STORE, node->assign.name);
            add_instruction(vm, OP_POP, 0); // Buang hasil dari stack karena disimpan di variabel
            break;

        case AST_CALL:
            for (int i = 0; i < node->call.arg_count; i++) {
                compile_node(vm, node->call.args[i]); // Compile argumen (hasilnya akan di-push ke stack)
            } 
            add_instruction_name(vm, OP_CALL, node->call.name); // Tambahkan instruksi
            add_instruction(vm, OP_POP, 0); // Buang hasil dari stack (misal untuk fungsi cetak)
            
            break;
    }
}

void vm_compile(VM* vm, ASTNode* ast) {
    compile_node(vm, ast);
    add_instruction(vm, OP_HALT, 0); // Tandai akhir program
}

// --- VM Execution Loop ---
void vm_run(VM* vm) {
    int ip = 0; // Instruction Pointer
    while (ip < vm->code_size) {
        BytecodeInstruction inst = vm->code[ip];
        switch (inst.op) {
            case OP_PUSH:
                vm->stack[vm->sp++] = inst.operand.value;
                break;
            case OP_LOAD:
                vm->stack[vm->sp++] = get_variable(vm, inst.operand.name);
                break;
            case OP_STORE:
                set_variable(vm, inst.operand.name, vm->stack[--vm->sp]);
                break;
            case OP_ADD:
                {
                    int b = vm->stack[--vm->sp];
                    int a = vm->stack[--vm->sp];
                    vm->stack[vm->sp++] = a + b;
                }
                break;
            case OP_SUB:
                {
                    int b = vm->stack[--vm->sp];
                    int a = vm->stack[--vm->sp];
                    vm->stack[vm->sp++] = a - b;
                }
                break;
            case OP_MUL:
                {
                    int b = vm->stack[--vm->sp];
                    int a = vm->stack[--vm->sp];
                    vm->stack[vm->sp++] = a * b;
                }
                break;
            case OP_DIV:
                {
                    int b = vm->stack[--vm->sp];
                    int a = vm->stack[--vm->sp];
                    if (b == 0) {
                        fprintf(stderr, "Runtime Error: Pembagian dengan nol.\n");
                        exit(1);
                    }
                    vm->stack[vm->sp++] = a / b;
                }
                break;
            case OP_POP:
                vm->sp--; // Buang nilai dari stack
                break;

            case OP_CALL:
                if (strcmp(inst.operand.name, "cetak") == 0) {
                    int value = vm->stack[--vm->sp];
                    printf("%d\n", value);
                } else {
                    fprintf(stderr, "Runtime Error: Fungsi '%s' tidak didefinisikan.\n", inst.operand.name);
                    exit(1);
                }
                break;
            case OP_HALT:
                return; // Selesai
        }
        ip++;
    }
}

// --- Konstruktor & Destruktor ---
VM* vm_create() {
    VM* vm = malloc(sizeof(VM));
    vm->code_capacity = 16;
    vm->code = malloc(sizeof(BytecodeInstruction) * vm->code_capacity);
    vm->code_size = 0;
    vm->sp = 0;
    vm->var_count = 0;
    return vm;
}

void vm_destroy(VM* vm) {
    free(vm->code);
    free(vm);
}

// --- Fungsi Debug ---
void vm_print_bytecode(VM* vm) {
    printf("=== Bytecode ===\n");
    for (int i = 0; i < vm->code_size; i++) {
        char* op_name;
        switch (vm->code[i].op) {
            case OP_PUSH: op_name = "PUSH"; break;
            case OP_LOAD: op_name = "LOAD"; break;
            case OP_STORE: op_name = "STORE"; break;
            case OP_ADD: op_name = "ADD"; break;
            case OP_SUB: op_name = "SUB"; break;
            case OP_MUL: op_name = "MUL"; break;
            case OP_DIV: op_name = "DIV"; break;
            case OP_POP: op_name = "POP"; break;
            case OP_HALT: op_name = "HALT"; break;
            default: op_name = "UNKNOWN"; break;
        }
        if (vm->code[i].op == OP_LOAD || vm->code[i].op == OP_STORE) {
            printf("%d: %s %s\n", i, op_name, vm->code[i].operand.name);
        } else {
            printf("%d: %s %d\n", i, op_name, vm->code[i].operand.value);
        }
    }
}

void vm_print_variables(VM* vm) {
    printf("=== Variables ===\n");
    for (int i = 0; i < vm->var_count; i++) {
        printf("%s = %d\n", vm->variables[i].name, vm->variables[i].value);
    }
}