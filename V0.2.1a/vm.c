#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// === VALUE OPERATIONS ===

static Value make_nil(void) {
    Value v = {VAL_NIL, {0}};
    return v;
}

static Value make_bool(int b) {
    Value v;
    v.type = VAL_BOOL;
    v.i = b;
    return v;
}

static Value make_int(int64_t i) {
    Value v;
    v.type = VAL_INT;
    v.i = i;
    return v;
}

static Value make_float(double f) {
    Value v;
    v.type = VAL_FLOAT;
    v.f = f;
    return v;
}

static Value make_string(const char* s) {
    Value v;
    v.type = VAL_STRING;
    v.s = strdup(s);
    return v;
}

static void free_value(Value* v) {
    if (v->type == VAL_STRING && v->s) {
        free(v->s);
        v->s = NULL;
    }
}

static void print_value(Value* v) {
    switch (v->type) {
        case VAL_NIL: printf("nil"); break;
        case VAL_BOOL: printf("%s", v->i ? "true" : "false"); break;
        case VAL_INT: printf("%ld", v->i); break;
        case VAL_FLOAT: printf("%g", v->f); break;
        case VAL_STRING: printf("%s", v->s); break;
        default: printf("<object>"); break;
    }
}

// Type coercion for arithmetic
static int to_number(Value* v, double* out) {
    switch (v->type) {
        case VAL_INT: *out = (double)v->i; return 1;
        case VAL_FLOAT: *out = v->f; return 1;
        case VAL_STRING: {
            char* end;
            *out = strtod(v->s, &end);
            return *end == '\0';
        }
        default: return 0;
    }
}

static int is_truthy(Value* v) {
    if (v->type == VAL_NIL) return 0;
    if (v->type == VAL_BOOL) return v->i != 0;
    if (v->type == VAL_INT) return v->i != 0;
    if (v->type == VAL_FLOAT) return v->f != 0.0;
    return 1; // string and others are truthy
}

// === VM MANAGEMENT ===

VM* vm_create(void) {
    VM* vm = calloc(1, sizeof(VM));
    if (!vm) {
        fprintf(stderr, "Error: Failed to allocate VM\n");
        exit(1);
    }
    
    vm->functions = calloc(MAX_FUNCTIONS, sizeof(FunctionProto));
    vm->num_functions = 0;
    vm->current_func = -1;
    vm->pc = 0;
    vm->call_depth = 0;
    vm->num_globals = 0;
    vm->string_count = 0;
    
    return vm;
}

void vm_destroy(VM* vm) {
    if (!vm) return;
    
    // Free functions
    for (int i = 0; i < vm->num_functions; i++) {
        FunctionProto* fn = &vm->functions[i];
        free(fn->name);
        free(fn->code);
        free(fn->constants);
    }
    free(vm->functions);
    
    // Free globals
    for (int i = 0; i < vm->num_globals; i++) {
        free(vm->globals[i].name);
        free_value(&vm->globals[i].value);
    }
    
    // Free string pool
    for (int i = 0; i < vm->string_count; i++) {
        free(vm->string_pool[i]);
    }
    
    free(vm);
}

// === COMPILER ===

typedef struct {
    VM* vm;
    FunctionProto* fn;
    int next_reg;
    int num_locals;
    struct {
        char* name;
        int reg;
    } locals[64];
} Compiler;

static Compiler* current_compiler = NULL;

static int alloc_reg(Compiler* comp) {
    if (comp->next_reg >= MAX_REGISTERS - 10) {
        fprintf(stderr, "Error: Out of registers\n");
        exit(1);
    }
    return comp->next_reg++;
}

static void free_reg(Compiler* comp) {
    if (comp->next_reg > 0) comp->next_reg--;
}

static int add_constant(Compiler* comp, Value val) {
    FunctionProto* fn = comp->fn;
    
    // Check for existing constant
    for (int i = 0; i < fn->num_constants; i++) {
        if (fn->constants[i].type != val.type) continue;
        if (val.type == VAL_INT && fn->constants[i].i == val.i) return i;
        if (val.type == VAL_FLOAT && fn->constants[i].f == val.f) return i;
        if (val.type == VAL_STRING && strcmp(fn->constants[i].s, val.s) == 0) {
            free_value(&val); // Don't leak
            return i;
        }
    }
    
    int idx = fn->num_constants++;
    fn->constants = realloc(fn->constants, sizeof(Value) * fn->num_constants);
    fn->constants[idx] = val;
    return idx;
}

static void emit(Compiler* comp, Instruction inst) {
    FunctionProto* fn = comp->fn;
    if (fn->code_size >= fn->code_capacity) {
        fn->code_capacity = fn->code_capacity ? fn->code_capacity * 2 : 64;
        fn->code = realloc(fn->code, sizeof(Instruction) * fn->code_capacity);
    }
    fn->code[fn->code_size++] = inst;
}

static int find_local(Compiler* comp, const char* name) {
    for (int i = comp->num_locals - 1; i >= 0; i--) {
        if (strcmp(comp->locals[i].name, name) == 0) {
            return comp->locals[i].reg;
        }
    }
    return -1;
}

static int add_local(Compiler* comp, const char* name) {
    int reg = alloc_reg(comp);
    comp->locals[comp->num_locals].name = strdup(name);
    comp->locals[comp->num_locals].reg = reg;
    comp->num_locals++;
    return reg;
}

// Forward declarations
static int compile_expr(Compiler* comp, ASTNode* node);
static void compile_stmt(Compiler* comp, ASTNode* node);

static int compile_binary(Compiler* comp, ASTNode* node) {
    int left = compile_expr(comp, node->binary.left);
    int right = compile_expr(comp, node->binary.right);
    int result = alloc_reg(comp);
    
    OpCode op;
    switch (node->binary.op) {
        case TOKEN_PLUS: op = OP_ADD; break;
        case TOKEN_MINUS: op = OP_SUB; break;
        case TOKEN_BINTANG: op = OP_MUL; break;
        case TOKEN_GARING: op = OP_DIV; break;
        case TOKEN_PERSEN: op = OP_MOD; break;
        case TOKEN_PANGKAT: op = OP_POW; break;
        case TOKEN_EQ: op = OP_EQ; break;
        case TOKEN_LT: op = OP_LT; break;
        case TOKEN_GT: op = OP_LT; break; // Swapped operands
        case TOKEN_LTE: op = OP_LE; break;
        case TOKEN_GTE: op = OP_LE; break; // Swapped operands
        case TOKEN_NEQ: op = OP_NE; break;
        case TOKEN_AND: op = OP_AND; break;
        case TOKEN_OR: op = OP_OR; break;
        default:
            fprintf(stderr, "Error: Unknown binary operator\n");
            exit(1);
    }
    
    // For comparison operators with swapped semantics
    if (node->binary.op == TOKEN_GT || node->binary.op == TOKEN_GTE) {
        emit(comp, MAKE_ABC(op, result, right, left));
    } else {
        emit(comp, MAKE_ABC(op, result, left, right));
    }
    
    return result;
}

static int compile_unary(Compiler* comp, ASTNode* node) {
    int operand = compile_expr(comp, node->unary.operand);
    int result = alloc_reg(comp);
    
    if (node->unary.op == TOKEN_MINUS) {
        emit(comp, MAKE_ABC(OP_NEG, result, operand, 0));
    } else if (node->unary.op == TOKEN_NOT) {
        emit(comp, MAKE_ABC(OP_NOT, result, operand, 0));
    }
    
    return result;
}

static int compile_expr(Compiler* comp, ASTNode* node) {
    switch (node->type) {
        case AST_NUMBER: {
            int reg = alloc_reg(comp);
            int k = add_constant(comp, make_int(node->number));
            emit(comp, MAKE_ABx(OP_LOADK, reg, k));
            return reg;
        }
        
        case AST_FLOAT: {
            int reg = alloc_reg(comp);
            int k = add_constant(comp, make_float(node->float_num));
            emit(comp, MAKE_ABx(OP_LOADK, reg, k));
            return reg;
        }
        
        case AST_STRING: {
            int reg = alloc_reg(comp);
            int k = add_constant(comp, make_string(node->string));
            emit(comp, MAKE_ABx(OP_LOADK, reg, k));
            return reg;
        }
        
        case AST_BOOLEAN: {
            int reg = alloc_reg(comp);
            emit(comp, MAKE_ABC(OP_LOADBOOL, reg, node->boolean ? 1 : 0, 0));
            return reg;
        }
        
        case AST_NULL: {
            int reg = alloc_reg(comp);
            emit(comp, MAKE_ABC(OP_LOADNIL, reg, 0, 0));
            return reg;
        }
        
        case AST_IDENTIFIER: {
            // Check locals first
            int local = find_local(comp, node->name);
            if (local >= 0) return local;
            
            // Global variable
            int reg = alloc_reg(comp);
            int k = add_constant(comp, make_string(node->name));
            emit(comp, MAKE_ABx(OP_GETGLOBAL, reg, k));
            return reg;
        }
        
        case AST_BINARY:
            return compile_binary(comp, node);
            
        case AST_UNARY:
            return compile_unary(comp, node);
            
        case AST_CALL: {
            // Compile arguments
            int arg_regs[16];
            for (int i = 0; i < node->call.arg_count && i < 16; i++) {
                arg_regs[i] = compile_expr(comp, node->call.args[i]);
            }
            
            // Move arguments to consecutive registers
            int base = alloc_reg(comp);
            for (int i = 0; i < node->call.arg_count && i < 16; i++) {
                if (arg_regs[i] != base + i) {
                    emit(comp, MAKE_ABC(OP_MOVE, base + i, arg_regs[i], 0));
                }
            }
            
            // Special handling for built-in functions
            if (strcmp(node->call.name, "cetak") == 0) {
                emit(comp, MAKE_ABC(OP_PRINT, base, 0, 0));
                free_reg(comp); // Return nil
                int result = alloc_reg(comp);
                emit(comp, MAKE_ABC(OP_LOADNIL, result, 0, 0));
                return result;
            }
            
            // Regular function call
            int func_reg = alloc_reg(comp);
            int k = add_constant(comp, make_string(node->call.name));
            emit(comp, MAKE_ABx(OP_GETGLOBAL, func_reg, k));
            emit(comp, MAKE_ABC(OP_CALL, func_reg, node->call.arg_count + 1, 0));
            
            return func_reg;
        }
        
        default:
            fprintf(stderr, "Error: Unknown expression type\n");
            return 0;
    }
}

static void compile_stmt(Compiler* comp, ASTNode* node) {
    switch (node->type) {
        case AST_BLOCK:
            for (int i = 0; i < node->block.count; i++) {
                compile_stmt(comp, node->block.statements[i]);
            }
            break;
            
        case AST_ASSIGN: {
            int val_reg = compile_expr(comp, node->assign.value);
            
            // Check if local exists
            int local = find_local(comp, node->assign.name);
            if (local >= 0) {
                emit(comp, MAKE_ABC(OP_MOVE, local, val_reg, 0));
            } else {
                // Global
                int k = add_constant(comp, make_string(node->assign.name));
                emit(comp, MAKE_ABx(OP_SETGLOBAL, val_reg, k));
            }
            break;
        }
        
        case AST_EXPR_STMT:
            compile_expr(comp, node);
            break;
            
        case AST_IF: {
            int cond_reg = compile_expr(comp, node->if_stmt.condition);
            
            // Emit conditional jump
            int jmp_if_not = comp->fn->code_size;
            emit(comp, MAKE_ABC(OP_JMP_IF_NOT, cond_reg, 0, 0)); // placeholder
            
            compile_stmt(comp, node->if_stmt.then_branch);
            
            if (node->if_stmt.else_branch) {
                int jmp_else = comp->fn->code_size;
                emit(comp, MAKE_ABC(OP_JMP, 0, 0, 0)); // placeholder
                
                // Patch jmp_if_not
                int else_start = comp->fn->code_size;
                comp->fn->code[jmp_if_not] = MAKE_ABC(OP_JMP_IF_NOT, cond_reg, 
                                                       (else_start - jmp_if_not) & 0xFFFF, 0);
                
                compile_stmt(comp, node->if_stmt.else_branch);
                
                // Patch jmp_else
                int end_pos = comp->fn->code_size;
                comp->fn->code[jmp_else] = MAKE_ABC(OP_JMP, 0, (end_pos - jmp_else) & 0xFFFF, 0);
            } else {
                // Patch jmp_if_not
                int end_pos = comp->fn->code_size;
                comp->fn->code[jmp_if_not] = MAKE_ABC(OP_JMP_IF_NOT, cond_reg,
                                                       (end_pos - jmp_if_not) & 0xFFFF, 0);
            }
            break;
        }
        
        case AST_WHILE: {
            int loop_start = comp->fn->code_size;
            int cond_reg = compile_expr(comp, node->while_stmt.condition);
            
            int jmp_if_not = comp->fn->code_size;
            emit(comp, MAKE_ABC(OP_JMP_IF_NOT, cond_reg, 0, 0)); // placeholder
            
            compile_stmt(comp, node->while_stmt.body);
            
            // Jump back
            int back_jmp = -(comp->fn->code_size - loop_start + 1);
            emit(comp, MAKE_ABC(OP_JMP, 0, back_jmp & 0xFFFF, 0));
            
            // Patch exit
            int end_pos = comp->fn->code_size;
            comp->fn->code[jmp_if_not] = MAKE_ABC(OP_JMP_IF_NOT, cond_reg,
                                                   (end_pos - jmp_if_not) & 0xFFFF, 0);
            break;
        }
        
        default:
            // Treat as expression
            compile_expr(comp, node);
            break;
    }
}

void vm_compile(VM* vm, ASTNode* ast) {
    // Create main function
    FunctionProto* main_fn = &vm->functions[0];
    main_fn->name = strdup("__main__");
    main_fn->num_params = 0;
    main_fn->num_locals = 0;
    main_fn->max_stack = MAX_REGISTERS;
    vm->num_functions = 1;
    vm->current_func = 0;
    
    Compiler comp = {
        .vm = vm,
        .fn = main_fn,
        .next_reg = 0,
        .num_locals = 0
    };
    current_compiler = &comp;
    
    compile_stmt(&comp, ast);
    
    // Add halt
    emit(&comp, MAKE_ABC(OP_HALT, 0, 0, 0));
    
    current_compiler = NULL;
}

// === EXECUTION ===

static const char* op_names[] = {
    "LOADK", "LOADBOOL", "LOADNIL", "MOVE",
    "ADD", "SUB", "MUL", "DIV", "MOD", "POW", "NEG",
    "EQ", "LT", "LE", "NE",
    "AND", "OR", "NOT",
    "JMP", "JMP_IF", "JMP_IF_NOT",
    "CALL", "RETURN",
    "GETGLOBAL", "SETGLOBAL",
    "NEWTABLE", "GETTABLE", "SETTABLE",
    "PRINT", "HALT"
};

void vm_run(VM* vm) {
    if (vm->num_functions == 0) {
        fprintf(stderr, "Error: No code to run\n");
        return;
    }
    
    FunctionProto* fn = &vm->functions[0];
    vm->pc = 0;
    
    #define R(i) (vm->registers[i])
    #define K(i) (fn->constants[i])
    
    while (vm->pc < fn->code_size) {
        Instruction inst = fn->code[vm->pc];
        OpCode op = GET_OP(inst);
        int a = GET_A(inst);
        int b = GET_B(inst);
        int c = GET_C(inst);
        int bx = GET_Bx(inst);
        int sbx = GET_sBx(inst);
        
        switch (op) {
            case OP_LOADK:
                R(a) = K(bx);
                break;
                
            case OP_LOADBOOL:
                R(a) = make_bool(b);
                break;
                
            case OP_LOADNIL:
                R(a) = make_nil();
                break;
                
            case OP_MOVE:
                R(a) = R(b);
                break;
                
            case OP_ADD: {
                double left, right;
                if (!to_number(&R(b), &left) || !to_number(&R(c), &right)) {
                    fprintf(stderr, "Error: Cannot add non-numeric values\n");
                    exit(1);
                }
                // Use integer if both are integers
                if (R(b).type == VAL_INT && R(c).type == VAL_INT) {
                    R(a) = make_int(R(b).i + R(c).i);
                } else {
                    R(a) = make_float(left + right);
                }
                break;
            }
            
            case OP_SUB: {
                double left, right;
                if (!to_number(&R(b), &left) || !to_number(&R(c), &right)) {
                    fprintf(stderr, "Error: Cannot subtract non-numeric values\n");
                    exit(1);
                }
                if (R(b).type == VAL_INT && R(c).type == VAL_INT) {
                    R(a) = make_int(R(b).i - R(c).i);
                } else {
                    R(a) = make_float(left - right);
                }
                break;
            }
            
            case OP_MUL: {
                double left, right;
                if (!to_number(&R(b), &left) || !to_number(&R(c), &right)) {
                    fprintf(stderr, "Error: Cannot multiply non-numeric values\n");
                    exit(1);
                }
                if (R(b).type == VAL_INT && R(c).type == VAL_INT) {
                    R(a) = make_int(R(b).i * R(c).i);
                } else {
                    R(a) = make_float(left * right);
                }
                break;
            }
            
            case OP_DIV: {
                double left, right;
                if (!to_number(&R(b), &left) || !to_number(&R(c), &right)) {
                    fprintf(stderr, "Error: Cannot divide non-numeric values\n");
                    exit(1);
                }
                if (right == 0) {
                    fprintf(stderr, "Error: Division by zero\n");
                    exit(1);
                }
                R(a) = make_float(left / right);
                break;
            }
            
            case OP_MOD: {
                if (R(b).type != VAL_INT || R(c).type != VAL_INT) {
                    fprintf(stderr, "Error: Modulo requires integers\n");
                    exit(1);
                }
                R(a) = make_int(R(b).i % R(c).i);
                break;
            }
            
            case OP_POW: {
                double left, right;
                if (!to_number(&R(b), &left) || !to_number(&R(c), &right)) {
                    fprintf(stderr, "Error: Cannot power non-numeric values\n");
                    exit(1);
                }
                R(a) = make_float(pow(left, right));
                break;
            }
            
            case OP_NEG: {
                double val;
                if (!to_number(&R(b), &val)) {
                    fprintf(stderr, "Error: Cannot negate non-numeric value\n");
                    exit(1);
                }
                if (R(b).type == VAL_INT) {
                    R(a) = make_int(-R(b).i);
                } else {
                    R(a) = make_float(-val);
                }
                break;
            }
            
            case OP_EQ: {
                int result = 0;
                if (R(b).type != R(c).type) {
                    result = 0;
                } else {
                    switch (R(b).type) {
                        case VAL_NIL: result = 1; break;
                        case VAL_BOOL: result = R(b).i == R(c).i; break;
                        case VAL_INT: result = R(b).i == R(c).i; break;
                        case VAL_FLOAT: result = R(b).f == R(c).f; break;
                        case VAL_STRING: result = strcmp(R(b).s, R(c).s) == 0; break;
                        default: result = 0;
                    }
                }
                R(a) = make_bool(result);
                break;
            }
            
            case OP_LT: {
                double left, right;
                if (!to_number(&R(b), &left) || !to_number(&R(c), &right)) {
                    fprintf(stderr, "Error: Cannot compare non-numeric values\n");
                    exit(1);
                }
                R(a) = make_bool(left < right);
                break;
            }
            
            case OP_LE: {
                double left, right;
                if (!to_number(&R(b), &left) || !to_number(&R(c), &right)) {
                    fprintf(stderr, "Error: Cannot compare non-numeric values\n");
                    exit(1);
                }
                R(a) = make_bool(left <= right);
                break;
            }
            
            case OP_NE: {
                int result = 0;
                if (R(b).type != R(c).type) {
                    result = 1;
                } else {
                    switch (R(b).type) {
                        case VAL_NIL: result = 0; break;
                        case VAL_BOOL: result = R(b).i != R(c).i; break;
                        case VAL_INT: result = R(b).i != R(c).i; break;
                        case VAL_FLOAT: result = R(b).f != R(c).f; break;
                        case VAL_STRING: result = strcmp(R(b).s, R(c).s) != 0; break;
                        default: result = 1;
                    }
                }
                R(a) = make_bool(result);
                break;
            }
            
            case OP_AND:
                R(a) = make_bool(is_truthy(&R(b)) && is_truthy(&R(c)));
                break;
                
            case OP_OR:
                R(a) = make_bool(is_truthy(&R(b)) || is_truthy(&R(c)));
                break;
                
            case OP_NOT:
                R(a) = make_bool(!is_truthy(&R(b)));
                break;
                
            case OP_JMP:
                vm->pc += sbx;
                break;
                
            case OP_JMP_IF:
                if (is_truthy(&R(a))) {
                    vm->pc += sbx;
                }
                break;
                
            case OP_JMP_IF_NOT:
                if (!is_truthy(&R(a))) {
                    vm->pc += sbx;
                }
                break;
                
            case OP_GETGLOBAL: {
                const char* name = K(bx).s;
                int found = 0;
                for (int i = 0; i < vm->num_globals; i++) {
                    if (strcmp(vm->globals[i].name, name) == 0) {
                        R(a) = vm->globals[i].value;
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    fprintf(stderr, "Error: Undefined variable '%s'\n", name);
                    exit(1);
                }
                break;
            }
            
            case OP_SETGLOBAL: {
                const char* name = K(bx).s;
                int found = 0;
                for (int i = 0; i < vm->num_globals; i++) {
                    if (strcmp(vm->globals[i].name, name) == 0) {
                        vm->globals[i].value = R(a);
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    int idx = vm->num_globals++;
                    vm->globals[idx].name = strdup(name);
                    vm->globals[idx].value = R(a);
                }
                break;
            }
            
            case OP_PRINT:
                print_value(&R(a));
                printf("\n");
                break;
                
            case OP_HALT:
                return;
                
            default:
                fprintf(stderr, "Error: Unknown opcode %d\n", op);
                exit(1);
        }
        
        vm->pc++;
    }
    
    #undef R
    #undef K
}

// === DEBUG ===

void vm_print_bytecode(VM* vm) {
    if (vm->num_functions == 0) return;
    
    FunctionProto* fn = &vm->functions[0];
    printf("\n=== BYTECODE [%s] ===\n", fn->name);
    printf("Constants:\n");
    for (int i = 0; i < fn->num_constants; i++) {
        printf("  [%d] = ", i);
        print_value(&fn->constants[i]);
        printf("\n");
    }
    printf("\nInstructions:\n");
    
    for (int i = 0; i < fn->code_size; i++) {
        Instruction inst = fn->code[i];
        OpCode op = GET_OP(inst);
        int a = GET_A(inst);
        int b = GET_B(inst);
        int c = GET_C(inst);
        int bx = GET_Bx(inst);
        
        printf("%04d: %-10s ", i, op_names[op]);
        
        switch (op) {
            case OP_LOADK:
            case OP_GETGLOBAL:
            case OP_SETGLOBAL:
                printf("R%d, K%d", a, bx);
                break;
            case OP_LOADBOOL:
                printf("R%d, %s", a, b ? "true" : "false");
                break;
            case OP_LOADNIL:
            case OP_PRINT:
            case OP_HALT:
                printf("R%d", a);
                break;
            case OP_MOVE:
                printf("R%d, R%d", a, b);
                break;
            case OP_JMP:
            case OP_JMP_IF:
            case OP_JMP_IF_NOT:
                printf("%+d", (int16_t)bx);
                break;
            default:
                printf("R%d, R%d, R%d", a, b, c);
                break;
        }
        printf("\n");
    }
}

void vm_print_registers(VM* vm) {
    printf("\n=== REGISTERS ===\n");
    for (int i = 0; i < 16; i++) {
        if (vm->registers[i].type != VAL_NIL) {
            printf("R%d: ", i);
            print_value(&vm->registers[i]);
            printf("\n");
        }
    }
}

void vm_print_globals(VM* vm) {
    printf("\n=== GLOBALS ===\n");
    for (int i = 0; i < vm->num_globals; i++) {
        printf("%s = ", vm->globals[i].name);
        print_value(&vm->globals[i].value);
        printf("\n");
    }
}
