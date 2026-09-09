# 附录 G：Wasm 组件模型深入

> 本附录介绍 Pony++ 编译到 WebAssembly 组件的技术细节。

---

## G.1 编译流程

```
Pony++ 源码 (.pny)
    ↓ ponyppc
AST (抽象语法树)
    ↓ codegen
Wasm 模块 (.wasm)
    ↓ 组件包装
Wasm 组件 (.ponypp)
```

---

## G.2 两个目标后端

| 目标 | 命令 | 输出 | 运行方式 |
|------|------|------|----------|
| `wasi-p2` (默认) | `ponyppc -o out.ponypp file.pny` | Wasm 组件 | `wasmtime run out.ponypp` |
| `native` | `ponyppc --target native -o out file.pny` | C 源码 | `gcc out.c -o out && ./out` |

---

## G.3 WIT 接口定义

Pony++ 使用 WIT（Wasm Interface Type）定义组件接口：

```wit
// greeter.wit
interface greeter {
  greet: func(name: string) -> string
}

world greeter-world {
  export greeter
}
```

---

## G.4 内存管理

### G.4.1 Wasm 线性内存

Wasm 使用线性内存（Linear Memory）：

- 初始大小：几 KB
- 最大大小：可配置（默认 4GB）
- 访问方式：`load` / `store` 指令

### G.4.2 Pony++ 的 GC

Pony++ 使用 Cheney 半空间复制 GC：

```
内存布局:
[已用空间][空闲空间]
    ↓ GC 触发
[新已用空间][新空闲空间]
```

---

## G.5 浏览器集成

### G.5.1 JavaScript 调用

```javascript
async function loadPonyApp() {
  const response = await fetch('app.ponypp');
  const bytes = await response.arrayBuffer();
  const module = await WebAssembly.compile(bytes);
  const instance = await WebAssembly.instantiate(module);
  
  // 调用导出的函数
  const result = instance.exports.greet("World");
  console.log(result);
}
```

### G.5.2 WASI Preview 2

Pony++ 支持 WASI Preview 2，提供：

- 文件系统访问
- 网络访问
- 环境变量
- 时钟

---

## G.6 性能特征

| 指标 | Wasm | Native |
|------|------|--------|
| 启动时间 | ~1ms | ~0.1ms |
| 执行速度 | ~80% native | 100% |
| 内存开销 | 中等 | 低 |
| 包大小 | 小 | 中等 |
| 可移植性 | 高 | 低 |

---

## G.7 调试技巧

### G.7.1 查看 Wasm 模块

```bash
wasm-objdump -x output.ponypp
```

### G.7.2 Wasmtime 调试

```bash
wasmtime run --invoke main output.ponypp
```

### G.7.3 浏览器 DevTools

Chrome DevTools 支持 Wasm 调试：
1. 打开 DevTools → Sources
2. 找到 .wasm 文件
3. 设置断点
