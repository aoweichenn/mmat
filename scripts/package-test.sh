#!/usr/bin/env bash
# Install mmat to a clean prefix and verify a source-tree-external consumer.
set -uo pipefail

# shellcheck source=_build.sh
. "$(dirname "$0")/_build.sh"

lib="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="$(mmat_build_dir test)"
install_dir="${MMAT_INSTALL_DIR:-$(mmat_build_root)/install}"
consumer_build="${MMAT_PACKAGE_DIR:-$(mmat_build_root)/package-consumer}"

if [ ! -f "$build_dir/libmmat.a" ]; then
    echo "missing $build_dir/libmmat.a; run scripts/build.sh test first" >&2
    exit 2
fi

echo "== install to $install_dir =="
rm -rf "$install_dir"
cmake --install "$build_dir" --prefix "$install_dir" > /dev/null || exit 1

echo "== consumer find_package + build =="
rm -rf "$consumer_build"
cmake -S "$lib/tests/package/consumer" -B "$consumer_build" -G Ninja \
      -DCMAKE_PREFIX_PATH="$install_dir" > /dev/null || exit 1
cmake --build "$consumer_build" -j"$(nproc)" > /dev/null || exit 1
"$consumer_build/mmat_consumer" || exit 1

echo "== dynamic dependency audit =="
if ldd "$consumer_build/mmat_consumer" 2>/dev/null | grep -qE "libmplatform|libmmat"; then
    echo "!! consumer unexpectedly links a shared mmat/mplatform library" >&2
    exit 1
fi
echo "CHECK OK: no extra shared library"
echo
echo "package test passed"
