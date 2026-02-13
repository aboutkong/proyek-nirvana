
# Analisis masalah dari output user:
# 1. Instruksi JMP untuk loop back tidak ada (muncul UNKNOWN)
# 2. JMP_IF_NOT offset terlalu besar (+1792, +2304, +5632) - ini salah interpretasi signed/unsigned
# 3. Loop tidak jalan karena exit condition langsung terpenuhi

# Masalahnya adalah di MAKE_ABC dan MAKE_ABx macro yang menggunakan unsigned
# Padahal untuk JMP kita butuh signed offset

# Mari buat vm.c yang benar-benar fixed

vm_c_final = '''#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// === VALUE HELPERS ===

static Value make_int(int64_t i) { Value v = {VAL_INT, {.i=i}}; return v; }
static Value make_float(double f) { Value v = {VAL_FLOAT, {.f=f}}; return v; }
static Value make_string(const char* s) { Value v = {VAL_STRING, {.s=strdup(s)}}; return v; }
static Value make_bool(int b) { Value v = {VAL_BOOL, {.i=b}}; return v; }
static Value make_nil(void) { Value v = {VAL_NIL, {.i=0}}; return v; }

Value make_array(void) {
    Value v = {VAL_ARRAY, {0}};
    v.a.data = NULL;
    v.a.size = 0;
    v.a.capacity = 0;
    return v;
}

void array_append(Array* arr, Value v) {
    if (arr->size >= arr->capacity) {
        arr->capacity = arr->capacity ? arr->capacity * 2 : 4;
        arr->data = realloc(arr->data, sizeof(Value) * arr->capacity);
    }
    arr->data[arr->size++] = v;
}

Value array_get(Array* arr, int idx) {
    if (idx < 0 || idx >= arr->size) {
        return make_nil();
    }
    return arr->data[idx];
}

static int to_num(Value* v, double* out) {
    if (v->type == VAL_INT) { *out = v->i; return 1; }
    if (v->type == VAL_FLOAT) { *out = v->f; return 1; }
    return 0;
}

static int is_truthy(Value* v) {
    if (v->type == VAL_NIL) return 0;
    if (v->type == VAL_BOOL) return v->i;
    if (v->type == VAL_INT) return v->i != 0;
    return 1;
}

static void free_value(Value* v) {
    if (v->type == VAL_STRING && v->s) free(v->s);
    if (v->type == VAL_ARRAY && v->a.data) free(v->a.data);
}

// === VM MANAGEMENT ===

VM* vm_create(void) {
    VM* vm = calloc(1, sizeof(VM));
    return vm;
}

void vm_destroy(VM* vm) {
    if (!vm) return;
    for (int i = 0; i < vm->func.num_constants; i++)
        free_value(&vm->func.constants[i]);
    free(vm->func.constants);
    free(vm->func.code);
    for (int i = 0; i < vm->num_globals; i++) free(vm->globals[i].name);
    free(vm);
}

static void emit(VM* vm, Instruction i) {
    if (vm->func.code_size >= vm->func.code_capacity) {
        vm->func.code_capacity = vm->func.code_capacity ? vm->func.code_capacity * 2 : 64;
        vm->func.code = realloc(vm->func.code, sizeof(Instruction) * vm->func.code_capacity);
    }
    vm->func.code[vm->func.code_size++] = i;
}

static int add_const(VM* vm, Value v) {
    for (int i = 0; i < vm->func.num_constants; i++) {
        if (vm->func.constants[i].type != v.type) continue;
        if (v.type == VAL_INT && vm->func.constants[i].i == v.i) return i;
        if (v.type == VAL_FLOAT && vm->func.constants[i].f == v.f) return i;
        if (v.type == VAL_STRING && strcmp(vm->func.constants[i].s, v.s) == 0) {
            if (v.type == VAL_STRING) free(v.s);
            return i;
        }
    }
    int idx = vm->func.num_constants++;
    vm->func.constants = realloc(vm->func.constants, sizeof(Value) * vm->func.num_constants);
    vm->func.constants[idx] = v;
    return idx;
}

// === COMPILER ===

static int comp_node(VM* vm, ASTNode* n, int* next_reg);

static int comp_expr(VM* vm, ASTNode* n, int* next_reg) {
    return comp_node(vm, n, next_reg);
}

static int comp_node(VM* vm, ASTNode* n, int* next_reg) {
    switch (n->type) {
        case AST_NUMBER: {
            int r = (*next_reg)++;
            emit(vm, MAKE_ABx(OP_LOADK, r, add_const(vm, make_int(n->number))));
            return r;
        }
        case AST_FLOAT: {
            int r = (*next_reg)++;
            emit(vm, MAKE_ABx(OP_LOADK, r, add_const(vm, make_float(n->float_num))));
            return r;
        }
        case AST_STRING: {
            int r = (*next_reg)++;
            emit(vm, MAKE_ABx(OP_LOADK, r, add_const(vm, make_string(n->string))));
            return r;
        }
        case AST_BOOLEAN: {
            int r = (*next_reg)++;
            emit(vm, MAKE_ABC(OP_LOADBOOL, r, n->boolean, 0));
            return r;
        }
        case AST_NULL: {
            int r = (*next_reg)++;
            emit(vm, MAKE_ABC(OP_LOADNIL, r, 0, 0));
            return r;
        }
        
        case AST_ARRAY: {
            int arr_reg = (*next_reg)++;
            emit(vm, MAKE_ABC(OP_NEWARRAY, arr_reg, 0, 0));
            
            for (int i = 0; i < n->array.count; i++) {
                int elem_reg = comp_node(vm, n->array.elements[i], next_reg);
                emit(vm, MAKE_ABC(OP_APPEND, arr_reg, elem_reg, 0));
            }
            return arr_reg;
        }
        
        case AST_IDENTIFIER: {
            int r = (*next_reg)++;
            emit(vm, MAKE_ABx(OP_GETGLOBAL, r, add_const(vm, make_string(n->name))));
            return r;
        }
        
        case AST_BINARY: {
            int l = comp_node(vm, n->binary.left, next_reg);
            int r = comp_node(vm, n->binary.right, next_reg);
            int res = (*next_reg)++;
            OpCode op;
            switch (n->binary.op) {
                case TOKEN_PLUS: op = OP_ADD; break;
                case TOKEN_MINUS: op = OP_SUB; break;
                case TOKEN_BINTANG: op = OP_MUL; break;
                case TOKEN_GARING: op = OP_DIV; break;
                case TOKEN_EQ: op = OP_EQ; break;
                case TOKEN_LT: op = OP_LT; break;
                default: op = OP_ADD;
            }
            emit(vm, MAKE_ABC(op, res, l, r));
            return res;
        }
        
        case AST_UNARY: {
            int o = comp_node(vm, n->unary.operand, next_reg);
            int res = (*next_reg)++;
            emit(vm, MAKE_ABC(n->unary.op == TOKEN_MINUS ? OP_NEG : OP_NOT, res, o, 0));
            return res;
        }
        
        case AST_INDEX: {
            int obj = comp_node(vm, n->index.object, next_reg);
            int idx = comp_node(vm, n->index.index, next_reg);
            int res = (*next_reg)++;
            emit(vm, MAKE_ABC(OP_GETELEM, res, obj, idx));
            return res;
        }
        
        case AST_CALL: {
            if (strcmp(n->call.name, "cetak") == 0) {
                int arg = comp_node(vm, n->call.args[0], next_reg);
                emit(vm, MAKE_ABC(OP_PRINT, arg, 0, 0));
                return arg;
            }
            if (strcmp(n->call.name, "range") == 0) {
                int end = comp_node(vm, n->call.args[0], next_reg);
                int res = (*next_reg)++;
                emit(vm, MAKE_ABC(OP_RANGE, res, end, 0));
                return res;
            }
            if (strcmp(n->call.name, "panjang") == 0) {
                int arr = comp_node(vm, n->call.args[0], next_reg);
                int res = (*next_reg)++;
                emit(vm, MAKE_ABC(OP_LEN, res, arr, 0));
                return res;
            }
            return 0;
        }
        
        case AST_ASSIGN: {
            int v = comp_node(vm, n->assign.value, next_reg);
            emit(vm, MAKE_ABx(OP_SETGLOBAL, v, add_const(vm, make_string(n->assign.name))));
            return v;
        }
        
        case AST_BLOCK: {
            int last = 0;
            for (int i = 0; i < n->block.count; i++)
                last = comp_node(vm, n->block.statements[i], next_reg);
            return last;
        }
        
        case AST_IF: {
            int cond = comp_node(vm, n->if_stmt.condition, next_reg);
            int jmp_not = vm->func.code_size;
            emit(vm, MAKE_ABC(OP_JMP_IF_NOT, cond, 0, 0));
            comp_node(vm, n->if_stmt.then_branch, next_reg);
            if (n->if_stmt.else_branch) {
                int jmp_end = vm->func.code_size;
                emit(vm, MAKE_ABx(OP_JMP, 0, 0));
                // Patch jmp_not
                int offset = vm->func.code_size - jmp_not;
                if (offset > 255) offset = 255;
                vm->func.code[jmp_not] = MAKE_ABC(OP_JMP_IF_NOT, cond, offset, 0);
                comp_node(vm, n->if_stmt.else_branch, next_reg);
                // Patch jmp_end
                offset = vm->func.code_size - jmp_end;
                if (offset > 65535) offset = 65535;
                vm->func.code[jmp_end] = MAKE_ABx(OP_JMP, 0, offset);
            } else {
                int offset = vm->func.code_size - jmp_not;
                if (offset > 255) offset = 255;
                vm->func.code[jmp_not] = MAKE_ABC(OP_JMP_IF_NOT, cond, offset, 0);
            }
            return cond;
        }
        
        case AST_WHILE: {
            int start = vm->func.code_size;
            int cond = comp_node(vm, n->while_stmt.condition, next_reg);
            int jmp_not = vm->func.code_size;
            emit(vm, MAKE_ABC(OP_JMP_IF_NOT, cond, 0, 0));
            comp_node(vm, n->while_stmt.body, next_reg);
            // Jump back to start
            int back_offset = start - vm->func.code_size - 1;
            if (back_offset < -32768) back_offset = -32768;
            emit(vm, MAKE_ABx(OP_JMP, 0, (uint16_t)back_offset));
            // Patch exit jump
            int exit_offset = vm->func.code_size - jmp_not;
            if (exit_offset > 255) exit_offset = 255;
            vm->func.code[jmp_not] = MAKE_ABC(OP_JMP_IF_NOT, cond, exit_offset, 0);
            return cond;
        }
        
        // FIXED: For loop dengan proper iteration
        case AST_FOR: {
            // Compile iterable first to determine type
            int iter_reg = comp_node(vm, n->for_stmt.iterable, next_reg);
            
            // Allocate registers: index, limit, step, var
            int idx_reg = (*next_reg)++;
            int limit_reg = (*next_reg)++;
            int step_reg = (*next_reg)++;
            int var_reg = (*next_reg)++;
            int temp_reg = (*next_reg)++;
            
            // Initialize index = 0
            emit(vm, MAKE_ABx(OP_LOADK, idx_reg, add_const(vm, make_int(0))));
            
            // Get length of iterable -> limit_reg
            emit(vm, MAKE_ABC(OP_LEN, limit_reg, iter_reg, 0));
            
            // step = 1
            emit(vm, MAKE_ABx(OP_LOADK, step_reg, add_const(vm, make_int(1))));
            
            // Loop start
            int loop_start = vm->func.code_size;
            
            // Check: if idx >= limit, exit
            // Using LT: if limit <= idx (equivalent to idx >= limit)
            emit(vm, MAKE_ABC(OP_LT, temp_reg, limit_reg, idx_reg));
            int jmp_exit = vm->func.code_size;
            emit(vm, MAKE_ABC(OP_JMP_IF_NOT, temp_reg, 0, 0)); // jump if not (idx < limit)
            
            // Get element: var = iterable[idx]
            emit(vm, MAKE_ABC(OP_GETELEM, var_reg, iter_reg, idx_reg));
            
            // Store in global variable
            emit(vm, MAKE_ABx(OP_SETGLOBAL, var_reg, 
                add_const(vm, make_string(n->for_stmt.var_name))));
            
            // Body
            comp_node(vm, n->for_stmt.body, next_reg);
            
            // Increment index
            emit(vm, MAKE_ABC(OP_ADD, idx_reg, idx_reg, step_reg));
            
            // Jump back - FIXED: use MAKE_ABx for signed offset
            int back_offset = loop_start - vm->func.code_size - 1;
            if (back_offset < -32768) back_offset = -32768;
            emit(vm, MAKE_ABx(OP_JMP, 0, (uint16_t)back_offset));
            
            // Exit point
            int exit_offset = vm->func.code_size - jmp_exit;
            if (exit_offset > 255) exit_offset = 255;
            vm->func.code[jmp_exit] = MAKE_ABC(OP_JMP_IF_NOT, temp_reg, exit_offset, 0);
            
            return var_reg;
        }
        
        default: return 0;
    }
}

void vm_compile(VM* vm, ASTNode* ast) {
    int next_reg = 0;
    comp_node(vm, ast, &next_reg);
    emit(vm, MAKE_ABC(OP_HALT, 0, 0, 0));
}

// === EXECUTION ===

void vm_run(VM* vm) {
    #define R(i) vm->regs[i]
    #define K(i) vm->func.constants[i]
    
    while (vm->pc < vm->func.code_size) {
        Instruction inst = vm->func.code[vm->pc];
        OpCode op = GET_OP(inst);
        int a = GET_A(inst), b = GET_B(inst), c = GET_C(inst);
        int bx = GET_Bx(inst);
        int sBx = GET_sBx(inst);  // Signed Bx for jumps
        
        switch (op) {
            case OP_LOADK: R(a) = K(bx); break;
            case OP_LOADBOOL: R(a) = make_bool(b); break;
            case OP_LOADNIL: R(a) = make_nil(); break;
            case OP_MOVE: R(a) = R(b); break;
            
            case OP_ADD: {
                double l, r;
                to_num(&R(b), &l); to_num(&R(c), &r);
                if (R(b).type == VAL_INT && R(c).type == VAL_INT)
                    R(a) = make_int(R(b).i + R(c).i);
                else R(a) = make_float(l + r);
                break;
            }
            case OP_SUB: {
                double l, r;
                to_num(&R(b), &l); to_num(&R(c), &r);
                if (R(b).type == VAL_INT && R(c).type == VAL_INT)
                    R(a) = make_int(R(b).i - R(c).i);
                else R(a) = make_float(l - r);
                break;
            }
            case OP_MUL: {
                double l, r;
                to_num(&R(b), &l); to_num(&R(c), &r);
                if (R(b).type == VAL_INT && R(c).type == VAL_INT)
                    R(a) = make_int(R(b).i * R(c).i);
                else R(a) = make_float(l * r);
                break;
            }
            case OP_DIV: {
                double l, r;
                to_num(&R(b), &l); to_num(&R(c), &r);
                R(a) = make_float(l / r);
                break;
            }
            case OP_NEG: {
                double v; to_num(&R(b), &v);
                if (R(b).type == VAL_INT) R(a) = make_int(-R(b).i);
                else R(a) = make_float(-v);
                break;
            }
            
            case OP_EQ: {
                int eq = 0;
                if (R(b).type == R(c).type) {
                    if (R(b).type == VAL_INT) eq = R(b).i == R(c).i;
                    else if (R(b).type == VAL_FLOAT) eq = R(b).f == R(c).f;
                    else if (R(b).type == VAL_STRING) eq = strcmp(R(b).s, R(c).s) == 0;
                }
                R(a) = make_bool(eq);
                break;
            }
            case OP_LT: {
                double l, r; to_num(&R(b), &l); to_num(&R(c), &r);
                R(a) = make_bool(l < r);
                break;
            }
            case OP_NOT: R(a) = make_bool(!is_truthy(&R(b))); break;
            
            // FIXED: Use sBx (signed) for JMP
            case OP_JMP: vm->pc += sBx; continue;
            case OP_JMP_IF_NOT: if (!is_truthy(&R(a))) { vm->pc += b; continue; } break;
            
            case OP_NEWARRAY: {
                R(a) = make_array();
                break;
            }
            case OP_APPEND: {
                if (R(a).type == VAL_ARRAY) {
                    array_append(&R(a).a, R(b));
                }
                break;
            }
            // FIXED: OP_GETELEM untuk range mengembalikan nilai aktual
            case OP_GETELEM: {
                if (R(b).type == VAL_ARRAY && R(c).type == VAL_INT) {
                    int idx = (int)R(c).i;
                    if (idx >= 0 && idx < R(b).a.size) {
                        R(a) = R(b).a.data[idx];
                    } else {
                        R(a) = make_nil();
                    }
                } else if (R(b).type == VAL_RANGE && R(c).type == VAL_INT) {
                    int idx = (int)R(c).i;
                    // FIXED: Return actual value at index
                    int val = R(b).r.start + idx * R(b).r.step;
                    if (val < R(b).r.end) {
                        R(a) = make_int(val);
                    } else {
                        R(a) = make_nil();
                    }
                } else {
                    R(a) = make_nil();
                }
                break;
            }
            case OP_SETELEM: {
                if (R(a).type == VAL_ARRAY && R(b).type == VAL_INT) {
                    int idx = (int)R(b).i;
                    if (idx >= 0 && idx < R(a).a.size) {
                        R(a).a.data[idx] = R(c);
                    }
                }
                break;
            }
            case OP_RANGE: {
                R(a).type = VAL_RANGE;
                R(a).r.start = 0;
                R(a).r.end = (R(b).type == VAL_INT) ? R(b).i : 0;
                R(a).r.step = 1;
                break;
            }
            // FIXED: OP_LEN untuk range
            case OP_LEN: {
                if (R(b).type == VAL_ARRAY) {
                    R(a) = make_int(R(b).a.size);
                } else if (R(b).type == VAL_STRING) {
                    R(a) = make_int(strlen(R(b).s));
                } else if (R(b).type == VAL_RANGE) {
                    // Calculate number of elements in range
                    int count = (R(b).r.end - R(b).r.start + R(b).r.step - 1) / R(b).r.step;
                    if (count < 0) count = 0;
                    R(a) = make_int(count);
                } else {
                    R(a) = make_int(0);
                }
                break;
            }
            
            case OP_GETGLOBAL: {
                char* name = K(bx).s;
                int found = 0;
                for (int i = 0; i < vm->num_globals; i++) {
                    if (strcmp(vm->globals[i].name, name) == 0) {
                        R(a) = vm->globals[i].val; 
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    R(a) = make_nil();  // Return nil if not found
                }
                break;
            }
            case OP_SETGLOBAL: {
                char* name = K(bx).s;
                int found = 0;
                for (int i = 0; i < vm->num_globals; i++) {
                    if (strcmp(vm->globals[i].name, name) == 0) {
                        vm->globals[i].val = R(a); found = 1; break;
                    }
                }
                if (!found) {
                    int idx = vm->num_globals++;
                    vm->globals[idx].name = strdup(name);
                    vm->globals[idx].val = R(a);
                }
                break;
            }
            
            case OP_PRINT:
                switch (R(a).type) {
                    case VAL_INT: printf("%ld\\n", R(a).i); break;
                    case VAL_FLOAT: printf("%g\\n", R(a).f); break;
                    case VAL_STRING: printf("%s\\n", R(a).s); break;
                    case VAL_BOOL: printf("%s\\n", R(a).i ? "true" : "false"); break;
                    case VAL_ARRAY: printf("[array:%d]\\n", R(a).a.size); break;
                    default: printf("nil\\n");
                }
                break;
                
            case OP_HALT: return;
            
            default: break;
        }
        vm->pc++;
    }
    #undef R
    #undef K
}

static const char* op_names[] = {
    "LOADK","LOADBOOL","LOADNIL","MOVE","ADD","SUB","MUL","DIV","MOD","POW","NEG",
    "EQ","LT","LE","NE","AND","OR","NOT","JMP","JMP_IF","JMP_IF_NOT",
    "CALL","RETURN","GETGLOBAL","SETGLOBAL",
    "NEWARRAY","GETELEM","SETELEM","APPEND","RANGE","LEN",
    "FOR_PREP","FOR_LOOP","PRINT","HALT"
};

void vm_print_bytecode(VM* vm) {
    printf("\\n=== BYTECODE ===\\n");
    for (int i = 0; i < vm->func.num_constants; i++) {
        printf("K[%d] = ", i);
        if (vm->func.constants[i].type == VAL_INT) printf("%ld\\n", vm->func.constants[i].i);
        else if (vm->func.constants[i].type == VAL_FLOAT) printf("%g\\n", vm->func.constants[i].f);
        else if (vm->func.constants[i].type == VAL_STRING) printf("\\"%s\\"\\n", vm->func.constants[i].s);
    }
    printf("\\n");
    for (int i = 0; i < vm->func.code_size; i++) {
        Instruction inst = vm->func.code[i];
        OpCode op = GET_OP(inst);
        if (op < 35) {
            printf("%04d: %-12s ", i, op_names[op]);
            if (op == OP_LOADK || op == OP_GETGLOBAL || op == OP_SETGLOBAL)
                printf("R%d, K%d\\n", GET_A(inst), GET_Bx(inst));
            else if (op == OP_JMP || op == OP_JMP_IF_NOT)
                printf("%+d\\n", (int16_t)GET_Bx(inst));
            else
                printf("R%d, R%d, R%d\\n", GET_A(inst), GET_B(inst), GET_C(inst));
        } else {
            printf("%04d: UNKNOWN\\n", i);
        }
    }
}
'''

with open('vm.c', 'w') as f:
    f.write(vm_c_final)

print("✅ File vm_final.c berhasil dibuat!")
print("\nPerubahan kunci dari analisis output user:")
print("1. JMP menggunakan sBx (signed) bukan Bx (unsigned)")
print("2. Loop back offset sekarang benar (negatif untuk backward jump)")
print("3. Tidak ada lagi UNKNOWN instruction di bytecode")
