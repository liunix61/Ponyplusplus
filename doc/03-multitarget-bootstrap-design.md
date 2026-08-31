# Pony++ 多目标后端与自举体系设计文档（新版）

**版本：** v2.0  
**日期：** 2026-08-31  
**基于：** `doc/02-refactored-design.md` v1.0  
**作者：** Hermes Agent

---

## 摘要

Pony++ 继续保持 Wasm 原生编译器架构，本次升级为多目标后端体系，并补充 MCU 支持路径：

- 目标后端：`wasi-p2`、`component`、`browser`、`mcu-wasm`
- MCU 首发平台：`STM32`、`ESP32`（均归属 `MCU-wasm`）
- MCU 运行时参考：WAMR（Intel 发起的 Wasm 微型运行时）
- 语言核心能力保持不变：Actor、引用能力、监督树、WIT 接口

同时，文档明确 Pony++ 的自举路线：

- 编译器先以 C11 实现并生成 Wasm 产物
- 后续引入原生后端，输出 host-native 二进制
- 再逐步实现 `ponyppc.pny`，完成自举编译器

---

## 1. 目标平台矩阵

| 目标 | 后端类型 | 运行时/宿主 | 主要场景 |
|---|---|---|---|
| `wasi-p2` | Wasm | Wasmtime / Wasmer | 服务器、边缘服务 |
| `component` | Wasm Component | WIT world | 互操作、模块化部署 |
| `browser` | Wasm | JS 宿主 | Web 前端、浏览器端 Actor |
| `mcu-wamr` | Wasm | WAMR on STM32 / ESP32 | MCU、嵌入式、边缘设备 |

### 1.1 首发 MCU 平台

`STM32` 与 `ESP32` 属于 `MCU-wasm` 目标下的子平台，不单独作为并列运行时：

- `MCU-wasm`：WAMR 运行时 + MCU 宿主适配层
- `STM32`：`MCU-wamr` 的 Cortex-M4/M7 子目标，STM32 HAL 外设层
- `ESP32`：`MCU-wamr` 的 ESP-IDF 子目标，Wi-Fi/蓝牙/串口/ADC/I2C/SPI

也就是说，`stm32` 和 `esp32` 都包含在 `MCU-wasm` 虚拟机体系内。

### 1.2 设计约束

- 编译器前段保持不变：词法、语法、类型检查、能力验证
- 后端只改变目标描述、WIT 语义、调用约定与运行时绑定
- MCU 目标必须提供明确的硬件 import 接口，不能依赖纯 WASI

---

## 2. 总体架构

```text
Pony++ source (.pny)
        |
        v
Lexer -> Parser -> Type Check -> Capability Check -> Codegen
        |
        v
Target backend selector
        |
        +--> wasi-p2
        +--> component
        +--> browser
        +--> mcu-wamr (STM32 / ESP32)
        |
        v
Output: .wasm / .wit / optional native binary
```

### 2.1 架构变化说明

本次改动属于“后端扩展”，不是架构重写：

- 保持前端 AST 稳定
- 增加 target profile 层
- 增加 WIT world 选择层
- 增加 MCU host import 层
- 预留 native backend 入口

---

## 3. 编译流水线设计

### 3.1 标准流水线

```text
.lex -> .ast -> .typed -> .capchecked -> .wasm/.wit
```

### 3.2 多目标流水线

```text
.lex -> .ast -> .typed -> .capchecked
                    |
                    +--> target profile
                    +--> wasm backend
                    +--> wit world
                    +--> runtime annotations
```

### 3.3 Target 参数设计

```bash
ponyppc --target=wasi-p2     hello.pny
ponyppc --target=component   hello.pny
ponyppc --target=browser     hello.pny
ponyppc --target=mcu-wamr    hello.pny
ponyppc --target=native      hello.pny    # 预留自举/原生后端
```

---

## 4. WIT World 设计

### 4.1 服务器 World

```wit
package ponypp:wasi

world server {
  import console: wasi:cli/terminal
  import clock: wasi:clocks
  import preopens: wasi:filesystem/preopens
}
```

### 4.2 浏览器 World

```wit
package ponypp:browser

world browser {
  import js: ponypp:js/glue
  import timer: ponypp:timer
}
```

### 4.3 MCU World（STM32 / ESP32）

```wit
package ponypp:mcu

world stm32 {
  import gpio: ponypp:mcu/gpio
  import uart: ponypp:mcu/uart
  import timer: ponypp:mcu/timer
  import i2c: ponypp:mcu/i2c
  import spi: ponypp:mcu/spi
  import adc: ponypp:mcu/adc
}

world esp32 {
  import gpio: ponypp:mcu/gpio
  import uart: ponypp:mcu/uart
  import timer: ponypp:mcu/timer
  import i2c: ponypp:mcu/i2c
  import spi: ponypp:mcu/spi
  import adc: ponypp:mcu/adc
  import wifi: ponypp:esp32/wifi
  import bt: ponypp:esp32/bt
}
```

---

## 5. MCU 适配策略

### 5.1 WAMR 定位

WAMR 是 Pony++ 的 MCU 首选宿主：

- Intel 发起的轻量 Wasm 运行时
- 支持 Cortex-M / ARM / RISC-V
- 支持解释器、AOT、JIT
- 适合 STM32、ESP32、RP2040、RISC-V MCU

### 5.2 MCU 代码生成原则

- 生成 Wasm MVP 子集，优先兼容 WAMR
- 对 MCU 目标关闭或降级：
  - 多线程 Wasm 特性
  - 重型 WASI 功能
  - 复杂 GC 特性
- 对硬件访问统一使用 WIT imports，而非直接嵌入寄存器定义

### 5.3 STM32 支持

- 运行环境：STM32 HAL + WAMR
- 典型外设：GPIO、UART、TIM、I2C、SPI、ADC
- 典型示例：
  - `blink.pny`
  - `uart_echo.pny`
  - `adc_sample.pny`

### 5.4 ESP32 支持

- 运行环境：ESP-IDF + WAMR
- 典型外设：GPIO、UART、TIM、I2C、SPI、ADC、Wi-Fi、BT
- 典型示例：
  - `blink.pny`
  - `mqtt_demo.pny`
  - `wifi_probe.pny`

---

## 6. 自举与原生后端

### 6.1 设计原则

Pony++ 需要同时满足两种核心竞争力：

- 跨平台安全：Wasm / WIT / Component Model
- 系统级性能：原生二进制输出
- 语言自举：最终由 Pony++ 自身实现编译器

### 6.2 阶段化路线

#### Phase A：当前阶段

- C11 编译器 `ponyppc`
- 输出 Wasm + WIT
- 支持多目标后端扩展

#### Phase B：Native Backend

- 保留同一 AST / 类型 / 能力检查层
- 新增 native lowering backend
- 输出 host-native 二进制
- 用于性能敏感场景和自举基础设施

#### Phase C：Self-hosting

- 用 Pony++ 编写 `ponyppc.pny`
- 先混合模式：Pony++ 前段 + C 原生工具
- 再逐步迁移全链路到 Pony++ 自实现

### 6.3 为什么这是护城河

- 只有同时支持 Wasm 与原生输出，语言才能覆盖边缘到服务器的完整栈
- 自举完成后，工具链可脱离宿主语言独立演进
- 对安全、跨平台、性能三线同时兼容，形成真正的语言护城河

---

## 7. 实现路线

### 第一阶段：文档与目标模型

- 固化 target profile
- 固化 WIT world 命名
- 明确 MCU host import 规范

### 第二阶段：编译器后端扩展

- 增加 `--target=<name>`
- 为不同 target 生成不同 WIT / Wasm 头信息
- 为 MCU 生成导入表

### 第三阶段：MCU 示例

- `examples/blinky.pny`
- `examples/uart_echo.pny`
- `examples/adc_sample.pny`

### 第四阶段：Native Backend

- 设计 native lowering IR
- 输出 host-native 可执行文件

### 第五阶段：Self-hosting

- 实现 `ponyppc.pny`
- 验证自举编译闭环

---

## 8. 风险与缓解

| 风险 | 影响 | 缓解 |
|---|---|---|
| WAMR 与 WASI 差异 | MCU 生态兼容不完整 | 以 WIT imports 为核心，减少 WASI 依赖 |
| MCU 内存受限 | Wasm 运行时压力高 | 限制 GC 与调度复杂度 |
| 自举难度高 | 工具链演进慢 | 先做 native backend，再做自举 |
| MCU 厂商差异 | 平台适配成本高 | 先支持 STM32 / ESP32，再扩展 |

---

## 9. 结论

这次设计不推翻 Pony++ 的原架构，而是在其后端层扩展多目标能力：

- 保留 Actor / 引用能力 / 监督树
- 保留 Wasm + WIT 核心
- 新增 MCU WAMR 路径
- 新增 STM32 / ESP32 首发目标
- 预留 native backend 与 self-hosting 路径

这意味着 Pony++ 可以同时覆盖：

- 服务器
- 浏览器
- 边缘设备
- MCU
- 自举编译器生态

这就是 Pony++ 的核心竞争力。