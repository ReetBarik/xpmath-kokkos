#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
#
# usage: perf/isa_metrics.sh <build-dir> <arch>
#
# Writes perf/results/<arch>/isa_<xpmath-tag-or-sha>.csv from a perf build
# (see perf/README.md). arch: gfx90a gfx942 sm_80 sm_100.
set -euo pipefail

if [ $# -ne 2 ]; then
  echo "usage: $0 <build-dir> <arch>" >&2
  exit 2
fi
build=$1
arch=$2
here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

ver_file="$build/perf/xp_version.txt"
if [ ! -f "$ver_file" ]; then
  echo "$ver_file not found: configure with -DXPMATH_KOKKOS_BUILD_PERF=ON" >&2
  exit 1
fi
tag=$(tr -d '\n' < "$ver_file" | tr '/ ' '__')
out="$here/results/$arch/isa_${tag}.csv"

case "$arch" in
  gfx90a|gfx942)
    exec python3 -I "$here/isa_metrics_hip.py" "$build" "$out"
    ;;
  sm_80|sm_100)
    exec python3 -I "$here/isa_metrics_cuda.py" "$build" "$out" "$arch"
    ;;
  *)
    echo "arch '$arch' is not supported (gfx90a gfx942 sm_80 sm_100)" >&2
    exit 2
    ;;
esac
