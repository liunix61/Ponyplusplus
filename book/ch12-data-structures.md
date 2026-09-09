# 第 12 章 数据结构：List 与 Map

> **本章目标**
> 掌握 `List[T]` 的创建、遍历、操作，了解 `Map[K, V]` 的基本用法。
>
> **学习时长**：35 分钟
> **前置要求**：第 2 章
> **对应 examples**：[`examples/01-syntax/05-lists.pny`](../examples/01-syntax/05-lists.pny)

---

## 12.1 List[T]：有序集合

### 12.1.1 创建与初始化

```pony
// list-create.pny - 创建 List

actor main {
  new create() => {
    // 字面量创建
    var numbers: List[U32] = [1, 2, 3, 4, 5]
    var names: List[String] = ["Alice", "Bob", "Carol"]
    var flags: List[Bool] = [true, false, true]

    print(numbers.len())   // 5
    print(names.len())     // 3
    print(flags.len())     // 3
  }
}
```

### 12.1.2 遍历

```pony
// list-iterate.pny - 遍历 List

actor main {
  new create() => {
    var names: List[String] = ["Alice", "Bob", "Carol"]

    for name in names {
      print(name)
    }
  }
}
```

预期输出：

```
Alice
Bob
Carol
```

### 12.1.3 嵌套 List

```pony
// list-nested.pny - 嵌套 List

actor main {
  new create() => {
    var matrix: List[List[U32]] = [[1, 2], [3, 4], [5, 6]]

    for row in matrix {
      for val in row {
        print(val)
      }
    }
  }
}
```

---

## 12.2 Map[K, V]：键值对

```pony
// map-basic.pny - Map 基本用法

actor main {
  new create() => {
    var ages: Map[String, U32] = Map[String, U32]()
    ages.set("Alice", 30)
    ages.set("Bob", 25)
    ages.set("Carol", 35)

    print(ages.get("Alice"))
    print(ages.size())
  }
}
```

---

## 12.3 本章实战：学生成绩管理

```pony
// grade-manager.pny - 学生成绩管理

actor main {
  new create() => {
    var grades: List[U32] = [85, 92, 78, 95, 88, 72, 90]

    // 统计
    var total: U32 = 0
    var count: U32 = 0
    var highest: U32 = 0

    for g in grades {
      total += g
      count += 1
      if g > highest {
        highest = g
      }
    }

    print("Count: " + count.string())
    print("Total: " + total.string())
    print("Highest: " + highest.string())
  }
}
```

---

## 12.4 习题

**习题 12.1** 用 `List[U32]` 实现冒泡排序。

**习题 12.2** 用 `Map[String, U32]` 统计一段文本中每个单词出现的次数。

**习题 12.3** 实现一个函数，返回 List 中所有偶数。

---

## 12.5 下一步

→ **第 13 章：IO、文件与日志**
