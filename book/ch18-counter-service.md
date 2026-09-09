# 第 18 章 综合实践：计数器服务

> **本章目标**
> 综合运用 Actor、消息传递、状态管理，构建一个完整的计数器服务。
>
> **学习时长**：30 分钟
> **前置要求**：第 6-7 章
> **对应 examples**：[`examples/10-mini-projects/01-counter-service.pny`](../examples/10-mini-projects/01-counter-service.pny)

---

## 18.1 需求分析

构建一个计数器服务，支持：

- 多个独立计数器（hits、visits、errors）
- 每个计数器支持 increment / decrement / reset
- 线程安全（Actor 保证）
- 查询当前值

---

## 18.2 完整实现

```pony
// counter-service.pny - 计数器服务

actor Counter {
  var name: String
  var value: U32

  new create(name: String) => {
    this.name = name
    this.value = 0
  }

  fun get_value(): U32 => {
    return this.value
  }

  fun get_name(): String => {
    return this.name
  }

  be increment() => {
    this.value += 1
    print(this.name + ": " + this.value.string())
  }

  be decrement() => {
    if this.value > 0 {
      this.value -= 1
    }
    print(this.name + ": " + this.value.string())
  }

  be reset() => {
    this.value = 0
    print(this.name + ": 0")
  }
}

actor CounterService {
  new create() => {
    var hits: Counter = Counter("hits")
    var visits: Counter = Counter("visits")
    var errors: Counter = Counter("errors")

    hits.increment()
    hits.increment()
    hits.increment()
    visits.increment()
    errors.increment()

    print("hits: " + hits.get_value().string())
    print("visits: " + visits.get_value().string())
    print("errors: " + errors.get_value().string())
  }
}

actor main {
  new create() => {
    var service: CounterService = CounterService()
  }
}
```

预期输出：

```
hits: 1
hits: 2
hits: 3
visits: 1
errors: 1
hits: 3
visits: 1
errors: 1
```

编译运行：

```bash
ponyppc -o counter-service.ponypp counter-service.pny
wasmtime run counter-service.ponypp
```

---

## 18.3 设计要点

1. **每个计数器是独立 Actor**：天然线程安全
2. **CounterService 是编排者**：创建和管理多个 Counter
3. **be 方法修改状态**：fun 方法只读取
4. **消息顺序保证**：同一 Counter 的操作按顺序执行

---

## 18.4 习题

**习题 18.1** 添加一个 `be batch_increment(n: U32)` 方法，一次增加 n。

**习题 18.2** 添加一个 `be report()` 方法，输出所有计数器的名称和值。

**习题 18.3** 用 Channel 实现一个异步版本：计数器结果通过 Channel 返回。

---

## 18.5 下一步

→ **第 19 章：键值存储**
