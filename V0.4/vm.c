#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> // NEW

// Forward declarations
static Value value_copy(Value v);
static int is_truthy(Value v);

// -------------------------------------------------------------------
// Environment
// -------------------------------------------------------------------
Environment* env_new(Environment* parent) {
    Environment* env = malloc(sizeof(Environment));
    env->parent = parent;
    env->bindings = NULL;
    return env;
}

void env_free(Environment* env) {
    if (!env) return;
    Binding* b = env->bindings;
    while (b) {
        Binding* next = b->next;
        free(b->name);
        value_free(b->value);
        free(b);
        b = next;
    }
    free(env);
}

void env_set(Environment* env, const char* name, Value value) {
    Binding* b = env->bindings;
    while (b) {
        if (strcmp(b->name, name) == 0) {
            value_free(b->value);
            b->value = value;
            return;
        }
        b = b->next;
    }
    b = malloc(sizeof(Binding));
    b->name = strdup(name);
    b->value = value;
    b->next = env->bindings;
    env->bindings = b;
}

Value env_get(Environment* env, const char* name) {
    for (Environment* e = env; e; e = e->parent) {
        Binding* b = e->bindings;
        while (b) {
            if (strcmp(b->name, name) == 0) {
                return value_copy(b->value);
            }
            b = b->next;
        }
    }
    fprintf(stderr, "Runtime Error: Undefined variable '%s'\n", name);
    exit(1);
}

// -------------------------------------------------------------------
// Value constructors
// -------------------------------------------------------------------
Value value_number(int n) {
    Value v;
    v.type = VAL_NUMBER;
    v.number = n;
    return v;
}

Value value_float(double f) {
    Value v;
    v.type = VAL_FLOAT;
    v.float_num = f;
    return v;
}

Value value_string(const char* s) {
    Value v;
    v.type = VAL_STRING;
    v.string = strdup(s);
    return v;
}

Value value_boolean(int b) {
    Value v;
    v.type = VAL_BOOLEAN;
    v.boolean = b;
    return v;
}

Value value_null() {
    Value v;
    v.type = VAL_NULL;
    return v;
}

Value value_array() {
    Value v;
    v.type = VAL_ARRAY;
    v.array.count = 0;
    v.array.capacity = 4;
    v.array.elements = malloc(sizeof(Value) * v.array.capacity);
    return v;
}

void array_append(Value* arr, Value v) {
    if (arr->type != VAL_ARRAY) return;
    if (arr->array.count >= arr->array.capacity) {
        arr->array.capacity *= 2;
        arr->array.elements = realloc(arr->array.elements, sizeof(Value) * arr->array.capacity);
    }
    arr->array.elements[arr->array.count++] = v;
}

Value value_function(ASTNode* func_node, Environment* closure) {
    Value v;
    v.type = VAL_FUNCTION;
    v.function.func_node = func_node;
    v.function.closure = closure;
    return v;
}

Value value_native(const char* name, Value (*func)(Value* args, int count)) {
    Value v;
    v.type = VAL_NATIVE;
    v.native.name = name;
    v.native.func = func;
    return v;
}

// -------------------------------------------------------------------
// Value copy and free
// -------------------------------------------------------------------
static Value value_copy(Value v) {
    Value res;
    res.type = v.type;
    switch (v.type) {
        case VAL_NUMBER: res.number = v.number; break;
        case VAL_FLOAT: res.float_num = v.float_num; break;
        case VAL_BOOLEAN: res.boolean = v.boolean; break;
        case VAL_NULL: break;
        case VAL_STRING: res.string = strdup(v.string); break;
        case VAL_ARRAY:
            res.array.count = v.array.count;
            res.array.capacity = v.array.capacity;
            res.array.elements = malloc(sizeof(Value) * res.array.capacity);
            for (int i = 0; i < res.array.count; i++) {
                res.array.elements[i] = value_copy(v.array.elements[i]);
            }
            break;
        case VAL_FUNCTION:
            res.function.func_node = v.function.func_node;
            res.function.closure = v.function.closure;
            break;
        case VAL_NATIVE:
            res.native.name = v.native.name;
            res.native.func = v.native.func;
            break;
    }
    return res;
}

void value_free(Value v) {
    switch (v.type) {
        case VAL_STRING: free(v.string); break;
        case VAL_ARRAY:
            for (int i = 0; i < v.array.count; i++) {
                value_free(v.array.elements[i]);
            }
            free(v.array.elements);
            break;
        case VAL_FUNCTION:
            // closure tidak di-free di sini (sementara biarkan)
            break;
        default: break;
    }
}

// -------------------------------------------------------------------
// Truthy check
// -------------------------------------------------------------------
static int is_truthy(Value v) {
    switch (v.type) {
        case VAL_NUMBER: return v.number != 0;
        case VAL_FLOAT: return v.float_num != 0.0;
        case VAL_BOOLEAN: return v.boolean;
        case VAL_STRING: return v.string[0] != '\0';
        case VAL_ARRAY: return v.array.count > 0;
        case VAL_FUNCTION: return 1;
        case VAL_NATIVE: return 1;
        case VAL_NULL: return 0;
        default: return 0;
    }
}

// -------------------------------------------------------------------
// Built-in functions
// -------------------------------------------------------------------
Value native_print(Value* args, int count) {
    for (int i = 0; i < count; i++) {
        print_value(args[i]);
        if (i < count - 1) printf(" ");
    }
    printf("\n");
    return value_null();
}

Value native_range(Value* args, int count) {
    int start = 0, end;
    if (count == 1) {
        if (args[0].type != VAL_NUMBER) {
            fprintf(stderr, "range: argumen harus angka\n");
            exit(1);
        }
        end = args[0].number;
    } else if (count == 2) {
        if (args[0].type != VAL_NUMBER || args[1].type != VAL_NUMBER) {
            fprintf(stderr, "range: argumen harus angka\n");
            exit(1);
        }
        start = args[0].number;
        end = args[1].number;
    } else {
        fprintf(stderr, "range: jumlah argumen salah\n");
        exit(1);
    }
    Value arr = value_array();
    for (int i = start; i < end; i++) {
        array_append(&arr, value_number(i));
    }
    return arr;
}

void print_value(Value v) {
    switch (v.type) {
        case VAL_NUMBER: printf("%d", v.number); break;
        case VAL_FLOAT: printf("%g", v.float_num); break;
        case VAL_STRING: printf("\"%s\"", v.string); break;
        case VAL_BOOLEAN: printf(v.boolean ? "benar" : "salah"); break;
        case VAL_NULL: printf("kosong"); break;
        case VAL_ARRAY:
            printf("[");
            for (int i = 0; i < v.array.count; i++) {
                print_value(v.array.elements[i]);
                if (i < v.array.count - 1) printf(", ");
            }
            printf("]");
            break;
        case VAL_FUNCTION: printf("<fungsi>"); break;
        case VAL_NATIVE: printf("<native %s>", v.native.name); break;
    }
}

// -------------------------------------------------------------------
// Evaluator
// -------------------------------------------------------------------
Value eval(ASTNode* node, Environment* env, bool* returned) {
    if (!node) return value_null();
    // NEW: If a return has already occurred in an outer scope, just propagate
    if (returned && *returned) return value_null();
    
    switch (node->type) {
        case AST_NUMBER: return value_number(node->number);
        case AST_FLOAT: return value_float(node->float_num);
        case AST_STRING: return value_string(node->string);
        case AST_BOOLEAN: return value_boolean(node->boolean);
        case AST_NULL: return value_null();
        case AST_ARRAY: {
            Value arr = value_array();
            for (int i = 0; i < node->array.count; i++) {
                Value elem = eval(node->array.elements[i], env, returned);
                array_append(&arr, elem);
            }
            return arr;
        }
        case AST_IDENTIFIER: {
            return env_get(env, node->name);
        }
        case AST_BINARY: {
            Value left = eval(node->binary.left, env, returned);
            Value right = eval(node->binary.right, env, returned);
            Value result;
            // Handle number operations
            if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                int a = left.number, b = right.number;
                switch (node->binary.op) {
                    case TOKEN_PLUS: result = value_number(a + b); break;
                    case TOKEN_MINUS: result = value_number(a - b); break;
                    case TOKEN_BINTANG: result = value_number(a * b); break;
                    case TOKEN_GARING: result = value_number(a / b); break;
                    case TOKEN_PERSEN: result = value_number(a % b); break;
                    case TOKEN_LT: result = value_boolean(a < b); break;
                    case TOKEN_GT: result = value_boolean(a > b); break;
                    case TOKEN_LTE: result = value_boolean(a <= b); break;
                    case TOKEN_GTE: result = value_boolean(a >= b); break;
                    case TOKEN_EQ: result = value_boolean(a == b); break;
                    case TOKEN_NEQ: result = value_boolean(a != b); break;
                    case TOKEN_AND: result = value_boolean(a && b); break;
                    case TOKEN_OR: result = value_boolean(a || b); break;
                    default: result = value_null();
                }
            } else if (left.type == VAL_FLOAT || right.type == VAL_FLOAT ||
                       left.type == VAL_NUMBER || right.type == VAL_NUMBER) {
                double a = (left.type == VAL_NUMBER) ? left.number :
                           (left.type == VAL_FLOAT) ? left.float_num : 0;
                double b = (right.type == VAL_NUMBER) ? right.number :
                           (right.type == VAL_FLOAT) ? right.float_num : 0;
                switch (node->binary.op) {
                    case TOKEN_PLUS: result = value_float(a + b); break;
                    case TOKEN_MINUS: result = value_float(a - b); break;
                    case TOKEN_BINTANG: result = value_float(a * b); break;
                    case TOKEN_GARING: result = value_float(a / b); break;
                    case TOKEN_LT: result = value_boolean(a < b); break;
                    case TOKEN_GT: result = value_boolean(a > b); break;
                    case TOKEN_LTE: result = value_boolean(a <= b); break;
                    case TOKEN_GTE: result = value_boolean(a >= b); break;
                    case TOKEN_EQ: result = value_boolean(a == b); break;
                    case TOKEN_NEQ: result = value_boolean(a != b); break;
                    default: result = value_null();
                }
            } else {
                fprintf(stderr, "Runtime Error: Operasi binary tidak didukung di baris %d\n", node->line);
                exit(1);
            }
            value_free(left);
            value_free(right);
            return result;
        }
        case AST_UNARY: {
            Value operand = eval(node->unary.operand, env, returned);
            Value result;
            if (node->unary.op == TOKEN_MINUS) {
                if (operand.type == VAL_NUMBER) result = value_number(-operand.number);
                else if (operand.type == VAL_FLOAT) result = value_float(-operand.float_num);
                else {
                    fprintf(stderr, "Runtime Error: Unary minus pada non-angka di baris %d\n", node->line);
                    exit(1);
                }
            } else if (node->unary.op == TOKEN_NOT) {
                result = value_boolean(!is_truthy(operand));
            } else {
                result = value_null();
            }
            value_free(operand);
            return result;
        }
        case AST_ASSIGN: {
            Value val = eval(node->assign.value, env, returned);
            env_set(env, node->assign.name, val); // env mengambil alih kepemilikan
            return value_copy(val); // kembalikan salinan untuk ekspresi
        }
        case AST_CALL: {
            Value callee = env_get(env, node->call.name);
            Value* args = malloc(sizeof(Value) * node->call.arg_count);
            for (int i = 0; i < node->call.arg_count; i++) {
                args[i] = eval(node->call.args[i], env, returned);
            }
            Value result;
            if (callee.type == VAL_NATIVE) {
                result = callee.native.func(args, node->call.arg_count);
                for (int i = 0; i < node->call.arg_count; i++) {
                    value_free(args[i]);
                }
                free(args);
            } else if (callee.type == VAL_FUNCTION) {
                ASTNode* func_node = callee.function.func_node;
                Environment* closure = callee.function.closure;

                // Check argument count
                if (node->call.arg_count != func_node->function.param_count) {
                    fprintf(stderr, "Runtime Error: Jumlah argumen salah untuk fungsi '%s'. Diharapkan %d, didapat %d di baris %d\n",
                            node->call.name, func_node->function.param_count, node->call.arg_count, node->line);
                    exit(1);
                }

                // Create a new environment for the function call
                Environment* call_env = env_new(closure);

                // Bind parameters to arguments
                for (int i = 0; i < node->call.arg_count; i++) {
                    env_set(call_env, func_node->function.params[i], args[i]);
                }
                
                // Execute function body
                bool func_returned = false; // New flag for this function call
                result = eval(func_node->function.body, call_env, &func_returned);

                // Clean up arguments
                for (int i = 0; i < node->call.arg_count; i++) {
                    value_free(args[i]);
                }
                free(args);

                // Clean up call environment
                env_free(call_env);

            } else {
                fprintf(stderr, "Runtime Error: Mencoba memanggil non-fungsi di baris %d\n", node->line);
                exit(1);
            }
            value_free(callee);
            return result;
        }
        case AST_INDEX: {
            Value obj = eval(node->index.object, env, returned);
            Value idx = eval(node->index.index, env, returned);
            if (obj.type != VAL_ARRAY) {
                fprintf(stderr, "Runtime Error: Pengindeksan pada non-array di baris %d\n", node->line);
                exit(1);
            }
            if (idx.type != VAL_NUMBER) {
                fprintf(stderr, "Runtime Error: Indeks array harus angka di baris %d\n", node->line);
                exit(1);
            }
            int i = idx.number;
            if (i < 0 || i >= obj.array.count) {
                fprintf(stderr, "Runtime Error: Indeks array di luar batas di baris %d\n", node->line);
                exit(1);
            }
            Value elem = value_copy(obj.array.elements[i]);
            value_free(obj);
            value_free(idx);
            return elem;
        }
        case AST_BLOCK: {
            Value last = value_null();
            for (int i = 0; i < node->block.count; i++) {
                value_free(last);
                last = eval(node->block.statements[i], env, returned);
            }
            return last;
        }
        case AST_IF: {
            Value cond = eval(node->if_stmt.condition, env, returned);
            int truth = is_truthy(cond);
            value_free(cond);
            if (truth) {
                return eval(node->if_stmt.then_branch, env, returned);
            } else if (node->if_stmt.else_branch) {
                return eval(node->if_stmt.else_branch, env, returned);
            }
            return value_null();
        }
        case AST_WHILE: {
            Value result = value_null();
            while (1) {
                Value cond = eval(node->while_stmt.condition, env, returned);
                int truth = is_truthy(cond);
                value_free(cond);
                if (!truth) break;
                value_free(result);
                result = eval(node->while_stmt.body, env, returned);
            }
            return result;
        }
        case AST_FOR: {
            Value iterable = eval(node->for_stmt.iterable, env, returned);
            if (iterable.type != VAL_ARRAY) {
                fprintf(stderr, "Runtime Error: Perulangan for memerlukan array di baris %d\n", node->line);
                exit(1);
            }
            Value result = value_null();
            for (int i = 0; i < iterable.array.count; i++) {
                Value elem = value_copy(iterable.array.elements[i]);
                env_set(env, node->for_stmt.var_name, elem);
                value_free(result);
                result = eval(node->for_stmt.body, env, returned);
            }
            value_free(iterable);
            return result;
        }
        // NEW: Handle Function Definition
        case AST_FUNCTION: {
            Value func_val = value_function(node, env);
            env_set(env, node->function.name, func_val);
            return value_null();
        }
        // NEW: Handle Return Statement
        case AST_RETURN: {
            *returned = true;
            Value ret_val = value_null();
            if (node->return_stmt.value) {
                ret_val = eval(node->return_stmt.value, env, returned);
            }
            return ret_val;
        }
        // NEW: Handle Expression Statement
        case AST_EXPR_STMT: {
            return eval(node->expr_stmt.expr, env, returned);
        }
        default:
            fprintf(stderr, "Unknown AST node type in eval at line %d\n", node->line);
            return value_null();
    }
}