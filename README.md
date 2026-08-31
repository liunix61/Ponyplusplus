# Pony++ 项目状态

**版本：** v0.0.1
**状态：** Phase 0 预研完成 → Phase 1 MVP 编译器开发中

## 项目结构

```
Ponyplusplus/
├── README.md                    # 项目说明
├── Makefile                     # 构建系统
├── include/                     # 头文件
│   └── ponypp/
│       ├── ponypp.h
│       ├── lexer.h
│       ├── parser.h
│       ├── ast.h
│       ├── types.h
│       ├── capabilities.h
│       ├── wasm.h
│       └── wit.h
├── src/                         # C 源码
│   ├── ponyppc.c
│   ├── lexer.c
│   ├── parser.c
│   ├── ast.c
│   ├── typecheck.c
│   ├── capability.c
│   ├── codegen.c
│   ├── wit.c
│   └── util.c
├── tests/                       # 测试
│   ├── test_runner.c
│   ├── test_lexer.c
│   └── test_util.c
├── benchmarks/                  # 性能基准
├── doc/                         # 文档
│   ├── 00-original-design.md
│   ├── 01-audit-report.md
│   └── 02-refactored-design.md
└── examples/                    # 示例
```

## 构建

```bash
make        # 编译所有目标
make clean  # 清理
make test   # 运行测试
```

## 依赖

- 零外部依赖（纯 C11）
- 编译器：GCC 9+ / Clang 10+
- 构建工具：GNU Make

## 当前状态

| 模块 | 状态 |
|------|------|
| 项目骨架 | ✅ 完成 |
| 词法分析器 | 🔄 进行中 |
| 语法分析器 | ⏳ 待开始 |
| 类型系统 | ⏳ 待开始 |
| 引用能力验证 | ⏳ 待开始 |
| Wasm 代码生成 | ⏳ 待开始 |
| WIT 接口生成 | ⏳ 待开始 |
| 测试框架 | 🔄 进行中 |