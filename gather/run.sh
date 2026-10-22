#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# On the target: bash gather/run.sh --samples 100 --verify
# Every entry runs independently; options are forwarded to each executable.
for id in 06 16 25 30 31 32 33 41 44 47 55 61 69 95 98; do
    dirs=("$ROOT"/kernels/"${id}"_*)
    binary="${dirs[0]}/build/benchmark"
    [[ -x "$binary" ]] || { echo "Missing benchmark: $binary" >&2; exit 1; }
    while IFS= read -r entry; do
        "$binary" --entry "$entry" --output "${dirs[0]}/build/gather-${entry}.csv" "$@"
    done < <("$binary" --list-entries)
done
