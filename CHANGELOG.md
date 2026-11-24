# Changelog

## 0.1.0 — 2026-11-24

首个独立版本。

* 从 ncnn `Mat` 抽取存储、引用计数、dims/strides/packing。
* `create` / `create_like` / `clone` / `clone_from` / `reshape`。
* `channel` / `depth` / `row` / `range` / `batch` 视图。
* `fill` 标量与 NEON/SSE/AVX/MSA/LSX/RISC-V 重载。
* 可选 `MMAT_BATCH`，与 ncnn 的 `NCNN_BATCH` 对齐。
* allocator 抽象为 `mmemory::Allocator`；未指定时走 `fast_malloc/fast_free`。
* mcnn 集成：`ncnn::Mat` 派生自 `mmat::Mat`；189/189 测试通过。
