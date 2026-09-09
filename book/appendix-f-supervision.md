# 附录 F：监督树策略详解

> 本附录深入分析三种监督策略的语义、适用场景和实现模式。

---

## F.1 三种策略对比

| 策略 | 重启范围 | 适用场景 | 复杂度 |
|------|----------|----------|--------|
| One-for-One | 只重启崩溃的 Actor | 独立 Worker | 低 |
| One-for-All | 重启所有子 Actor | 有状态依赖 | 中 |
| Rest-for-One | 重启崩溃的及其后续 | 有序依赖 | 高 |

---

## F.2 One-for-One 详细语义

### F.2.1 行为定义

当子 Actor A 崩溃时：
1. 监督者收到 A 的崩溃通知
2. 监督者只重启 A
3. 其他子 Actor（B、C、D...）不受影响

### F.2.2 实现模式

```pony
actor Worker {
  var name: String
  var healthy: Bool

  new create(name: String) => {
    this.name = name
    this.healthy = true
  }

  be work() => {
    if this.healthy {
      print(this.name + " working")
    }
  }

  be crash() => {
    this.healthy = false
    print(this.name + " crashed!")
  }

  be restart() => {
    this.healthy = true
    print(this.name + " restarted")
  }
}

actor OneForOneSupervisor {
  var workers: ActorGroup

  new create() => {
    workers = ActorGroup()
  }

  be register(w: Worker) => {
    workers.add(w)
  }

  be handle_crash(worker: Worker) => {
    worker.restart()
  }
}
```

### F.2.3 适用场景

- Web 服务器的请求处理器（每个请求独立）
- 日志写入器（每条日志独立）
- 数据处理管道的各个阶段

---

## F.3 One-for-All 详细语义

### F.3.1 行为定义

当任意子 Actor 崩溃时：
1. 监督者停止所有子 Actor
2. 监督者重启所有子 Actor
3. 所有子 Actor 从初始状态开始

### F.3.2 实现模式

```pony
actor ClusterWorker {
  var name: String
  var generation: U32

  new create(name: String) => {
    this.name = name
    this.generation = 0
  }

  be work() => {
    print(this.name + " gen-" + this.generation.string() + " working")
  }

  be restart() => {
    this.generation += 1
    print(this.name + " restarted as gen-" + this.generation.string())
  }
}

actor OneForAllSupervisor {
  var workers: ActorGroup

  new create() => {
    workers = ActorGroup()
  }

  be register(w: ClusterWorker) => {
    workers.add(w)
  }

  be restart_all() => {
    print("Restarting ALL workers")
    // 实际实现需要遍历所有 worker 并调用 restart
  }
}
```

### F.3.3 适用场景

- 分布式缓存集群（节点间有状态同步）
- 分片数据库（分片间有一致性要求）
- 主从复制系统

---

## F.4 Rest-for-One 详细语义

### F.4.1 行为定义

当子 Actor A 崩溃时：
1. 重启 A
2. 重启 A 之后启动的所有 Actor
3. A 之前的 Actor 不受影响

### F.4.2 启动顺序依赖

```
启动顺序: ConnectionPool → Cache → Service → API
                        ↑
                    Cache 崩溃
                        ↓
重启范围: Cache + Service + API（ConnectionPool 不受影响）
```

### F.4.3 适用场景

- 有初始化顺序依赖的系统
- 数据库连接池 → 缓存 → 服务
- 网络连接 → 协议解析 → 业务逻辑

---

## F.5 监督树设计原则

### F.5.1 层级结构

```
        Root Supervisor
        /            \
   Sup-A             Sup-B
   /    \            /    \
  W1    W2         W3    W4
```

**原则**：

1. 根监督者负责全局策略
2. 子监督者负责局部管理
3. Worker 只做业务逻辑

### F.5.2 重启频率限制

防止崩溃循环：

```pony
actor RateLimitedSupervisor {
  var restart_count: U32
  var max_restarts: U32

  new create(max_restarts: U32) => {
    this.restart_count = 0
    this.max_restarts = max_restarts
  }

  be handle_crash() => {
    if this.restart_count < this.max_restarts {
      this.restart_count += 1
      print("Restarting (attempt " + this.restart_count.string() + ")")
    } else {
      print("Max restarts reached, escalating to parent")
    }
  }
}
```

### F.5.3 优雅关闭

```pony
actor GracefulWorker {
  var running: Bool

  new create() => {
    this.running = true
  }

  be shutdown() => {
    this.running = false
    print("Worker shutting down gracefully")
  }

  be work() => {
    if this.running {
      print("Working...")
    }
  }
}
```

---

## F.6 策略选择决策树

```
需要重启子 Actor 吗？
├── 是 → 子 Actor 之间有依赖吗？
│   ├── 没有依赖 → One-for-One
│   ├── 有共享状态 → One-for-All
│   └── 有启动顺序 → Rest-for-One
└── 否 → 仅记录日志，不重启
```

---

## F.7 与 Erlang/OTP 的比较

| 特性 | Erlang/OTP | Pony++ |
|------|------------|--------|
| 监督策略 | 三种（one_for_one, one_for_all, rest_for_one） | 同样三种 |
| 重启强度 | max_restarts / max_seconds | 可自定义 |
| 退出原因 | exit signal | 崩溃消息 |
| 链接 | link / monitor | 消息传递 |
| 状态恢复 | 自动（从初始状态） | 手动（需自行实现） |

Pony++ 的监督树更灵活——你可以完全自定义重启逻辑，而不受 OTP 框架约束。
