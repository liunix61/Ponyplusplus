# Pony++ Examples

Pony++ 语言示例程序集合，覆盖从入门到综合项目的所有场景。

## 快速开始

```bash
# 编译
./bin/ponyppc -o output.ponypp examples/<dir>/<file>.pny

# 运行 (Wasm)
wasmtime run output.ponypp

# 编译 (native)
./bin/ponyppc --target native -o output examples/<dir>/<file>.pny
gcc output.c -o output && ./output
```

## 目录索引

| 目录 | 主题 | 文件数 | 前置 |
|------|------|--------|------|
| [00-hello](00-hello/) | Hello World 入门 | 4 | 无 |
| [01-syntax](01-syntax/) | 变量/控制流/函数/对象/列表 | 5 | 00-hello |
| [02-actor](02-actor/) | Actor 模型/消息传递/多 actor | 3 | 01-syntax |
| [03-capabilities](03-capabilities/) | val/ref/trn/iso/box/tag 能力 | 3 | 02-actor |
| [03-concurrency](03-concurrency/) | Channel/Future/ActorGroup | 3 | 03-capabilities |
| [05-supervisor](05-supervisor/) | 监督树/容错/错误恢复 | 3 | 03-concurrency |
| [06-stdlib](06-stdlib/) | String/JSON/Log/Time | 4 | 02-actor |
| [07-ffi](07-ffi/) | FFI 外部接口 | 1 | 06-stdlib |
| [08-mcu](08-mcu/) | STM32/ESP32 嵌入式 | 2 | 06-stdlib |
| [09-web](09-web/) | 浏览器 WASM 组件 | 2 | 06-stdlib |
| [10-mini-projects](10-mini-projects/) | 综合项目 | 3 | 05-supervisor + 06-stdlib |

**总计**: 35 个 example, 全部编译通过

## 分类浏览

### 入门级 (L0-L1)
- `00-hello/` — Hello World, 变量, 字符串拼接
- `01-syntax/` — 类型系统, if/match/while, 函数, class, List

### 中级 (L2-L4)
- `02-actor/` — actor 模型, be/fun, 消息传递
- `03-capabilities/` — 引用能力系统 (Pony++ 核心)
- `03-concurrency/` — Channel, Future, ActorGroup

### 进阶级 (L5-L6)
- `05-supervisor/` — 监督树, 容错策略
- `06-stdlib/` — 标准库 API

### 高级 (L7-L9)
- `07-ffi/` — C 函数调用
- `08-mcu/` — 嵌入式硬件控制
- `09-web/` — 浏览器组件

### 综合项目 (L10)
- `10-mini-projects/01-counter-service.pny` — 计数器服务
- `10-mini-projects/02-key-value-store.pny` — 键值存储
- `10-mini-projects/03-reactive-pipeline.pny` — 响应式管道

## 与书籍的对应关系

每个 example 目录对应《Pony++ 程序设计语言》的一个章节:

| Example | 书章节 |
|---------|--------|
| 00-hello | 第1章: 入门 |
| 01-syntax | 第2-5章: 语言基础 |
| 02-actor | 第6-7章: Actor 模型 |
| 03-capabilities | 第8章: 能力系统 |
| 03-concurrency | 第9章: 并发原语 |
| 05-supervisor | 第10章: 监督树 |
| 06-stdlib | 第11-14章: 标准库 |
| 07-ffi | 第15章: FFI |
| 08-mcu | 第16章: MCU 嵌入式 |
| 09-web | 第17章: 浏览器 WASM |
| 10-mini-projects | 第18-20章: 综合实践 |

详见: [`docs/book-plan.md`](../docs/book-plan.md)

## 验证

```bash
# 编译所有 example
for f in $(find examples -name "*.pny"); do
  ./bin/ponyppc -o /tmp/v.ponypp "$f" 2>&1 | tail -1
done
```

## 贡献

添加新 example:
1. 在对应目录创建 `.pny` 文件
2. 顶部注释说明: `// <name>.pny - <一句话说明>`
3. 用 `// [CONCEPT]` 标注关键概念
4. 确保 `ponyppc -o test.ponypp <file>.pny` 编译通过
