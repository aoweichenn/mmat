#!/usr/bin/env bash
# 跑测试。默认全部；带标签名则只跑该标签（如 unit / component / cpu）。
set -uo pipefail

# shellcheck source=_build.sh
. "$(dirname "$0")/_build.sh"

build_dir="${MMAT_BUILD_DIR:-$(mmat_build_dir test)}"

if [ ! -d "$build_dir" ]; then
    echo "找不到构建目录 $build_dir；先运行 scripts/build.sh test" >&2
    exit 2
fi

args=(--output-on-failure)
if [ $# -gt 0 ]; then
    case "$1" in
        -*) args+=("$@") ;;
        *)  args+=(-L "$1") ;;
    esac
fi

exec ctest --test-dir "$build_dir" "${args[@]}"
