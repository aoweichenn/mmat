#!/usr/bin/env bash
# 构建目录解析：所有脚本共用，避免各自硬编码 /tmp 路径。
#
# 用法（在其它脚本里）：
#   . "$(dirname "$0")/_build.sh"
#   dir=$(mmat_build_dir test)      # → $MMAT_BUILD_ROOT/test
set -uo pipefail

mmat_build_root() {
    printf '%s\n' "${MMAT_BUILD_ROOT:-/tmp/mmat_build}"
}

mmat_build_dir() {
    local purpose="$1"
    printf '%s\n' "$(mmat_build_root)/$purpose"
}

# 若目标目录属于别的源码树则清空，返回 0。
mmat_ensure_build_dir() {
    local lib="$1"
    local dir="$2"
    local cache="$dir/CMakeCache.txt"

    if [ ! -f "$cache" ]; then
        mkdir -p "$dir"
        return 0
    fi

    local recorded
    recorded=$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "$cache" | head -1)
    if [ "$recorded" != "$lib" ]; then
        printf '构建目录 %s 属于 %s，与本源码树不同，清空重建\n' "$dir" "$recorded" >&2
        rm -rf "$dir"
        mkdir -p "$dir"
    fi
    return 0
}
