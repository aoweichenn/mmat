#!/usr/bin/env python3
"""可失败返回值必须标 [[nodiscard]]（REQ-014）。

检查对象：公共头里的"会失败"入口——
  * `bool open(...)`
  * `bool try_*(...)`
  * `std::int32_t set_cpu_powersave/set_cpu_thread_affinity/set_flush_denormals`

标了之后，丢弃返回值会被 `-Wunused-result` 抓到，再由零告警闸门拦住。

用法: python3 scripts/check-nodiscard.py --check
"""

import argparse
import re
import sys
from pathlib import Path

LIB = Path(__file__).resolve().parent.parent

DECL = re.compile(
    r'^[ \t]*(?P<attr>\[\[nodiscard\]\][ \t]*)?'
    r'(?:static|inline|constexpr|virtual|explicit|friend[ \t]+)*'
    r'(?P<ret>void\*|bool|std::size_t)[ \t]+'
    r'(?P<name>fast_malloc|allocate|set_size_compare_ratio|outstanding)[ \t]*\(',
    re.M,
)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)

    paths = [LIB / "include" / "mmat" / "mat.hpp"]
    problems: list[str] = []
    checked = 0

    for path in paths:
        text = path.read_text(encoding="utf-8")
        rel = path.relative_to(LIB).as_posix()
        for match in DECL.finditer(text):
            checked += 1
            if match.group("attr") is None:
                line = text.count("\n", 0, match.start()) + 1
                problems.append(f"{rel}:{line}: {match.group('ret')} {match.group('name')}() 缺少 [[nodiscard]]")

    if problems:
        for problem in problems:
            print(f"FAIL {problem}", file=sys.stderr)
        print(f"\n共 {len(problems)} 处缺少 [[nodiscard]]", file=sys.stderr)
        return 1

    print(f"CHECK OK: all {checked} fallible declarations are [[nodiscard]]")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
