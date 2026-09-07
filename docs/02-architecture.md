# Pony++ 编译器架构

```
┌─────────────────────────────────────────────────────────┐
│                    ponyppc (Driver)                       │
├─────────────────────────────────────────────────────────┤
│  ┌─────────┐  ┌─────────┐  ┌──────────┐  ┌──────────┐  │
│  │  Lexer  │→ │  Parser │→ │Typecheck │→ │Capability│  │
│  └─────────┘  └─────────┘  └──────────┘  └──────────┘  │
│       ↓                                                   │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────────┐   │
│  │ Codegen  │→ │ Native   │→ │ Wasm / WIT / REpl    │   │
│  │ (C11)    │  │ Backend  │  │ Backends             │   │
│  └──────────┘  └──────────┘  └──────────────────────┘   │
│       ↓                                                   │
│  ┌──────────────────────────────────────────────────┐    │
│  │  Runtime: List / Set / Map / GC / Distributed    │    │
│  └──────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
```

## 模块列表

| 模块 | 文件 | 职责 |
|------|------|------|
| Lexer | src/lexer.c | 词法分析 |
| Parser | src/parser.c | 语法分析 |
| Typecheck | src/typecheck.c | 类型检查 |
| Capability | src/capabilities.c | 能力验证 |
| Codegen | src/codegen.c | 代码生成 |
| Runtime | src/ponypp/runtime.c | 运行时 |
| List | src/ponypp/stdlib.c | List/Set/Map |
| GC | src/ponypp/gc.c | 垃圾回收 |
| Distributed | src/ponypp/distributed.c | 分布式 |
| Wasm | src/wamr.c | WASM 后端 |
| WIT | src/wit.c | WIT 接口 |
| REpl | src/ponypp/repl.c | 交互模式 |
| Network | src/ponypp/network.c | 网络 |
| Profiler | src/ponypp/profiler.c | 性能分析 |
| FFI | src/ponypp/ffi.c | 外部函数 |
| Pkg | src/ponypp/pkg.c | 包管理 |
