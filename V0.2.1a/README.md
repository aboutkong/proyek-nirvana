================================================================================
  🌀 NIRVANA LANG v0.2.1a - REGISTER-BASED VM (v.alpha)
================================================================================

"Pythonic Syntax, Lua's Speed, Nirvana's Peace"

Nirvana Lang adalah bahasa pemrograman scripting minimalis yang dibangun dengan 
C murni. Fokus utama proyek ini adalah menciptakan VM yang sangat efisien 
menggunakan arsitektur register-based, mirip dengan desain LuaJIT.

--------------------------------------------------------------------------------
🚀 FITUR UTAMA
--------------------------------------------------------------------------------

* [CORE] Register-Based VM (256 Virtual Registers R0-R255).
* [CORE] 32-bit Instruction Encoding (iABC & iABx formats).
* [LEX]  Hybrid Syntax: 
           - Mode Indentasi (Gaya Python menggunakan ':')
           - Mode Kurung Kurawal (Gaya C menggunakan '{ }')
* [TYPE] Dynamic Typing (Integer, Float, String, Bool, Nil).
* [MEM]  String Pooling & Tagged Union Value.

--------------------------------------------------------------------------------
📂 STRUKTUR PROYEK
--------------------------------------------------------------------------------
```txt
.
├── main.c          # Entry point, REPL, dan CLI Manager.
├── lexer.c/h       # Tokenizer dengan dukungan Indentation Stack.
├── parser.c/h      # Recursive Descent Parser -> AST.
├── vm.c/h          # Jantung Nirvana (Register execution, Value tagging).
└── Makefile        # Script build otomatis.
```
--------------------------------------------------------------------------------
⚙️ SPESIFIKASI TEKNIS (VM INTERNALS)
--------------------------------------------------------------------------------

Nirvana menggunakan instruksi tetap 32-bit untuk efisiensi CPU Cache:

1. Format iABC: [Opcode:8] [A:8] [B:8] [C:8]
   Digunakan untuk operasi register standar: R(A) = R(B) op R(C)

2. Format iABx: [Opcode:8] [A:8] [Bx:16]
   Digunakan untuk loading konstanta atau lompatan (Jump): R(A) = K(Bx)

Register Map:
- R0 - R255: Tersedia untuk alokasi lokal dan ekspresi sementara.
- PC (Program Counter): Menunjuk ke instruksi aktif.

--------------------------------------------------------------------------------
📝 CONTOH SYNTAX
--------------------------------------------------------------------------------

# 1. Gaya Nirvana (Python-like)
```bash
jika A == 10:
    cetak("A adalah sepuluh")
lain:
    cetak("Bukan sepuluh")
```bash
# 2. Gaya Klasik (C-like)
```bash
fungsi hitung(x, y) maka {
    kembalikan x * y
}
```

--------------------------------------------------------------------------------
🛠️ CARA KOMPILASI
--------------------------------------------------------------------------------

Gunakan GCC atau Clang untuk mengompilasi seluruh source code:
```
$ gcc -o nirvana main.c lexer.c parser.c vm.c -lm
```
Untuk menjalankan file skrip:
```
$ ./nirvana my_code.nv
```
Untuk menjalankan mode debug (Bytecode dump):
```
$ ./nirvana -d my_code.nv
```

--------------------------------------------------------------------------------
⚠️ STATUS PENGEMBANGAN (ROADMAP)
--------------------------------------------------------------------------------
[✔] Register-based Execution
[✔] Hybrid Lexing (Indent/Brace)
[ ] Mark-and-Sweep Garbage Collector (Upcoming)
[ ] Hash-Table for Global Variables (Upcoming)
[ ] First-class Tables/Dictionaries (Upcoming)

================================================================================
Copyright (c) 2026 - Nirvana Lang Team
"Elevate your code to the state of Nirvana."
================================================================================
