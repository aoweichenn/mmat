# mmat

`mcnn` 的 `Mat` 存储与布局模块：引用计数、dims/strides/packing、形状变换与视图。
C++20，无外部运行时依赖；分配走 `mmemory`，CPU 探测走 `mplatform`。

**范围刻意很窄**：不包含像素/图像、Vulkan `VkMat`、Layer 量化或
`substract_mean_normalize`。这些留在 ncnn 派生层或后续 `mimage`。

## 能力

| 项 | 状态 |
| --- | --- |
| 1D–4D `Mat`，`elemsize` / `elempack` / `cstep` | ✅ |
| 引用计数共享与深拷贝 | ✅ |
| `create` / `create_like` / `reshape` / `clone` | ✅ |
| `channel` / `depth` / `row` / `range` / `batch` 视图 | ✅ |
| 标量与 SIMD `fill` | ✅ |
| 可选 batch 维（`MMAT_BATCH`） | ✅ |
| `mmemory::Allocator` 接入 | ✅ |
| 分配失败返回空 `Mat`，不抛异常 | ✅ |

## 快速开始

```cpp
#include <mmat/mmat.hpp>

mmat::Mat mat;
mat.create(224, 224, 3, 4U);       // w, h, c, elemsize
mat.fill(0.0F);

mmat::Mat flat = mat.reshape(224 * 224 * 3);
mmat::Mat view = mat.channel(0);
```

作为 mcnn 的 Mat 核心：

```cpp
ncnn::Mat m;
m.create(224, 224, 3, 4u);         // ncnn::Mat : public mmat::Mat
ncnn::Mat b = m.clone();
```

## 构建

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DMMAT_BATCH=ON
cmake --build build -j"$(nproc)"
ctest --test-dir build
```

作为子项目：

```cmake
add_subdirectory(path/to/mmat)
target_link_libraries(app PRIVATE mmat::mmat)
```

它默认从 `../mmemory` 引入 allocator 层，而 `mmemory` 再引入 `../mplatform`。
独立构建需要三者位于同一父目录，或用 `-DMMAT_MMEMORY_DIR=...` 指定。

`MMAT_BATCH` 改变 `Mat` 的字段布局与 ABI，消费方必须与库保持一致。

## 质量门

```bash
scripts/check-all.sh
```

包含 format、固定宽度类型、头文件自包含、分层、无直接输出、无 throw、具名常量、
追溯矩阵、API 覆盖、用例断言、零告警、单元测试、安装包测试。

## 与 mcnn 的关系

ncnn 的 `Mat` 其余部分（像素、Vulkan、`substract_mean_normalize`、
`from_float16` / `from_bfloat16`）保留为派生类成员。`ncnn::Allocator` 继承
`mmemory::Allocator`，旧 `fastMalloc` / `fastFree` API 不变。

许可证与第三方声明见 `LICENSE`、`THIRD_PARTY_NOTICES.md`。
