# M3: wasi-p2 后端百分百实现路线图 / WASI-p2 Backend Roadmap

> 日期: 2026-09-15 · 状态: **W0 完成，W1-W5 待做**
> Bug#46 台账: ponydb.wasm 此前为 3.4KB 空壳（类程序被静默丢弃）
> 运行时: wasmtime 25.0.2 (aarch64, ~/.local/bin/wasmtime, ghfast.top 预编译)

## 现状盘点（W0 后）

wasm.c 后端（797 行）当前支持：
- print(字符串字面量 / i32 表达式)、var 局部变量、i32 算术/比较、if/while（Bug#33）
- WASI imports: fd_write/fd_read/proc_exit/clock_time_get/random_get
- 导出: memory + main + **_start（W0 新增）**

明确缺口（ponydb 所需）：
1. **字符串运行时**：线性内存分配器、字符串值（拼接/切片/查找/itoa/比较/替换）
2. **类系统**：类=线性内存结构体，new/方法调用/字段读写/方法表
3. **内建全集**：~40 个（str 系、file IO via path_open、env via environ_get）
4. **执行沙箱语义**：exec 子进程类内建在 wasm 无对应 —— 文档标注不支持（诚实边界）

## 阶段计划

| 阶段 | 内容 | 验收 |
|---|---|---|
| **W0 ✓** | `_start` 导出 + wasmtime 就位 | `wasmtime run plain.pny` 输出正确；03dad64 |
| **W1** | 表达式/控制流/顶层 fun 补全：U64/I64、字符串比较、return 链、多语句块 | gtest 扩容 + e2e 程序 20 个 |
| **W2** | 字符串运行时：bump 分配器 + concat/slice/field/find/find_from/itoa/contains/replace + fd_write print | 与 native 后端同套 gtest 行为对拍 |
| **W3** | 类系统：struct 布局、new、方法直调（静态分派先行）、字段读写、字符串字段 | wasm_probe.pny 类程序在 wasmtime 输出 CLASS-OK |
| **W4** | 内建全集 + WASI：file_read/write（path_open）、env_get（environ_get）、stdin_line（fd_read） | ponydb 依赖子集全覆盖 |
| **W5** | ponydb 落地：ponydb.pny 编 wasi-p2，wasmtime 跑 m0 测试子集（内存模式） | m0-core/m0-basic/m0-5xx 系列在 wasm 下通过 |

## 纪律

- 每阶段: gtest + spec + CHANGELOG + ctest 全绿才提交
- native 后端零回归（175/175 是硬门）
- 内存护栏：wasmtime 运行 ponydb 用 `--max-memory-size` + ulimit -v 1048576
- /tmp 污染纪律照旧：wasm 产物用后即删，复现脚本落 workspace
