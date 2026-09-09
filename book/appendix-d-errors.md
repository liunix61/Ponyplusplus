# 附录 D：常见错误与调试

> 本附录收录 Pony++ 编译和运行时的常见错误，以及调试技巧。

---

## D.1 编译错误

### D.1.1 类型不匹配

**错误信息**：`type mismatch`

```pony
var x: U32 = 42
var y: I32 = x    // ✗ U32 不能赋给 I32
```

**修复**：显式转换 `var y: I32 = x.i32()`

### D.1.2 未定义标识符

**错误信息**：`undefined identifier`

```pony
print(unknown_var)  // ✗ unknown_var 未定义
```

**修复**：先声明变量。

### D.1.3 条件不是 Bool

**错误信息**：`condition must be Bool`

```pony
var x: U32 = 42
if x { ... }  // ✗ U32 不是 Bool
```

**修复**：`if x != 0 { ... }`

### D.1.4 be 返回值

**错误信息**：`behavior cannot have return type`

```pony
be compute(): U32 => { ... }  // ✗ be 不能有返回值
```

**修复**：用 `fun` 或去掉返回类型。

### D.1.5 fun 修改状态

**错误信息**：`pure function cannot modify state`

```pony
fun increment() => {
  count += 1  // ✗ fun 不能修改 actor 状态
}
```

**修复**：用 `be` 代替。

### D.1.6 val 修改

**错误信息**：`cannot modify immutable value`

```pony
val x: U32 = 10
x = 20  // ✗ val 不可修改
```

**修复**：用 `var` 代替。

---

## D.2 编译器限制

### D.2.1 .to_string() 方法

**症状**：编译器挂起（无输出，不返回）

**原因**：编译器在处理 `.to_string()` 方法调用时挂起。

**规避**：避免使用 `.to_string()`，用 `+` 拼接或直接 `print`。

### D.2.2 String 方法名

**错误信息**：`method not found`

```pony
var s: String = "hello"
s.length()  // ✗ 没有 length() 方法
```

**修复**：用 `s.len()`。

---

## D.3 运行时错误

### D.3.1 死循环

**症状**：程序不退出

```pony
var i: U32 = 0
while i < 5 {
  print(i)
  // 忘记 i += 1
}
```

**修复**：循环体内更新计数器。

### D.3.2 除零

**症状**：运行时崩溃

```pony
var x: U32 = 0
var y: U32 = 10 / x  // 除零
```

**修复**：先检查 `if x != 0`。

---

## D.4 调试技巧

### D.4.1 用 print 调试

```pony
fun calculate(x: U32): U32 => {
  print("input: " + x.string())
  var result: U32 = x * 2
  print("result: " + result.string())
  return result
}
```

### D.4.2 查看 AST

```bash
ponyppc --ast file.pny
```

### D.4.3 查看生成的 C 代码

```bash
ponyppc --target native -o output file.pny
cat output.c
```

### D.4.4 查看 DOT 格式 AST

```bash
ponyppc --ast-dot file.pny
```

---

## D.5 错误信息速查表

| 错误信息 | 常见原因 | 修复 |
|----------|----------|------|
| `type mismatch` | 类型不匹配 | 显式转换 |
| `undefined identifier` | 变量未声明 | 先声明 |
| `condition must be Bool` | 条件非布尔 | 用比较运算 |
| `cannot modify immutable` | 修改 val | 用 var |
| `method not found` | 方法名错误 | 查 API 参考 |
| `expected =>` | 语法错误 | 检查函数体 |
| `missing return` | fun 缺少 return | 添加返回 |
