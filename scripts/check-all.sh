#!/usr/bin/env bash
# mmat 本地全量质量门。任何一步失败即整体失败。
# 不包含 sanitize / coverage / valgrind（按需单独跑）。
set -uo pipefail

# shellcheck source=_build.sh
. "$(dirname "$0")/_build.sh"

lib="$(cd "$(dirname "$0")/.." && pwd)"
fail=0

run() {
    local name="$1"; shift
    printf '\n== %s ==\n' "$name"
    if ! "$@"; then
        printf '!! %s 失败\n' "$name"
        fail=1
    fi
}

run "格式"        "$(dirname "$0")/format.sh" --check
run "类型"        python3 "$(dirname "$0")/check-types.py" --check "$lib"
run "分层"        "$(dirname "$0")/check-layers.sh" "$lib"
run "无直接输出"  python3 "$(dirname "$0")/check-no-printf.py" --check "$lib"
run "不抛异常"    python3 "$(dirname "$0")/check-no-throw.py" --check
run "无常量字面量" python3 "$(dirname "$0")/check-magic-numbers.py" --check "$lib"
run "追溯矩阵"    python3 "$(dirname "$0")/check-traceability.py" --check
run "nodiscard"   python3 "$(dirname "$0")/check-nodiscard.py" --check
run "API 覆盖"    python3 "$(dirname "$0")/check-api-coverage.py" --check
run "用例断言"    python3 "$(dirname "$0")/check-test-assertions.py" --check
run "构建零告警"  "$(dirname "$0")/build.sh" test
run "头文件"      python3 "$(dirname "$0")/check-headers.py" --check "$lib" \
                      --build-include "$(mmat_build_dir test)/include" \
                      --build-include "$lib/../mmemory/include" \
                      --build-include "$(mmat_build_dir test)/mmemory/include" \
                      --build-include "$lib/../mplatform/include" \
                      --build-include "$(mmat_build_dir test)/mmemory/mplatform/include"
run "测试"        ctest --test-dir "$(mmat_build_dir test)" --output-on-failure
run "安装包"      "$(dirname "$0")/package-test.sh"

printf '\n'
if [ "$fail" -eq 0 ]; then
    printf '全部通过\n'
else
    printf '存在失败项\n'
fi
exit "$fail"
