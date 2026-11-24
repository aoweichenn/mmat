#!/usr/bin/env bash
# 强制 .cpp-layers 声明单向依赖：rank 只能 include rank <= 自己的头。
# 用法: scripts/check-layers.sh <lib_dir>
set -uo pipefail

lib="${1:-.}"
layers_file="$lib/.cpp-layers"

if [ ! -f "$layers_file" ]; then
    echo "缺少 $layers_file" >&2
    exit 1
fi

python3 - "$lib" "$layers_file" <<'PY'
import pathlib, re, sys

lib = pathlib.Path(sys.argv[1])
layers_file = pathlib.Path(sys.argv[2])

layers = []          # (name, rank, src_dirs, include_prefixes)
for raw in layers_file.read_text(encoding='utf-8').split('\n'):
    line = raw.strip()
    if not line or line.startswith('#'):
        continue
    parts = line.split('\t')
    if len(parts) < 4:
        continue
    layers.append((parts[0], int(parts[1]), parts[2].split(), parts[3].split()))

def classify(path: pathlib.Path):
    """返回该文件所属层的 (name, rank)，找不到返回 None。"""
    rel = path.relative_to(lib).as_posix()
    for name, rank, src_dirs, _ in layers:
        for src_dir in src_dirs:
            if rel.startswith(src_dir.rstrip('/') + '/'):
                return name, rank
    return None

def target_layer(include: str):
    """include 的尖括号路径属于哪一层。"""
    for name, rank, _, prefixes in layers:
        for prefix in prefixes:
            if include.startswith(prefix):
                return name, rank
    return None

problems = []
for path in sorted(lib.glob('src/**/*.cpp')) + sorted(lib.glob('src/**/*.hpp')):
    owner = classify(path)
    if owner is None:
        continue
    for match in re.finditer(r'^\s*#\s*include\s+<([^>]+)>', path.read_text(encoding='utf-8'), re.M):
        target = target_layer(match.group(1))
        if target is None:
            continue
        if target[1] > owner[1]:
            problems.append(f"{path.relative_to(lib)}: {owner[0]}(rank {owner[1]}) "
                            f"依赖了更高层 {target[0]}(rank {target[1]}) via <{match.group(1)}>")

if problems:
    for problem in problems:
        print(f"FAIL {problem}", file=sys.stderr)
    print(f"\n共 {len(problems)} 处倒挂", file=sys.stderr)
    sys.exit(1)

print(f"CHECK OK: layers clean ({len(layers)} layers)")
PY
