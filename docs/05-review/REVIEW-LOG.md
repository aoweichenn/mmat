# 复审记录 · mmat

## RV-01 · 公共头内联实现带来 200+ 严格警告（已修，P1）

**发现**：把 ncnn `Mat` 的 inline 定义整体搬进公共头后，测试/新增代码用
严格警告集编译时，头文件里的 `-Wsign-conversion`、`-Wold-style-cast`
产生 200+ 条告警；这些告警会污染所有消费方。

**修复**：只保留 5 个模板在头里；其余实现移到 `src/mat.cpp`。移植实现使用
ncnn 上游同级警告集，新增代码继续用严格集。头文件从 ~1279 行降到 ~330 行。

## RV-02 · `shape()` 返回 `mmat::Mat` 导致重载歧义（已修，P1）

**发现**：`resolve_crop_roi(bottom_blob.shape(), ..., reference_blob.shape(), ...)`
在 ncnn 中有 `(Mat, Mat)` 与 `(Mat, const int*)` 两个重载。`mmat::Mat` 既能
转换成 `ncnn::Mat`，又有 `operator T*`，两个重载都可行 → ambiguous。

**修复**：`ncnn::Mat` 重新暴露 `shape()`，返回 `ncnn::Mat`。同一处理应用到
`clone`、`reshape`、`channel`、`depth`、`range`、`row_range`、
`channel_range`、`depth_range`、`batch`、`batch_range`，保证原有重载集
与三元表达式无需改动。

## RV-03 · 三元表达式两侧类型不一致（已修，P1）

**发现**：`Adims == 2 ? A.reshape(...) : A` 中，前者是 `mmat::Mat`，
后者是 `const ncnn::Mat`，条件运算符无法统一。

**修复**：同上，视图在 `ncnn::Mat` 层重新包装。

## RV-04 · allocator 指针类型回写失败（已修，P1）

**发现**：`Mat::allocator` 现在是 `mmemory::Allocator*`，而
`Option::blob_allocator` 等仍是 `ncnn::Allocator*`；44 处
`opt.blob_allocator = blob.allocator;` 编译失败。

**修复**：这些位置显式 `static_cast<Allocator*>(blob.allocator)`。所有
进入 ncnn `Mat` 的 allocator 都是 `ncnn::Allocator` 派生对象，向下转换有效。
保留 `Option` 的公共类型不变。

## RV-05 · 测试中 `release()` 后解引用 refcount（已修，P1）

**发现**：新增的 `Mat.AddrefIncrementsSharedCount` 在 `release()` 后读取
`*a.refcount`，而 `release()` 会把对象清空，触发 SEGFAULT。

**修复**：改为断言 `a.empty()`，不再解引用已清空对象。

## RV-06 · 移植代码的警告基线（决定）

**背景**：全局约定要求 `-Wconversion` / `-Wold-style-cast` 等 24 项。

**决定**：`src/mat.cpp` 使用 ncnn 上游同级警告集，仍要求 0 warning；
测试、头模板、脚本使用完整严格集。理由与边界写在
`docs/00-conventions/QUALITY-GATES.md`。这是本库唯一的有意偏差。

## RV-07 · 非 x86 分支

**决定**：ARM NEON、MIPS MSA、LoongArch LSX、RISC-V Vector 的 `fill`
分支只做编译期与代码审查，未在真实硬件上运行。`DES` 与 `VAL` 均明确标注。

---

## 复审结论

* 公共头、重载歧义、allocator 回写、测试生命周期问题均已修复。
* 当前无警告允许清单、无 catch 白名单、无 NOLINT。
* 唯一有意偏差是移植 `src/mat.cpp` 的警告集与上游保持一致，已在质量策略中
  说明。
