# 追溯矩阵 · mmat

> `scripts/check-traceability.py --check` 校验：REQ-001..010 无缺号、每条有 `●`、
> 引用的文件存在、`Suite.Case` 真实存在、**每个 TEST 都被本文提及**。

## 1. 需求

| 需求 | 设计 | 实现 | 测试 |
| --- | --- | --- | --- |
| **REQ-001** 默认/外部数据构造与空语义 | DES §2 | ● `include/mmat/mat.hpp`<br>● `src/mat.cpp` | ● `Mat.DefaultIsEmpty`<br>● `Mat.ExternalDataIsNotOwned`<br>● `Mat.OneTwoThreeFourDimensions` |
| **REQ-002** 1D–4D 形状与 packed 元数据 | DES §2 | ● `include/mmat/mat.hpp`<br>● `src/mat.cpp` | ● `Mat.OneTwoThreeFourDimensions`<br>● `Mat.ElemsizeBitsPerElement` |
| **REQ-003** 引用计数共享与释放 | DES §3 | ● `src/mat.cpp` | ● `Mat.CopySharesStorage`<br>● `Mat.AddrefIncrementsSharedCount` |
| **REQ-004** 深拷贝 | DES §4 | ● `src/mat.cpp` | ● `Mat.CloneIsDeepCopy`<br>● `Mat.CloneFromReplacesContents` |
| **REQ-005** reshape 视图与失败语义 | DES §4 | ● `src/mat.cpp` | ● `Mat.ReshapePreservesStorage` |
| **REQ-006** 视图与形状元数据 | DES §5 | ● `include/mmat/mat.hpp`<br>● `src/mat.cpp` | ● `Mat.ChannelRowDepthAndRangeViews`<br>● `Mat.ShapeReturnsMetadataOnly` |
| **REQ-007** 分配与 allocator 复用 | DES §6 | ● `src/mat.cpp` | ● `Mat.CreateLikeCopiesShapeAndType`<br>● `Mat.PoolAllocatorBackedStorage` |
| **REQ-008** 填充 | DES §2 | ● `include/mmat/mat.hpp`<br>● `src/mat.cpp` | ● `Mat.FillWritesEveryElement` |
| **REQ-009** 可选 batch 维 | DES §2 | ● `include/mmat/mat.hpp`<br>● `src/mat.cpp` | ● `Mat.BatchAndBatchRangeViews` |
| **REQ-010** 可机械验证的质量 | QUALITY | ● `scripts/check-all.sh` | ● `scripts/check-all.sh`（全绿） |

## 2. 用例清单（反向检查）

● `Mat.DefaultIsEmpty`
● `Mat.OneTwoThreeFourDimensions`
● `Mat.CopySharesStorage`
● `Mat.CloneIsDeepCopy`
● `Mat.CloneFromReplacesContents`
● `Mat.CreateLikeCopiesShapeAndType`
● `Mat.ElemsizeBitsPerElement`
● `Mat.ReshapePreservesStorage`
● `Mat.ChannelRowDepthAndRangeViews`
● `Mat.FillWritesEveryElement`
● `Mat.ExternalDataIsNotOwned`
● `Mat.ShapeReturnsMetadataOnly`
● `Mat.PoolAllocatorBackedStorage`
● `Mat.AddrefIncrementsSharedCount`
● `Mat.BatchAndBatchRangeViews`

## 3. 未覆盖项

| 项 | 原因 |
| --- | --- |
| 真实 OOM 注入 | 分配失败路径靠 allocator 返回 `nullptr` 的代码审查；未做 RLIMIT_AS 子进程 |
| 非 x86 运行时 | 只验证 Linux x86_64；NEON/MSA/LSX/RISC-V 分支做编译期与代码审查 |
| ncnn Vulkan 路径 | 本次集成以 `NCNN_VULKAN=OFF` 验证；`Mat::create_like(VkMat)` 等保留在 ncnn 层 |
| pixel 接口 | 属后续 `mimage`，当前只在 ncnn 测试中间接覆盖 |
