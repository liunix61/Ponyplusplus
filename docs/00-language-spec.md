# Phase 0: 语言规范设计

## 0.7 名字解析优先级 / Name resolution precedence

方法体内标识符解析顺序（codegen 强制）：**局部变量/形参 > 类字段 > 全局**。同名遮蔽合法：方法内 `var cur` 遮蔽字段 `cur`，引用即局部；访问被遮蔽字段须 `this.cur`（Bug#47，2026-09-15 修复）。

## 0.6 编译器环境变量 / Compiler environment variables

| 变量 | 作用 |
|---|---|
| `PONYPPC_CFLAGS` / `PONYPPC_LDFLAGS` | 追加 gcc 编译/链接旗标（挂外部 C 库） |
| `PONYPP_GC=1` | **GC 模式**（native）：生成 C 把 malloc/calloc/realloc/free/strdup 宏映射到 Boehm GC（GC_malloc/GC_realloc/GC_free/GC_strdup），并链接 `-lgc`。适用于分配密集长跑程序（泄漏修复）。头文件路径 `PONYPP_GC_CFLAGS`（如 `-I$HOME/.local/include`），库路径 `PONYPP_GC_LDFLAGS`（如 `-L$HOME/.local/lib -Wl,-rpath,$HOME/.local/lib`）。注意：GC 模式下不得混用对同一指针的 libc free。 |

## 0.1 语言定位

Pony++ 是"天生云原生"的并发编程语言，融合：
- Pony 的引用能力系统（val/ref/trn/box/tag）
- Erlang 的监督树容错
- WebAssembly 的通用沙箱

## 0.2 核心特性

| 特性 | 实现 |
|------|------|
| 并发模型 | Actor 模型 |
| 内存安全 | 能力系统（引用类型约束） |
| 目标平台 | Wasm 组件 + Native 可执行文件 |
| 容错机制 | 监督树（Erlang 风格） |
| 编译器 | 纯 C11，零外部依赖 |

## 0.3 参考语言

| 语言 | 参考点 |
|------|--------|
| Pony | Actor 模型、引用能力系统 |
| Erlang | 监督树、容错架构 |
| Rust | 类型安全、零成本抽象 |
| WebAssembly | 通用沙箱、组件模型 |

## 0.4 内建函数（以 ponyppc 源码为准，截至 0.2.0）

### 字符串
| 函数 | 签名 | 说明 |
|---|---|---|
| `str_field(s, i, sep)` | String → String | 按分隔符取第 i 段（0 起）；越界返回 "" |
| `str_hash(s)` | String → String | djb2 哈希（16 位 hex） |
| `str_to_int(s)` | String → U32 | atoi |
| `str_from_char(c)` | U32 → String | 单字符构造（0.2.0 新增） |

### 文件 IO
| 函数 | 说明 |
|---|---|
| `file_read(path)` / `file_write(path, content)` / `file_append(path, content)` | 全量读 / 覆盖写 / 追加 |
| `file_exists(path)` | 1/0 |

### HTTP（native 后端内联运行时，0.2.0 keep-alive 版）
| 函数 | 说明 |
|---|---|
| `http_post(url, body)` | POST；客户端 keep-alive（按 host:port 缓存连接）+ Content-Length 帧读取 + 失败自动重试 1 次 |
| `http_post_h(url, body, extra_headers)` | 带自定义头（0.2.0 新增，extra_headers 为 `\r\n` 分隔的头行） |
| `http_accept(port)` / `http_respond(json)` | 服务端：阻塞 accept，连接复用（poll 300ms 无数据回退新连接；请求头含 `Connection: close` 则响应后关闭）。请求头/体均**块读**（1024B/次）。**accept 只返回 body**（路由在头里，需单端点 JSON 分发） |
| `http_accept_unix(path)` / `http_respond_unix(json)` | UNIX domain socket 服务端（同 TCP 版语义：keep-alive 复用 + 块读），本机零拷贝通道（0.2.0） |
| `http_post_unix(path, body)` | UNIX domain socket 客户端 POST，返回响应体（0.2.0） |

### 系统 / 沙箱 / JSON
| 函数 | 说明 |
|---|---|
| `arg(i)` | 命令行参数（argv[0]=程序名） |
| `sys_exec(cmd)` | popen 捕获，输出截断 8KB |
| `sandbox_exec(path, fuel)` | PonyExecution WASM 沙箱执行 |
| `stdin_line()` | 读一行（去尾部换行） |
| `json_raw_get(json, key)` | 原始提取（支持嵌套对象/数组值；key 冒号跟随验证防同名值碰撞） |
| `parse_json(...)` | Ponypi M0 解析器 |

## 0.5 String / List 方法（codegen 特判）

| 方法 | 语义 |
|---|---|
| `s.len()` | strlen → U32 |
| `s.charAt(i)` | 单字节取值 → U32 |
| `s.to_string()` | 恒等（返回原串） |
| `s.startsWith(p)` | 1/0 |
| `s.toUpperCase()` | ASCII 大写 |
| `s.find(sub)` | 子串下标；未找到返回 4294967295（0.2.0 新增） |
| `s.contains(sub)` | 1/0（0.2.0 新增） |
| `s.slice(start)` / `s.slice(start, end)` | 子串；end 缺省到尾；越界钳位（0.2.0 新增，Bug#38） |
| `"a\x1fb"` | `\xNN` 十六进制转义（真二进制字节；0.2.0 修复 Bug#44） |
- **`file_size(path): U32`** — 文件字节数（不存在返回 0）。`file_append(path, data): U32` — 追加写（1=成功）。二者为 PonyDB M2 持久化页日志的内建支撑。
- **`char_code(s): U32`** — 首字节 ASCII 码（空串返回 0）。用于 PonyDB M2 页日志校验和。
- **`env_get(name): String`** — 环境变量读取，未设置返回空串。native 与 wasip2 双目标可用（wasi-libc getenv），替代 sys_exec("printenv")。
| `s1 < s2` 等关系比较 | String 关系比较按字典序（strcmp 语义；0.2.0 修复 Bug#43 — 此前裸指针比较） |
| `f(x).len/slice/find/contains(...)` | 链式方法：函数返回值上直接调用 String 内建（0.2.0 新增，Bug#41 — 此前解析器死循环） |
| `str_replace_all(s, old, new)` | 全量替换（0.2.0 新增，Bug#40） |
| `list.append(x)` | 追加；`list.len()` / `list.get(i)` / `list.set(i, v)` |

特判守卫（0.2.0 起）：receiver 为 `this` 或解析为注册类类型的字段/变量时，**跳过** String/List 特判，走统一方法分派 `{Type}_{method}(recv, ...)` — 否则类方法 `append/len/...` 会被劫持为 List 内建。

## 0.6 编译语义规则（实证沉淀）

1. **方法定义顺序**：方法体内的调用目标必须先定义（编译器不生成前向声明）；构造器不能调用后定义的方法。
2. **顶层自由函数**：`fun name(...)` 不受支持 — 调用会被错误分派为当前 actor 的方法。工具函数必须放进 class。
3. **字符串 `==`**：任意表达式两侧均按 strcmp 语义处理（不限字面量）。
4. **字符串拼接 `+`**：两侧任一为 String 即拼接；非 String 侧自动 `pny_itoa`。内联方法调用参与拼接时按 str_ret_methods 注册表判定 String 返回。
5. **局部变量表容量 256**（0.2.0 起，原 32 静默丢弃）；单个 class/actor 作用域内生效。
6. **WASM 后端**（0.2.0 起真实语义）：locals 表 / 二元运算符 / if / while / var 初始化 / signed LEB i32.const / `memory` export（WASI fd_write 需要）；运算符节点 AST 形状为 `NODE_EMPTY(data=op)`。
7. **JSON 提取**：`json_raw_get` 对嵌套对象/数组按括号深度整体提取；key 匹配带冒号跟随验证。
