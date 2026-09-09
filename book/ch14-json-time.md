# 第 14 章 JSON 与时间

> **本章目标**
> 掌握 JSON 的解析与生成，了解时间处理函数。
>
> **学习时长**：30 分钟
> **前置要求**：第 11 章
> **对应 examples**：[`examples/06-stdlib/02-json.pny`](../examples/06-stdlib/02-json.pny)、[`examples/06-stdlib/04-time.pny`](../examples/06-stdlib/04-time.pny)

---

## 14.1 JSON 处理

### 14.1.1 解析 JSON

```pony
// json-parse.pny - 解析 JSON

actor main {
  new create() => {
    var json_str: String = "{\"name\":\"Pony++\",\"version\":\"1.0\"}"
    var obj: JSON = parse_json(json_str)
    print(obj.get("name"))      // Pony++
    print(obj.get("version"))   // 1.0
  }
}
```

### 14.1.2 生成 JSON

```pony
// json-create.pny - 生成 JSON

actor main {
  new create() => {
    var obj: JSON = JSON()
    obj.set("name", "Pony++")
    obj.set("version", "1.0")
    obj.set("stable", "true")
    print(obj.to_string())
  }
}
```

### 14.1.3 JSON API

| 函数/方法 | 说明 |
|-----------|------|
| `parse_json(str)` | 解析 JSON 字符串 |
| `JSON()` | 创建空 JSON 对象 |
| `obj.get(key)` | 获取字段值 |
| `obj.set(key, value)` | 设置字段 |
| `obj.to_string()` | 序列化为字符串 |

---

## 14.2 时间处理

### 14.2.1 获取当前时间

```pony
// time-basic.pny - 时间基础

actor main {
  new create() => {
    var now: U64 = time_now()
    print(now)

    var elapsed: U64 = time_elapsed(now)
    print(elapsed)
  }
}
```

### 14.2.2 时间 API

| 函数 | 返回类型 | 说明 |
|------|----------|------|
| `time_now()` | `U64` | 当前时间戳（毫秒） |
| `time_elapsed(start)` | `U64` | 从 start 到现在的耗时（毫秒） |

---

## 14.3 本章实战：API 响应构建器

```pony
// api-response.pny - API 响应构建器

actor ApiResponse {
  new create() => {}

  be success(data: String) => {
    var obj: JSON = JSON()
    obj.set("status", "ok")
    obj.set("data", data)
    print(obj.to_string())
  }

  be error(code: U32, message: String) => {
    var obj: JSON = JSON()
    obj.set("status", "error")
    obj.set("code", code.string())
    obj.set("message", message)
    print(obj.to_string())
  }
}

actor main {
  new create() => {
    var start: U64 = time_now()

    var resp: ApiResponse = ApiResponse()
    resp.success("{\"users\":[]}")
    resp.error(404, "Not Found")

    var elapsed: U64 = time_elapsed(start)
    print("Elapsed: " + elapsed.string() + "ms")
  }
}
```

---

## 14.4 习题

**习题 14.1** 解析一个 JSON 字符串并提取其中的嵌套字段。

**习题 14.2** 用 `JSON` 生成一个包含用户列表的 JSON 响应。

**习题 14.3** 用 `time_elapsed` 测量一个 actor 处理 100 条消息的耗时。

---

## 14.5 下一步

恭喜完成标准库篇！下一章进入**多目标平台**——学习 FFI、MCU 嵌入式和浏览器 Wasm。

→ **第 15 章：外部接口（FFI）**
