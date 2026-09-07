# Pony++ 快速开始

## 构建

```bash
make all
```

## 测试

```bash
make test
```

## 运行

```bash
./bin/ponyppc examples/hello.pny -o /tmp/hello
/tmp/hello
```

## 清理

```bash
make clean
```

## 常用命令

| 命令 | 说明 |
|------|------|
| `make all` | 构建编译器 |
| `make test` | 运行单元测试 |
| `make lint` | 代码检查 |
| `make clean` | 清理构建产物 |
| `make help` | 显示帮助 |

## 编译目标

| 目标 | 说明 |
|------|------|
| `--target native` | 编译为本地可执行文件 |
| `--target wasi-p2` | 编译为 WASI Preview 2 组件 |
| `--target component` | 编译为 WebAssembly 组件 |
| `--target browser` | 编译为浏览器组件 |
| `--target mcu-wasm` | 编译为 MCU WASM |

## 示例

```bash
# 编译为 native
./bin/ponyppc examples/hello.pny --target native -o /tmp/hello_native

# 编译为 wasm
./bin/ponyppc examples/hello.pny --target wasi-p2 -o /tmp/hello.wasm

# 生成 WIT 接口
./bin/ponyppc examples/hello.pny --wit-only -o /tmp/hello.wit

# Bootstrap 自举
./bin/ponyppc bootstrap
```
