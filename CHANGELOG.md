# Changelog

All notable changes to Pony++ are documented in this file.

## [0.2.16] - 2026-09-17

### Added
- **`extern fun` FFI 声明（nanonode P1 地基）**：顶层 `extern fun name(p: T, ...): Ret` 声明外部 C 函数。
  - lexer：`extern` 关键字；parser：parse_program extern 分支 → NODE_EXTERN（data=函数名，children=[params容器, 返回类型]）
  - native codegen：发射 C 原型（Pony 类型→C 类型经 cg_builtin_type：String→`const char *`/U32→`unsigned int`/I64→`signed long long` 等）+ 调用点直发 C 调用（NODE_CALL 命中 extern 注册表优先于内建分派）
  - 链接外部库：`PONYPPC_LDFLAGS=/path/lib.o`（已有通道）
  - 实测：toy_hash/toy_verify 双函数 FFI 调用输出正确（nanonode probe/ffi2.pny）
  - gtest：Codegen.ExternFunFFI（原型字节+直调分派）
  - wasi-p2 端暂不支持（NODE_EXTERN 被忽略，后续 W 阶段映射 wasm import）

## [0.2.15] - 2026-09-17

### Added
- **W5（M3 路线图收尾）：ponydb 全功能 wasi-p2 通过** — 16 项编译器修复驱动 928 行 ponydb.pny（B+树/MVCC 快照/WASI 持久化）在 wasmtime 25 上全链正确：put/get/del/count/stat/dump/snapshot/getat/rangeat/close/readers + 重启恢复 + 相对/绝对路径落盘。
- **break/continue 语句（Bug#55）** — break/continue 非 lexer 关键字，parser 产出 `NODE_IDENT(data="break/continue")`；native 靠 C 关键字裸输出蒙对，wasm 端被当变量静默丢弃（`while i < 1000000 { ... if e > 4000000000 { break } }` 退不出循环 → okc 恒 512 → 每个 put 误 split → get MISS）。WasmGen 新增 loop/if 深度追踪栈（loop_depth/brk_tgt/cont_tgt），NODE_WHILE/NODE_IF 发射点维护相对深度，break=br 跳出外层 block、continue=br 回 loop 头。
- **main actor 方法分派（Bug#56）** — 收集器显式跳过 main actor，其方法不进类表不发射，`this.m()` 在 main 上下文（cur_class=-1）静默发 `i32.const 0`（`this.u32_to_str(v.len())` 整体消失）。main 收为伪类（`is_main` 标记）：入口方法（`fun ref main`/`new create`）不入表不发射（已由 func 0 承担）、方法无 self 签名（参数从 local 0 起，0 参→type 0，N 参→ty[N-1]）、分派时不发 receiver；w3_try_class_call/wasm_expr_is_str 的 `this` 分支增加 main 类 fallback。
- **rt_strcmp 字符串字典序比较（Bug#58，b+20）** — 6 种比较运算（`==/!=/</>/<=/>=`）此前对 String 操作数直接发 i32 指针比较（比地址），`leaf_put` 的 `k > key` 有序插入全乱（新键永远排最前 → root 页只剩最后一个键 → 多键 get 全 MISS）。新增 strcmp 运行时（逐字节无符号，返回 -1/0/1），二元比较发射点两处（emit_expr/emit_stmt）在两操作数均为 String 时改发 `call strcmp; i32.const 0; <cmp>`。类 fn_base 后移 b+21。
- **rt_resolve 相对路径支持（Bug#59）** — resolve 只做 preopen 前缀匹配，相对路径（`PONYDB_PATH=w5db`）无匹配返回 0 → file_append 全失败（文件从未创建）。非 `/` 开头路径直接走 preopen fd=3 + 原路径。

### Fixed
- **wasm 端 else 块整块发射失败（Bug#57）** — parser 把 else 包进 `NODE_ELSE` 节点（child[2]=NODE_ELSE→block），wasm emit_stmt 没有 NODE_ELSE case → fallback `i32.const 0; drop`，else 分支静默死亡（`if v == "" { MISS } else { VAL }` 永远走不出 else → get 恒 MISS）。NODE_IF 发射处解包 NODE_ELSE 的 block。
- **2 参 slice 形态** — `data.slice(1)`（省略 end=到末尾）不满足 3 参分派条件发 0；两处分派点（裸名+recv 形态）补 2 参 → rt_slice en=-1 末尾语义。
- **`e > 4000000000` 字面量超 int32** — `if e > 4000000000 { break }` 发有符号比较恒 false（哨兵值判断失效）；U32 局部变量比较改发 GT_U/LT_U/GE_U/LE_U（local_is_u32[64] + wasm_expr_is_u32 两处发射点）。
- **wasm_expr_is_str 缺 recv String 判定** — `this.f.slice()`/String 字段 receiver 的 builtin 方法被当整数；wasm_try_builtin_call/wasm_expr_is_str 的 split_dot 分支增加 local_is_str/字段 is_str 判定。
- **NODE_BOOL 发射** — `done == false` 中 Bool 字面量被当字符串处理（strcmp 比较），改为 bool 字节语义。
- **contains 内联** — `raw.contains("\x01")` 分派缺失 → find 组合内联。
- **fields/methods[32]→[64]** — ponydb BTree 40+ 方法静默丢弃（分派发 0）。
- **rt_alloc memory.grow 兜底** — ponydb 大字符串拼接超 16 页静态内存即 trap；alloc 前检查 `global0+n > size<<16` 则 grow。
- **memory 段页数动态化** — 静态 16 页不够装 ponydb 字面量区；预扫描字符串池算页数。
- **wasm 入口 create fallback** — ponydb 入口是 `new create()` 而非 `fun ref main()`，入口查找增加 main actor create 兜底。
- **type 段 12 = 5 参方法** — 4 参类方法（self+4）钳位错位。

### Changed
- 索引终态（wasi-p2 b=15）：import 0-14、main=14、print=15、itoa=16、alloc=17、strlen=18、concat→19…（W4b 布局）+ strcmp=b+20，类方法从 b+21=36 起；type 13 个；func/code 段计数 22+ncf。gtest 字节断言更新（ctor=36/add·greet=37），175/175 ✓。
- ponydb native 49/49 回归 ✓；wasmtime e2e：put×3/get×3/count/stat/del/dump/snapshot/getat/rangeat/close/readers/重启恢复 全绿。
- ponydb.wasm 76+ bodies，w5scan violations=0。

### Known Issues
- Bug#52：单字符字符串字面量 `print("[")` 在 wasi-p2 无输出（疑似 lexer/字符串池 bug，待查）。
- 输出尾杂散 `0`（每次 print 后多一个 0，疑 drop 缺失，不影响功能判定）。
- main actor 字段（this.f）在伪类模式下未初始化——ponydb main 无字段，通用场景待补。
- Bug#53（NODE_BOOL strcmp）、#54（字段名 tag 撞能力关键字）沿袭。

## [0.2.14] - 2026-09-16

### Added
- **W4b（M3 路线图）：wasm 系统接口全集（env/file/sys_exec）+ preopen 绝对路径解析** — 5 个新运行时函数：`env_get`（environ_sizes_get+environ_get 线性扫描 `K=V`，缺省返回 ""，b+14）、`file_exists`（path_filestat_get errno==0 → 1，b+15）、`file_read`（path_open+fd_read 循环防短读+bump alloc 拼接，b+16）、`file_append`（O_CREAT+fd_seek(END)+fd_write，b+17）、`sys_exec`（wasm 沙箱安全返回 ""，ponydb printenv 有 fallback，b+18）。import 11→14（+environ_sizes_get/environ_get/path_open/fd_close/path_filestat_get/fd_filestat_get/fd_prestat_get/fd_prestat_dir_name/fd_seek，P2/P3 统一），type 9→12（+path_open 9 参/path_filestat_get 5 参/fd_seek i64 混参）。分派表+json_raw_get 表各 +5 条目。
- **rt_resolve：preopen 绝对路径解析（b+19）** — `(path, out_fd_ptr) → relpath_ptr`。wasmtime 25 沙箱硬约束：preopen 目录外绝对路径一律 errno 63=ENOTCAPABLE（fd3+`/tmp/x` 也拒），wasi-libc 的做法是运行时剥前缀。rt_resolve 遍历 fd 3..27 调 fd_prestat_get（tag==0 目录）→ fd_prestat_dir_name 取目录名 → 最长前缀匹配（边界检查：路径后继须 `/`/NUL，或目录名自带 `/`）→ 返回 (preopen fd 写 *out_fd_ptr, 相对路径指针)。arena 技巧：存/恢复堆指针（global 0）实现 namebuf 零泄漏。三个文件调用点（fexists/fread/fappend）先 resolve 再操作。
- **wasm 内存布局**：iovec@8/12、nwritten@24、environ 计数@28/大小@32、fd_seek newoff@32、fd 槽@36、prestat tag@40/name_len@44、resolve out_fd@48、statbuf/sz/数据区 alloc 从 bump 堆。

### Fixed
- **wasmtime 25 的 fdflags=APPEND 不生效**（fd_write 从偏移 0 覆盖写）— wat 级探针实锤（O_CREAT|APPEND 两次写只留后者）；fd_seek(fd,0,SEEK_END=2) 官方语义兜底，rights 相应加 FD_SEEK。ponydb persist 日志从此无丢行。
- http.c 缺 `_POSIX_C_SOURCE 200809L`/`<strings.h>`、incremental.c 缺 `<time.h>`（预存构建问题）。
- `WASM_OPCODE_EQZ` 宏不存在 → `WASM_OPCODE_I32_EQZ`（2 处）。

### Changed
- 索引全体后移两次：W4b 基线（11 imports → main=11/print=12）→ preopen 解析（13 imports → main=13/print=14）→ fd_seek（14 imports → main=14/print=15，类 fn_base=b+20=35）。w3_collect_classes 硬编码基址同步。gtest 字节断言全量更新（concat=18/print_str=22/field=25/slice=20/find_from=24/chr=26/repl=27/json=28/ctor=35/add·greet=36），19/19 ✓。
- ponydb 49/49 回归 ✓；wasmtime e2e 全绿：env_get（需 `--env` 显式注入）、file_exists/read/append（`--dir=/tmp`）、sys_exec→""。

### Known Issues
- Bug#52：单字符字符串字面量 `print("[")` 在 wasi-p2 无输出（疑似 lexer/字符串池 bug，待查）。

## [0.2.13] - 2026-09-16

### Added
- **W4（M3 路线图）：wasm 内建全集（纯字符串部分）** — 3 个新运行时函数：`chr`（alloc(2)+单字节转串，b+11）、`repl`（find_from/slice/concat 组合扫描替换，b+12）、`json`（`pny_json_raw_get` 语义对齐：找 `"key":` 带后随冒号检查（Bug#29 防键名碰撞）→ `{`/`[` 深度扫描（in_str/esc 跟踪）/ 引号串剥引号 / 标量扫到 `,}]` 尾部去空白，b+13）。分派表新增 `str_from_char/str_replace_all/json_raw_get`，`str_field` 别名到 field。类 fn_base 后移 b+14。wasmtime 实测 w4_probe 五行全对：`Hi,PonyDB`（类+String 字段+return）、`b`（str_field）、`AB`（chr+concat）、`a-b-c`（repl）、`k1`（json 剥引号）。

### Fixed
- **Bug#51：wasm 端 return 语句静默丢失** — parser 的 return 是 `NODE_EMPTY(data="return")` 而非 `NODE_RETURN`（parser 内 NODE_RETURN 分支是死代码，永不生成），native codegen 认前者而 wasm 端不认，所有带返回值的方法/函数返回兜底 0。emit_expr NODE_EMPTY 分支新增 `"return"` 识别：发射实参表达式（或 0）+ return 指令。wasmtime 实测 w4h=`5|7`、w4g=`Hi|Hi,PonyDB|PonyDB`。
- **wasm rt_repl 拼接错源** — 替换分支拼的是 old（local 1）而非 what（local 2），输出原样返回。
- **wasm rt_json 引号串分支未剥引号** — slice 起点用了开引号位置，改为 p+1。
- **wasm rt_json/rt_repl slice 调用参数错** — 误传 `s+p` 指针算术（2 参），rt_slice 语义是 `(s, st, en)` 绝对坐标 3 参；三处调用点全部修正。

### Changed
- gtest WasmBackend.ClassSystemDispatch 类方法索引 17/18 → 20/21（fn_base 后移）；新增 WasmBackend.W4BuiltinsAndReturn（chr/repl/json 分派 + return 指令 + ctor/greet 分派字节校验）。

## [0.2.12] - 2026-09-16

### Added
- **W3（M3 路线图）：wasm 类系统** — 类字段布局（4 字节槽顺序排布，String=i32 指针）、构造器（alloc+init，返回 self 指针）、实例方法（self=local 0，独立 wasm 函数）、字段读写（i32.load/store align=2 offset=字段偏移）。表达式层分派：`C()`/`C.create()`→构造器、`c.m(args)`→类方法、`this.m(args)`→self 方法、`this.f`/裸字段名（方法体内）/`c.f`→字段访问。局部变量类类型追踪（local_class）+ 方法返回 String 判定。类收集先于 type/func 段发射（段计数依赖方法数）。wasmtime 实测：`HELLO1`（ctor+inc 字段读写）、`6`（ctor+add 链）、`PonyDBW3-OK7`（String 字段读写/传参/返回）全对。gtest WasmBackend.ClassSystemDispatch（ctor/add 分派 + store/load 字节校验）。

### Fixed
- **wasm 类调用零参实参容器误发射** — `c.inc()` 的空 args 容器（child_count=0）未展开，容器节点被当表达式发射成 const 0，栈上多一值致函数体校验失败。现容器判断与 child_count 无关。
- **wasm String 字段在 main 中误判 int** — wasm_expr_is_str 的字段分支带 `cur_class >= 0` 门槛，main（cur_class=-1）里 `h.name` 落 itoa 输出指针值。门槛移除，由 w3_resolve_field 内部判断。

## [0.2.11] - 2026-09-16

### Added
- **W2（M3 路线图）：wasm 字符串运行时** — 手写 10 个 wasm 运行时函数（alloc/strlen/concat/itoa/slice/streq/print_str/find/find_from/field）+ global 0 bump 分配器（main 开头初始化为字面量区末尾 8 对齐，内存 16 页）。`+` 自动分派 concat、`==`/`!=` 分派 streq、print(String) 走 print_str（strlen+fd_write）、print(int) 走 itoa。内建 `field/slice/find/find_from/len` 实参为 NODE_EMPTY 容器打包，分派前展开。wasmtime 实测：`PonyDB!42-701`（concat/itoa/负数/streq）与 `v1v2PonyDB37-16`（field/slice/find_from/len）全对。gtest WasmBackend.StringRuntimeDispatch + StringBuiltinsDispatch。

### Fixed
- **wasm opcode GT_S/LE_S/GE_S 定义错位**（0x49/0x4A/0x4B → 真值 0x4A/0x4C/0x4E；0x49/0x4B/0x4D/0x4F 是无符号变体）。此前 `>=`/`<=` 发射 gt_u/gt_s 错误指令。
- **wasm main 体空壳** — main 发射 walk 只认 NODE_NEW，`fun ref main` 是 NODE_FUN 从未发射（早前 e2e 通过系 stale binary）。现认 NODE_FUN/NODE_BE/NODE_NEW 且名为 main。
- **wasm memop 缺 align/offset 立即数**（i32.store/load8_u/store8）——运行时函数写内存全量补齐。
- **wasm alloc 缺 global.get/set 索引字节**（0x23/0x24 后必须跟 0x00）与函数索引错位一格。
- **wasm find_from 内层循环 br 0 在 if 块内指向 if 自身**（永远走不匹配路径）→ br 1。

## [0.2.10] - 2026-09-15

### Fixed
- **Bug#50：parse 错误静默吞掉** — set_error 置标志但 AST 非 NULL 即继续编译，坏语法产出坏二进制（如 `while ... do` 缺 `{`）。新增 `parser_has_error()`，ponyppc 硬失败。gtest Parser.HasErrorFlag + NoErrorOnValidBraces。

### Added
- **W1（M3 路线图）**：wasm 后端赋值 `x = e`、复合赋值 `+= -= *= /=`、一元 `not`/`neg` 发射（此前赋值被静默丢弃）。wasmtime 实测：while+if/else+负数全通。gtest WasmBackend.AssignEmitsLocalSet。

## [0.2.9] - 2026-09-15

### Fixed
- **Bug#46 W0：wasi-p2 缺 `_start` 导出** — 模块只导出 `main`，`wasmtime run` 找不到命令入口静默零输出。现双导出 `_start`+`main`（WAMR e2e 兼容保留）。wasmtime 25.0.2 已装入 ~/.local/bin（aarch64 预编译，ghfast.top）。gtest WasmBackend.WasiStartExport。

## [0.2.8] - 2026-09-15

### Changed
- **P1-4：GC atomic 字符串分配** — PONYPP_GC 模式下纯字符缓冲（str_concat/itoa/from_char/slice/field/hash）改走 `GC_malloc_atomic`（免保守扫描），新增 `pny_xmalloc` 宏（非 GC 模式退化为 malloc）。ponydb 实测 10k put 8.2→4.8s / 20k 37.9→22.3s（各 -41%）。gtest GcAtomicStrings + NoGcXmallocFallback。

## [0.2.7] - 2026-09-15

### Fixed
- **Bug#48：`%` 取模静默失效** — lexer 有 TK_PERCENT 但 parser 乘法层不含该 token，`pos % 2` 被静默编译成 `pos`（残留 token 被语句恢复吞掉）。修复：parse_mul_expr 接收 `%` + codegen 算术分派含 `%`（gtest Codegen.ModuloOperator；ponydb internal_child 奇偶判定曾被此 bug 打穿）。

## [0.2.6] - 2026-09-15

### Added
- **`str.find_from(sub, off)` 内建**：从偏移 off 起查找子串，未找到/越界返回 4294967295。为单趟增量字段扫描设计（ponydb 解析热点：str_field(i) 每次从头扫 = O(n^2)，find_from 单趟 O(n)）。链式/非链式双分派点（gtest Codegen.FindFromBuiltin）。

## [0.2.5] - 2026-09-15

### Fixed
- **Bug#47：裸局部变量被同名类字段劫持** — `var cur: U32` 在含同名字段 `cur` 的类方法内，引用/传参被 codegen 编译成 `self->cur`（指针当值）。修复：`cg_emit_field_access` 局部变量/参数表优先于字段表（gtest Codegen.LocalShadowsField；ponydb min_key_of 曾被此 bug 打穿，绕行改名 cid 可还原）。

## [0.2.4] - 2026-09-15

### Added
- **PONYPP_GC=1 GC 模式**（native 后端）：生成 C 注入 `#include <gc/gc.h>` + malloc/calloc/realloc/free/strdup 宏映射 Boehm GC（GC_malloc 等），ponyppc 自动加 `-lgc`；头/库路径经 `PONYPP_GC_CFLAGS`/`PONYPP_GC_LDFLAGS`。背景：native 运行时此前纯 malloc 无 free（泄漏引擎），ponydb 800 键耗尽 8GB 机器触发 OOM killer 误杀网关进程；GC 模式实测 20000 键在 1GB 护栏内 52.3s 全量正确（gtest Codegen.GcModeMacros）。

## [0.2.0] - 2026-09-14

App-as-test 实证期（PonyHarness H0-H4 + PonyAgents M1/M2 驱动），累计修复 13 个真实 bug。

### Added
- `str_from_char(c)`：单字符构造内建（Bug#37）
- `str_replace_all(s, old, new)`：全量替换内建（Bug#40 — 此前无分派被误分派成类方法，现直连运行时）
- 类型推断修复（Bug#39）：`recv.slice(...)` 点号形态此前绕过 String 返回表，concat 内联被 itoa 包裹成数字
- `env_get(name): String` 内建：环境变量读取（native/wasi-libc getenv）— wasip2 零信任部署替代 sys_exec(printenv)
- 修正 is_string 名单：file_size/char_code（U32 返回）误入字符串名单已撤（Bug#45 段错误诱因之一）
- `char_code(s): U32` 内建：首字节 ASCII 码（空串=0）— 页日志校验和用
- `file_size(path)` 内建：返回文件字节数（不存在=0）；`file_append(path, data)` 确认可用（复用 PEX 时代运行时）— PonyDB M2 COW 页文件地基
- `\xNN` 十六进制转义（Bug#44）：字符串/字符字面量此前无 'x' 分支，反斜杠被吞成字面 x — 现支持真二进制字节
- 字符串关系比较修复（Bug#43）：`< > <= >=` 对 String 此前落裸 C 指针比较（仅 ==/!= 有 strcmp），字面量/同缓冲区偶然正确、分配字符串必错 — 现全部比较统一 strcmp
- 链式方法（Bug#41）：`f(x).method(args)` 此前**解析器死循环**（TK_DOT 未消费）；现 parser 补链式后缀 + codegen 新约定 `.method`（receiver 为表达式）分派 len/slice/find/contains
- `s.find(sub)` / `s.contains(sub)`：String 方法（Bug#37）
- `s.slice(start[, end])`：子串内建（Bug#38 — 此前未知 String 方法被静默丢弃生成空表达式；现钳位语义 + 缺省 end 到尾）
- `http_post_h(url, body, extra_headers)`：带自定义头 POST（H2）
- HTTP 运行时 keep-alive：客户端连接缓存（按 host:port）+ Content-Length 帧读取 + 失败重试 1 次；服务端连接复用（poll 300ms 回退）+ `Connection: close` 语义
- HTTP 服务端块读优化：请求头/体 1024B/次读取（替代逐字节 read，~150 syscall → ~2）
- UNIX domain socket 三内建：`http_accept_unix(path)` / `http_respond_unix(json)` / `http_post_unix(path, body)` — 本机零拷贝通道，语义同 TCP 版（keep-alive 复用 + 块读）
- wasm 后端真实语义（Bug#31-33）：locals 表、二元运算符、if/while、var 初始化、`memory` export、signed LEB i32.const
- 链式解析基础设施 `cg_chain_resolve` + 全局类型字段表 `type_field_types`（Bug#25/26）
- String 返回方法注册表 `str_ret_methods`（Bug#16/36）

### Fixed
- Bug#25：三段链 String 判定 warning
- Bug#26：三段方法调用 `bus.sandbox.run()` 分派错误；recv_out 漏拼后续段
- Bug#27：parser 字段链后接方法调用被拆两段
- Bug#28：cstr_escape 不转义 `\r`
- Bug#29：`pny_json_raw_get` 同名值碰撞（key 冒号跟随验证）
- Bug#30：字符串 `==` 只认字面量 → 全面表达式判定
- Bug#31：wasm 后端缺 `memory` export（WASI fd_write 需要）
- Bug#32：i32.const 误用 unsigned LEB（0x40 解码为 -64）→ signed LEB
- Bug#33：wasm 运算符 stub 成 const 0 / if 无条件 / 无 locals
- Bug#34：String/List 内建特判劫持类方法（`this.field.append()` → `pny_list_append` 堆损坏）→ receiver 类型守卫
- Bug#35：局部变量表 32 上限静默丢弃 → String 变量被误判 int；上限提至 256
- Bug#36：concat 内联方法调用 String 返回误判 int（`cg_expr_is_string` 补 str_ret_methods 查询）

### Semantics
- 详见 docs/00-language-spec.md §0.4-0.6（内建清单 / String 方法 / 编译语义规则）

## [0.1.0] - 2026-09-07

### Added
- Pony++ 编译器 `ponyppc`：纯 C11 实现，零外部依赖
- Native backend：编译为本地可执行文件
- Wasm backend (Path B)：编译为 WASM 组件
- 引用能力系统：val/ref/trn/box/tag
- Actor 模型：actor 声明、be/fun 方法、new 构造器
- 标准库：list、json、concurrent、actor、time、log、io、math、string
- Bootstrap 自举：编译器自编译闭环
- 分布式运行时：actor 网络通信
- WIT 接口生成
- REPL 交互模式

### Fixed
- typecheck: `this.field` 前缀剥离，修复 unknown identifier 误报
- codegen: `be` 方法返回类型传播（actor 类型 → void *，泛型 List[T] → PnyList *）
- parser: TK_CAP 在表达式中作标识符，修复解析死循环
- lexer: 单引号多字符字符串识别为 TK_STRING（Pony 语言规范）
- stdlib: actor.pny ActorRef → Actor 自引用，消除跨文件依赖
- stdlib: concurrent.pny pny_list_len/pny_list_get/return 类型传播

### Architecture
- 7 层编译管线：Lexer → Parser → Typecheck → Capability → Codegen → Link → Output
- 2 个后端：Native (C11) + Wasm (Binaryen)
- 模块化：28 个 C 源文件，每个模块独立可测
