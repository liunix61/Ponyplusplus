# Pony++ Phase 3: 标准库、I/O、并发原语与工具链

> 设计日期: 2026-09-01  
> 状态: 设计中

---

## 1. 目标

Phase 2 完成后，Phase 3 实现：

1. **标准库 (stdlib.pny)** — 内置库，覆盖常用功能
2. **I/O 系统** — 文件、网络、日志
3. **并发原语** — 锁、通道、Future/Promise、Actor Group
4. **包管理与模块系统** — `import`、包依赖、版本管理
5. **编译器前端增强** — REPL、格式化工具、LSP 支持
6. **IDE 集成** — VS Code 插件、语法高亮
7. **性能分析工具** — `ponypp profile`

---

## 2. 标准库设计

### 2.1 模块结构

```
stdlib/
├── std/
│   ├── actor.pny      # Actor 基类、spawn、supervise
│   ├── io.pny         # 文件 I/O、网络
│   ├── concurrent/
│   │   ├── mutex.pny  # 互斥锁
│   │   ├── channel.pny # 通道
│   │   ├── future.pny # Future/Promise
│   │   └── group.pny  # Actor Group
│   ├── data/
│   │   ├── list.pny   # 动态数组
│   │   ├── map.pny    # 哈希表
│   │   ├── set.pny    # 集合
│   │   └── queue.pny  # 队列
│   ├── time.pny       # 时间、定时器
│   ├── math.pny       # 数学运算
│   ├── string.pny     # 字符串工具
│   ├── json.pny       # JSON 序列化/反序列化
│   ├── log.pny        # 结构化日志
│   └── crypto.pny     # 密码学（哈希、HMAC）
├── net/
│   ├── http.pny       # HTTP 客户端/服务器
│   ├── tcp.pny        # TCP 套接字
│   └── grpc.pny       # gRPC 客户端
└── test/
    └── testing.pny    # 测试框架
```

### 2.2 核心 API

```pony
# Actor 基类
actor Actor {
  be spawn() => {}           # 启动 Actor
  be stop() => {}            # 优雅停止
  be supervise(child: Actor) => {}  # 添加被监督子 Actor
}

# I/O
actor Stdio {
  be write(data: String) => {}
  be read() => {}
}

actor File {
  var path: String
  new create(path: String, mode: String = "r") => {}
  be open() => {}
  be close() => {}
  be read(n: U64 = 0) => {}
  be write(data: String) => {}
}

# 并发原语
actor Mutex {
  be lock() => {}
  be unlock() => {}
}

actor Channel[T] {
  be send(value: T) => {}
  be receive() => {}
}

actor Timer {
  new create(interval_ms: U64, handler: Actor) => {}
  be start() => {}
  be stop() => {}
}

# JSON
actor Json {
  static be parse(input: String) => {}
  static be stringify(obj: Any) => {}
}

# 日志
actor Logger {
  new create(level: String = "INFO") => {}
  be info(msg: String) => {}
  be warn(msg: String) => {}
  be error(msg: String) => {}
}
```

---

## 3. I/O 系统设计

### 3.1 文件 I/O

```c
/* Native 后端直接映射到 POSIX
 * Wasm 后端使用 WASI fd_read / fd_write
 */
typedef struct FileDesc {
    int fd;
    bool is_socket;
    bool is_piped;
    FILE *stream;          /* buffered stream */
    size_t buffer_size;
} FileDesc;

FileDesc *io_open(const char *path, const char *mode);
int io_read(FileDesc *fd, void *buf, size_t size);
int io_write(FileDesc *fd, const void *buf, size_t size);
void io_close(FileDesc *fd);
```

### 3.2 网络

```c
/* Wasm: WASI Preview 2 Socket API
 * Native: POSIX socket API
 */
typedef struct Socket {
    int fd;
    bool is_listening;
    bool is_connected;
    struct Socket *next;
    void (*on_accept)(struct Socket *);
    void (*on_data)(struct Socket *, const void *, size_t);
    void (*on_close)(struct Socket *);
} Socket;

Socket *net_tcp_listen(const char *host, int port);
Socket *net_tcp_connect(const char *host, int port);
int net_read(Socket *s, void *buf, size_t size);
int net_write(Socket *s, const void *buf, size_t size);
void net_close(Socket *s);
```

---

## 4. 并发原语设计

### 4.1 Mutex

```c
typedef struct PnyMutex {
    pthread_mutex_t lock;   /* Native */
    int holder_id;          /* 持有者 Actor ID */
    bool locked;
} PnyMutex;

PnyMutex *mutex_new(void);
int mutex_lock(PnyMutex *m);
int mutex_try_lock(PnyMutex *m);
int mutex_unlock(PnyMutex *m);
void mutex_free(PnyMutex *m);
```

### 4.2 Channel

```c
typedef enum {
    CHANNEL_UNBUFFERED,
    CHANNEL_BUFFERED,
    CHANNEL_BROADCAST
} ChannelType;

typedef struct PnyChannel {
    ChannelType type;
    void **queue;
    size_t capacity;
    size_t count;
    size_t head;
    size_t tail;
    int read_fd;            /* 基于 pipe 的同步 */
    int write_fd;
} PnyChannel;

PnyChannel *channel_new(size_t capacity);
int channel_send(PnyChannel *ch, const void *data, size_t size);
int channel_receive(PnyChannel *ch, void **data, size_t *size);
int channel_close(PnyChannel *ch);
```

### 4.3 Timer / Scheduler Tick

```c
/* 定时器使用事件驱动的 ticker */
typedef struct TimerEntry {
    uint64_t fire_at_ms;
    ActorRef actor;
    const char *method;
    struct TimerEntry *next;
} TimerEntry;

typedef struct TimerHeap {
    TimerEntry *head;
    uint64_t count;
} TimerHeap;

void timer_schedule(TimerHeap *heap, uint64_t delay_ms, ActorRef actor, const char *method);
void timer_tick(TimerHeap *heap);  /* 每 ms 调用一次 */
```

---

## 5. 包管理与模块系统

### 5.1 Import 语法

```pony
import std.io.File
import std.concurrent.Mutex
import myproject.models.User

actor App {
  use File
  use Mutex
  be run() => {
    val f = File("/tmp/data.txt")
    f.open()
  }
}
```

### 5.2 包解析

```c
typedef struct Package {
    char *name;
    char *version;
    char **modules;
    size_t module_count;
    struct Package **deps;
    size_t dep_count;
} Package;

typedef struct ModuleResolver {
    char **search_paths;
    size_t path_count;
    Package **loaded;
    size_t loaded_count;
} ModuleResolver;

Package *resolve_import(ModuleResolver *resolver, const char *path);
```

### 5.3 包管理文件格式 (ponypp.toml)

```toml
[package]
name = "myproject"
version = "0.1.0"

[dependencies]
std = "*"
http = ">=1.0"
json = ">=2.0"

[build]
target = "wasm"
optimize = 2
```

---

## 6. REPL 设计

```pony
$ ponypp repl
> actor Hello { be run() => { print("hi") } }
> val h = Hello.new()
> h.run()
hi
>
```

```c
typedef struct REPL {
    ModuleResolver *resolver;
    TypeContext *type_ctx;
    ASTNode **history;
    size_t history_count;
    bool multiline;
} REPL;

REPL *repl_new(void);
void repl_loop(REPL *r);
int repl_eval(REPL *r, const char *input);
```

---

## 7. 编译器前端增强

### 7.1 格式化工具 (ponypp fmt)

```
ponypp fmt file.pny        # 格式化单文件
ponypp fmt ./src/          # 递归格式化目录
```

基于 AST 重写：解析 -> 美化 AST -> 源码生成。

### 7.2 LSP 支持

```c
/* LSP 服务器，基于 JSON-RPC over stdio */
typedef struct LSPServer {
    int stdin_fd;
    int stdout_fd;
    ModuleResolver *resolver;
    TypeContext *type_ctx;
} LSPServer;

void lsp_handle_initialize(LSPServer *s, const char *params);
void lsp_handle_did_open(LSPServer *s, const char *params);
void lsp_handle_definition(LSPServer *s, const char *params);
void lsp_handle_completion(LSPServer *s, const char *params);
```

### 7.3 性能分析 (ponypp profile)

```
ponypp profile app.pny --output profile.json
ponypp profile --flamegraph app.pny
```

```c
typedef struct ProfSample {
    uint64_t timestamp;
    int actor_id;
    const char *method;
    uint64_t duration_ns;
} ProfSample;

typedef struct Profiler {
    ProfSample **samples;
    size_t count;
    size_t capacity;
    uint64_t start_time;
} Profiler;

void profiler_start(Profiler *p);
void profiler_stop(Profiler *p);
void profiler_emit(Profiler *p, const char *path);
```

---

## 8. IDE 插件

### 8.1 VS Code 扩展结构

```
vscode-ponypp/
├── package.json
├── syntaxes/
│   ├── ponypp.tmLanguage.json
│   └── ponypp-language-configuration.json
├── snippets/
│   └── ponypp.json
├── src/
│   ├── extension.ts
│   ├── language-server.ts
│   └── completion.ts
└── assets/
    └── icon.svg
```

### 8.2 语法高亮规则

```json
{
  "name": "Keyword",
  "match": "\\b(actor|be|fun|new|var|let|if|else|while|for|return|supervise|import|use|static|override)\\b",
  "scope": "keyword.control.ponypp"
},
{
  "name": "Type",
  "match": "\\b(U8|U16|U32|U64|I8|I16|I32|I64|F32|F64|String|Bool|None|Any|ActorRef)\\b",
  "scope": "storage.type.ponypp"
}
```

---

## 9. 构建系统增强

### 9.1 ponypp build

```
ponypp build               # 默认 wasm
ponypp build --target wasm
ponypp build --target native
ponypp build --optimize 3
ponypp build --debug
ponypp build --release
ponypp build --output ./dist/
```

### 9.2 交叉编译

| 源 | 目标 | 状态 |
|----|------|------|
| Native | WASI Preview 2 | ✓ Phase 1 |
| Native | Native C | ✓ Phase 1 |
| Native | Component Model | Phase 3 |
| Native | Browser WASM | Phase 3 |
| Native | MCU (ESP32) | Phase 3 |
| Native | Browser JS | Phase 3 |

---

## 10. 开发里程碑

| 里程碑 | 内容 | 估计时间 |
|--------|------|----------|
| M1 | 标准库 stubs (actor/io/concurrent) | 1 周 |
| M2 | Mutex + Channel + Timer 实现 | 1 周 |
| M3 | 包管理 (ponypp.toml, import 解析) | 1 周 |
| M4 | REPL 实现 | 3 天 |
| M5 | 格式化工具 | 3 天 |
| M6 | LSP 基础 (goto definition, completion) | 1 周 |
| M7 | VS Code 插件 | 3 天 |
| M8 | 性能分析工具 | 3 天 |
| M9 | 网络 I/O (TCP/HTTP) | 1 周 |
| M10 | WASI Component Model 支持 | 1 周 |

**总计: 约 7 周**

---

## 11. 测试计划

| 测试套件 | 测试用例数 | 覆盖模块 |
|----------|-----------|----------|
| test_stdlib | 30+ | 标准库 API |
| test_io | 20+ | 文件、网络 |
| test_concurrent | 25+ | Mutex、Channel、Timer |
| test_package | 15+ | Import、包解析 |
| test_repl | 10+ | REPL 交互 |
| test_fmt | 10+ | 格式化 |
| test_lsp | 10+ | LSP 协议 |
| test_net | 15+ | TCP/HTTP |
| test_profile | 8+ | 性能分析 |
| **合计** | **143+** | |
