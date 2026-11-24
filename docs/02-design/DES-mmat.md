# 设计 · mmat

## 1. 边界

`mmat` 只负责 `Mat` 的：

* 存储分配与释放；
* 引用计数；
* dims / strides / packing 元数据；
* 形状变换与视图；
* 填充、float16/bfloat16 之外的纯布局操作。

**不包含**：

* 像素/图像编解码（后续 `mimage`）；
* Vulkan `VkMat` / `VkImageMat`；
* 基于 `Layer` / `ParamDict` 的 `substract_mean_normalize`；
* `from_float16` / `from_bfloat16`（依赖 ncnn 的 cast layer）。

这些留在 ncnn 的派生 `Mat` 中。

## 2. 字段与布局

```
data       存储首地址（外部数据时不为空但不拥有）
refcount   引用计数，位于分配尾部 4 字节对齐区
elemsize   单个元素字节数
elempack   打包数量
allocator  mmemory::Allocator*，为空时用 fast_malloc/fast_free
dims       1..4
w h d c    各维长度
cstep      一个 channel 的元素步长（按 16 字节对齐）
#if MMAT_BATCH
n           batch 数
nstep       一个 batch 的元素步长（按 4096 字节对齐）
#endif
```

`total()` 返回 `cstep * c`；batch 分配时存储量为 `nstep * n`。

## 3. 引用计数

* `addref()` 用 `std::atomic_ref<std::int32_t>::fetch_add(1, acq_rel)`。
* `release()` 用 `fetch_add(-1, acq_rel)`，返回旧值 1 时释放。
* 分配时 `refcount` 放在 `data + totalsize`，按 4 字节对齐。
* 外部数据构造不设置 `refcount`，析构不动调用方内存。

这与 ncnn 的 `NCNN_XADD` 语义一致：`__sync_fetch_and_add` 返回旧值。

## 4. create / reshape / clone

* `create` 先判断形状与 allocator 是否完全一致，一致则直接返回，避免重复分配。
* 需要分配时先 `release()`，再计算 `cstep`/`nstep`，最后请求
  `allocator->allocate()` 或 `mmemory::fast_malloc()`。
* `reshape` 校验元素总数；一致则构造共享存储的视图，不一致返回空 `Mat`。
* `clone` 分配同样形状的新存储并 `memcpy`；`clone_from` 复用目标已有的
  分配器语义。

## 5. 视图

`channel` / `depth` / `row` / `range` 系列通过指针偏移构造新 `Mat`：

* channel 偏移：`cstep * index * elemsize`；
* depth 偏移：`w * h * index * elemsize`；
* row 偏移：`w * index * elemsize`；
* batch 偏移：`nstep * index * elemsize`。

视图共享 `refcount`，不额外分配。

## 6. allocator 桥接

`mmat::Mat` 的 `allocator` 类型是 `mmemory::Allocator*`。ncnn 侧：

```cpp
class Allocator : public mmemory::Allocator
{
public:
    void* allocate(std::size_t size) override { return fastMalloc(size); }
    void deallocate(void* ptr) override { fastFree(ptr); }
    virtual void* fastMalloc(std::size_t size) = 0;
    virtual void fastFree(void* ptr) = 0;
};
```

这样 `Mat` 只依赖 mmemory 的抽象接口，ncnn 的 `PoolAllocator` /
`UnlockedPoolAllocator` 继续实现 `fastMalloc/fastFree`，旧 API 不变。

## 7. ncnn 集成

`ncnn::Mat` 改为 `public mmat::Mat` 的派生类：

* 继承构造函数与赋值；
* 补 `VkMat` 相关 `create_like`、pixel 成员、`substract_mean_normalize`、
  `from_float16` / `from_bfloat16`；
* 将 `mmat::Mat` 返回的视图（`clone`、`reshape`、`shape`、`channel` 等）
  重新包装为 `ncnn::Mat`，保持原有重载集与三元表达式可编译；
* `ncnn::Mat` 仍是无数据成员的薄派生类，拷贝/移动只操作基类。

`MMAT_BATCH` 必须等于 `NCNN_BATCH`，否则字段布局不一致。

## 8. 自包含静态库

`libmmat.a` 合并 `mmat_objs`、`mmemory_objs`、`mplatform_objs`，并把
`mmemory` / `mplatform` 的公共头安装到同一 include 根。消费方只需
`find_package(mmat CONFIG)`，不额外声明依赖。
