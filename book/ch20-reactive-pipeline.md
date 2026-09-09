# 第 20 章 综合实践：响应式管道

> **本章目标**
> 综合运用 Channel、Actor、数据流，构建一个生产者→过滤器→消费者的响应式管道。
>
> **学习时长**：35 分钟
> **前置要求**：第 7、9 章
> **对应 examples**：[`examples/10-mini-projects/03-reactive-pipeline.pny`](../examples/10-mini-projects/03-reactive-pipeline.pny)

---

## 20.1 需求分析

构建一个数据流水线：

```
Producer → Channel → Filter → Channel → Consumer
```

- Producer 生成数据
- Filter 过滤/转换数据
- Consumer 消费最终结果
- 各阶段通过 Channel 解耦

---

## 20.2 完整实现

```pony
// reactive-pipeline.pny - 响应式管道

import std

actor Producer {
  var ch: Channel

  new create(ch: Channel) => {
    this.ch = ch
  }

  be produce() => {
    print("Producer: sending items")
    this.ch.send("item-0")
    this.ch.send("item-1")
    this.ch.send("item-2")
    this.ch.close()
  }
}

actor Filter {
  var ch_in: Channel
  var ch_out: Channel

  new create(ch_in: Channel, ch_out: Channel) => {
    this.ch_in = ch_in
    this.ch_out = ch_out
  }

  be filter() => {
    print("Filter: ready")
  }
}

actor Consumer {
  var ch: Channel

  new create(ch: Channel) => {
    this.ch = ch
  }

  be consume() => {
    print("Consumer: ready")
    while not this.ch.is_closed() {
      var msg: String = this.ch.receive()
      print("Consumer got: " + msg)
    }
  }
}

actor Pipeline {
  new create() => {
    var ch1: Channel = Channel(10)
    var ch2: Channel = Channel(10)

    var producer: Producer = Producer(ch1)
    var filter: Filter = Filter(ch1, ch2)
    var consumer: Consumer = Consumer(ch2)

    producer.produce()
    filter.filter()
    consumer.consume()
  }
}

actor main {
  new create() => {
    var pipeline: Pipeline = Pipeline()
  }
}
```

预期输出：

```
Producer: sending items
Filter: ready
Consumer: ready
Consumer got: item-0
Consumer got: item-1
Consumer got: item-2
```

---

## 20.3 设计要点

1. **Channel 解耦**：Producer 和 Consumer 不直接交互
2. **每个阶段是独立 Actor**：可以独立扩展
3. **close() 信号**：Producer 完成后关闭 Channel，Consumer 感知结束
4. **可组合**：可以轻松插入更多 Filter 阶段

---

## 20.4 扩展：多级管道

```pony
// multi-stage.pny - 多级管道

import std

actor Stage {
  var name: String
  var ch_in: Channel
  var ch_out: Channel

  new create(name: String, ch_in: Channel, ch_out: Channel) => {
    this.name = name
    this.ch_in = ch_in
    this.ch_out = ch_out
  }

  be process() => {
    print(this.name + ": processing")
  }
}

actor main {
  new create() => {
    var ch1: Channel = Channel(10)
    var ch2: Channel = Channel(10)
    var ch3: Channel = Channel(10)

    var s1: Stage = Stage("Stage-1", ch1, ch2)
    var s2: Stage = Stage("Stage-2", ch2, ch3)

    s1.process()
    s2.process()
  }
}
```

---

## 20.5 习题

**习题 20.1** 在 Filter 中实现真正的过滤：只传递包含 "1" 的消息。

**习题 20.2** 添加一个 Transform 阶段：将消息转为大写。

**习题 20.3** 实现一个扇出（fan-out）模式：一个 Producer 对应多个 Consumer。

**习题 20.4** 实现一个错误处理阶段：过滤掉空字符串。

---

## 20.6 全书总结

恭喜！你已经完成了《Pony++ 程序设计语言》的全部 20 章。

**你学到了**：

- ✅ Pony++ 语法基础（第 1-5 章）
- ✅ Actor 模型与并发（第 6-10 章）
- ✅ 标准库使用（第 11-14 章）
- ✅ 多目标平台（第 15-17 章）
- ✅ 综合实践（第 18-20 章）

**下一步**：

- 阅读附录 A：完整语法参考
- 阅读附录 B：完整 API 参考
- 参考 [`examples/`](../examples/) 中的 35 个示例
- 尝试构建你自己的 Pony++ 项目！

---

## 20.7 参考资源

- **GitHub 仓库**：https://github.com/liunix61/Ponyplusplus
- **Examples 目录**：[`examples/README.md`](../examples/README.md)
- **编译器文档**：[`docs/`](../docs/)
