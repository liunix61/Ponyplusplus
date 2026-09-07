# Changelog

All notable changes to Pony++ are documented in this file.

## [0.1.0] - 2026-09-07

### Added
- Pony++ 编译器 `ponyppc`：纯 C11 实现，零外部依赖
- Native backend：编译为本地可执行文件
- Wasm backend (Path B)：编译为 WASM 组件
- 引用能力系统：val/ref/trn/box/tag
- Actor 模型：actor 声明、be/fun 方法、new 构造器
- 标准库：list、json、concurrent、actor、time、log、io、math、string
- Bootstrap 自举：编译器自编译闭环
- 分布式运行时：actor 网络通信
- WIT 接口生成
- REPL 交互模式

### Fixed
- typecheck: `this.field` 前缀剥离，修复 unknown identifier 误报
- codegen: `be` 方法返回类型传播（actor 类型 → void *，泛型 List[T] → PnyList *）
- parser: TK_CAP 在表达式中作标识符，修复解析死循环
- lexer: 单引号多字符字符串识别为 TK_STRING（Pony 语言规范）
- stdlib: actor.pny ActorRef → Actor 自引用，消除跨文件依赖
- stdlib: concurrent.pny pny_list_len/pny_list_get/return 类型传播

### Architecture
- 7 层编译管线：Lexer → Parser → Typecheck → Capability → Codegen → Link → Output
- 2 个后端：Native (C11) + Wasm (Binaryen)
- 模块化：28 个 C 源文件，每个模块独立可测
