# 验证 · M1

## 环境

| 项 | 值 |
| --- | --- |
| 主机 | Linux x86_64（Fedora 44） |
| CPU | 支持 AVX2 / AVX-VNNI，无 AVX-512 |
| GCC | 16.2.1 |
| Clang | 22.1.8 |
| CMake | 4.3.0 |
| GTest | 1.17.0（CMake config） |
| 依赖 | `../mmemory`、`../mplatform`（同工作区源码） |

## 1. 质量门

```bash
scripts/check-all.sh
```

**全部通过**：

| 项 | 结果 |
| --- | --- |
| format / types / headers / layers | 通过 |
| no-printf / no-throw / magic | 通过 |
| traceability | 10 条需求、15 个用例 |
| API coverage | 21/21 |
| nodiscard | 0/0（无“会失败”返回值的公开入口） |
| 用例断言 | 15/15 |
| GCC 零告警 | 0 |
| 单元测试 | 15/15 |
| 安装包消费方 | 通过 |

## 2. batch 配置

| 配置 | 构建 | 告警 | 测试 |
| --- | --- | --- | --- |
| `MMAT_BATCH=OFF`（默认） | 通过 | 0 | 15/15 |
| `MMAT_BATCH=ON` | 通过 | 0 | 15/15（含 batch 用例） |

## 3. 双编译器

```bash
scripts/build.sh test
scripts/build.sh test --clang
```

两者均 `零告警 ✓`，Clang 下 15/15 通过。

## 4. 内存、线程与静态分析

```bash
scripts/sanitize.sh
scripts/tsan.sh
scripts/analyze.sh tidy
scripts/analyze.sh memcheck
```

| 项 | 结果 |
| --- | --- |
| ASan + UBSan | 通过，无泄漏/越界 |
| TSan | 通过 |
| clang-tidy | 通过（0 warning） |
| valgrind memcheck | 通过，无 definite leak |

## 5. 安装包

```bash
scripts/package-test.sh
```

`libmmat.a` 安装到临时前缀，外部消费方通过 `find_package(mmat CONFIG)` 构建并
运行；`ldd` 审计确认没有额外共享库依赖。

## 6. ncnn 集成验证

```bash
cmake --build ncnn/build-mplatform -j"$(nproc)"
ctest --test-dir ncnn/build-mplatform --output-on-failure -j"$(nproc)"
```

构建参数：`NCNN_BATCH=ON`、`NCNN_VULKAN=OFF`、CPU-only。

* 构建成功；
* **189/189 测试通过**（含 squeezenet）；
* ncnn 安装到临时前缀后，外部程序 `#include <mat.h>`、`find_package(ncnn)`
  构建并运行成功，验证 `include/ncnn/{mmat,mmemory,mplatform}` 的包含关系。

## 7. 未验证项

| 项 | 原因 |
| --- | --- |
| ARM NEON / MIPS MSA / LoongArch LSX / RISC-V Vector | 只在 x86_64 上运行；分支做编译期与代码审查 |
| ncnn Vulkan 路径 | 本次以 `NCNN_VULKAN=OFF` 验证 |
| ncnn pixel 接口 | 属后续 `mimage`；当前由 ncnn 测试间接覆盖 |
| 真实 OOM | 未做 RLIMIT_AS 子进程注入 |
