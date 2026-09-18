# Pony 生态工程化管理规范（母本 v1）

> 适用: Pony 语言一切项目（现有 8 个 + 未来任何新项目）
> 分发: 每个项目 docs/00-工程化管理规范.md 为本规范的项目内副本

## 1. 统一工程化文件（每项目必备四件套）

| 文件 | 作用 |
|---|---|
| `scripts/regression.sh` | 统一回归 runner: 清理→build.sh→测试自动发现→汇总报告 |
| `Makefile` | 薄壳入口: all/regression/test/clean（make regression = 全量回归） |
| `docs/00-工程化管理规范.md` | 本规范副本 |
| `.gitignore` | build/、*.o、*.jsonl、*.log、tests/**/*.wasm、__pycache__/ |

测试入口自动发现（放对位置即纳入回归，零注册）:
```
tests/*_tests.sh          顶层测试脚本（ponypi/ponyharness 模式）
tests/*/run_tests.sh      分目录测试（ponyexecution m1a/ponyagents m1 模式）
tests/*/run_unit.sh       单元测试（ponyexecution unit 模式）
tests/*/test_*.sh         分目录单测
```
报告: build/regression_report.txt + build/test_<name>.log；单测试 timeout 300s

## 2. 项目清单与测试入口（2026-09-18）

| 项目 | 状态 | 测试入口 | 工程化 |
|---|---|---|---|
| Ponyplusplus (编译器) | 活跃 d1f81e1+ | make test(10 C) + make gtest(175) + make regression | ✅ 专业版 runner |
| ponypi | v0.1 M0-M4 ✓ | tests/ponypi_tests.sh (14项) | ✅ |
| ponyexecution | M4 ✓ | tests/m1a/run_tests.sh + tests/unit/run_unit.sh | ✅ |
| ponyharness | v0.2 ✓ 19/19 | tests/harness_tests.sh (H0-H4) | ✅ |
| ponyagents | 活跃 | tests/m1/run_tests.sh + tests/m4/test_api.sh | ✅ |
| ponydb | M0-M4 推进 | ponybuild test (pony.toml [[test]]) + ponydb_tests.sh | ponybuild 体系 |
| nanonode | P0-P4 ✓ | ponybuild test s6-s32 (32套) | ponybuild 体系 |
| ponymail | 冻结(设计完) | 骨架(regression 报 no entries) | ✅ 预置 |
| ponyget | 骨架(未开发) | 骨架 | ✅ 预置 |
| purl | 骨架(未开发) | 骨架 | ✅ 预置 |

## 3. 依赖链纪律

```
Ponyplusplus (编译器) ──make regression 全绿──> 才允许下游项目回归
     │ 10 C + 175 gtest
     ▼
下游项目 (ponypi/ponyexecution/ponyharness/ponyagents/ponydb/nanonode/...)
     │ 编译器变更后: ponybuild test / make regression 抽查
     ▼
应用项目 (ponymail/ponyget/purl — 开发启动时)
```

- **编译器 codegen/parser/typecheck 变更**: Ponyplusplus regression 全绿 → nanonode s9/s21/s26/s28 抽查 → ponydb/ponypi 回归
- ponyppc 路径: /home/liunix/Ponyplusplus/build/ponyppc（$PPC 可覆盖）
- ponybuild: /home/liunix/Ponyplusplus/tools/ponybuild.py（pony.toml [[test]] manifest 模式）

## 4. 提交与推送规范

- 回归全绿才提交；`feat/fix/docs(scope): 中文描述`
- 每项目独立 git，main 主分支，推送 github.com/liunix61
- 产物不入库: build/、*.a、*.o、审计日志 *.jsonl、可重建 wasm
- RPi5 护栏: 内存敏感测试 ulimit -v 1048576；压测前 free -m

## 5. 新项目 checklist（任何未来 Pony 项目）

1. 目录: /home/liunix/workspace/<project>/（⛔严禁 home 主目录）+ git init -b main
2. 预置四件套: scripts/regression.sh + Makefile + docs/00-工程化管理规范.md + .gitignore
3. build.sh: ponyppc 构建入口（$PPC 覆盖约定）
4. 测试放标准位置（tests/*_tests.sh 等）→ 自动纳入 make regression
5. docs/01-总体方案.md 先行（用户偏好: 先文档后代码）
6. 首次提交即含工程化四件套
