# 第 13 章 IO、文件与日志

> **本章目标**
> 掌握 Pony++ 的 IO 操作——控制台输出、文件读写、结构化日志。
>
> **学习时长**：35 分钟
> **前置要求**：第 11 章
> **对应 examples**：[`examples/06-stdlib/03-log.pny`](../examples/06-stdlib/03-log.pny)

---

## 13.1 控制台输出

### 13.1.1 print 函数

`print` 是最基本的输出函数：

```pony
// print-demo.pny - print 输出

actor main {
  new create() => {
    print("Hello, World!")     // 字符串
    print(42)                  // 整数
    print(3.14)                // 浮点数
    print(true)                // 布尔值
  }
}
```

### 13.1.2 字符串拼接输出

```pony
// print-concat.pny - 拼接输出

actor main {
  new create() => {
    var name: String = "Pony++"
    var version: String = "1.0"
    print("Language: " + name)
    print("Version: " + version)

    var x: U32 = 42
    print("Value: " + x.string())
  }
}
```

---

## 13.2 结构化日志

### 13.2.1 日志级别

Pony++ 提供四个日志级别：

```pony
// log-levels.pny - 日志级别

actor main {
  new create() => {
    log_debug("debug message")     // 调试信息
    log_info("app started")        // 一般信息
    log_warn("low memory")         // 警告
    log_error("failed to connect") // 错误
  }
}
```

### 13.2.2 日志使用场景

| 级别 | 场景 | 示例 |
|------|------|------|
| `log_debug` | 开发调试 | 变量值、函数入口 |
| `log_info` | 正常运行 | 服务启动、请求处理 |
| `log_warn` | 需要注意 | 内存不足、重试 |
| `log_error` | 出错了 | 连接失败、数据损坏 |

---

## 13.3 本章实战：请求日志记录器

```pony
// request-logger.pny - 请求日志记录器

actor RequestLogger {
  var total: U32
  var errors: U32

  new create() => {
    total = 0
    errors = 0
    log_info("RequestLogger initialized")
  }

  be log_request(method: String, path: String) => {
    total += 1
    log_info("[" + total.string() + "] " + method + " " + path)
  }

  be log_error(method: String, path: String, code: U32) => {
    total += 1
    errors += 1
    log_error("[" + total.string() + "] " + method + " " + path + " -> " + code.string())
  }

  be summary() => {
    log_info("Total: " + total.string() + ", Errors: " + this.errors.string())
  }
}

actor main {
  new create() => {
    var logger: RequestLogger = RequestLogger()
    logger.log_request("GET", "/api/users")
    logger.log_request("POST", "/api/orders")
    logger.log_error("GET", "/api/items", 500)
    logger.summary()
  }
}
```

---

## 13.4 习题

**习题 13.1** 写一个 actor，将所有 `log_warn` 和 `log_error` 消息同时输出到控制台和统计计数。

**习题 13.2** 实现一个简单的访问日志格式：`[时间] 方法 路径 状态码 耗时`。

---

## 13.5 下一步

→ **第 14 章：JSON 与时间**
