# 第 19 章 综合实践：键值存储

> **本章目标**
> 构建一个线程安全的键值存储 Actor，理解状态管理、查找和更新模式。
>
> **学习时长**：30 分钟
> **前置要求**：第 6 章、第 12 章
> **对应 examples**：[`examples/10-mini-projects/02-key-value-store.pny`](../examples/10-mini-projects/02-key-value-store.pny)

---

## 19.1 需求分析

- 支持 `put(key, value)`、`get(key)`、`delete(key)` 操作
- 线程安全（Actor 保证）
- 支持计数查询

---

## 19.2 完整实现

```pony
// key-value-store.pny - 键值存储

actor KeyValueStore {
  var keys: List[String]
  var values: List[String]

  new create() => {
    keys = []
    values = []
    print("Store created")
  }

  be put(key: String, value: String) => {
    var found: Bool = false
    var i: U32 = 0
    while i < this.keys.len() {
      if this.keys[i] == key {
        this.values[i] = value
        found = true
        break
      }
      i += 1
    }
    if not found {
      this.keys.append(key)
      this.values.append(value)
    }
    print("PUT: " + key + " = " + value)
  }

  fun get(key: String): String => {
    var i: U32 = 0
    while i < this.keys.len() {
      if this.keys[i] == key {
        return this.values[i]
      }
      i += 1
    }
    return ""
  }

  be delete(key: String) => {
    print("DELETE: " + key)
  }

  fun count(): U32 => {
    return this.keys.len()
  }
}

actor main {
  new create() => {
    var store: KeyValueStore = KeyValueStore()
    store.put("name", "Pony++")
    store.put("version", "1.0")
    store.put("language", "Actor")
    print("count")
    print(store.count())
  }
}
```

预期输出：

```
Store created
PUT: name = Pony++
PUT: version = 1.0
PUT: language = Actor
count
3
```

---

## 19.3 设计要点

1. **两个平行 List**：keys 和 values 用相同索引对应
2. **put 是 be**：修改状态，异步执行
3. **get 是 fun**：只读查询，同步返回
4. **Actor 保证线程安全**：不需要锁

---

## 19.4 习题

**习题 19.1** 实现 `be delete(key)` 的完整逻辑（从两个 List 中移除）。

**习题 19.2** 添加 `fun exists(key): Bool` 方法。

**习题 19.3** 用 Channel 实现一个异步查询版本：get 结果通过 Channel 返回。

---

## 19.5 下一步

→ **第 20 章：响应式管道**
