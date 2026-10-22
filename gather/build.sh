#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$ROOT/build.sh"
ids=(06 16 25 30 31 32 33 41 44 47 55 61 69 95 98)
if (($#)); then ids=("$@"); fi
for id in "${ids[@]}"; do
    [[ "$id" =~ ^(06|16|25|30|31|32|33|41|44|47|55|61|69|95|98)$ ]] || { echo "Unknown gather kernel ID: $id" >&2; exit 2; }
    dirs=("$ROOT"/kernels/"${id}"_*)
    dir="${dirs[0]}"
    kernel="${dir##*/}"
    build="$dir/build"
    mkdir -p "$build"
    flags=("${EXTRA_FLAGS[@]}" -ffp-contract=off -fno-lto)
    compiler="$CXX"
    if [[ "${GATHER_HOST_ONLY:-0}" == 1 ]]; then
        compiler="${HOST_CXX:-/usr/bin/clang++}"
        flags=(-O2 -std=c++17 -ffp-contract=off -fno-lto -DGATHER_HOST_ONLY)
        if [[ -n "${HOST_CXXFLAGS:-}" ]]; then read -r -a extra <<< "$HOST_CXXFLAGS"; flags+=("${extra[@]}"); fi
    fi
    case "${compiler##*/}" in *clang*) flags+=(-Wno-register) ;; esac
    # Python escapes metadata as C string literals; it is never evaluated by a shell.
    version="$("$compiler" --version)"
    python3 - "$build/build_info.h" "$kernel" "$compiler" "$version" "${flags[@]}" <<'PY'
import json, pathlib, sys
path, kernel, compiler, version, *flags = sys.argv[1:]
meta = compiler + ' | ' + version.splitlines()[0] + ' | ' + ' '.join(flags)
pathlib.Path(path).write_text('#define GATHER_KERNEL '+json.dumps(kernel)+'\n#define GATHER_BUILD_INFO '+json.dumps(meta)+'\n')
PY
    sources=("$dir/src/kernel.cpp" "$ROOT/gather/adapters/$id.cpp" "$ROOT/gather/main.cpp" "$ROOT/gather/input.cpp" "${GATHER_PLATFORM_SOURCE:-$ROOT/gather/platform.cpp}")
    if [[ "${GATHER_HOST_ONLY:-0}" != 1 ]]; then sources+=("$dir/variants/rvv.cpp"); fi
    echo "Building $kernel"
    if ! "$compiler" "${flags[@]}" -I"$dir/include" -I"$ROOT/gather" -I"$build" "${sources[@]}" -lm -o "$build/benchmark" >"$build/benchmark-build.log" 2>&1; then
        cat "$build/benchmark-build.log" >&2; exit 1
    fi
 done
