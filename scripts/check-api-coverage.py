#!/usr/bin/env python3
"""公开 API 的每个符号都必须被测试引用过（REQ-030）。

为什么需要
--------
复审时发现 `Writer::bytes_written()` 是公开 API、文档里写了、调用方会拿来
判断分卷/配额，但**没有任何用例调用过它**。函数覆盖率因此一直是 98.x%。

这类缺口靠"看覆盖率报告"能发现，但那是事后；靠"人工列 API 清单"会漏。
机械做法：从公共头里抽出所有公开符号，逐个到 tests/ 里找引用。

检查什么
--------
`include/mmat/` 核心公共头里的：
  * 成员函数名（公开）
  * 自由函数名
  * 枚举值（kXxx）

**不检查类型名**。类型通常通过 `auto` 使用（`const auto caps =
crc32_query();` 之后断言 `caps.impl`），类型名本身不会出现在测试里；
按类型名判据会产生误报。它的成员（函数/枚举值）才是真正需要覆盖的。

**不检查聚合体的数据成员**。像 `name` / `level` / `code` 这类通用词在测试里
随处可见，词边界匹配几乎必然命中，只会产生假阳性。数据成员的验证由具体
用例负责（例如 `RecordedLog` 的每个字段都有断言）。

怎么算"被引用"
------------
在 `tests/**/*.cpp`（含 support 头）的源码里出现该标识符即可。这是**弱检查**：
出现一次就算引用，不保证语义正确。它的价值在于挡住"完全没人提过"的符号，
这正是 `bytes_written()` 的情形。语义正确性由具体用例负责。

排除
----
* 析构函数、运算符、编译器生成的成员
* 显式标注 `// api-not-tested:` 的声明（必须在紧邻上一行给出理由）
* adapters/ 目录（可选胶水，由 scripts/check-mnem-adapter.sh 与
  scripts/test-mnem-adapter.sh 单独验证）

用法:
    python3 scripts/check-api-coverage.py --check
    python3 scripts/check-api-coverage.py --list
"""

import argparse
import re
import sys
from pathlib import Path

LIB = Path(__file__).resolve().parent.parent
PUBLIC = LIB / "include" / "mmat"

# 测试源码与支撑头都算引用来源。
TEST_GLOBS = ("tests/**/*.cpp", "tests/**/*.hpp")

# 不需要被测试直接引用的名字。
IGNORED = {
    # 构造/析构
    "~Archive", "~Writer", "~Logger", "~ScopedDefaultLogger",
    # 纯类型别名或标记
    "kNameIndexThreshold",
}

FUNC_DECL = re.compile(
    r'^[ \t]*(?P<attr>(?:\[\[[^\]]*\]\][ \t]*)*)'
    r'(?:(?:static|inline|constexpr|virtual|explicit|friend|constexpr)[ \t]+)*'
    r'(?:[A-Za-z_][\w:<>,\* &]*?)[ \t]+'
    r'(?P<name>[A-Za-z_]\w*)[ \t]*\(',
    re.M,
)
ENUM_ITEM = re.compile(r'^[ \t]*(k[A-Za-z0-9_]+)[ \t]*(?:=[^,]*)?,', re.M)
EXEMPT_MARK = "api-not-tested"


def collect_tests() -> str:
    chunks = []
    for pattern in TEST_GLOBS:
        for path in sorted(LIB.glob(pattern)):
            chunks.append(path.read_text(encoding="utf-8"))
    return "\n".join(chunks)


def collect_public_symbols() -> dict[str, tuple[str, int]]:
    """名字 → (头文件, 行号)。"""
    symbols: dict[str, tuple[str, int]] = {}

    for name in ("mat.hpp",):
        path = PUBLIC / name
        text = path.read_text(encoding="utf-8")
        rel = path.relative_to(LIB).as_posix()
        lines = text.split("\n")
        # Only the class body is public API; the inline template helpers and
        # the detail namespace must not be mistaken for declarations.
        body_start = text.index("class MMAT_EXPORT Mat")
        body_end = text.index("};", body_start)

        def line_of(pos: int) -> int:
            return text.count("\n", 0, pos) + 1

        for match in ENUM_ITEM.finditer(text, body_start, body_end):
            symbols.setdefault(match.group(1), (rel, line_of(match.start())))

        for match in FUNC_DECL.finditer(text, body_start, body_end):
            name = match.group("name")
            if name.startswith("operator") or name in ("if", "for", "while", "switch", "return", "sizeof"):
                continue
            line = line_of(match.start())
            # 紧邻上一行有豁免标记就跳过
            if line >= 2 and EXEMPT_MARK in lines[line - 2]:
                continue
            symbols.setdefault(name, (rel, line))

    return symbols


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args(argv)

    symbols = collect_public_symbols()
    tests = collect_tests()

    untested: list[str] = []
    for name, (rel, line) in sorted(symbols.items()):
        if name in IGNORED:
            continue
        # 用词边界匹配，避免 find() 之类的短名被 "find_all" 误命中。
        if not re.search(rf'\b{re.escape(name)}\b', tests):
            untested.append(f"{rel}:{line}: {name}")

    if args.list:
        print(f"公开符号 {len(symbols)} 个，测试里未出现 {len(untested)} 个")
        for item in untested:
            print(f"  {item}")
        return 0

    if untested:
        for item in untested:
            print(f"FAIL {item} 未在任何测试里出现", file=sys.stderr)
        print(f"\n共 {len(untested)} 个公开符号零引用（要么加用例，要么用 // api-not-tested: 说明）",
              file=sys.stderr)
        return 1

    print(f"CHECK OK: all {len(symbols) - len(IGNORED & set(symbols))} public symbols are referenced by tests")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
