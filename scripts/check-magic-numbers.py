#!/usr/bin/env python3
"""常量裸字面量检查（CODE-STYLE F-6）。

除 0/1/2 外，整数与浮点字面量必须提成具名常量（constexpr 局部或 kPascalCase）。

豁免：
  * 常量定义行（含 constexpr）
  * enum 定义体内的值（枚举值本身就是"具名"的）
  * sizeof/alignof、数组维度、模板参数、case 标签、static_assert
  * 字符串与字符字面量内部（错误消息里写 "0x0001" 不是常量）
  * 预处理指令

用法: python3 scripts/check-magic-numbers.py --check <lib_dir>
"""

import argparse
import re
import sys
from pathlib import Path

# 允许的字面量
ALLOWED = {"0", "1", "2"}

# 正则：十进制/十六进制整数、浮点
LITERAL = re.compile(r'(?<![\w.])(0[xX][0-9a-fA-F]+|\d+\.\d+|\d+)(?![\w.])')

# 豁免行：常量定义本身、数组维度、模板参数、case 标签、位运算、static_assert
EXEMPT = re.compile(
    r'constexpr|static_assert|case |sizeof|alignof|'
    r'#\s*(if|elif|define|include)|'
    r'\b(bit|shift|mask|index|version)\s*=|'
    r'std::array<|std::span<|std::byte\{|\[[^\]]*\]')


def strip_comments(text: str) -> str:
    text = re.sub(r'//[^\n]*', '', text)
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    return text


def blank_string_literals(line: str) -> str:
    """把字符串/字符字面量的内容替换成等长空白，保留引号。

    不这样做的话，错误消息里的 "0x0001" 或 "65535" 会被当成裸字面量。
    """
    out = []
    index = 0
    quote = None
    while index < len(line):
        char = line[index]
        if quote is None:
            if char in ('"', "'"):
                quote = char
            out.append(char)
            index += 1
            continue
        if char == '\\' and index + 1 < len(line):
            out.append('  ')
            index += 2
            continue
        if char == quote:
            quote = None
            out.append(char)
        else:
            out.append(' ')
        index += 1
    return ''.join(out)


def enum_body_lines(lines: list[str]) -> set[int]:
    """返回处于 enum 定义体内的行号（0 基）。

    enum 的 '{' 可能在声明的下一行（clang-format 就会那么排），
    所以不能只在"看到 enum 关键字的那一行"判断是否进入花括号。
    """
    inside: set[int] = set()
    depth = 0
    pending = False
    for number, raw_line in enumerate(lines):
        # 花括号计数必须在抹掉字面量之后做：pickle 里 kEmptyDict = '}'
        # 这种"字符字面量恰好是花括号"的情况会骗到朴素实现。
        line = blank_string_literals(raw_line)
        if depth > 0:
            inside.add(number)
            depth += line.count('{') - line.count('}')
            continue
        if pending:
            inside.add(number)
            opens = line.count('{')
            if opens > 0:
                depth = opens - line.count('}')
                pending = False
            continue
        if re.search(r'\benum\s+(class\s+)?\w+', line):
            inside.add(number)
            opens = line.count('{')
            if opens > 0:
                depth = opens - line.count('}')
            else:
                pending = True
    return inside


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument("root", nargs="?", default=".")
    args = parser.parse_args(argv)

    root = Path(args.root).resolve()
    problems = []

    for path in sorted(root.glob("src/**/*.cpp")) + sorted(root.glob("src/**/*.hpp")):
        rel = path.relative_to(root).as_posix()
        if any(part in ("third_party", "_deps") for part in path.parts):
            continue
        text = strip_comments(path.read_text(encoding="utf-8"))
        lines = text.split("\n")
        enum_lines = enum_body_lines(lines)

        for number, line in enumerate(lines, 1):
            if EXEMPT.search(line):
                continue
            if (number - 1) in enum_lines:
                continue
            stripped = line.strip()
            if stripped.startswith("*"):
                continue
            cleaned = blank_string_literals(line)
            for match in LITERAL.finditer(cleaned):
                token = match.group(1)
                if token in ALLOWED:
                    continue
                problems.append(f"{rel}:{number}: 裸字面量 '{token}'，提成具名常量")

    if problems:
        for problem in problems[:40]:
            print(f"FAIL {problem}", file=sys.stderr)
        if len(problems) > 40:
            print(f"... 还有 {len(problems) - 40} 处", file=sys.stderr)
        return 1

    print("CHECK OK: no magic numbers")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
