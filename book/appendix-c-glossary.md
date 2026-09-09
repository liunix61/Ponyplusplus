# 附录 C：术语表（中英对照）

> 全书术语统一表。术语首次出现时用 `**术语**（English）` 格式标注。

---

## C.1 语言基础

| 中文 | English | 说明 |
|------|---------|------|
| 静态类型 | Static Type | 编译时确定类型 |
| 类型推断 | Type Inference | 编译器自动推断类型 |
| 类型注解 | Type Annotation | 显式标注类型 |
| 字面量 | Literal | 源代码中的固定值 |
| 变量 | Variable | 可存储值的命名空间 |
| 常量 | Constant | 不可变的值 |
| 字段 | Field | 对象的数据成员 |
| 方法 | Method | 对象的行为成员 |
| 构造函数 | Constructor | 创建对象时调用的函数 |
| 继承 | Inheritance | 子类获取父类特性 |
| 组合 | Composition | 对象包含其他对象 |
| 覆写 | Override | 子类重新定义父类方法 |

---

## C.2 Actor 模型

| 中文 | English | 说明 |
|------|---------|------|
| Actor 模型 | Actor Model | 基于消息传递的并发模型 |
| 并发单元 | Concurrent Unit | Actor 的本质 |
| 邮箱 | Mailbox | Actor 的消息队列 |
| 消息传递 | Message Passing | Actor 间通信方式 |
| 异步 | Asynchronous | 不等待执行完成 |
| 同步 | Synchronous | 等待执行完成 |
| 行为方法 | Behavior | Actor 的 `be` 方法 |
| 纯函数 | Pure Function | 无副作用的函数 |
| 状态隔离 | State Isolation | Actor 状态私有 |
| 顺序保证 | Ordering Guarantee | 消息按发送顺序处理 |

---

## C.3 能力系统

| 中文 | English | 说明 |
|------|---------|------|
| 引用能力 | Reference Capability | 控制引用的可变性和共享性 |
| 独占引用 | Isolated Reference | `iso`，唯一所有者 |
| 可转移引用 | Transition Reference | `trn`，可修改可转移 |
| 可变引用 | Mutable Reference | `ref`，可修改 |
| 不可变引用 | Immutable Reference | `val`，不可修改 |
| 共享只读 | Shared Read-Only | `box`，多引用只读 |
| 身份标识 | Tag | `tag`，只知道身份 |
| 能力转换 | Capability Conversion | 能力间的自动转换 |

---

## C.4 并发原语

| 中文 | English | 说明 |
|------|---------|------|
| 通道 | Channel | Actor 间通信管道 |
| 未来 | Future | 异步计算的结果占位符 |
| 广播 | Broadcast | 向所有成员发送消息 |
| 生产者 | Producer | 生成数据的一方 |
| 消费者 | Consumer | 消费数据的一方 |
| 管道 | Pipeline | 多阶段数据处理 |

---

## C.5 容错

| 中文 | English | 说明 |
|------|---------|------|
| 监督树 | Supervision Tree | 层级容错结构 |
| 监督者 | Supervisor | 监控子 Actor 的 Actor |
| 一对一 | One-for-One | 只重启崩溃的 Actor |
| 一对全 | One-for-All | 一个崩溃全部重启 |
| 让它崩溃 | Let it crash | 不防御错误，让 Actor 崩溃 |
| 快速失败 | Fail fast | 尽早暴露错误 |
| 自愈 | Self-healing | 系统自动恢复 |

---

## C.6 编译与平台

| 中文 | English | 说明 |
|------|---------|------|
| 编译器 | Compiler | 源码到目标代码的翻译器 |
| 词法分析 | Lexical Analysis | 源码到 Token |
| 语法分析 | Syntax Analysis | Token 到 AST |
| 类型检查 | Type Check | 验证类型正确性 |
| 代码生成 | Code Generation | AST 到目标代码 |
| 目标后端 | Target Backend | 编译目标（wasm/native/mcu） |
| WebAssembly | Wasm | 浏览器可执行的字节码 |
| 组件模型 | Component Model | Wasm 的模块化标准 |
| 外部接口 | FFI | 调用其他语言的接口 |
| 微控制器 | MCU | 嵌入式处理器 |

---

## C.7 标准库

| 中文 | English | 说明 |
|------|---------|------|
| 标准库 | Standard Library | 语言内置的库 |
| 泛型 | Generic | 参数化类型 |
| 集合 | Collection | 数据容器 |
| 序列化 | Serialization | 对象转字符串 |
| 反序列化 | Deserialization | 字符串转对象 |
| 时间戳 | Timestamp | 时间的数值表示 |
| 日志 | Log | 运行时记录 |
