# 第三方声明

## ncnn `Mat`

`include/mmat/mat.hpp` 与 `src/mat.cpp` 的 `Mat` 存储/布局算法来自 ncnn 的
`src/mat.h` / `src/mat.cpp`（Tencent，BSD-3-Clause，Copyright 2017 Tencent）。

本仓库的改动：

* 命名空间与类型名改为 `mmat` / `Mat`；
* allocator 抽象改为 `mmemory::Allocator`；
* dims/strides/packing 常量具名化（`kCstepAlignment` 等）；
* 去除像素、Vulkan、Layer 量化相关成员，保留在 ncnn 派生层；
* 引用计数改用 `std::atomic_ref`，语义与 `NCNN_XADD` 一致；
* 增加独立测试、安装包与质量闸门。

## mcnn 平台层与 allocator

实现依赖 `mmemory`（BSD-3-Clause）与 `mplatform`（BSD-3-Clause）。两者的
公共头随 `mmat` 安装，便于消费方直接使用；`mmat` 的公共 API 不暴露
`mplatform` 类型。
