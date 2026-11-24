# 代码风格 · mmat

全局 C++ 约定见本机 `cpp-conventions` 规范；本文件记录本库的具体决定。

## 命名

| 类别 | 形式 | 例 |
| --- | --- | --- |
| 类型 | `PascalCase` | `Mat` |
| 函数/变量 | `snake_case` | `create_like`、`elemsize` |
| 私有成员 | `snake_case_` | 本库暂无私有数据成员 |
| 常量 | `kPascalCase` | `kDims3`、`kCstepAlignment` |
| 宏 | `MMAT_*` | `MMAT_BATCH`、`MMAT_FORCEINLINE` |

## 兼容字段

`Mat` 的公开字段沿用 ncnn 名称，属于刻意保留的兼容契约：

```
data refcount elemsize elempack allocator dims w h d c cstep
#if MMAT_BATCH
n nstep
#endif
```

这些字段被 ncnn 层和调用方直接读取，因此不改成 `w_` / `cstep_` 之类的形式。

## 类型与内存

* 只用固定宽度类型与 `std::size_t`；字节用 `std::uint8_t`。
* 分配统一走 `mmemory::Allocator`；未指定 allocator 时使用
  `mmemory::fast_malloc` / `fast_free`。
* 引用计数用 `std::atomic_ref` 的 `acq_rel` 语义，等价于 ncnn 原来的
  `__sync_fetch_and_add`：自增返回旧值，减到 1 时释放存储。
* 布局常量集中在 `src/mat.cpp`：
  * `kCstepAlignment = 16`：每个 channel 的字节对齐；
  * `kRefcountAlignment = 4`：refcount 追加区的对齐；
  * `kBatchStrideAlignment = 4096`：batch 步长的对齐。

## 错误处理

* 分配失败时 `data == nullptr`，`Mat::empty()` 为真。
* `reshape` 尺寸不匹配时返回空 `Mat`，不修改原对象。
* `create` 失败后对象保持空，不抛异常。
* 本库不包含 `throw` / `catch`。

## 分层

`public`（include/mmat）→ `core`（src）。公共头不 include `src/`。
`.cpp-layers` + `check-layers.sh` 强制执行。

## 移植代码的例外

`src/mat.cpp` 的算法主体来自 ncnn 的 BSD-3-Clause `Mat` 实现，数值与布局
行为必须逐行保持一致。移植部分允许保留上游的 C 风格指针算术，编译时使用
上游同级的警告集（`-Wall -Wextra -Wpedantic` 等），详见
`docs/00-conventions/QUALITY-GATES.md`。新增代码（测试、脚本、公共头模板）
仍使用完整严格警告集。
