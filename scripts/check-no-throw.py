#!/usr/bin/env python3
"""库代码不得用异常报告失败（REQ-013）。

规则
----
1. `src/` 与 `include/mmat/` 里不得出现 `throw`。
2. `catch` 只允许把分配失败转成返回值；当前没有允许文件。
3. 库代码不得 `catch (...)`。

为什么
------
平台层要在 freestanding/嵌入式环境工作，异常可能被关闭；同时
`noexcept` 函数里抛异常会直接 terminate，连错误码都拿不到。
分配失败统一走 `std::nothrow` / 返回 false。

用法: python3 scripts/check-no-throw.py --check
"""

import argparse
import re
import sys
from pathlib import Path

LIB = Path(__file__).resolve().parent.parent

CATCH_ALLOWED: set[str] = set()
CATCH_ALL_ALLOWED: set[str] = set()

THROW_RE = re.compile(r"\bthrow\b")
CATCH_RE = re.compile(r"\bcatch\s*\(")
CATCH_ALL_RE = re.compile(r"\bcatch\s*\(\s*\.\.\.\s*\)")


def strip_comments(text: str) -> str:
    text = re.sub(r"//[^\n]*", "", text)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return text


def scan_files() -> list[Path]:
    files = list(LIB.glob("src/**/*.cpp")) + list(LIB.glob("src/**/*.hpp"))
    files += list(LIB.glob("include/mmat/**/*.hpp"))
    return [path for path in sorted(files)
            if not any(part in ("_deps", "third_party", "build") for part in path.parts)]


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)

    problems: list[str] = []
    catches = 0

    for path in scan_files():
        rel = path.relative_to(LIB).as_posix()
        text = strip_comments(path.read_text(encoding="utf-8"))
        for number, line in enumerate(text.split("\n"), 1):
            if THROW_RE.search(line):
                problems.append(f"{rel}:{number}: 出现 throw（失败用返回值表达）")

        for match in CATCH_RE.finditer(text):
            catches += 1
            if rel not in CATCH_ALLOWED:
                line = text.count("\n", 0, match.start()) + 1
                problems.append(f"{rel}:{line}: catch 不在白名单里")

        for match in CATCH_ALL_RE.finditer(text):
            if rel not in CATCH_ALL_ALLOWED:
                line = text.count("\n", 0, match.start()) + 1
                problems.append(f"{rel}:{line}: 库代码不得 catch(...)")

    if problems:
        for problem in problems:
            print(f"FAIL {problem}", file=sys.stderr)
        print(f"\n共 {len(problems)} 处问题", file=sys.stderr)
        return 1

    print(f"CHECK OK: no throw; {catches} catch site(s), all allowlisted")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
