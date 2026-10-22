#!/bin/bash
set -euo pipefail

BUILD_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Environment overrides preserve the locally selected toolchain by default.
CXX="${CXX:-/opt/homebrew/opt/llvm/bin/clang++}"
RISCV_TOOLCHAIN="${RISCV_TOOLCHAIN:-/Users/stevenyin/opt/toolchains/riscv-gnu-toolchain-2026-08-27/install}"
RISCV_SYSROOT="${RISCV_SYSROOT:-${RISCV_TOOLCHAIN}/sysroot}"
RISCV_TARGET="${RISCV_TARGET:-riscv64-unknown-linux-gnu}"
RISCV_ARCH="${RISCV_ARCH:-rv64gcv}"
RISCV_ABI="${RISCV_ABI:-lp64d}"
EXTRA_FLAGS=()
case "${CXX##*/}" in
    *clang*) EXTRA_FLAGS+=("--target=${RISCV_TARGET}" "--gcc-toolchain=${RISCV_TOOLCHAIN}") ;;
esac
EXTRA_FLAGS+=("--sysroot=${RISCV_SYSROOT}" "-march=${RISCV_ARCH}" "-mabi=${RISCV_ABI}" -O3 -std=c++17)
# Whitespace-separated extra flags (no shell evaluation). For arguments with
# spaces, source this file and append to the EXTRA_FLAGS array directly.
if [[ -n "${CXXFLAGS:-}" ]]; then
    read -r -a USER_CXX_FLAGS <<< "$CXXFLAGS"
    EXTRA_FLAGS+=("${USER_CXX_FLAGS[@]}")
fi

compile_kernel() {
    local assembly="" output_dir is_assembly=0
    local -a args=("$@") report_flags=()
    local i

    for ((i = 0; i < ${#args[@]}; i++)); do
        case "${args[i]}" in
            -S) is_assembly=1 ;;
            -o) assembly="${args[i+1]:-}" ;;
        esac
    done

    if [[ "$is_assembly" == 0 || -z "$assembly" ]]; then
        "$CXX" "${EXTRA_FLAGS[@]}" "${args[@]}"
        return
    fi

    case "$assembly" in
        */*) output_dir="${assembly%/*}" ;;
        *) output_dir=. ;;
    esac

    case "${CXX##*/}:$("$CXX" --version)" in
        *clang*)
            report_flags=(
                '-Rpass=loop-vectorize|slp-vectorizer'
                '-Rpass-missed=loop-vectorize|slp-vectorizer'
                '-Rpass-analysis=loop-vectorize|slp-vectorizer'
                -fsave-optimization-record
                "-foptimization-record-file=${output_dir}/vectorization.yaml"
            )
            ;;
        *GCC*|*gcc*|*g++*) report_flags=(-fopt-info-vec-all) ;;
        *)
            printf 'Unsupported compiler for vectorization reports: %s\n' "$CXX" >&2
            return 1
            ;;
    esac

    if "$CXX" "${EXTRA_FLAGS[@]}" "${report_flags[@]}" "${args[@]}" \
        > "${output_dir}/vectorization.txt" 2>&1; then
        return 0
    else
        local status=$?
        printf 'Assembly compilation failed; see %s/vectorization.txt\n' "$output_dir" >&2
        return "$status"
    fi
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    exec bash "${BUILD_ROOT}/kernels/build.sh"
fi
