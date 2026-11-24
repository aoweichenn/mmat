# 实现 · M1

## 文件地图

```
include/mmat/
  mat.hpp          公共 API：Mat 类声明 + 5 个 must-inline 模板
  config.hpp       CMake 生成：版本、MMAT_BATCH、导出/内联宏

src/mat.cpp        构造/赋值、引用计数、clone/reshape/create、视图、fill

tests/unit/mat_tests.cpp
tests/package/consumer/
scripts/            质量闸门
docs/               00–05 + traceability
```

## 公共头只保留声明

`mat.hpp` 现在约 330 行：

* 字段与类声明；
* 5 个必须在头里的模板：`fill<T>`、`row<T>`（const/non-const）、
  `operator T*`（const/non-const）。

其余实现全部在 `src/mat.cpp`。这样：

1. 公共头自包含、可读；
2. 移植代码的警告基线可以独立控制；
3. 消费方编译时不再解析大段内联实现。

## 与 ncnn 原实现的差异

| 项 | ncnn `Mat` | `mmat::Mat` |
| --- | --- | --- |
| allocator 类型 | `ncnn::Allocator*` | `mmemory::Allocator*` |
| 引用计数 | `NCNN_XADD` | `std::atomic_ref` / `detail::xadd` |
| 布局常量 | 裸字面量 `16`、`4`、`4096` | `kCstepAlignment` 等 |
| 像素/Vulkan/量化 | 同类成员 | 留在 ncnn 派生层 |
| batch 宏 | `NCNN_BATCH` | `MMAT_BATCH`（集成时强制相等） |
| 日志 | `NCNN_LOGE` | 本库无日志；分配失败返回空 |
| 头文件 | 全量 inline | 声明 + 少量模板，其余 out-of-line |

数值与布局行为保持一致：`cstep` 16 字节对齐、refcount 4 字节对齐、
batch 步长 4096 字节对齐、`total()` 公式均未改。

## ncnn 集成要点

`ncnn/src/mat.h`：

* `class Mat : public mmat::Mat`，`using mmat::Mat::Mat`；
* 保留 Vulkan、pixel、`substract_mean_normalize`、`from_float16` /
  `from_bfloat16`；
* 把返回 `mmat::Mat` 的视图重新包装成 `ncnn::Mat`，避免重载歧义；
* `using mmat::Mat::create_like;` 防止派生类隐藏基类重载。

`ncnn/src/mat.cpp`：

* 删除 CPU 核心实现（已移入 mmat），保留 Vulkan、像素相关入口、
  `substract_mean_normalize`、float16/bfloat16 转换与 cast/quantize 辅助函数。

`ncnn/src/CMakeLists.txt`：

* `MMAT_BATCH = NCNN_BATCH`；
* `add_subdirectory(mmat)`，把 `mmat_objs` merge 进 `libncnn`；
* 安装 `include/ncnn/mmat`。

`ncnn::Allocator` 继承 `mmemory::Allocator`，`allocate/deallocate` 转发
`fastMalloc/fastFree`。ncnn 代码中 44 处把 `Mat::allocator`（基类指针）
赋回 `ncnn::Allocator*` 的位置使用显式 `static_cast`；这些 Mat 的分配来源
都是 ncnn allocator，转换有效。
