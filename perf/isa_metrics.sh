#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
#
# usage: perf/isa_metrics.sh <build-dir> <arch> [science]
#
# Writes perf/results/<arch>/isa_<xpmath-tag-or-sha>.csv from a perf build
# (see perf/README.md). arch: gfx90a gfx942 sm_80 sm_100 sm_103.
# With "science": writes science_isa_<tag>.csv for the science-shaped kernel
# (build target xpk_perf_throughput).
set -euo pipefail

if [ $# -lt 2 ] || [ $# -gt 3 ] || { [ $# -eq 3 ] && [ "$3" != science ]; }; then
  echo "usage: $0 <build-dir> <arch> [science]" >&2
  exit 2
fi
build=$1
arch=$2
prefix=isa
extra=()
if [ $# -eq 3 ]; then
  prefix=science_isa
  extra=(--science)
fi
here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

ver_file="$build/perf/xp_version.txt"
if [ ! -f "$ver_file" ]; then
  echo "$ver_file not found: configure with -DXPMATH_KOKKOS_BUILD_PERF=ON" >&2
  exit 1
fi
tag=$(tr -d '\n' < "$ver_file" | tr '/ ' '__')
out="$here/results/$arch/${prefix}_${tag}.csv"

case "$arch" in
  gfx90a|gfx942)
    exec python3 -I "$here/isa_metrics_hip.py" "$build" "$out" ${extra[@]+"${extra[@]}"}
    ;;
  sm_80|sm_100|sm_103)
    exec python3 -I "$here/isa_metrics_cuda.py" "$build" "$out" "$arch" ${extra[@]+"${extra[@]}"}
    ;;
  *)
    echo "arch '$arch' is not supported (gfx90a gfx942 sm_80 sm_100 sm_103)" >&2
    exit 2
    ;;
esac
