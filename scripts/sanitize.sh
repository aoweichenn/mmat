#!/usr/bin/env bash
# Sanitizer 构建与测试（ASan + UBSan）。
set -uo pipefail

# shellcheck source=_build.sh
. "$(dirname "$0")/_build.sh"

lib="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="$(mmat_build_dir sanitize)"

"$lib/scripts/build.sh" sanitize --sanitize || exit 1

echo "== sanitizer 测试 =="
ctest --test-dir "$build_dir" --output-on-failure
