# 附录 I：练习题答案

> 本附录提供各章习题的参考答案。

---

## 第 1 章

**习题 1.1** 编写并运行 Hello World。→ 见 1.3 节。

**习题 1.2** 解释 `actor main` 和 `new create()` 的作用。
- `actor main`：程序入口 Actor
- `new create()`：构造函数，程序启动时自动调用

---

## 第 2 章

**习题 2.1**
```pony
actor main {
  new create() => {
    var age: U32 = 25
    var name: String = "Alice"
    print(name)
    print(age)
  }
}
```

**习题 2.2**
```pony
actor main {
  new create() => {
    var r: F64 = 5.0
    var area: F64 = 3.14159 * r * r
    print(area)  // 78.53975
  }
}
```

**习题 2.3** 问题：`U8` 范围是 0-255，300 溢出；`+` 不能拼接 String 和 U8。
```pony
var x: U32 = 300
var y: String = "count: " + x.string()
```

**习题 2.4**
```pony
actor main {
  new create() => {
    var total: U32 = 3661
    var hours: U32 = total / 3600
    var minutes: U32 = (total % 3600) / 60
    var seconds: U32 = total % 60
    print(hours)
    print(minutes)
    print(seconds)
  }
}
```

**习题 2.5** `var` 可变，`val` 不可变值，`let` 绑定不可变。

---

## 第 3 章

**习题 3.1**
```pony
actor main {
  new create() => {
    var x: U32 = 7
    if x % 2 == 0 {
      print("even")
    } else {
      print("odd")
    }
  }
}
```

**习题 3.2**
```pony
actor main {
  new create() => {
    var result: U64 = 1
    var i: U32 = 1
    while i <= 10 {
      result = result * i.u64()
      i += 1
    }
    print(result)  // 3628800
  }
}
```

**习题 3.3**
```pony
fun calculate(a: F64, b: F64, op: String): F64 => {
  match op {
    "+" => return a + b
    "-" => return a - b
    "*" => return a * b
    "/" => return a / b
    _   => return 0.0
  }
}

actor main {
  new create() => {
    print(calculate(10.0, 3.0, "+"))
    print(calculate(10.0, 3.0, "-"))
  }
}
```

**习题 3.4** 缺少通配符 `_`，非穷尽匹配。
```pony
match x {
  0 => print("zero")
  5 => print("five")
  _ => print("other")  // 添加通配符
}
```

**习题 3.5**
```pony
actor main {
  new create() => {
    var n: U32 = 2
    while n <= 20 {
      var is_prime: Bool = true
      var d: U32 = 2
      while d * d <= n {
        if n % d == 0 {
          is_prime = false
          break
        }
        d += 1
      }
      if is_prime {
        print(n)
      }
      n += 1
    }
  }
}
```

---

## 第 4 章

**习题 4.1**
```pony
fun sum_list(numbers: List[U32]): U32 => {
  var total: U32 = 0
  for n in numbers {
    total += n
  }
  return total
}
```

**习题 4.2**
```pony
fun is_prime(n: U32): Bool => {
  if n < 2 {
    return false
  }
  var d: U32 = 2
  while d * d <= n {
    if n % d == 0 {
      return false
    }
    d += 1
  }
  return true
}
```

**习题 4.3**
```pony
actor Logger {
  var count: U32

  new create() => {
    count = 0
  }

  be log(msg: String) => {
    count += 1
    print("[" + count.string() + "] " + msg)
  }
}
```

**习题 4.4** `be` 是异步消息，调用者不等待结果，所以不能有返回值。
获取状态：通过另一个 `be` 方法发送结果，或用 `fun` 同步查询。

**习题 4.5**
```pony
fun power(base: U32, exp: U32): U64 => {
  if exp == 0 {
    return 1
  }
  return base.u64() * power(base, exp - 1)
}
```

---

## 第 5 章

**习题 5.1**
```pony
class Circle {
  var radius: F64

  new create(radius: F64) => {
    this.radius = radius
  }

  fun area(): F64 => {
    return 3.14159 * this.radius * this.radius
  }
}
```

**习题 5.2**
```pony
class BankAccount {
  var balance: U64

  new create(initial: U64) => {
    this.balance = initial
  }

  fun get_balance(): U64 => {
    return this.balance
  }

  be deposit(amount: U64) => {
    this.balance += amount
  }

  be withdraw(amount: U64) => {
    if this.balance >= amount {
      this.balance -= amount
    }
  }
}
```

**习题 5.3** 继承：`Vehicle` 基类 → `Car`、`Bicycle` 子类。→ 见 5.5 节。

**习题 5.4** 组合：`Library` 包含 `List[Book]`。→ 见 5.6 节。

**习题 5.5** 继承用于 "是一个" 关系，组合用于 "有一个" 关系。

---

## 第 6 章

**习题 6.1** Actor vs 线程：私有内存 vs 共享内存；无需锁 vs 需要锁；百万个 vs 数百个。

**习题 6.2** Actor 一次只处理一条消息，没有并发访问，所以不需要锁。

**习题 6.3** → 见 6.9 节聊天室示例。

**习题 6.4** 用 `List[T]` 存储任务，`be process()` 逐个处理。

**习题 6.5** `fun` 不能修改 actor 状态，应用 `be`。

---

## 第 7-20 章

第 7-20 章的习题答案请参考对应章节的"本章实战"部分，实战代码即为习题的完整参考实现。

---

## 答案使用建议

1. 先自己尝试完成习题
2. 遇到困难时参考提示
3. 完成后对照答案检查
4. 理解答案背后的原理，而不仅仅是抄写代码
