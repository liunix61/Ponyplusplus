# 《Pony++ 程序设计语言》

> **The Pony++ Programming Language**
> 一门为云原生与嵌入式而生的 Actor 语言
>
> **作者**：liunix
> **版本**：v1.0
> **章数**：20 章 + 10 附录

---

## 目录

### 第 0 部分 序章

| 章节 | 标题 | 文件 |
|------|------|------|
| 第 1 章 | 入门：Hello World 与开发环境 | [ch01-intro.md](ch01-intro.md) |

### 第 1 部分 语言基础

| 章节 | 标题 | 文件 |
|------|------|------|
| 第 2 章 | 数据类型与变量 | [ch02-datatypes.md](ch02-datatypes.md) |
| 第 3 章 | 控制流 | [ch03-controlflow.md](ch03-controlflow.md) |
| 第 4 章 | 函数：fun 与 be | [ch04-functions.md](ch04-functions.md) |
| 第 5 章 | 对象与结构体 | [ch05-objects.md](ch05-objects.md) |

### 第 2 部分 Actor 模型（Pony++ 核心）

| 章节 | 标题 | 文件 |
|------|------|------|
| 第 6 章 | Actor 模型导论 | [ch06-actor-model.md](ch06-actor-model.md) |
| 第 7 章 | 消息传递与并发 | [ch07-messaging.md](ch07-messaging.md) |
| 第 8 章 | 能力系统 | [ch08-capabilities.md](ch08-capabilities.md) |
| 第 9 章 | 并发原语 | [ch09-concurrent.md](ch09-concurrent.md) |
| 第 10 章 | 监督树与容错 | [ch10-supervisor.md](ch10-supervisor.md) |

### 第 3 部分 标准库

| 章节 | 标题 | 文件 |
|------|------|------|
| 第 11 章 | 数学与字符串 | [ch11-math-string.md](ch11-math-string.md) |
| 第 12 章 | 数据结构：List 与 Map | [ch12-data-structures.md](ch12-data-structures.md) |
| 第 13 章 | IO、文件与日志 | [ch13-io-logging.md](ch13-io-logging.md) |
| 第 14 章 | JSON 与时间 | [ch14-json-time.md](ch14-json-time.md) |

### 第 4 部分 多目标平台

| 章节 | 标题 | 文件 |
|------|------|------|
| 第 15 章 | 外部接口（FFI） | [ch15-ffi.md](ch15-ffi.md) |
| 第 16 章 | MCU 嵌入式（STM32/ESP32） | [ch16-mcu.md](ch16-mcu.md) |
| 第 17 章 | 浏览器 WASM 组件 | [ch17-web-wasm.md](ch17-web-wasm.md) |

### 第 5 部分 综合实践

| 章节 | 标题 | 文件 |
|------|------|------|
| 第 18 章 | 计数器服务 | [ch18-counter-service.md](ch18-counter-service.md) |
| 第 19 章 | 键值存储 | [ch19-kv-store.md](ch19-kv-store.md) |
| 第 20 章 | 响应式管道 | [ch20-reactive-pipeline.md](ch20-reactive-pipeline.md) |

### 附录（计划中）

| 附录 | 标题 |
|------|------|
| A | 完整语法参考 |
| B | 完整 API 参考 |
| C | 术语表（中英对照） |
| D | 常见错误与调试 |
| E | 能力系统形式化语义 |
| F | 监督树策略详解 |
| G | Wasm 组件模型深入 |
| H | 编译器架构概览 |
| I | 练习题答案 |
| J | 参考资料 |

---

## 配套资源

- **Examples**：[`../examples/README.md`](../examples/README.md) — 35 个可运行示例
- **书方案**：[`../docs/book-plan.md`](../docs/book-plan.md) — 写作计划与风格规范
- **Examples 计划**：[`../docs/examples-plan.md`](../docs/examples-plan.md) — 示例实施计划
- **GitHub**：https://github.com/liunix61/Ponyplusplus

---

## 阅读建议

| 读者类型 | 推荐路径 |
|----------|----------|
| 初学者 | 第 1-5 章 → 第 6-7 章 → 第 18 章 |
| 有并发经验 | 第 1 章 → 第 6-10 章 → 第 18-20 章 |
| 嵌入式开发者 | 第 1-6 章 → 第 16 章 → 第 18 章 |
| 后端开发者 | 第 1-7 章 → 第 11-14 章 → 第 18-20 章 |
