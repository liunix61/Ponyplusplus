# 附录 B：完整 API 参考

> 本附录列出 Pony++ 标准库的全部 API，基于 `stdlib/std/` 目录实际实现。

---

## B.1 String 方法

| 方法 | 返回类型 | 说明 |
|------|----------|------|
| `len()` | `U32` | 字符串长度 |
| `upper()` | `String` | 转大写 |
| `lower()` | `String` | 转小写 |
| `trim()` | `String` | 去除首尾空白 |
| `contains(sub)` | `Bool` | 是否包含子串 |
| `starts_with(prefix)` | `Bool` | 是否以指定前缀开头 |
| `ends_with(suffix)` | `Bool` | 是否以指定后缀结尾 |
| `substring(start, end)` | `String` | 截取子串 |
| `split(sep)` | `List[String]` | 按分隔符分割 |
| `+` | `String` | 字符串拼接 |
| `==` | `Bool` | 字符串比较 |

---

## B.2 List[T] 方法

| 方法 | 返回类型 | 说明 |
|------|----------|------|
| `len()` | `U32` | 列表长度 |
| `append(item)` | — | 追加元素 |
| `[index]` | `T` | 按索引访问 |
| `for item in list` | — | 遍历 |

**创建方式**：

```pony
var list: List[U32] = [1, 2, 3]
```

---

## B.3 数学函数

| 函数 | 说明 |
|------|------|
| `max(a, b)` | 最大值 |
| `min(a, b)` | 最小值 |
| `abs_val(a)` | 绝对值 |
| `power(base, exp)` | 幂运算 |

---

## B.4 IO 函数

| 函数 | 说明 |
|------|------|
| `print(value)` | 输出到控制台 |

---

## B.5 日志函数

| 函数 | 说明 |
|------|------|
| `log_debug(msg)` | 调试日志 |
| `log_info(msg)` | 信息日志 |
| `log_warn(msg)` | 警告日志 |
| `log_error(msg)` | 错误日志 |

---

## B.6 JSON API

| 函数/方法 | 说明 |
|-----------|------|
| `parse_json(str)` | 解析 JSON 字符串 |
| `JSON()` | 创建空 JSON 对象 |
| `obj.get(key)` | 获取字段值 |
| `obj.set(key, value)` | 设置字段 |
| `obj.to_string()` | 序列化为字符串 |

---

## B.7 时间 API

| 函数 | 返回类型 | 说明 |
|------|----------|------|
| `time_now()` | `U64` | 当前时间戳（毫秒） |
| `time_elapsed(start)` | `U64` | 从 start 到现在的耗时 |

---

## B.8 并发 API（import std.concurrent）

| 类型/方法 | 说明 |
|-----------|------|
| `Channel(capacity)` | 创建通道 |
| `ch.send(msg)` | 发送消息 |
| `ch.receive()` | 接收消息 |
| `ch.close()` | 关闭通道 |
| `ch.is_closed()` | 检查是否关闭 |
| `Future()` | 创建 Future |
| `f.resolve(value)` | 设置结果 |
| `f.await()` | 等待结果 |
| `f.is_done()` | 检查是否完成 |
| `ActorGroup()` | 创建 Actor 组 |
| `group.add(actor)` | 添加成员 |
| `group.count()` | 成员数量 |
| `group.broadcast(msg)` | 广播消息 |
