#!/usr/bin/env python3
"""ponytest — Pony++ 测试运行器 (独立使用)

用法:
    ponytest BINARY [--expect PATTERN ...] [--timeout N]
    ponytest --manifest pony.toml [NAME]
"""
import subprocess
import sys
import argparse
import tomllib
from pathlib import Path


def run_single(binary, expect, timeout):
    """运行单个二进制, 检查输出包含所有 expect 模式"""
    try:
        result = subprocess.run(
            [binary], capture_output=True, text=True, timeout=timeout
        )
    except FileNotFoundError:
        print(f"FAIL: binary not found: {binary}")
        return False
    except subprocess.TimeoutExpired:
        print(f"FAIL: timeout ({timeout}s)")
        return False

    out = result.stdout
    missing = [p for p in expect if p not in out]
    if missing:
        print(f"FAIL: missing {missing}")
        lines = out.strip().split("\n")
        for line in lines[-5:]:
            print(f"  | {line}")
        return False

    print(f"PASS ({len(expect)} assertions)")
    return True


def run_manifest(manifest_path, name=None):
    """从 pony.toml 读取测试定义并运行"""
    with open(manifest_path, "rb") as f:
        m = tomllib.load(f)
    tests = m.get("test", [])
    if name:
        tests = [t for t in tests if t["name"] == name]
    if not tests:
        print("no tests to run")
        return True

    base = Path(manifest_path).parent.resolve()
    passed = failed = 0
    for t in tests:
        binary = base / "build" / f"test_{t['name']}"
        expect = t.get("expect", [])
        timeout = t.get("timeout", 10)
        print(f"  {t['name']}: ", end="", flush=True)
        if run_single(str(binary), expect, timeout):
            passed += 1
        else:
            failed += 1

    total = passed + failed
    print(f"\n{passed} passed / {failed} failed / {total} total")
    return failed == 0


def main():
    parser = argparse.ArgumentParser(
        prog="ponytest", description="Pony++ 测试运行器"
    )
    parser.add_argument("binary", nargs="?", help="测试二进制路径")
    parser.add_argument(
        "--expect", nargs="+", default=[], help="期望输出模式"
    )
    parser.add_argument("--timeout", type=int, default=10)
    parser.add_argument("--manifest", help="pony.toml 路径")
    parser.add_argument("--name", help="指定测试名 (配合 --manifest)")
    args = parser.parse_args()

    if args.manifest:
        sys.exit(0 if run_manifest(args.manifest, args.name) else 1)
    elif args.binary:
        expect = args.expect or ["OK"]  # 默认至少包含 OK
        sys.exit(0 if run_single(args.binary, expect, args.timeout) else 1)
    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
