# 附录 H：编译器架构概览

> 本附录介绍 Pony++ 编译器（ponyppc）的内部架构。

---

## H.1 编译流程

```
源文件 (.pny)
    ↓ [1/6] 词法分析 (lexer.c)
Token 流
    ↓ [2/6] 语法分析 (parser.c)
AST (抽象语法树)
    ↓ [3/6] 类型检查 (typecheck.c)
带类型标注的 AST
    ↓ [4/6] 引用能力验证 (capabilities.c)
验证通过的 AST
    ↓ [5/6] 代码生成 (codegen.c)
目标代码 (Wasm / C)
    ↓ [6/6] 清理
输出文件
```

---

## H.2 核心模块

| 模块 | 文件 | 职责 |
|------|------|------|
| 词法分析器 | `src/lexer.c` | 源码 → Token |
| 语法分析器 | `src/parser.c` | Token → AST |
| 类型检查器 | `src/typecheck.c` | 类型验证 |
| 能力检查器 | `src/capabilities.c` | 引用能力验证 |
| 代码生成器 | `src/codegen.c` | AST → 目标代码 |
| 工具链 | `src/ponypp/tool.c` | 编译器命令行 |
| 包管理 | `src/ponypp/pkg.c` | 依赖管理 |
| 运行时 | `src/ponypp/runtime.c` | Actor 运行时 |

---

## H.3 AST 节点类型

| 节点类型 | 说明 |
|----------|------|
| `NODE_PROGRAM` | 程序根节点 |
| `NODE_ACTOR` | Actor 声明 |
| `NODE_CLASS` | Class 声明 |
| `NODE_FUN` | fun 方法 |
| `NODE_BE` | be 方法 |
| `NODE_NEW` | 构造函数 |
| `NODE_VAR` | 变量声明 |
| `NODE_LET` | let 声明 |
| `NODE_IF` | if 语句 |
| `NODE_WHILE` | while 循环 |
| `NODE_FOR` | for 循环 |
| `NODE_MATCH` | match 表达式 |
| `NODE_RETURN` | return 语句 |
| `NODE_CALL` | 函数调用 |
| `NODE_IMPORT` | import 声明 |
| `NODE_CAP` | 能力修饰符 |
| `NODE_IDENT` | 标识符 |
| `NODE_INT` | 整数字面量 |
| `NODE_STRING` | 字符串字面量 |
| `NODE_BOOL` | 布尔字面量 |

---

## H.4 编译器命令行选项

| 选项 | 说明 |
|------|------|
| `-o <file>` | 输出文件名 |
| `--target <target>` | 目标后端（wasi-p2/native/mcu） |
| `--mcu <mcu>` | MCU 型号（stm32f4/esp32） |
| `--ast` | 输出 AST |
| `--ast-dot` | 输出 DOT 格式 AST |
| `--pretty` | 美化输出 |
| `--bootstrap` | 编译 bootstrap 模块 |
| `--wit-only` | 仅生成 WIT |
| `--version` | 显示版本 |
| `-h, --help` | 显示帮助 |

---

## H.5 类型系统实现

### H.5.1 内置类型识别

```c
// typecheck.c
static const char *builtin_types[] = {
    "U8", "U16", "U32", "U64",
    "I8", "I16", "I32", "I64",
    "F32", "F64",
    "String", "Bool",
    NULL
};
```

### H.5.2 类型推断

编译器在 `var x = 42` 时自动推断 `x` 为 `I32`：

1. 检查字面量类型（整数 → I32，浮点 → F64）
2. 检查赋值目标类型
3. 验证类型兼容性

### H.5.3 Import 处理

`NODE_IMPORT` 处理在 `typecheck_program()` 中：

```c
case NODE_IMPORT:
    // 解析 import 节点
    // 将 std 类型添加到 actor_types 数组
    // Channel/Future/Mutex/ActorGroup 等
```

---

## H.6 代码生成

### H.6.1 Native 目标

生成 C 代码，然后由 gcc 编译：

```pony
var x: I32 = 42
```

生成：

```c
signed int x = 42;
```

### H.6.2 Wasm 目标

直接生成 Wasm 字节码。

### H.6.3 类型映射

| Pony++ 类型 | C 类型 |
|-------------|--------|
| `U8` | `unsigned char` |
| `U16` | `unsigned short` |
| `U32` | `unsigned int` |
| `U64` | `unsigned long long` |
| `I8` | `signed char` |
| `I16` | `short` |
| `I32` | `signed int` |
| `I64` | `signed long long` |
| `F32` | `float` |
| `F64` | `double` |
| `Bool` | `int` (0/1) |
| `String` | `const char*` |

---

## H.7 已知限制

| 限制 | 说明 | 状态 |
|------|------|------|
| `.to_string()` | 编译器挂起 | 规避中 |
| `for` 循环 | 仅支持 for-in | 已实现 |
| 泛型 | 有限支持 | 开发中 |
| 异常处理 | try/catch 语法存在 | 开发中 |
| trait | 语法存在 | 开发中 |

---

## H.8 测试架构

| 测试套件 | 文件 | 覆盖范围 |
|----------|------|----------|
| gtest_e2e | `tests/gtest_e2e.cpp` | 端到端编译 |
| gtest_runtime | `tests/gtest_runtime.cpp` | 运行时 + bootstrap |
| gtest_stdlib | `tests/gtest_stdlib*.cpp` | 标准库 |
| gtest_util | `tests/gtest_util.cpp` | 工具函数 |
| gtest_wamr | `tests/gtest_wamr.cpp` | WAMR 集成 |
| gtest_gc | `tests/gtest_gc.cpp` | 垃圾回收 |
