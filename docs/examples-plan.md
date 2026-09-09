# Pony++ Examples 实施计划

**版本**：v1.0
**编制日期**：2026-09-09
**目标**：为用户学习与使用 Pony++ 语言提供完整、可运行、循序渐进的示范案例

---

## 0. 设计原则

| 原则 | 说明 |
|------|------|
| 每个 example 对应书的一个章节 | examples/ 与 docs/book 同步演进 |
| 代码必须可运行 | `./bin/ponyppc build <file>` + `./<binary>` 可执行 |
| 最小可运行单元 | 遵循 K&R 风格：最短代码演示最核心概念 |
| 完整工程示例 | 遵循 Prime Plus 风格：每章至少 1 个"实用"完整示例 |
| 多目标演示 | 每个 example 同时给出 native / wasm / mcu-wasm 编译命令 |
| 配套 README | 每个 example 目录含：`README.md` + `.pny` 源码 + 预期输出 |

---

## 1. 目录结构

```
examples/
├── README.md                          # 总索引：分级、依赖关系、运行说明
├── 00-hello/                          # 入门：Hello World
│   ├── 00-hello-world.pny
│   ├── 00-hello-native.pny
│   └── README.md
├── 01-syntax/                         # 基础语法
│   ├── 01-variables.pny              # 变量与类型
│   ├── 02-control-flow.pny           # if / match / while
│   ├── 03-functions.pny              # fun / be / return
│   ├── 04-objects.pny                # 结构体、引用、组合
│   └── README.md
├── 02-actor/                          # Actor 模型
│   ├── 01-single-actor.pny           # 单个 actor
│   ├── 02-message-passing.pny        # actor 间消息
│   ├── 03-multiple-actors.pny        # 多 actor 协作
│   └── README.md
├── 03-capabilities/                   # 能力系统（Pony++ 特色）
│   ├── 01-val-ref.pny               # val / ref 引用
│   ├── 02-trn-iso.pny               # trn / iso 转移
│   ├── 03-box-tag.pny               # box / tag 弱引用
│   └── README.md
├── 04-concurrent/                     # 并发原语
│   ├── 01-channel.pny               # Channel 发送/接收
│   ├── 02-mutex.pny                 # Mutex 互斥
│   ├── 03-future.pny                # Future 异步结果
│   ├── 04-pipeline.pny              # actor 管道
│   └── README.md
├── 05-supervisor/                     # 监督树（Erlang 风格）
│   ├── 01-one-for-one.pny           # 一对一策略
│   ├── 02-one-for-all.pny           # 一对全策略
│   ├── 03-error-recovery.pny        # 错误恢复
│   └── README.md
├── 06-stdlib/                         # 标准库
│   ├── 01-math.pny                  # 数学运算
│   ├── 02-string.pny                # 字符串处理
│   ├── 03-list.pny                  # 列表 + 高阶函数
│   ├── 04-json.pny                  # JSON 序列化
│   ├── 05-io.pny                    # 文件读写
│   ├── 06-log.pny                   # 结构化日志
│   ├── 07-time.pny                  # 定时器
│   └── README.md
├── 07-ffi/                            # 外部接口（可选）
│   ├── 01-call-c.pny                # 调用 C 函数
│   └── README.md
├── 08-mcu/                            # MCU 嵌入式
│   ├── 01-blink-led.pny             # STM32 闪烁
│   ├── 02-read-adc.pny              # ESP32 ADC
│   └── README.md
├── 09-web/                            # 浏览器 WASM 组件
│   ├── 01-greeter.pny               # 简单消息组件
│   ├── 02-counter.pny               # 交互计数器
│   └── README.md
├── 10-mini-projects/                  # 综合项目
│   ├── 01-counter-service.pny       # 计数器服务
│   ├── 02-key-value-store.pny       # 键值存储
│   ├── 03-reactive-pipeline.pny     # 响应式管道
│   └── README.md
└── scripts/
    ├── run-all.sh                    # 一键运行所有 example
    └── verify-all.sh                 # 校验所有 example 编译通过
```

---

## 2. 分级与依赖关系

| 级别 | 目录 | 主题 | 前置 | 对应书章节 |
|------|------|------|------|-----------|
| L0 | 00-hello | 环境、Hello World | 无 | 第1章 |
| L1 | 01-syntax | 变量、控制流、函数 | L0 | 第2-4章 |
| L2 | 02-actor | Actor 模型、消息传递 | L1 | 第5章 |
| L3 | 03-capabilities | val/ref/trn/iso/box/tag | L2 | 第6章 |
| L4 | 04-concurrent | Channel/Mutex/Future | L3 | 第7章 |
| L5 | 05-supervisor | 监督树、容错 | L4 | 第8章 |
| L6 | 06-stdlib | Math/String/List/JSON/IO/Log/Time | L2 | 第9-11章 |
| L7 | 07-ffi | 外部接口 | L6 | 第12章 |
| L8 | 08-mcu | STM32/ESP32 嵌入式 | L6 | 第13章 |
| L9 | 09-web | 浏览器 WASM 组件 | L6 | 第14章 |
| L10 | 10-mini-projects | 综合项目 | L5+L6 | 第15章 |

---

## 3. 每个 Example 的标准模板

每个 example 目录包含：

```
examples/<NN-topic>/<NN>-<name>.pny      # 源码
examples/<NN-topic>/README.md            # 讲解文档
```

### 3.1 .pny 源码规范

- 顶部注释：`// <name>.pny - <一句话说明>`
- 关键概念标注：`// [CONCEPT] 注释解释当前代码演示的概念`
- 末尾标注预期输出：`// 预期输出: <内容>`
- 编译命令注释：
  ```
  // 编译: ./bin/ponyppc build 00-hello.pny -o 00-hello
  // 运行: ./00-hello
  ```

### 3.2 README.md 规范

每个 example 目录的 README.md 包含：

1. **学习目标**（3-5 行）
2. **核心概念**（概念清单 + 1-2 句解释）
3. **代码演示**（代码 + 逐步注释）
4. **运行方式**（native / wasm / mcu 三种目标）
5. **变体练习**（2-3 个修改任务）
6. **下一步**（指向下一 example 或下一章）

---

## 4. 分级详解

### L0 — 00-hello（入门）

| 文件 | 内容 | 演示点 |
|------|------|--------|
| 00-hello-world.pny | `actor main { new create() => { print("Hello, World!") } }` | 最小可运行程序、actor main 入口 |
| 00-hello-native.pny | 含变量 + print | 变量、类型标注、原生目标 |

### L1 — 01-syntax（基础语法）

| 文件 | 内容 | 演示点 |
|------|------|--------|
| 01-variables.pny | 各种类型变量（U8/I64/F64/String/Bool/Unit） | 类型系统 |
| 02-control-flow.pny | if / else / match / while / for | 控制流 |
| 03-functions.pny | fun（纯函数）、be（行为方法）、return | 函数语义 |
| 04-objects.pny | 结构体、字段、方法、组合 | 对象模型 |

### L2 — 02-actor（Actor 模型）

| 文件 | 内容 | 演示点 |
|------|------|--------|
| 01-single-actor.pny | 单 actor + be 方法 | actor 生命周期 |
| 02-message-passing.pny | 两个 actor 互相 send | 异步消息 |
| 03-multiple-actors.pny | 3+ actor 协作处理数据 | actor 池 |

### L3 — 03-capabilities（能力系统）★ Pony++ 核心特色

| 文件 | 内容 | 演示点 |
|------|------|--------|
| 01-val-ref.pny | val（不可变）vs ref（可变） | 值/引用语义 |
| 02-trn-iso.pny | trn（转移）vs iso（独占） | 所有权转移 |
| 03-box-tag.pny | box（弱引用）vs tag（标签） | 跨 actor 共享 |

### L4 — 04-concurrent（并发原语）

| 文件 | 内容 | 演示点 |
|------|------|--------|
| 01-channel.pny | Channel 发送/接收 | 有界/无界通道 |
| 02-mutex.pny | Mutex 互斥锁 | 临界区保护 |
| 03-future.pny | Future 异步结果 | 异步回调 |
| 04-pipeline.pny | actor 管道（生产者→处理器→消费者） | 真实场景 |

### L5 — 05-supervisor（监督树）★ Pony++ 核心特色

| 文件 | 内容 | 演示点 |
|------|------|--------|
| 01-one-for-one.pny | 子 actor 崩溃只重启自己 | 一对一策略 |
| 02-one-for-all.pny | 子 actor 崩溃重启所有兄弟 | 一对全策略 |
| 03-error-recovery.pny | 错误重试 + 最终失败处理 | 容错模式 |

### L6 — 06-stdlib（标准库）

| 文件 | 内容 | 演示点 |
|------|------|--------|
| 01-math.pny | Math.sqrt / pow / min / max / random | 数学 API |
| 02-string.pny | String.len / concat / split / substring | 字符串处理 |
| 03-list.pny | List.append / map / filter / fold | 高阶函数 |
| 04-json.pny | Json.parse / stringify | JSON 处理 |
| 05-io.pny | IO.file_open / read / write | 文件 IO |
| 06-log.pny | Logger.info / warn / error | 结构化日志 |
| 07-time.pny | Timer 定时任务 | 异步时间 |

### L7 — 07-ffi（外部接口）

| 文件 | 内容 | 演示点 |
|------|------|--------|
| 01-call-c.pny | 调用 C 标准库函数 | FFI 声明与调用 |

### L8 — 08-mcu（嵌入式）

| 文件 | 内容 | 演示点 |
|------|------|--------|
| 01-blink-led.pny | STM32 闪烁 LED | `--target mcu-wasm --mcu stm32f4` |
| 02-read-adc.pny | ESP32 ADC 读取 | `--target mcu-wasm --mcu esp32` |

### L9 — 09-web（浏览器组件）

| 文件 | 内容 | 演示点 |
|------|------|--------|
| 01-greeter.pny | 暴露 WIT 接口的消息组件 | `--target browser` |
| 02-counter.pny | 交互式计数器组件 | 状态管理 |

### L10 — 10-mini-projects（综合项目）

| 文件 | 内容 | 演示点 |
|------|------|--------|
| 01-counter-service.pny | 计数器 actor 服务 | actor + channel + supervisor |
| 02-key-value-store.pny | 分布式键值存储 | 多 actor + 消息传递 |
| 03-reactive-pipeline.pny | 响应式数据处理管道 | 高阶函数 + 并发 |

---

## 5. 实施阶段

### Phase 1（第 1 周）— 入门与基础

- [ ] 00-hello（2 个）
- [ ] 01-syntax（4 个）
- [ ] examples/README.md（总索引）
- [ ] scripts/run-all.sh + verify-all.sh

**产出**：6 个 example，覆盖 L0-L1

### Phase 2（第 2 周）— Actor 与能力

- [ ] 02-actor（3 个）
- [ ] 03-capabilities（3 个）

**产出**：6 个 example，覆盖 L2-L3

### Phase 3（第 3 周）— 并发与容错

- [ ] 04-concurrent（4 个）
- [ ] 05-supervisor（3 个）

**产出**：7 个 example，覆盖 L4-L5

### Phase 4（第 4 周）— 标准库

- [ ] 06-stdlib（7 个）

**产出**：7 个 example，覆盖 L6

### Phase 5（第 5 周）— 多目标与综合

- [ ] 07-ffi（1 个）
- [ ] 08-mcu（2 个）
- [ ] 09-web（2 个）
- [ ] 10-mini-projects（3 个）

**产出**：8 个 example，覆盖 L7-L10

**总计**：34 个 example，5 个 Phase，1 个月完成

---

## 6. 验证机制

### 6.1 CI 自动验证

```yaml
# .github/workflows/examples.yml
on: [push, pull_request]
jobs:
  verify-examples:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Build compiler
        run: make all
      - name: Verify all examples
        run: bash examples/scripts/verify-all.sh
```

### 6.2 verify-all.sh 逻辑

```bash
#!/usr/bin/env bash
set -euo pipefail
FAIL=0
for f in $(find examples -name "*.pny" | sort); do
  echo "==> $f"
  ./bin/ponyppc build "$f" -o "/tmp/$(basename $f .pny)" --target native || FAIL=1
done
exit $FAIL
```

### 6.3 run-all.sh 逻辑

```bash
#!/usr/bin/env bash
set -euo pipefail
for f in $(find examples -name "*.pny" | sort); do
  echo "=== Running $f ==="
  ./bin/ponyppc run "$f" --target native
  echo ""
done
```

---

## 7. 与书章节的对应

| 书章节 | Examples 目录 | 核心概念 |
|--------|---------------|---------|
| 第1章 入门 | 00-hello | Hello World、环境搭建 |
| 第2章 数据类型 | 01-syntax/01 | 类型系统 |
| 第3章 控制流 | 01-syntax/02 | if/match/loop |
| 第4章 函数 | 01-syntax/03 | fun/be/return |
| 第5章 Actor 模型 | 02-actor | actor、消息传递 |
| 第6章 能力系统 | 03-capabilities | val/ref/trn/iso/box/tag |
| 第7章 并发原语 | 04-concurrent | Channel/Mutex/Future |
| 第8章 监督树 | 05-supervisor | 容错、错误恢复 |
| 第9章 标准库 I | 06-stdlib/01-03 | Math/String/List |
| 第10章 标准库 II | 06-stdlib/04-05 | JSON/IO |
| 第11章 标准库 III | 06-stdlib/06-07 | Log/Time |
| 第12章 外部接口 | 07-ffi | FFI |
| 第13章 MCU 嵌入式 | 08-mcu | STM32/ESP32 |
| 第14章 浏览器组件 | 09-web | WASM 组件 |
| 第15章 综合项目 | 10-mini-projects | 完整工程 |

---

## 8. 后续演进

- 每个 example 增加 `tests/` 子目录，配套单元测试
- 集成 REPL 交互演示
- 提供 VS Code 插件一键运行 example
- 在线版 examples（REPL + 编辑器）
- 翻译为英文版本（examples/EN/）
