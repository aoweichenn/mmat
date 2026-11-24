#!/usr/bin/env bash
# 构建 + 零告警检查。
# 用法: scripts/build.sh [purpose] [--clang] [--sanitize]
#   purpose  test（默认）/ release
# 环境变量:
#   MMAT_BUILD_ROOT  构建树根目录（默认 /tmp/mmat_build）
set -uo pipefail

# shellcheck source=_build.sh
. "$(dirname "$0")/_build.sh"

lib="$(cd "$(dirname "$0")/.." && pwd)"
purpose="test"
compiler="gcc"
sanitize="OFF"

while [ $# -gt 0 ]; do
    case "$1" in
        --clang)      compiler=clang ;;
        --sanitize)   sanitize=ON ;;
        -*) echo "unknown option $1" >&2; exit 2 ;;
        *)  purpose="$1" ;;
    esac
    shift
done

build_dir=$(mmat_build_dir "$purpose")
# 编译器切换必须换构建目录：CMake 会因 CMAKE_CXX_COMPILER 变化清空 cache，
# 避免 CMake 在切换编译器时重置 cache。
if [ "$compiler" = "clang" ]; then
    build_dir="${build_dir}-clang"
fi
log_dir="$(mmat_build_root)/logs"
mkdir -p "$log_dir"
mmat_ensure_build_dir "$lib" "$build_dir"
log="$log_dir/$purpose.log"

cmake_args=(-S "$lib" -B "$build_dir" -G Ninja -Dmmat_build_tests=ON -DMMAT_SANITIZE="$sanitize")
if [ "$compiler" = "clang" ]; then
    cmake_args+=(-DCMAKE_CXX_COMPILER=clang++)
fi
if [ "$purpose" = "release" ]; then
    cmake_args+=(-DCMAKE_BUILD_TYPE=Release)
else
    cmake_args+=(-DCMAKE_BUILD_TYPE=Debug)
fi

echo "== configure ($compiler, $purpose) =="
cmake "${cmake_args[@]}" > "$log" 2>&1 || { tail -20 "$log"; exit 1; }

echo "== build =="
cmake --build "$build_dir" -j"$(nproc)" >> "$log" 2>&1 || { grep -E "error" "$log" | head -20; exit 1; }

echo "== warnings =="
python3 "$(dirname "$0")/check-warnings.py" "$log"
