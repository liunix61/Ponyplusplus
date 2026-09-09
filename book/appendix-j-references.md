# 附录 J：参考资料

> 本附录列出与 Pony++ 相关的参考资料、论文和项目。

---

## J.1 Actor 模型

| 资料 | 说明 |
|------|------|
| Carl Hewitt, "A Universal Modular ACTOR Formalism for Artificial Intelligence" (1973) | Actor 模型原始论文 |
| Gul Agha, "Actors: A Model of Concurrent Computation in Distributed Systems" (1986) | Actor 模型经典教材 |
| Erlang/OTP Documentation | https://www.erlang.org/docs |
| Akka Documentation | https://doc.akka.io/docs/akka/current/ |

---

## J.2 类型系统与能力

| 资料 | 说明 |
|------|------|
| John C. Reynolds, "The Essence of Algol" (1981) | 引用能力的理论基础 |
| Pony Language Reference | https://stdlib.ponylang.io/ |
| Pony Tutorial | https://tutorial.ponylang.io/ |

---

## J.3 WebAssembly

| 资料 | 说明 |
|------|------|
| WebAssembly Specification | https://webassembly.github.io/spec/ |
| WASI Documentation | https://wasi.dev/ |
| Component Model | https://github.com/WebAssembly/component-model |
| Wasmtime | https://wasmtime.dev/ |

---

## J.4 并发编程

| 资料 | 说明 |
|------|------|
| Joe Armstrong, "Programming Erlang" (2013) | Erlang 并发编程 |
| Rob Pike, "Concurrency Is Not Parallelism" (2012) | 并发 vs 并行 |
| John Ousterhout, "A Philosophy of Software Design" (2018) | 深度模块设计 |

---

## J.5 编译器

| 资料 | 说明 |
|------|------|
| Alfred Aho et al., "Compilers: Principles, Techniques, and Tools" (2006) | 龙书，编译器经典教材 |
| Terence Parr, "Language Implementation Patterns" (2010) | 语言实现模式 |
| LLVM Documentation | https://llvm.org/docs/ |

---

## J.6 嵌入式

| 资料 | 说明 |
|------|------|
| STM32 Reference Manual | https://www.st.com/resource/en/reference_manual/ |
| ESP32 Technical Reference | https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf |
| ARM Cortex-M Programming Guide | https://developer.arm.com/documentation |

---

## J.7 Pony++ 项目资源

| 资源 | 链接 |
|------|------|
| GitHub 仓库 | https://github.com/liunix61/Ponyplusplus |
| Examples 目录 | [`../examples/README.md`](../examples/README.md) |
| 书方案文档 | [`../docs/book-plan.md`](../docs/book-plan.md) |
| Examples 计划 | [`../docs/examples-plan.md`](../docs/examples-plan.md) |
| 编译器文档 | [`../docs/`](../docs/) |

---

## J.8 相关语言

| 语言 | 关系 |
|------|------|
| [Pony](https://www.ponylang.io/) | Pony++ 的灵感来源 |
| [Erlang](https://www.erlang.org/) | Actor 模型先驱 |
| [Rust](https://www.rust-lang.org/) | 所有权系统 |
| [Go](https://go.dev/) | CSP 并发模型 |
| [Zig](https://ziglang.org/) | 系统编程 |
| [Carbon](https://github.com/carbon-language) | C++ 后继 |

---

## J.9 推荐阅读顺序

1. **入门**：第 1-5 章 → examples/00-hello/ → examples/01-syntax/
2. **并发**：第 6-10 章 → examples/02-actor/ → examples/03-concurrency/
3. **标准库**：第 11-14 章 → examples/06-stdlib/
4. **平台**：第 15-17 章 → examples/07-ffi/ → examples/08-mcu/ → examples/09-web/
5. **实践**：第 18-20 章 → examples/10-mini-projects/
6. **深入**：附录 A-J
