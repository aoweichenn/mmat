#!/usr/bin/env bash
# 组件安装包测试：确认 simpleomp/simplestl/simplemath 的导出 target 可用。
set -uo pipefail

# shellcheck source=_build.sh
. "$(dirname "$0")/_build.sh"

lib="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="$(mplatform_build_dir components)"
install_dir="${MPLATFORM_COMPONENTS_INSTALL_DIR:-$(mplatform_build_root)/install-components}"
consumer_build="${MPLATFORM_COMPONENTS_PACKAGE_DIR:-$(mplatform_build_root)/package-components}"

if [ ! -f "$build_dir/libmplatform_simpleomp.a" ]; then
    echo "找不到组件构建；先运行 scripts/build.sh components" >&2
    exit 2
fi

echo "== 安装组件到 $install_dir =="
rm -rf "$install_dir"
cmake --install "$build_dir" --prefix "$install_dir" > /dev/null || exit 1

echo "== 组件消费方 find_package + 编译 =="
rm -rf "$consumer_build"
cmake -S "$lib/tests/package/components" -B "$consumer_build" -G Ninja \
      -DCMAKE_PREFIX_PATH="$install_dir" > /dev/null || exit 1
cmake --build "$consumer_build" -j"$(nproc)" > /dev/null || exit 1
"$consumer_build/mplatform_components_consumer" || exit 1

echo "== 动态依赖审计 =="
if ldd "$consumer_build/mplatform_components_consumer" 2>/dev/null | grep -qE "libomp|libgomp|libz\.so|libvulkan"; then
    echo "!! 组件消费方链接了不该出现的动态库：" >&2
    ldd "$consumer_build/mplatform_components_consumer" | grep -E "libomp|libgomp|libz|libvulkan" >&2
    exit 1
fi
echo "CHECK OK: 无额外动态依赖"
echo
echo "组件包测试通过"
