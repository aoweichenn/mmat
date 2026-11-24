#!/usr/bin/env bash
# ThreadSanitizer 构建与测试（线程原语 / 日志并发）。
set -uo pipefail

# shellcheck source=_build.sh
. "$(dirname "$0")/_build.sh"

lib="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="$(mmat_build_dir tsan)"
mmat_ensure_build_dir "$lib" "$build_dir"

cmake -S "$lib" -B "$build_dir" -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -Dmmat_build_tests=ON \
      -DMMAT_SANITIZE=OFF \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" > /dev/null || exit 1
cmake --build "$build_dir" -j"$(nproc)" > /dev/null || exit 1

echo "== TSan 测试 =="
ctest --test-dir "$build_dir" --output-on-failure
