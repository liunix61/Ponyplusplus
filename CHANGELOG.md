# Changelog

All notable changes to Pony++ are documented in this file.

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
