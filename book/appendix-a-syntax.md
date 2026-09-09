# 附录 A：完整语法参考

> 本附录基于 Pony++ v0.1.0 编译器实际支持的语法。
> 所有条目均经过 `ponyppc` 编译验证。

---

## A.1 关键字（Keywords）

| 关键字 | 类别 | 说明 |
|--------|------|------|
| `actor` | 声明 | 定义 Actor |
| `class` | 声明 | 定义类 |
| `trait` | 声明 | 定义特征 |
| `be` | 方法 | 行为方法（异步） |
| `fun` | 方法 | 纯函数（同步） |
| `new` | 构造 | 构造函数 |
| `var` | 变量 | 可变变量声明 |
| `let` | 变量 | 绑定变量声明 |
| `if` / `else` | 控制流 | 条件分支 |
| `while` | 控制流 | 条件循环 |
| `for` | 控制流 | 遍历循环 |
| `match` | 控制流 | 模式匹配 |
| `return` | 控制流 | 函数返回 |
| `import` | 模块 | 导入模块 |
| `use` | 模块 | 引用模块 |
| `as` | 模块 | 别名 |
| `and` / `or` / `not` | 逻辑 | 逻辑运算 |
| `is` | 比较 | 身份比较 |
| `in` | 遍历 | for-in 遍历 |
| `where` | 约束 | 类型约束 |
| `then` | 控制流 | then 子句 |
| `try` / `catch` / `finally` | 异常 | 异常处理 |
| `throw` | 异常 | 抛出异常 |
| `supervise` | 监督 | 监督声明 |
| `supertree` | 监督 | 监督树 |
| `true` / `false` | 字面量 | 布尔字面量 |

---

## A.2 内置类型（Built-in Types）

### A.2.1 整数类型

| 类型 | 位宽 | 范围 |
|------|------|------|
| `U8` | 8 | 0 ~ 255 |
| `U16` | 16 | 0 ~ 65,535 |
| `U32` | 32 | 0 ~ 4,294,967,295 |
| `U64` | 64 | 0 ~ 18,446,744,073,709,551,615 |
| `I8` | 8 | -128 ~ 127 |
| `I16` | 16 | -32,768 ~ 32,767 |
| `I32` | 32 | -2,147,483,648 ~ 2,147,483,647 |
| `I64` | 64 | -9.2×10¹⁸ ~ 9.2×10¹⁸ |

### A.2.2 浮点类型

| 类型 | 位宽 | 精度 |
|------|------|------|
| `F32` | 32 | ~7 位有效数字 |
| `F64` | 64 | ~15 位有效数字 |

### A.2.3 其他类型

| 类型 | 说明 |
|------|------|
| `Bool` | 布尔值（`true` / `false`） |
| `String` | UTF-8 字符串 |
| `None` | 空值（void 等价） |
| `Any` | 任意类型 |
| `List[T]` | 泛型列表 |
| `Set[T]` | 泛型集合 |
| `Array[T]` | 泛型数组 |
| `Option[T]` | 可选值 |
| `Reply[T]` | 回复类型 |

---

## A.3 能力修饰符（Reference Capabilities）

| 能力 | 说明 |
|------|------|
| `iso` | 独占引用（唯一所有者） |
| `trn` | 可转移引用 |
| `ref` | 可变引用 |
| `val` | 不可变引用 |
| `box` | 共享只读引用 |
| `tag` | 身份标识引用 |
| `this` | 当前对象引用 |

---

## A.4 运算符

### A.4.1 算术运算符

| 运算符 | 说明 | 示例 |
|--------|------|------|
| `+` | 加法 | `a + b` |
| `-` | 减法 | `a - b` |
| `*` | 乘法 | `a * b` |
| `/` | 除法 | `a / b` |
| `%` | 取模 | `a % b` |
| `+=` | 加并赋值 | `a += 1` |
| `-=` | 减并赋值 | `a -= 1` |
| `*=` | 乘并赋值 | `a *= 2` |
| `/=` | 除并赋值 | `a /= 2` |

### A.4.2 比较运算符

| 运算符 | 说明 |
|--------|------|
| `==` | 等于 |
| `!=` | 不等于 |
| `<` | 小于 |
| `>` | 大于 |
| `<=` | 小于等于 |
| `>=` | 大于等于 |

### A.4.3 逻辑运算符

| 运算符 | 说明 | 短路 |
|--------|------|------|
| `and` | 逻辑与 | 是 |
| `or` | 逻辑或 | 是 |
| `not` | 逻辑非 | — |

### A.4.4 运算符优先级（从高到低）

1. `not`
2. `*` `/` `%`
3. `+` `-`
4. `<` `>` `<=` `>=`
5. `==` `!=`
6. `and`
7. `or`

---

## A.5 声明语法

### A.5.1 Actor 声明

```pony
actor ActorName {
  var field: Type

  new create(args) => {
    // 构造函数
  }

  fun method(): ReturnType => {
    return value
  }

  be behavior() => {
    // 异步行为
  }
}
```

### A.5.2 Class 声明

```pony
class ClassName {
  var field: Type
  val const_field: Type = default

  new create(args) => {
    // 构造函数
  }

  fun method(): ReturnType => {
    return value
  }
}
```

### A.5.3 继承

```pony
class Child extends Parent {
  new create(args) => {
    super(parent_args)
  }
}
```

### A.5.4 Trait 声明

```pony
trait TraitName {
  fun required_method(): ReturnType
}
```

### A.5.5 顶层函数

```pony
fun function_name(params: Type): ReturnType => {
  return value
}
```

### A.5.6 变量声明

```pony
var x: Type = value     // 可变变量
val y: Type = value     // 不可变值
let z: Type = value     // 绑定变量
var w = value           // 类型推断
```

---

## A.6 控制流语法

### A.6.1 if/else

```pony
if condition {
  // true 分支
} else if other_condition {
  // else if 分支
} else {
  // else 分支
}
```

### A.6.2 while

```pony
while condition {
  // 循环体
}
```

### A.6.3 for-in

```pony
for item in collection {
  // 遍历体
}
```

### A.6.4 match

```pony
match expression {
  pattern1 => statement1
  pattern2 => statement2
  _        => default_statement
}
```

### A.6.5 return

```pony
fun example(): U32 => {
  if condition {
    return 1
  }
  return 0
}
```

---

## A.7 模块导入

```pony
import std.concurrent    // 导入并发模块
import std               // 导入标准库
use module_name          // 引用模块
import module as alias   // 别名导入
```

---

## A.8 类型转换方法

| 方法 | 说明 |
|------|------|
| `.u8()` `.u16()` `.u32()` `.u64()` | 转为无符号整数 |
| `.i8()` `.i16()` `.i32()` `.i64()` | 转为有符号整数 |
| `.f32()` `.f64()` | 转为浮点数 |
| `.string()` | 转为字符串 |

---

## A.9 注释语法

```pony
// 卌行注释

/* 块注释
   多行
*/
```

---

## A.10 字面量

| 类型 | 示例 |
|------|------|
| 整数 | `42`, `0`, `255` |
| 浮点 | `3.14`, `2.71828` |
| 布尔 | `true`, `false` |
| 字符串 | `"hello"`, `"Pony++"` |
| 列表 | `[1, 2, 3]`, `["a", "b"]` |
