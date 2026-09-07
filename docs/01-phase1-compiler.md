# Phase 1: 编译器与 Native Backend

## 1.1 编译管线

```
Lexer → Parser → Typecheck → Capability → Codegen → Link → Output
```

7 层编译管线，每层独立可测。

## 1.2 Lexer

- 词法分析：C11 实现
- 支持：actor/val/ref/trn/box/tag/be/fun/new/this/if/for/while/return/match
- 字面量：字符串（双引号+单引号）、数字、字符、布尔、None
- 运算符：11 个（赋值、比较、算术、逻辑、位运算）
- 注释：`//` 单行

## 1.3 Parser

- 递归下降解析器
- AST 节点类型：NODE_ACTOR / NODE_VAR / NODE_LET / NODE_VAL / NODE_FUN / NODE_BE / NODE_NEW / NODE_BLOCK / NODE_IF / NODE_FOR / NODE_WHILE / NODE_RETURN / NODE_MATCH / NODE_CALL / NODE_IDENT / NODE_EMPTY
- 方法解析：参数 + 返回类型 + 方法体

## 1.4 Typecheck

- 类型检查器（Phase 1 简化版）
- 检查：已知标识符（字段/类型/内置）
- 支持 `this.field` 前缀剥离

## 1.5 Capability

- 引用能力验证
- val/ref/trn/box/tag 语义检查

## 1.6 Codegen

- Native backend：生成 C11 代码
- 类型映射：I64→long long, U64→unsigned long long, String→const char *, Bool→int, Actor→void *
- 泛型映射：List[T]→PnyList *, Set[T]→PnySet *, Map[K,V]→PnyMap *

## 1.7 标准库

| 文件 | 内容 |
|------|------|
| list.pny | List 容器 |
| json.pny | JSON 序列化 |
| concurrent.pny | 并发原语 |
| actor.pny | Actor 模型 |
| time.pny | 时间 |
| log.pny | 日志 |
| io.pny | I/O |
| math.pny | 数学 |
| string.pny | 字符串 |

## 1.8 Bootstrap

自举流程：
1. 编译 compiler/lexer.pny
2. 自编译二进制运行成功
3. 检查 9 个标准库全部通过
