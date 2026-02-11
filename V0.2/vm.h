// vm.h
#ifndef VM_H
#define VM_H

#include "parser.h" // Kita butuh akses ke AST
#include <stdint.h> // Untuk tipe data seperti uint8_t

// Definisikan opcode (jenis instruksi)
typedef enum {
    OP_PUSH,      // Push angka ke stack
    OP_LOAD,      // Load nilai dari variabel ke stack
    OP_STORE,     // Store nilai dari stack ke variabel
    OP_ADD,       // Pop dua nilai, jumlahkan, push hasilnya
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_POP,       // Hapus nilai dari stack (biasanya untuk hasil assignment)
    OP_CALL,      // Panggil fungsi (belum diimplementasikan)
    OP_HALT       // Berhenti
} OpCode;

// Instruksi bytecode sederhana
typedef struct {
    OpCode op;
    union { // Operand bisa berupa angka atau nama variabel
        int value;      // Untuk OP_PUSH
        char name[32];  // Untuk OP_LOAD / OP_STORE
    } operand;
} BytecodeInstruction;

// Struktur VM
typedef struct {
    BytecodeInstruction *code; // Array dari instruksi bytecode
    int code_size;             // Jumlah instruksi
    int code_capacity;         // Kapasitas array (untuk realokasi)

    int stack[256];            // Stack sederhana untuk perhitungan
    int sp;                    // Stack pointer

    // Tabel variabel sederhana (nama -> nilai)
    struct {
        char name[32];
        int value;
    } variables[64];
    int var_count;
} VM;

// Fungsi-fungsi API VM
VM* vm_create(void);
void vm_destroy(VM* vm);

void vm_compile(VM* vm, ASTNode* ast); // <-- Fungsi utama: ubah AST jadi bytecode
void vm_run(VM* vm);                   // <-- Jalankan bytecode
void vm_print_bytecode(VM* vm);        // <-- Debug: cetak bytecode
void vm_print_variables(VM* vm);       // <-- Debug: cetak semua variabel

#endif // VM_H