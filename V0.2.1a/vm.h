#ifndef VM_H
#define VM_H

#include "parser.h"
#include <stdint.h>

// === REGISTER-BASED VM DESIGN ===
// Inspired by LuaJIT: Uses 256 registers (R0-R255)
// Instructions are 32-bit: [OP:8][A:8][B:8][C:8] or [OP:8][A:8][Bx:16]

#define MAX_REGISTERS 256
#define MAX_CONSTANTS 65536
#define MAX_FUNCTIONS 256

// Value types (tagged union for dynamic typing)
typedef enum {
    VAL_NIL,
    VAL_BOOL,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_FUNCTION,
    VAL_NATIVE
} ValueType;

typedef struct {
    ValueType type;
    union {
        int64_t i;
        double f;
        char* s;
        struct {
            int idx;        // Function index
            int num_upvals;
        } func;
        struct {
            int (*fn)(int argc, int64_t* argv);
        } native;
    };
} Value;

// Instruction format
typedef enum {
    // Load/Store
    OP_LOADK,       // R(A) = K(Bx)
    OP_LOADBOOL,    // R(A) = (bool)B
    OP_LOADNIL,     // R(A) = nil
    OP_MOVE,        // R(A) = R(B)
    
    // Arithmetic (3-register like LuaJIT)
    OP_ADD,         // R(A) = R(B) + R(C)
    OP_SUB,         // R(A) = R(B) - R(C)
    OP_MUL,         // R(A) = R(B) * R(C)
    OP_DIV,         // R(A) = R(B) / R(C)
    OP_MOD,         // R(A) = R(B) % R(C)
    OP_POW,         // R(A) = R(B) ^ R(C)
    OP_NEG,         // R(A) = -R(B)
    
    // Comparison
    OP_EQ,          // R(A) = R(B) == R(C)
    OP_LT,          // R(A) = R(B) < R(C)
    OP_LE,          // R(A) = R(B) <= R(C)
    OP_NE,          // R(A) = R(B) != R(C)
    
    // Logical
    OP_AND,         // R(A) = R(B) && R(C)
    OP_OR,          // R(A) = R(B) || R(C)
    OP_NOT,         // R(A) = !R(B)
    
    // Control flow
    OP_JMP,         // PC += sBx
    OP_JMP_IF,      // if R(A) goto PC + sBx
    OP_JMP_IF_NOT,  // if !R(A) goto PC + sBx
    
    // Function call
    OP_CALL,        // R(A) = call(R(A), args=R(A+1)..R(A+B-1))
    OP_RETURN,      // return R(A)
    
    // Variables (global)
    OP_GETGLOBAL,   // R(A) = G[K(Bx)]
    OP_SETGLOBAL,   // G[K(Bx)] = R(A)
    
    // Table operations (for arrays/dicts)
    OP_NEWTABLE,    // R(A) = new table
    OP_GETTABLE,    // R(A) = R(B)[R(C)]
    OP_SETTABLE,    // R(A)[R(B)] = R(C)
    
    // Misc
    OP_PRINT,       // print(R(A))
    OP_HALT         // stop execution
} OpCode;

// Instruction encoding
typedef uint32_t Instruction;

#define GET_OP(i)   ((i) >> 24)
#define GET_A(i)    (((i) >> 16) & 0xFF)
#define GET_B(i)    (((i) >> 8) & 0xFF)
#define GET_C(i)    ((i) & 0xFF)
#define GET_Bx(i)   ((i) & 0xFFFF)
#define GET_sBx(i)  ((int16_t)((i) & 0xFFFF))

#define MAKE_ABC(op, a, b, c)   (((op) << 24) | ((a) << 16) | ((b) << 8) | (c))
#define MAKE_ABx(op, a, bx)     (((op) << 24) | ((a) << 16) | (bx))

// Function prototype
typedef struct {
    char* name;
    int num_params;
    int num_locals;
    int max_stack;
    Instruction* code;
    int code_size;
    int code_capacity;
    Value* constants;
    int num_constants;
} FunctionProto;

// VM State
typedef struct {
    // Code
    FunctionProto* functions;
    int num_functions;
    int current_func;
    
    // Registers (like LuaJIT)
    Value registers[MAX_REGISTERS];
    int pc;                     // Program counter
    
    // Global variables
    struct {
        char* name;
        Value value;
    } globals[256];
    int num_globals;
    
    // Call stack
    struct {
        int func_idx;
        int pc;
        int base;
    } call_stack[64];
    int call_depth;
    
    // Memory management
    char* string_pool[1024];    // For GC
    int string_count;
} VM;

// API
VM* vm_create(void);
void vm_destroy(VM* vm);

// Compilation
void vm_compile(VM* vm, ASTNode* ast);
int vm_add_constant(VM* vm, Value val);
int vm_add_function(VM* vm, const char* name);

// Execution
void vm_run(VM* vm);
Value vm_execute(VM* vm, int func_idx);

// Debug
void vm_print_bytecode(VM* vm);
void vm_print_registers(VM* vm);
void vm_print_globals(VM* vm);

// Native functions
void vm_register_native(VM* vm, const char* name, int (*fn)(int, int64_t*));

#endif // VM_H
