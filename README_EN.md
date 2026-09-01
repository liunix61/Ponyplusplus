# Pony++

A Pony-style actor language compiler implemented in pure C11. Supports Wasm and native backend codegen.

## Features

- **Lexer**: Full keyword/identifier/operator tokenizer with escape handling
- **Parser**: Recursive descent parser producing AST with actor/method/field/supervise constructs
- **Type System**: U64/I64/F64/String/Bool/None/Any with generics support
- **Type Checker**: Semantic analysis with error collection
- **Capability System**: iso/trn/ref/val/box/tag capability verification
- **Wasm Codegen**: Generates valid `.wasm` binary modules
- **WIT Generation**: Generates WebAssembly Interface Type descriptions
- **Native Backend**: Generates standalone C code linked with Pony++ runtime
- **Dual Build**: CMake + Makefile both fully supported
- **Comprehensive Testing**: 9 GTest suites with ASAN/UBSAN/TSAN/Valgrind/cppcheck all passing

## Quick Start

```bash
# Build with Makefile
make all

# Run tests
make test

# Build with CMake
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make

# Run GTest
ctest --output-on-failure

# Compile a Pony++ file
./build-cmake/ponyppc hello.pny -o hello.wasm
./build-cmake/ponyppc hello.pny --target native -o hello_native
```

## Examples

```pony
actor Main {
  var count: U64 = 0
  new create() => {}
  be run() => {
    print("hello from Pony++")
  }
}
```

```pony
actor Counter(val initial: U64 = 0) {
  var count: U64 = initial
  new create(initial: U64) => {}
  fun value(): U64 => { return count }
  be increment() => { count = count + 1 }
}

supervise Counter one_for_one
```

## Build Systems

### Makefile
```bash
make all          # Build ponyppc
make test         # Run all unit tests
make clean        # Clean build artifacts
make run          # Compile hello.pny to wasm
```

### CMake
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug          # Normal
cmake .. -DUSE_ASAN=ON                     # Address sanitizer
cmake .. -DUSE_UBSAN=ON                    # Undefined behavior sanitizer
cmake .. -DUSE_TSAN=ON                     # Thread sanitizer
cmake .. -DCOVERAGE=ON                     # Code coverage
```

## Quality Gates

| Check          | Status |
|----------------|--------|
| GTest (9 suites) | ✓ 9/9 pass |
| ASAN (memory leaks) | ✓ 0 leaks |
| UBSAN (undefined behavior) | ✓ 0 errors |
| TSAN (data races) | ✓ 0 races |
| Valgrind (memory errors) | ✓ 9/9 0 errors |
| cppcheck (static analysis) | ✓ 0 errors 0 warnings |
| Code coverage | 55.8% (target: 80%+) |

## Project Structure

```
Ponyplusplus/
├── CMakeLists.txt          # CMake build
├── Makefile                # Makefile build
├── README.md               # 中文文档
├── README_EN.md            # English documentation
├── src/
│   ├── ast.c               # AST node management
│   ├── capabilities.c      # Capability system
│   ├── codegen.c           # C/Wasm codegen
│   ├── lexer.c             # Lexer
│   ├── parser.c            # Parser
│   ├── typecheck.c         # Type checker
│   ├── types.c             # Type system
│   ├── util.c              # Utilities
│   ├── wasm.c              # Wasm binary writer
│   ├── wit.c               # WIT generator
│   └── ponypp/
│       └── runtime.c       # Native backend runtime
├── include/ponypp/
│   └── *.h                 # Public headers
├── tests/
│   ├── gtest_*.cpp         # GTest test suites
│   └── test_*.c            # Legacy C tests
├── benchmarks/
├── examples/
│   ├── hello.pny
│   └── hello_native.pny
└── doc/
    ├── 00-original-design.md
    ├── 01-audit-report.md
    ├── 02-refactored-design.md
    └── 03-multitarget-bootstrap-design.md
```

## Roadmap

- **Phase 1** (done): Lexer, parser, type system, capabilities, Wasm/native codegen, tests
- **Phase 2**: Full type safety, standard library, error handling (try/catch, recovery)
- **Phase 3**: Generics monomorphization, WIT 1.0, Wasm component model
- **Phase 4**: Native runtime scheduler, actor supervision trees
- **Phase 5**: Wasm component linking, multi-target deployment

## License

MIT
