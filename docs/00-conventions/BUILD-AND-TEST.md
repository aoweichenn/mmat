# 构建与测试 · mmat

## 依赖

* CMake ≥ 3.24、Ninja、GCC 或 Clang（C++20）。
* 同工作区的 `../mmemory` 与 `../mplatform` 源码树。
* 测试需要 GTest ≥ 1.17 的 CMake config。

## 单独构建

```bash
cmake -S . -B /tmp/mmat_build -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -Dmmat_build_tests=ON \
      -DMMAT_BATCH=ON
cmake --build /tmp/mmat_build -j"$(nproc)"
ctest --test-dir /tmp/mmat_build --output-on-failure
```

脚本封装：

```bash
scripts/build.sh test          # GCC，Debug，构建 + 零告警检查
scripts/build.sh test --clang  # Clang，独立构建目录
scripts/test.sh                # ctest
scripts/sanitize.sh            # ASan + UBSan
scripts/tsan.sh                # TSan
scripts/analyze.sh             # clang-tidy + valgrind
scripts/coverage.sh            # gcovr（可选）
scripts/package-test.sh        # 安装到临时前缀 + 外部消费方
scripts/check-all.sh           # 全部本地闸门
```

## 关键选项

| 选项 | 默认 | 说明 |
| --- | --- | --- |
| `MMAT_BATCH` | `OFF` | 打开 batch 维；**必须与消费方一致** |
| `mmat_build_tests` | 顶层工程为 `ON` | 是否构建测试 |
| `MMAT_SANITIZE` | `OFF` | ASan + UBSan |
| `MMAT_MMEMORY_DIR` | `../mmemory` | mmemory 源码目录 |

`MMAT_BATCH` 进入生成头 `mmat/config.hpp`，改变 `Mat` 的字段布局与 ABI。
ncnn 集成时会强制 `MMAT_BATCH = NCNN_BATCH`。

## 安装包

```bash
scripts/package-test.sh
```

脚本安装到 `/tmp/mmat_build/install`，再用 `find_package(mmat CONFIG)` 构建
`tests/package/consumer`。`libmmat.a` 已合并 `mmemory` 与 `mplatform` 的目标
文件，消费方不需要额外链接依赖。

## ncnn 集成

ncnn 侧由 `ncnn/src/CMakeLists.txt` 执行：

```cmake
set(MMAT_BATCH ${NCNN_BATCH} CACHE BOOL "" FORCE)
add_subdirectory(${NCNN_MMAT_DIR} ${CMAKE_CURRENT_BINARY_DIR}/mmat)
target_sources(ncnn PRIVATE $<TARGET_OBJECTS:mmat_objs>)
```

安装时把 `include/mmat` 放到 `include/ncnn/mmat`，消费方仍只需
`#include <ncnn/mat.h>`。
