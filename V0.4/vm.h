#ifndef VM_H
#define VM_H

#include "parser.h"
#include <stdbool.h>

typedef enum {
    VAL_NUMBER,
    VAL_FLOAT,
    VAL_STRING,
    VAL_BOOLEAN,
    VAL_NULL,
    VAL_ARRAY,
    VAL_FUNCTION,
    VAL_NATIVE
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        int number;
        double float_num;
        char* string;
        int boolean;
        struct {
            struct Value* elements;
            int count;
            int capacity;
        } array;
        struct {
            struct ASTNode* func_node;   // AST_FUNCTION node
            struct Environment* closure; // environment saat definisi
        } function;
        struct {
            const char* name;
            struct Value (*func)(struct Value* args, int arg_count);
        } native;
    };
} Value;

// Definisikan Binding terlebih dahulu
typedef struct Binding {
    char* name;
    Value value;
    struct Binding* next;
} Binding;

typedef struct Environment {
    struct Environment* parent;
    Binding* bindings;
} Environment;

Environment* env_new(Environment* parent);
void env_free(Environment* env);
void env_set(Environment* env, const char* name, Value value);
Value env_get(Environment* env, const char* name);

Value eval(ASTNode* node, Environment* env, bool* returned);
void print_value(Value v);
void value_free(Value v);

Value value_number(int n);
Value value_float(double f);
Value value_string(const char* s);
Value value_boolean(int b);
Value value_null(void);
Value value_array(void);
void array_append(Value* arr, Value v);
Value value_function(ASTNode* func_node, Environment* closure);
Value value_native(const char* name, Value (*func)(Value* args, int count));

// Built-in functions
Value native_print(Value* args, int count);
Value native_range(Value* args, int count);

#endif // VM_H