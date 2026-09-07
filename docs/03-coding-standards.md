# Pony++ 编码规范

## 1. C 代码风格

- **标准**: C11
- **编译标志**: `-std=c11 -Wall -Wextra -Wpedantic`
- **命名**:
  - 函数：`snake_case`（如 `lexer_new`, `cg_type_of`）
  - 类型：`PascalCase` + `_t`（如 `Lexer`, `Token`, `PnyList`）
  - 宏/常量：`UPPER_SNAKE_CASE`（如 `TK_STRING`, `NODE_ACTOR`）
  - 模块前缀：`pny_`（如 `pny_list_new`, `pny_runtime_init`）

## 2. Pony++ 语言风格

- **缩进**: 2 空格
- **方法**: `be`（行为）/ `fun`（函数）
- **字段**: `var`（可变）/ `val`（不可变）
- **类型**: 大写字母开头（如 `Actor`, `String`, `Bool`）
- **泛型**: `List[T]`, `Map[K, V]`

## 3. 注释规范

- **C 代码**: `/* ... */` 块注释 + `//` 行注释
- **Pony++ 代码**: `//` 行注释
- **文档注释**: `/** ... */`

## 4. 错误处理

- 函数返回 `-1` 表示错误
- 调用者检查返回值
- 不抛出异常（C 语言）

## 5. 内存管理

- `s_malloc` / `s_strdup` / `s_free`：项目级内存管理
- 对象创建：`xxx_new()`
- 对象释放：`xxx_free()`
- 配对释放：每个 new 必须有对应 free

## 6. 测试规范

- 每个模块对应 `tests/test_<module>.c`
- CHECK 宏：`CHECK(condition, "message")`
- 失败时输出 `✗`，通过时输出 `✓`

## 7. Git 提交规范

```
fix: 修复描述
feat: 新功能描述
docs: 文档更新
test: 测试更新
refactor: 重构
```

## 8. CI/CD

- GitHub Actions: gcc + clang 双编译器矩阵
- 构建 → 单元测试 → E2E 测试
- 触发：push + PR 到 main 分支
