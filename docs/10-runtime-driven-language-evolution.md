# Pony++ 语言演进溯源：Runtime 项目驱动的语言增益
# Pony++ Language Evolution Traceability: Runtime-Project-Driven Gains

> 目的 / Purpose：记录每个 runtime 实证项目对 Pony++ 语言、编译器、运行时的关键增益、
> 提升、优化与语义变化，作为后续开发的根据（哪些能力由什么需求催产、当前状态、还缺什么）。
>
> 版本依据：CHANGELOG.md（0.1.0 → 0.2.12）+ 各项目 README + Bug 台账（#1–#50）。
> 最后更新：2026-09-16（HEAD f9b1d0e）

---

## 0. 总图 / Overview

| 项目 | 角色 | 催生的语言能力域 | 验收基线 |
|---|---|---|---|
| PonyExecution | 不可信代码沙箱（wasm+fuel+审计链） | wasi-p2 target、FFI 调用约定、确定性执行语义 | ponyexecd 单二进制自举 |
| PonyAgents | 多智能体编排 runtime | async actor、trait、模块系统、类型注解、链式方法 | 150 测试 |
| PonyHarness | 插件化 Agent Harness | 函数指针/回调 FFI 稳定性、双向 wasm 宿主、HTTP/UNIX socket 内建 | H0-H4 全通过 |
| Ponypi | 轻量 Agent runtime（自举替代 Python ponyagent） | 文件 I/O 内建、字符串协议、ponyppc 作为运行时依赖 | 14 项测试 + ponyrun |
| PonyDB | 键值数据库（LMDB 对标） | **语义精度**：复合赋值/取模/字符串重载/类系统 wasm/内建全集/GC | 49/49 + 175/175 回归 |

**演进规律 / Evolution pattern**：
PonyAgents 定语法骨架（并发与模块）→ PonyExecution/PonyHarness/Ponypi 压 FFI 与系统边界
→ PonyDB 磨语义精度（双后端逐字节一致）。
The agent projects defined the syntactic skeleton; the sandbox/harness projects hardened the
FFI and system boundaries; PonyDB forced semantic precision (byte-level parity across both
backends).

---

## 1. PonyExecution 的增益 / Gains from PonyExecution

**项目**：`workspace/ponyexecution`，产品级 Linux 不可信代码沙箱。单二进制 `ponyexecd`、
fuel 精确计量、哈希链审计收据、MCP 原生（tool=code_exec）。

### 1.1 语言/编译器
- **wasi-p2 target 的存在本身**（M1 时代）：第一次证明 Pony++ 能编译出系统级 Linux 服务
  （HTTP/UNIX socket/MCP 三模式守护进程）。
- **确定性执行语义**：fuel 计量要求整数运算零隐式转换、无未定义行为路径——推动 U32/I32
  严格语义与 `%`/除法语义对齐（后在 0.2.7 落地为 C99 语义双后端一致）。
- **wasmtime C API FFI**：结构体跨语言传参、回调注册——FFI 调用约定的第一次重度实战。

### 1.2 运行时
- TRAP/fuel 耗尽语义（ponydb 后续 W5 落地 wasm 版时直接复用这套概念）。
- 审计收据哈希链 → 逼出稳定的字节级序列化（`pny_str_field` 分隔符协议的前身场景）。

### 1.3 遗留问题
- GitHub push 443 超时（liunix61/PonyExecution，55e9b4d 待推）。

---

## 2. PonyAgents 的增益 / Gains from PonyAgents

**项目**：`workspace/ponyagents`，多智能体编排 runtime（自举实证 #2）。四层能力 M1-M4
（AgentActor 内核 / 五模式编排 / 自进化 LessonPoolActor / HTTP+MCP+CLI）。

### 2.1 语法骨架（本项目是最大语法催产素）
- **async actor 并发模型**（W0-W4）：actor 隔离消息传递、`be` 行为方法、promise 点语法。
- **AsyncResult**（Bug#36）：async 方法返回值传播。
- **类方法声明解析**（Bug#40a）：`class Foo { fun m() {} }` 先声明后调用。
- **类型注解参数**（Bug#40b）：`fun f(x: U32)` 解析修复。
- **trait 默认方法**：接口带实现体（编排模式的公共协议层需要）。
- **链式方法**（Bug#41，0.2.0）：`f(x).method(args)` 此前**解析器死循环**（TK_DOT 未消费）——
  编排代码 `bus.sandbox.run()` 这类三段链直接触发。
- **模块系统**：chained import（Bug#41/42 区间）、导入含 `actor main` 的文件边界。

### 2.2 类型系统
- 三段链 String 判定（Bug#25/26）：`cg_chain_resolve` + 全局类型字段表 `type_field_types`。
- String 返回方法注册表 `str_ret_methods`（Bug#16/36）——方法链的返回类型传播。
- 类型推断修复（Bug#39）：`recv.slice(...)` 点号形态绕过 String 返回表。

### 2.3 运行时
- 崩溃隔离 + 零锁并发 → actor 调度与 GC 强绑定，推动 G 系列 GC 演进。
- 死锁检测（DAG 就绪集）→ 循环依赖静态检查。

---

## 3. PonyHarness 的增益 / Gains from PonyHarness

**项目**：`workspace/ponyharness`，插件化 Agent Harness（对标 dsh，钩子命名对齐）。插件总线
（注册/挂载序/启停/故障隔离）、6 钩子、Turn 状态机、profiles、动态 Wasm 插件宿主。

### 3.1 FFI 与系统边界
- **函数指针表/回调约定**：6 钩子 false 短路语义 → C FFI 边界稳定性被反复压测。
- **双向 wasm 宿主**：Pony++ 进程经 ponyexecd 加载他人 wasm 模块（零信任）——wasip2 组件
  模型双向验证。
- **HTTP 运行时强化**（0.2.0）：keep-alive 客户端连接缓存 + Content-Length 帧读取 + 失败重试；
  服务端连接复用 + 块读优化（1024B/次，~150 syscall → ~2）。
- **UNIX domain socket 三内建**：`http_accept_unix/http_respond_unix/http_post_unix`——
  本机零拷贝通道（ponyexecd.sock 通道的直接需求）。
- `http_post_h(url, body, extra_headers)`：带自定义头 POST（H2）。

### 3.2 字符串/序列化
- 插件装配表、trajectory replay 全是字符串协议 → `str_field` 分隔符切字段场景的前身。
- cstr_escape `\r` 转义修复（Bug#28）——JSON-RPC 帧正确性。

---

## 4. Ponypi 的增益 / Gains from Ponypi

**项目**：`workspace/ponypi`，Pony++ 写的轻量 Agent runtime（自举替代 Python ponyagent）。
单文件核心 + ReAct 循环 + Session 树（JSONL 落盘）+ streaming steering + 扩展 API。

### 4.1 内建与 I/O
- Session JSONL 落盘 + fork/n/rename/compact/undo → 重度文件 I/O：`file_size`（不存在=0）、
  `file_append` 内建确认可用（0.2.0，PonyDB M2 COW 页文件直接复用）。
- `ponyrun` 工具（M1）：**ponyppc 首次作为运行时依赖**——编译器速度/稳定性被日常化压测；
  native 即时编译 + 沙箱 wasm 双路径。

### 4.2 执行语义
- M4 死循环 TRAP(5M fuel) + 审计拦截钩子 → 与 PonyExecution 同源的确定性需求。
- streaming steering（输入即注入不打断工具）→ 并发输入通道，async actor 的最早试验田。

### 4.3 架构约束（记录在案的设计决策）
- Ponypi 保持轻量级，**不做 PonyAgents（生产级）的核心**——两者基座共享基础类型不 fork。

---

## 5. PonyDB 的增益 / Gains from PonyDB

**项目**：`workspace/ponydb`，LMDB 对标键值库（49/49 测试）。当前 M3 wasi-p2 落地进行中。
PonyDB 是**语义精度**的磨刀石：双后端（native C / wasm）逐字节一致是硬标准。

### 5.1 运算符与表达式语义
| 增益 | 版本 | 背景 |
|---|---|---|
| `%` 取模（parser+codegen 双补，C99 语义） | 0.2.7 (Bug#48) | 奇偶判定被打穿才发现静默失效 |
| `+ - * /` 算术分派补全 | 0.2.0 (Bug#33) | wasm 后端此前 stub 成 const 0 |
| `+= -= *= /=` 复合赋值 | 0.2.10 (W1) | wasm 后端此前静默丢弃 |
| 一元 `not`/`neg` | 0.2.10 (W1) | 同上 |
| 字符串 `+` 重载（concat 分派） | 0.2.11 (W2) | 类型推导驱动：任一侧 String 走字符串语义 |
| 字符串 `==/!=` 值比较（streq） | 0.2.11 (W2) / 0.2.0 (Bug#30) | 此前只认字面量 |
| 字符串 `< > <= >=` 统一 strcmp | 0.2.0 (Bug#43) | 此前裸 C 指针比较，分配字符串必错 |

### 5.2 字符串内建全集（ponydb 需求面直接驱动）
| 内建 | 版本 | ponydb 用途 |
|---|---|---|
| `str_field(s, i, sep)` | 0.2.x 早期 | **57 处调用**——分隔符协议（\x1e/\x1c/\x1f）核心 |
| `find_from(s, sub, off)` | 0.2.6 | 解析热点：单趟 O(n) 替代 str_field 的 O(n²) |
| `json_raw_get(s, key)` | 0.2.x | 14 处——CLI JSON 协议解析（含冒号跟随验证 Bug#29） |
| `str_from_char(c)` | 0.2.0 (Bug#37) | 数字转字符串 |
| `str_replace_all(s,o,n)` | 0.2.0 (Bug#40) | script \n 转义处理 |
| `s.slice(start[,end])` | 0.2.0 (Bug#38) | 钳位语义 + 缺省 end |
| `s.find/contains` | 0.2.0 (Bug#37) | — |
| `char_code/file_size/env_get` | 0.2.0 | 校验和/页文件/printenv 替代 |
| `\xNN` 十六进制转义 | 0.2.0 (Bug#44) | 真二进制字节（分隔符就是 \x1e 等） |

### 5.3 类系统（W3，0.2.12，wasm 后端）
- 字段 4 字节槽线性布局（String=i32 指针）；构造器 alloc+init 返回 self 指针；
  实例方法 self=local 0 独立 wasm 函数。
- `this.f` / 裸字段（方法体内）/ `c.f`（main 局部实例）三形态字段访问统一解析。
- `C()`/`C.create()`/`c.m()`/`this.m()` 分派 + local_class 类型追踪。
- 驱动源：ponydb 的 BTree/Node 等 10 个类、`this.` 179 处、方法最大参数 2。

### 5.4 内存纪律（血泪史，铁律级）
- 0.2.4：**PONYPP_GC=1 Boehm GC 模式**——此前纯 malloc 无 free 是泄漏引擎，ponydb 800 键
  耗尽 8GB 触发 OOM killer 误杀网关；GC 模式 20000 键 1GB 护栏内通过。
- 0.2.8：**GC_malloc_atomic** 纯字符缓冲免保守扫描 → 10k put 8.2→4.8s / 20k 37.9→22.3s（-41%）。
- 测试护栏：`ulimit -v 1048576` 全套；wasmtime 不能套 ulimit -v（8GB mmap 预留被拦）。

### 5.5 编译器工程化
- Bug#47：裸局部变量被同名类字段劫持（局部/参数表优先于字段表）。
- Bug#50：parse 错误静默吞掉 → `parser_has_error()` 硬失败。
- Bug#49（推测区间）：局部变量表 32→256（Bug#35）。
- 类收集必须先于 type/func 段发射（段计数依赖方法数）——W3 实现纪律。

---

## 6. wasm 后端陷阱台账（开发必读）

累积自 W0-W4，任何 wasm.c 修改前先过一遍：

1. i32 有符号比较真值：GT_S=0x4A / LE_S=0x4C / GE_S=0x4E（0x49/4B/4D/4F 是无符号变体）。
2. load/store 必带 align+offset 立即数（i32.load align=2 offset=N）。
3. global.get/set (0x23/0x24) 后必须跟索引字节 0x00。
4. 函数索引 off-by-one：import 数决定 base（wasip2: main=5, print=6, 运行时 b+1 起）。
5. NODE_EMPTY 参数容器要展开——含 child_count=0 的零参调用（否则容器被发射成 const 0）。
6. main walk 须认 NODE_FUN 名为 main（不止 NODE_NEW）。
7. 块=NODE_EMPTY(data=NULL)，没有 NODE_BLOCK；if 内 br 0 指向 if 自身。
8. wasm_expr_is_str 字段分支勿带 cur_class 门槛（main=-1）。
9. 类收集（w3_collect_classes）必须在 type/func 段发射之前。
10. i32.const 用 signed LEB128（unsigned 会把 0x40 解码为 -64）。
11. **func 段条目顺序必须与 code 段函数体顺序逐一对齐**（W4 已踩：find_from 行被补丁脚本
    误删 → func#11 类型与体错位 → unknown local 8）。
12. wasm print 无换行（native print 有）。
13. wasmtime 勿带 ulimit -v；ponyppc 带护栏、wasmtime 裸跑，两者分开执行。
14. 长 sed -i 链式命令会被平台拦 → 用 patch 工具或拆短；大内联 heredoc 同理。

---

## 7. 语法陷阱台账（.pny 源码级）

1. if/while 必须花括号（`do` 关键字不支持——Bug#50 修复后会硬报错）。
2. 方法先定义后调用（尤其类方法）。
3. 类声明 `class Foo {` / `actor main {` 带大括号。
4. 作用域优先级：局部 > 字段 > 全局（Bug#47 修复后可靠）。
5. 内联 print 复杂算术表达式曾段错误 → 用中间变量。
6. 类型注解 `var x: T = init`；String 与 U32 槽位同为 i32，混用靠类型追踪。

---

## 8. 当前状态与缺口（2026-09-16）

### 已完成
- 0.1.0 编译器基座 → 0.2.0 App-as-test 实证期（13 bug）→ 0.2.4-0.2.8 GC/性能 →
  0.2.9-0.2.12 M3 W0-W3（wasi-p2：_start/赋值/字符串运行时/类系统）。
- 回归基线：Pony++ 175/175（含 WasmBackend gtest 5 项）+ ponydb 49/49。

### 进行中（M3-W4，内建全集 + WASI）
- 已实现：`str_field` 别名、`str_from_char`、`str_replace_all`、`json_raw_get` 手写 wasm
  运行时函数（chr/repl/json，索引 b+11..13）；func 段错位已修复待回归。
- 待做：`env_get`（WASI environ_sizes_get+environ_get）、`file_exists`（path_open 探测）、
  `sys_exec` 空返回语义确认（ponydb 仅 printenv 用法且有 fallback）。

### 待做（M3-W5）
- ponydb 完整 wasip2 落地：13 个 op 全协议 + 49 测试 wasm 版重放。
- ponyexecution push（443 超时）。

### 已知 native 后端 bug（另立 bug，不在 M3 范围）
- 类构造器初始化被丢弃；`c.count` 编译 C 报 member 访问错误（/tmp/w3c.c:728）。

---

## 9. 开发纪律（延续既有铁律）

1. 每次语言级修补必须同步 docs/00-language-spec.md + CHANGELOG.md + gtest 一起提交。
2. 绝不碰系统服务；测试进程 ulimit -v 1048576 护栏；动大内存前 free -m 确认。
3. 构建后 `nm -D` 防伪（/tmp 会被清理和污染）；会话工件落 workspace 子目录。
4. 分步落盘小脚本（write_file → 单跑 → patch 修正），绕平台命令拦截。
5. 双后端语义对齐是硬标准：native C 与 wasm 的同输入同输出逐字节验证。
6. 探针源文件先核对再排查（W3 曾因误读探针源码浪费 30 分钟）。
