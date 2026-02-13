#ifndef VM_H
#define VM_H

#include "parser.h"
#include <stdint.h>

#define MAX_REGS 256
#define MAX_ARRAY_SIZE 1024

typedef enum {
    VAL_NIL,
    VAL_BOOL,
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_ARRAY,      // NEW: Array type
    VAL_RANGE       // NEW: Range type for for-loop
} ValueType;

// Forward declaration
struct Value;

typedef struct {
    struct Value* data;
    int size;
    int capacity;
} Array;

typedef struct {
    int start;
    int end;
    int step;
} Range;

typedef struct Value {
    ValueType type;
    union {
        int64_t i;
        double f;
        char* s;
        Array a;        // NEW
        Range r;        // NEW
    };
} Value;

typedef enum {
    OP_LOADK, OP_LOADBOOL, OP_LOADNIL, OP_MOVE,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_POW, OP_NEG,
    OP_EQ, OP_LT, OP_LE, OP_NE, OP_AND, OP_OR, OP_NOT,
    OP_JMP, OP_JMP_IF, OP_JMP_IF_NOT,
    OP_CALL, OP_RETURN,
    OP_GETGLOBAL, OP_SETGLOBAL,
    
    // NEW: Array operations
    OP_NEWARRAY,    // Create new array
    OP_GETELEM,     // Get element: R(A) = R(B)[R(C)]
    OP_SETELEM,     // Set element: R(A)[R(B)] = R(C)
    OP_APPEND,      // Append to array
    OP_RANGE,       // Create range object
    OP_LEN,         // Get length
    
    // NEW: For loop
    OP_FOR_PREP,    // Prepare for loop
    OP_FOR_LOOP,    // Loop iteration
    
    OP_PRINT, OP_HALT
} OpCode;

typedef uint32_t Instruction;
#define GET_OP(i) ((i) >> 24)
#define GET_A(i) (((i) >> 16) & 0xFF)
#define GET_B(i) (((i) >> 8) & 0xFF)
#define GET_C(i) ((i) & 0xFF)
#define GET_Bx(i) ((i) & 0xFFFF)
#define GET_sBx(i) ((int16_t)((i) & 0xFFFF))
#define MAKE_ABC(op,a,b,c) (((op)<<24)|((a)<<16)|((b)<<8)|(c))
#define MAKE_ABx(op,a,bx) (((op)<<24)|((a)<<16)|(bx))

typedef struct {
    Value* constants;
    int num_constants;
    Instruction* code;
    int code_size, code_capacity;
} Func;

typedef struct {
    Func func;
    Value regs[MAX_REGS];
    int pc;
    struct { char* name; Value val; } globals[256];
    int num_globals;
} VM;

VM* vm_create(void);
void vm_destroy(VM* vm);
void vm_compile(VM* vm, ASTNode* ast);
void vm_run(VM* vm);
void vm_print_bytecode(VM* vm);

// Helper functions
Value make_array(void);
void array_append(Array* arr, Value v);
Value array_get(Array* arr, int idx);
void array_set(Array* arr, int idx, Value v);

#endif
