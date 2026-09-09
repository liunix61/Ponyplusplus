# 00-hello: Hello World 与开发环境

## 学习目标

- 搭建 Pony++ 开发环境
- 编写第一个 Pony++ 程序
- 理解 `actor main` 是入口
- 用 `ponyppc` 编译并运行

## 核心概念

- **actor main**：Pony++ 程序的入口 actor，编译器自动识别
- **new create()**：构造方法，对象创建时调用
- **ponyppc**：Pony++ 编译器，纯 C11 实现，零外部依赖

## 代码演示

### 00-hello-world.pny

```pony
// 00-hello-world.pny - 最简单的 Pony++ 程序
actor main {
  new create() => {
    print("Hello, World!")
  }
}
```

**预期输出**：
```
Hello, World!
```

**编译**：`./bin/ponyppc build 00-hello-world.pny -o 00-hello-world`
**运行**：`./00-hello-world`

### 00-hello-native.pny

```pony
// 00-hello-native.pny - 含变量与多目标编译
actor main {
  var message: String
  var count: U32

  new create() => {
    message = "Hello, World!"
    count = 42
    print(message)
    print(count)
  }
}
```

**预期输出**：
```
Hello, World!
42
```

**编译为 native**：`./bin/ponyppc build 00-hello-native.pny --target native -o 00-hello-native`
**编译为 wasm**：`./bin/ponyppc build 00-hello-native.pny --target wasi-p2 -o 00-hello-native.wasm`

## 变体练习

1. 把 `print("Hello, World!")` 改成 `print("Hello, Pony++!")`，重新编译运行
2. 在 `count` 后面再加一个 `var name: String = "liunix"`，并 `print(name)`
3. 尝试编译 `--target browser`，查看生成的 WIT 接口

## 下一步

进入 `01-syntax/` 学习变量、控制流、函数等基础语法。
