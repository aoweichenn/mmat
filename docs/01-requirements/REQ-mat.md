# 需求 · Mat 存储与布局

## REQ-001 · 默认/外部数据构造与空语义

**契约**

* 默认构造得到 `dims == 0`、`data == nullptr`、`refcount == nullptr` 的空 `Mat`。
* 外部数据构造不接管所有权：`refcount == nullptr`，析构不释放调用方内存。
* `empty()` 在 `data == nullptr` 或 `total() == 0` 时为真。

**验收**

`Mat.DefaultIsEmpty`、`Mat.ExternalDataIsNotOwned`、`Mat.OneTwoThreeFourDimensions`。

## REQ-002 · 1D–4D 形状与 packed 元数据

**契约**

* 支持 1D/2D/3D/4D 构造，记录 `w/h/d/c` 与 `cstep`。
* packed 构造记录 `elemsize`、`elempack`；`elembits()` 返回单个元素位宽。
* `total()` 返回 `cstep * c`（batch 时按 `nstep * n` 取存储量）。

**验收**

`Mat.OneTwoThreeFourDimensions`、`Mat.ElemsizeBitsPerElement`。

## REQ-003 · 引用计数共享与释放

**契约**

* 拷贝构造/赋值共享 `data` 与 `refcount`，引用计数加一。
* `addref()` 显式加一；`release()` 减一，减到 0 时按 allocator 释放。
* `release()` 后对象回到空状态。

**验收**

`Mat.CopySharesStorage`、`Mat.AddrefIncrementsSharedCount`。

## REQ-004 · 深拷贝

**契约**

* `clone()` 返回新分配且内容一致的 `Mat`。
* `clone(allocator)` 使用指定 allocator。
* `clone_from()` 用源内容替换目标，不共享源存储。

**验收**

`Mat.CloneIsDeepCopy`、`Mat.CloneFromReplacesContents`。

## REQ-005 · reshape 视图与失败语义

**契约**

* `reshape` 在元素总数一致时返回共享存储的新形状视图。
* 元素总数不一致时返回空 `Mat`，不修改原对象。
* 支持 1D–4D 目标形状。

**验收**

`Mat.ReshapePreservesStorage`。

## REQ-006 · 视图与形状元数据

**契约**

* `channel` / `depth` / `row` / `range` / `channel_range` / `depth_range` /
  `row_range` 返回共享存储的视图，偏移按 `cstep` / `elemsize` 计算。
* `shape()` 只复制形状字段，`data` 与 `refcount` 为空。
* const 与非 const 版本语义一致。

**验收**

`Mat.ChannelRowDepthAndRangeViews`、`Mat.ShapeReturnsMetadataOnly`。

## REQ-007 · 分配与 allocator 复用

**契约**

* `create` / `create_like` 按给定形状与类型分配。
* 传入 `mmemory::Allocator` 时，分配/释放都经过该 allocator。
* 分配失败时对象为空，不抛异常。

**验收**

`Mat.CreateLikeCopiesShapeAndType`、`Mat.PoolAllocatorBackedStorage`。

## REQ-008 · 填充

**契约**

* `fill(float)` / `fill(std::int32_t)` / `fill<T>` 覆盖 `total()` 个元素。
* SIMD 分支（NEON/SSE/MSA/LSX/RISC-V）与标量语义一致。

**验收**

`Mat.FillWritesEveryElement`。

## REQ-009 · 可选 batch 维

**契约**

* `MMAT_BATCH=ON` 时 `Mat` 增加 `n` / `nstep`，`total()` 与 `batch()` /
  `batch_range()` 按 batch 步长计算。
* `MMAT_BATCH=OFF` 时 `n == 1`，不增加字段。
* 消费方必须与库使用相同的 `MMAT_BATCH`。

**验收**

`Mat.BatchAndBatchRangeViews`。

## REQ-010 · 可机械验证的质量

**契约**

* 固定宽度类型、公共头自包含、无裸字面量、无直接输出、无 throw。
* API 覆盖、用例断言、追溯矩阵、零告警、安装包消费方全部纳入
  `scripts/check-all.sh`。

**验收**

`scripts/check-all.sh`（全绿）。
