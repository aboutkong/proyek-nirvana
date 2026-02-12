📁 nirvana-lang/
├── src/
│   ├── lexer.c/h
│   ├── parser.c/h
│   ├── vm.c/h
│   ├── compiler.c/h      # NEW: Optimizing compiler
│   └── gc.c/h            # NEW: Garbage collector
├── lib/
│   ├── stdio.niv         # Standard I/O
│   ├── math.niv          # Math functions
│   └── string.niv        # String manipulation
├── tools/
│   ├── nivana            # CLI compiler
│   ├── nivana-repl       # NEW: Interactive shell
│   └── nivana-fmt        # NEW: Code formatter
└── examples/
    ├── hello.niv
    
    ├── faktorial.niv
    └── sorting.niv