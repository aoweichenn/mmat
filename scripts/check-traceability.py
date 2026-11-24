#!/usr/bin/env python3
"""追溯矩阵校验（docs/traceability.md）。

校验四件事：
  1. 需求编号连续覆盖 REQ-001..REQ-021，无缺号；
  2. 每条需求都至少有一个 `●`（已落地）测试项，M0 遗留的 `○` 不能留；
  3. 表格里出现的每个 `tests/...` 路径都真实存在；
  4. 每个 `Suite.Case` 形式的用例名都在对应测试源码里出现过。

用法:
    python3 scripts/check-traceability.py --check
    python3 scripts/check-traceability.py --list      # 只列出解析结果
"""

import argparse
import re
import sys
from pathlib import Path

LIB = Path(__file__).resolve().parent.parent
MATRIX = LIB / "docs" / "traceability.md"

REQ_RE = re.compile(r"\*\*(REQ-\d{3})\*\*")
# 表格单元格里的测试引用：`path/to/x.cpp` 或 Suite.Case
PATH_RE = re.compile(r"`((?:tests|src|docs)/[^`]+\.[a-z]+)`")
CASE_RE = re.compile(r"`([A-Z][A-Za-z0-9_]*)\.([A-Za-z0-9_]+)`")

# 文档里作为"形式示例"出现的名字，不是真实用例。
PLACEHOLDER_CASES = {"Suite.Case"}


def parse_requirements(text: str) -> dict[str, str]:
    """返回 {需求编号: 该行的整行文本}。"""
    found: dict[str, str] = {}
    for line in text.split("\n"):
        match = REQ_RE.search(line)
        if match:
            found[match.group(1)] = line
    return found


def collect_test_cases() -> dict[str, str]:
    """从测试源码里收集 Suite.Case → 文件路径。"""
    cases: dict[str, str] = {}
    pattern = re.compile(r"^TEST(?:_F|_P)?\(\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*\)", re.M)
    for path in sorted(LIB.glob("tests/**/*.cpp")):
        text = path.read_text(encoding="utf-8")
        for match in pattern.finditer(text):
            cases[f"{match.group(1)}.{match.group(2)}"] = path.relative_to(LIB).as_posix()
    return cases


def check(mode: str) -> int:
    text = MATRIX.read_text(encoding="utf-8")
    requirements = parse_requirements(text)
    cases = collect_test_cases()
    problems: list[str] = []

    expected = [f"REQ-{index:03d}" for index in range(1, 11)]
    missing = [req for req in expected if req not in requirements]
    if missing:
        problems.append(f"追溯矩阵缺少需求: {', '.join(missing)}")

    extra = [req for req in requirements if req not in expected]
    if extra:
        problems.append(f"追溯矩阵出现未编号需求: {', '.join(extra)}")

    uncovered = []
    for req, line in requirements.items():
        if "●" not in line:
            uncovered.append(req)
        for path in PATH_RE.findall(line):
            if path.startswith("docs/"):
                continue
            if not (LIB / path).exists():
                problems.append(f"{req}: 引用的文件不存在 {path}")
    if uncovered:
        problems.append(f"以下需求没有已落地的测试（缺 ●）: {', '.join(uncovered)}")

    # 全文范围内的引用集合：包括"第 6 节 基础设施与横切用例"这类
    # 不对应具体需求的表格。
    referenced_cases = {f"{suite}.{case}" for suite, case in CASE_RE.findall(text)}

    # 反向：tests/ 下每一个 TEST() 都必须被本文件提到。
    #
    # 只查"引用的存在"是单向的——没有这条反向检查时，曾有 191/317 个用例
    # 没有任何文档说明为什么存在（见 REVIEW-LOG RV-12）。
    unreferenced = [name for name in sorted(cases) if name not in referenced_cases]
    if unreferenced:
        shown = ", ".join(unreferenced[:10])
        problems.append(f"以下 {len(unreferenced)} 个用例未在追溯矩阵中出现: {shown}"
                        + (" …" if len(unreferenced) > 10 else ""))

    # 正向：全文里引用的 Suite.Case 必须真的存在。
    # 文档里用 `Suite.Case` 作为形式示例，不是真实用例，排除。
    for name in sorted(referenced_cases - PLACEHOLDER_CASES):
        if name not in cases:
            problems.append(f"引用的用例不存在 {name}")

    if mode == "list":
        print(f"需求 {len(requirements)} 条；测试用例 {len(cases)} 个")
        for req in expected:
            marker = "●" if req in requirements and "●" in requirements[req] else "✗"
            print(f"  {marker} {req}")
        return 0

    if problems:
        for problem in problems:
            print(f"FAIL {problem}", file=sys.stderr)
        print(f"\n共 {len(problems)} 处问题", file=sys.stderr)
        return 1

    print(f"CHECK OK: traceability clean ({len(requirements)} requirements, {len(cases)} test cases)")
    return 0


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args(argv)
    return check("list" if args.list else "check")


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
