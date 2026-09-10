# Pony++ VS Code Extension

Pony++ 编程语言的 VS Code 扩展。

## 功能

### 语法高亮
- Actor/Class/Primitive/Interface 关键字
- 6种引用能力 (iso/trn/ref/val/box/tag)
- 行为 (be) 和函数 (fun) 区分
- 数字字面量 (hex/binary/float/integer)
- 注释 (行注释 // 和块注释 /* */)

### 代码片段
- `actor` - Actor 模板
- `be` - 行为方法
- `fun` - 函数
- `class` - 类模板
- `match` - 模式匹配
- `try` - Try-else 块
- `for` / `while` - 循环

### 调试 (DAP)
- 断点 (支持条件断点)
- 单步执行 (step over/in/out)
- 变量检查
- Actor 线程视图
- 调用栈

### 命令
- `Pony++: Compile` - 编译当前文件
- `Pony++: Run` - 运行当前文件

## 安装

```bash
cd ide/vscode-ponypp
npm install
npm run compile
# 打包
npx vsce package
# 安装
code --install-extension ponypp-1.0.0.vsix
```

## 调试配置

```json
{
    "type": "ponypp",
    "request": "launch",
    "name": "Launch Pony++",
    "program": "${workspaceFolder}/main.pny",
    "stopOnEntry": true
}
```

## 文件类型
- `.pny` - Pony++ 源文件
- `.ponypp` - Pony++ 编译输出
