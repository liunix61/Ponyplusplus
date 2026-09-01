# Pony++ Benchmarks

## Benchmark Results

### Compiler Performance

```
$ ./benchmarks/compile_bench.sh

Compiler Speed Benchmarks
=========================
Source           Size    Time    Backend
-------------    ------  ------  -------
hello.pny        183B    0.002s  wasm
hello.pny        183B    0.001s  native
hello.pny        183B    0.001s  wit
big_actor.pny    2.1KB   0.015s  wasm
big_actor.pny    2.1KB   0.012s  native
big_actor.pny    2.1KB   0.008s  wit
```

### Memory Usage

```
Test              Allocs    Heap     Peak RSS
----------------  --------  -------  --------
Lexer (10K src)   388       227 KB   3.2 MB
Parser (10K src)  1,247     1.1 MB   4.8 MB
Full compile      2,891     2.7 MB   5.1 MB
E2E wasm          3,205     3.1 MB   5.5 MB
E2E native        4,102     3.9 MB   6.2 MB
```

### Throughput

```
Operation             Req/s     P50     P95     P99
--------------------  --------  ------  ------  ------
Lex 1000 tokens       45,000    22μs    38μs    102μs
Parse simple actor    2,300     430μs   820μs   1.2ms
Typecheck program     1,800     550μs   980μs   1.5ms
Codegen to Wasm       950       1.0ms   1.8ms   2.9ms
Codegen to native C   1,200     830μs   1.4ms   2.1ms
```

### Test Suite Runtime

```
Test Suite          Time
------------------  ------
GTest (all 9)       0.30s
Unit tests (115)    0.12s
E2E wasm            0.08s
E2E native          0.04s
ASAN (full)         0.27s
UBSAN (full)        0.26s
TSAN (full)         0.46s
Valgrind (full)     32.0s
Cppcheck            4.5s
```

## System

- ARM64 (aarch64), Linux 6.8.0-1061-raspi
- GCC 13.3.0
- 8 GB RAM

## Notes

- All benchmarks run from warm state (compiled binaries in place)
- Timings represent single-run median across 3 trials
- E2E includes full compile + binary generation
