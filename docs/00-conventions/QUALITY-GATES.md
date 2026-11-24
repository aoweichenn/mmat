# 质量策略 · mmat

沿用 mzip/mplatform/mmemory 的本地优先、零告警基线。

## 闸门

| 闸门 | 脚本 |
| --- | --- |
| 格式 | `format.sh --check` |
| 固定宽度类型 | `check-types.py` |
| 头文件自包含/尖括号 | `check-headers.py` |
| 分层 | `check-layers.sh` |
| 无直接输出 | `check-no-printf.py` |
| 无 throw | `check-no-throw.py` |
| 具名常量 | `check-magic-numbers.py` |
| 追溯矩阵 | `check-traceability.py` |
| API 覆盖 | `check-api-coverage.py` |
| `[[nodiscard]]` | `check-nodiscard.py` |
| 用例断言 | `check-test-assertions.py` |
| 零告警 | `build.sh test` |
| 单元测试 | `ctest` |
| 安装包 | `package-test.sh` |

## 警告基线

分两套，按文件责任划分：

* **新增代码**（`tests/`、公共头里的模板、脚本）使用完整严格集：
  `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
  -Wold-style-cast -Wcast-qual -Wdouble-promotion -Wformat=2 -Wundef
  -Wpointer-arith -Wwrite-strings -Wnull-dereference
  -Wzero-as-null-pointer-constant`，GCC 追加 `-Wmissing-declarations` 等。
* **移植的 Mat 核心**（`src/mat.cpp`）使用 ncnn 上游同级警告集：
  `-Wall -Wextra -Wpedantic -Wundef -Wpointer-arith -Wwrite-strings
  -Wnull-dereference`。原因：该文件是 ncnn BSD-3-Clause `Mat` 的逐行行为
  移植，保留上游 C 风格指针算术可以降低与上游合并时的漂移风险；要求它
  在更严格的 `-Wconversion` / `-Wold-style-cast` 下干净会迫使大范围改写
  数值代码，收益低于风险。`src/mat.cpp` 仍要求 **0 warning**。

两套警告都按 target 单独设置，禁止 `add_compile_options()`。

## 例外

当前无警告允许清单、无 catch 白名单、无 `// NOLINT`。

## 未实测平台

运行时只验证 Linux x86_64。ARM NEON、MIPS MSA、LoongArch LSX、RISC-V
Vector 的 `fill` / 布局分支只做编译期与代码审查，未在真实硬件上跑数值用例。
