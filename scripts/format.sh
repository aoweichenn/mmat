#!/usr/bin/env bash
# clang-format 检查或就地格式化。
# 用法: scripts/format.sh --check   或   scripts/format.sh
set -uo pipefail

lib="$(cd "$(dirname "$0")/.." && pwd)"
mode="apply"
[ "${1:-}" = "--check" ] && mode="check"

files=$(cd "$lib" && find include src tests benchmarks -name '*.hpp' -o -name '*.cpp' 2>/dev/null | grep -v third_party)

if [ "$mode" = "check" ]; then
    fail=0
    for f in $files; do
        if ! clang-format --dry-run --Werror "$lib/$f" > /dev/null 2>&1; then
            echo "需要格式化: $f" >&2
            fail=1
        fi
    done
    [ "$fail" -eq 0 ] && echo "CHECK OK: formatting clean"
    exit "$fail"
fi

for f in $files; do
    clang-format -i "$lib/$f"
done
echo "formatted $(echo "$files" | wc -l) files"
