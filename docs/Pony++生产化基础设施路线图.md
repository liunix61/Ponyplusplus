# Pony++ 生产化基础设施路线图 — 必须原生实现的库与设施清单

> 日期: 2026-09-18
> 范围: Pony++ 生态全项目（编译器/runtime/stdlib/协议栈/工具链）
> 结论导向: 生产化投入前，按判据识别"必须 Pony 原生实现"的缺口，沿 App-as-test 飞轮按序补齐。

---

## 1. 判定框架 — 什么"必须原生" vs 可 C+FFI

| 判据 | 说明 | 示例 |
|---|---|---|
| ① 语言语义强绑定 | actor 调度/GC/能力模型——外部实现语义不匹配，只能原生 | 调度器、GC、capabilities |
| ② 目标后端约束 | wasi-p2 component / MCU+WAMR 无完整 libc——必须自产 | MCU 分配器、协作式调度 |
| ③ 零信任审计面 | crypto/沙箱——第三方库=审计面失控，自产可控 | TLS、ponyexecd |
| ④ 自举路线 | 编译器/工具链最终用 Pony++ 重写 | ponyppc、ponybuild |

**C+FFI 模式可行性**（nanonode 已验证模式：crypto/net/tls/wire 全 C+FFI）：
- Linux native（RPi5/x86）：全可行
- wasi-p2：wasi-sdk 可编译 C→wasm.o，可行但需逐库适配
- MCU（STM32/ESP32+WAMR）：需 freestanding C 编译，最受限
- **只有满足上述判据的组件才必须原生**；其余延续 C+FFI 即可

---

## 2. 现状盘点（2026-09-18，d1f81e1）

codegen preamble 已内联 **46 个 pny_\* 运行时函数**（超出一般预期）：

```
✅ actor 模型:  pny_actor_new/register/send + pny_msg_new + pny_runtime_new/free
✅ 集合:        pny_list_new/append/len、pny_set_new、pny_map_new
✅ 字符串:      pny_str_hash/replace_all/concat/slice/find/find_from/contains/field/to_int/from_char + pny_itoa
✅ 文件 I/O:    pny_file_append/read/write/exists/size
✅ JSON:        pny_json_new/parse/get/set/stringify/raw_get
✅ HTTP:        pny_http_accept/respond/post (+ _unix 变体 + post_h) — 已原生内建
✅ 进程/env:    pny_exec_capture、pny_stdin_line、pny_env_get、pny_arg
🟡 能力系统:    capabilities.c Phase 1（iso 唯一性/trn 不进 be 签名/ref|box 不进跨 actor 参数）
```

生态项目状态：
- **PonyDB**（ponydb 7d2abee）：M0-M4 推进，W4a 探针 49/49，Bug#L1 已修，vs-LMDB 基准存在
- **nanonode**（f6d501e）：P0-P4 ✓（wire/双进程/ORV 选举/E2E 共识），RPC 16+HTTP，测试 32 套；TLS C 层握手 18/18
- **PonyGUI**：M0 文档 ✓（docs/00-03），P1 TUI 零 shim 待实现
- **Ponymail**：设计文档 ✓（对标 dbmail+sqwebmail），冻结中，依赖 PonyDB+TLS+MIME
- **Ponyget/PURL**：规划中（W5 后），客户端 TLS 依赖已就绪
- **ticktock-rtos / ponyagents / ponyexecution / ponyharness**：workspace 内并行项目

---

## 3. 必须原生的缺口清单（四梯队）

### 第一梯队 — 运行时心脏（判据①②，所有后端前置，最大缺口）

| 缺口 | 现状 | 说明 |
|---|---|---|
| **actor 异步调度器** | ✗ | pny_actor_send 存在但测试全为顺序执行——真实 mailbox 消费/多核 work-stealing/MCU 协作式调度未落地；当前 actor≈对象+同步方法调用的简化模型 |
| **GC/内存回收** | ✗ | 46 函数无 gc/rc；生产需要 RC+cycle collector 或 arena（MCU 无 malloc） |
| **MCU 原生分配器** | ✗ | STM32/ESP32 必须 bump/pool allocator（判据②最强约束） |
| **futures/promises** | ✗ | 跨 actor 异步结果——服务端并发必备 |
| **定时器/时间轮** | ✗ | ticktock-rtos 为 RTOS 向；stdio/服务端事件循环同样需要 |
| **panic 回溯** | 🟡 | error 语法 ✓（codegen 修复链完成），运行时栈回溯/诊断路径待完善 |

### 第二梯队 — 协议栈与安全（判据③，生态项目驱动）

| 缺口 | 现状 | 说明 |
|---|---|---|
| **crypto 原语原生化** | 🟡 C 库（libnncrypto.a via FFI） | PURL 已规划：TLS1.3 纯原生（CHACHA20 绕 AES、ECDSA P-256+ARMv8 CE）——路线在，未动工 |
| **TLS 服务端 + 原生化** | 🟡 C 层客户端 18/18 | POLY1305 单测 FAIL 待核查（AEAD 路径 PASS，疑测试向量问题）+ 服务端状态机/ECDSA P-256/openssl 互操作未做 |
| **TCP transport 原生** | 🟡 nanonode net C 层 UDP ✓ | 生产 TCP 非阻塞 IO（epoll / wasi poll / MCU 中断驱动——逐后端差异化） |
| **DNS resolver** | ✗ | Ponyget/PURL 硬需求——UDP DNS 客户端可纯原生 |
| **HTTP 生产化** | 🟡 内建简易版 | keep-alive/chunked/HTTPS 待完善 |
| **ponyexecd 零信任沙箱** | ✗ | PonyDB M3 里程碑（wasip2+fuel 限额）——生态核心卖点，零信任哲学的语言级落地 |
| **能力系统深化** | 🟡 Phase 1 | iso/trn 雏形 → 完整 sendable/aliased 语义 |

### 第三梯队 — 领域 stdlib（判据②，应用项目驱动，App-as-test 飞轮）

| 缺口 | 驱动项目 | 说明 |
|---|---|---|
| MIME 编解码（base64/quoted-printable/UTF-8↔GBK） | Ponymail | 邮件解析硬需求 |
| 压缩（deflate/gzip） | Ponymail/Ponyget/HTTP | 可 C 交叉编译；wasi/MCU 场景倾向原生 |
| 正则表达式 | 通用文本处理 | 可延后 |
| 编码/Unicode 完整性 | Ponymail/PonyGUI | TUI P1 渲染依赖 |

### 第四梯队 — 工具链自举（判据④，长期）

| 缺口 | 现状 | 说明 |
|---|---|---|
| ponyppc 自举 | C 实现 | 终极目标：Pony++ 编译器用 Pony++ 重写 |
| ponybuild 原生化 | Python（tools/ponybuild.py） | 自举路线要求最终原生 |
| 包管理/依赖解析 | pony.toml 有声明 | 解析器在 Python 侧 |
| 测试框架 ponyharness | workspace 内存在 | 状态待确认；生产 CI 必备 |

---

## 4. 后端差异化约束矩阵（跨平台必须逐 SoC 分析）

| 组件 | Linux native (RPi5/x86) | wasi-p2 component | MCU (STM32/ESP32+WAMR) |
|---|---|---|---|
| 分配器 | malloc 可用 | dlmalloc via wasi | **必须原生** bump/pool |
| 调度器 | pthread work-stealing | 单线程事件循环 | **协作式调度 + 时间轮** |
| IO 多路复用 | epoll | wasi poll_oneoff | 中断驱动 / DMA |
| C+FFI | 全可行 | wasi-sdk 编 C 可行 | 需 freestanding C |
| TLS/crypto | C 库模式可续 | C→wasm 可行 | ARMv8 CE 指令原生化收益最大 |

---

## 5. 生产化优先级路线（App-as-test 飞轮模式）

ponydb 已验证模式：**应用需求 → 编译器内建扩展 → 缺陷即修**（DB 驱动 file_append/定长读写，PonyHarness 抓 Bug#41/42）。延续该模式，按应用开发顺序倒逼基础设施落地：

| 序 | 任务 | 驱动/判据 | 量级 |
|---|---|---|---|
| 1 | **actor 调度器 + GC + 定时器** | 语言心脏，一切应用前置 | 大 |
| 2 | **TLS 服务端补全 + POLY1305 核查** | nanonode 内，解锁 Ponymail | ~3-4 天 |
| 3 | **ponyexecd 沙箱**（wasip2+fuel） | PonyDB M3，零信任卖点 | 中 |
| 4 | **MIME/编码** | Ponymail 解冻驱动 | 中 |
| 5 | **crypto/TLS 原生化 + DNS/TCP** | Ponyget/PURL 驱动 | 大 |
| 6 | **能力系统 Phase 2** | iso/trn 完整语义 | 中 |
| 7 | **工具链自举**（ponyppc/ponybuild） | 长期线 | 大 |

**不建议**一次性铺开 stdlib——按 Ponymail → Ponyget → PonyGUI 的开发顺序倒逼缺口补齐，每个缺口都有真实负载在测试它。

---

## 6. 参考

- 现状数据来源：`/home/liunix/Ponyplusplus/src/codegen.c`（runtime preamble 函数清单）、`src/capabilities.c`
- 生态项目状态快照：`ponyplusplus-development` skill `references/pony-ecosystem-status-2026-09.md`
- ponydb 里程碑定义：`workspace/ponydb/docs/01-总体方案.md` §4
- PonyGUI 阶段定义：`workspace/PonyGUI/docs/00-总体方案.md`
- Ponymail 架构：`workspace/ponymail/docs/01-总体方案与系统架构.md`
