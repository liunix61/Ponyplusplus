# Pony++

Pony++ 是一门"天生云原生"的并发编程语言，融合 Pony 的引用能力系统 + Erlang 的监督树容错 + WebAssembly 的通用沙箱。

编译器 `ponyppc` 用纯 C11 实现，零外部依赖，编译产物支持 Wasm 组件和 Native 可执行文件。

## 快速开始

```bash
make all          # 编译 ponyppc
make test         # 运行全部测试 (115 cases)
make clean        # 清理构建产物

./bin/ponyppc examples/hello.pny -o hello.wasm           # 编译为 Wasm
./bin/ponyppc examples/hello.pny --wit-only -o hello.wit  # 生成 WIT 接口
./bin/ponyppc examples/hello.pny --target native -o hello # 编译为 Native
```

## 项目结构

```
Ponyplusplus/
├── Makefile                     # 构建系统
├── README.md                    # 本文档
├── include/                     # 头文件
│   └── ponypp/
│       ├── ponypp.h             # 公共类型定义
│       ├── ast.h                # AST 节点
│       ├── lexer.h              # 词法分析
│       ├── parser.h             # 语法分析
│       ├── types.h              # 类型系统
│       ├── typecheck.h          # 类型检查
│       ├── capabilities.h       # 引用能力验证
│       ├── codegen.h            # Native C 代码生成
│       ├── wasm.h               # Wasm 模块生成
│       ├── wit.h                # WIT 接口生成
│       ├── runtime.h            # Actor 运行时
│       └── util.h               # 工具函数
├── src/                         # C 源码
│   ├── ponyppc.c                # 编译器驱动
│   ├── ast.c
│   ├── lexer.c
│   ├── parser.c
│   ├── types.c
│   ├── typecheck.c
│   ├── capabilities.c
│   ├── codegen.c
│   ├── wasm.c
│   ├── wit.c
│   ├── util.c
│   └── ponypp/
│       └── runtime.c            # 运行时实现
├── tests/                       # 单元测试 (8 文件, 115 cases)
│   ├── test_lexer.c
│   ├── test_parser.c
│   ├── test_types.c
│   ├── test_typecheck.c
│   ├── test_capabilities.c
│   ├── test_codegen.c
│   ├── test_wasm.c
│   ├── test_wit.c
│   └── test_e2e.c
├── examples/                    # 示例程序
│   └── hello.pny
├── doc/                         # 设计文档
│   ├── 00-original-design.md
│   ├── 01-audit-report.md
│   ├── 02-refactored-design.md
│   └── 03-multitarget-bootstrap-design.md
├── .github/workflows/ci.yml     # GitHub Actions CI
└── .gitignore
```

## 测试状态

| 模块 | 用例数 | 状态 |
|------|--------|------|
| 词法分析 | 39 | ✓ |
| 语法分析 | 22 | ✓ |
| 类型系统 | 11 | ✓ |
| 类型检查 | 9 | ✓ |
| 引用能力 | 4 | ✓ |
| Native 代码生成 | 7 | ✓ |
| Wasm 生成 | 7 | ✓ |
| WIT 生成 | 8 | ✓ |
| 端到端 | 8 | ✓ |
| **合计** | **115** | **100% 通过** |

## 构建

- 编译器: `CC ?= gcc` (支持 gcc / clang, 可通过环境变量覆盖)
- 标准: C11
- 依赖: 无 (纯 C，仅链接 libm)

## 编译目标

| 目标 | 命令 | 说明 |
|------|------|------|
| Wasm | `ponyppc file.pny` | 默认，生成 WASI Preview 2 模块 |
| Native | `ponyppc --target native file.pny` | 生成可执行文件 |
| WIT 接口 | `ponyppc --wit-only file.pny` | 生成 WIT 接口定义 |

## License

MIT
