#!/usr/bin/env python3
"""ponybuild — Pony++ 项目构建工具

用法:
    ponybuild build [--target native|wasm] [--entry FILE]
    ponybuild test [NAME]
    ponybuild clean
    ponybuild init NAME [--dir DIR]

项目结构:
    project/
    ├── pony.toml          # 项目清单
    ├── src/               # 库源码 (无 actor main)
    ├── probe/ 或 tests/   # 入口/测试 (有 actor main)
    ├── build/             # 构建输出 (自动创建)
    └── deps/              # C 静态库等

pony.toml 格式:
    [project]
    name = "myproject"
    version = "0.1.0"
    entry = "src/main.pny"       # 默认入口

    [build]
    target = "native"
    sources = [
        { path = "lib/engine.pny", split = "actor main {" },  # 只取前半
        "src/message.pny",                                      # 全文件
    ]
    link = ["lib/libfoo.a"]
    output = "build/myproject"

    [[test]]
    name = "hello"
    entry = "tests/hello.pny"
    expect = ["HELLO-OK"]
    timeout = 10
"""
import tomllib
import subprocess
import sys
import os
import shutil
import argparse
from pathlib import Path

PONYPPC = os.environ.get(
    "PONYPPC", os.path.expanduser("~/Ponyplusplus/bin/ponyppc")
)


def die(msg):
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)


def find_manifest():
    d = Path.cwd()
    while d != d.parent:
        if (d / "pony.toml").exists():
            return d / "pony.toml"
        d = d.parent
    die("pony.toml not found (run 'ponybuild init NAME' to create a project)")


def load_manifest(path):
    with open(path, "rb") as f:
        return tomllib.load(f)


def read_source(base, src):
    """读取单个源文件, 支持 split 截断"""
    if isinstance(src, str):
        src = {"path": src}
    p = base / src["path"]
    if not p.exists():
        die(f"source not found: {src['path']}")
    text = p.read_text()
    split_marker = src.get("split")
    if split_marker:
        idx = text.find(split_marker)
        if idx < 0:
            die(f"split marker not found in {src['path']}: {split_marker!r}")
        text = text[:idx]
    return text


def concat_sources(base, sources, entry):
    parts = []
    for src in sources:
        parts.append(read_source(base, src))
    ep = base / entry
    if not ep.exists():
        die(f"entry not found: {entry}")
    parts.append(ep.read_text())
    return "".join(parts)


def build_one(base, sources, entry, link, target, output):
    """编译单个二进制, 返回是否成功"""
    combined = concat_sources(base, sources, entry)
    build_dir = base / "build"
    build_dir.mkdir(exist_ok=True)
    stem = Path(entry).stem
    combined_path = build_dir / f".combined_{stem}.pny"
    combined_path.write_text(combined)

    env = os.environ.copy()
    if link:
        # link 路径相对于项目根目录; "-lxxx"/绝对路径 直接透传 (系统库支持)
        env["PONYPPC_LDFLAGS"] = " ".join(
            l if l.startswith("-") or l.startswith("/") else str(base / l)
            for l in link
        )
    else:
        env.pop("PONYPPC_LDFLAGS", None)

    output = Path(output)
    if not output.is_absolute():
        output = base / output
    output.parent.mkdir(parents=True, exist_ok=True)

    cmd = [PONYPPC, "--target", target, "-o", str(output), str(combined_path)]
    result = subprocess.run(
        cmd, env=env, capture_output=True, text=True, cwd=base
    )
    if result.returncode != 0:
        print(f"  BUILD-FAIL: {entry}", file=sys.stderr)
        for line in result.stderr.strip().split("\n"):
            if "error" in line.lower() or "错误" in line:
                print(f"    {line}", file=sys.stderr)
        return False
    return True


# ---- commands ----


def cmd_build(args, manifest, base):
    bc = manifest.get("build", {})
    target = args.target or bc.get("target", "native")
    entry = args.entry or manifest["project"]["entry"]
    sources = bc.get("sources", [])
    link = bc.get("link", [])
    output = bc.get("output", f"build/{manifest['project']['name']}")

    ok = build_one(base, sources, entry, link, target, output)
    if ok:
        print(f"built: {output}")
    return ok


def cmd_test(args, manifest, base):
    tests = manifest.get("test", [])
    if not tests:
        die("no tests defined in pony.toml ([[test]] section)")
    if args.name:
        tests = [t for t in tests if t["name"] == args.name]
        if not tests:
            die(f"test not found: {args.name}")

    bc = manifest.get("build", {})
    target = bc.get("target", "native")
    sources = bc.get("sources", [])
    link = bc.get("link", [])

    passed = failed = 0
    for t in tests:
        name = t["name"]
        entry = t["entry"]
        expect = t.get("expect", [])
        timeout = t.get("timeout", 10)

        output = base / "build" / f"test_{name}"
        ok = build_one(base, sources, entry, link, target, output)
        if not ok:
            print(f"  {name}: BUILD-FAIL")
            failed += 1
            continue

        try:
            result = subprocess.run(
                [str(output)],
                capture_output=True,
                text=True,
                timeout=timeout,
            )
            out = result.stdout
        except subprocess.TimeoutExpired:
            print(f"  {name}: TIMEOUT ({timeout}s)")
            failed += 1
            continue

        missing = [p for p in expect if p not in out]
        if missing:
            print(f"  {name}: FAIL (missing {missing})")
            # 打印最后 3 行输出帮助调试
            lines = out.strip().split("\n")
            for line in lines[-3:]:
                print(f"    | {line}")
            failed += 1
        else:
            print(f"  {name}: PASS ({len(expect)} assertions)")
            passed += 1

    total = passed + failed
    print(f"\n{passed} passed / {failed} failed / {total} total")
    return failed == 0


def cmd_clean(args, manifest, base):
    build_dir = base / "build"
    if build_dir.exists():
        shutil.rmtree(build_dir)
        print("cleaned: build/")
    else:
        print("nothing to clean")
    return True


def cmd_init(args, manifest, base):
    name = args.name
    d = Path(args.dir) / name if args.dir else Path.cwd() / name
    if d.exists():
        die(f"directory already exists: {d}")
    for sub in ["src", "tests", "build"]:
        (d / sub).mkdir(parents=True)

    (d / "pony.toml").write_text(
        f'''[project]
name = "{name}"
version = "0.1.0"
entry = "src/main.pny"

[build]
target = "native"
sources = []
link = []
output = "build/{name}"

[[test]]
name = "hello"
entry = "tests/hello.pny"
expect = ["HELLO-OK"]
timeout = 10
'''
    )

    (d / "src" / "main.pny").write_text(
        """actor main {
  new create() => {
    print("hello from ponybuild")
  }
}
"""
    )

    (d / "tests" / "hello.pny").write_text(
        """actor main {
  new create() => {
    print("HELLO-OK")
  }
}
"""
    )

    (d / ".gitignore").write_text("build/\n")
    print(f"created: {d}/")
    print(f"  pony.toml  src/main.pny  tests/hello.pny  .gitignore")
    print(f"\ncd {name} && ponybuild test")
    return True


def main():
    parser = argparse.ArgumentParser(
        prog="ponybuild", description="Pony++ 项目构建工具"
    )
    sub = parser.add_subparsers(dest="command")

    p_build = sub.add_parser("build", help="构建默认入口")
    p_build.add_argument("--target", choices=["native", "wasm"])
    p_build.add_argument("--entry", help="覆盖默认入口")

    p_test = sub.add_parser("test", help="运行测试")
    p_test.add_argument("name", nargs="?", help="指定测试名 (默认全部)")

    sub.add_parser("clean", help="清理 build/")

    p_init = sub.add_parser("init", help="创建新项目")
    p_init.add_argument("name")
    p_init.add_argument("--dir", help="父目录 (默认当前目录)")

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        sys.exit(1)

    if args.command == "init":
        sys.exit(0 if cmd_init(args, None, None) else 1)

    manifest_path = find_manifest()
    base = manifest_path.parent.resolve()
    manifest = load_manifest(manifest_path)

    cmds = {"build": cmd_build, "test": cmd_test, "clean": cmd_clean}
    ok = cmds[args.command](args, manifest, base)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
