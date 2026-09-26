#!/usr/bin/env bash
# ===========================================================================
# validation/mi250/run_mi250_bit_identity.sh — K5 MI250 (gfx90a) Cobalt job
# ===========================================================================
# Rebuilds against ~/xpm_device/kokkos-hip-gfx90a and runs bit_identity_test
# on the allocated GPU. Wrapper and core are both evaluated inside the same
# HIP kernel (see tests/bit_identity_test.cpp header).
#
# SUBMIT:
#     qsub -A pepper_hep -n 1 -t 180 -q gpu_amd_mi250 --mode script \
#          validation/mi250/run_mi250_bit_identity.sh
#
#COBALT -A pepper_hep
#COBALT -n 1
#COBALT -t 180
#COBALT -q gpu_amd_mi250

set -uo pipefail

_self=$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)
REPO_ROOT=${REPO_ROOT:-$(cd "$_self/../.." && pwd)}
BUILD_DIR=${BUILD_DIR:-$REPO_ROOT/build-mi250-bit-identity}
KOKKOS_PREFIX=${KOKKOS_PREFIX:-$HOME/xpm_device/kokkos-hip-gfx90a}

if [ ! -f "$REPO_ROOT/include/Kokkos_xpmath/Kokkos_xpmath.hpp" ]; then
  echo "FATAL: REPO_ROOT=$REPO_ROOT does not look like xpmath-kokkos." >&2
  exit 2
fi

LOGDIR="$REPO_ROOT/validation/mi250/logs"
mkdir -p "$LOGDIR"
STAMP=$(date +%Y%m%d_%H%M%S)
LOG="$LOGDIR/mi250_bit_identity_${STAMP}.log"

exec > >(tee -a "$LOG") 2>&1

echo "==========================================================="
echo " MI250 bit-identity (K5)"
echo " date      : $(date -Is)"
echo " host      : $(hostname)"
echo " repo      : $REPO_ROOT"
echo " commit    : $(cd "$REPO_ROOT" && git rev-parse HEAD 2>/dev/null || echo unknown)"
echo " cobalt id : ${COBALT_JOBID:-unset}"
echo " build dir : $BUILD_DIR"
echo " kokkos    : $KOKKOS_PREFIX"
echo " log       : $LOG"
echo "==========================================================="

if ! command -v module >/dev/null 2>&1 && [ -r /etc/profile.d/modules.sh ]; then
  # shellcheck disable=SC1091
  . /etc/profile.d/modules.sh
fi
if command -v module >/dev/null 2>&1; then
  module use /soft/modulefiles 2>/dev/null
  for m in gcc/13.3.0 cmake/3.28.3 rocm/7.0.2; do
    echo "  module load $m"
    module load "$m" || echo "  WARNING: module load $m failed"
  done
fi
export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:${LD_LIBRARY_PATH:-}

echo
echo "--- provenance ---"
rocminfo 2>/dev/null | awk '/Marketing Name:|Name: +gfx/{print}' | head -8 | sed 's/^/    /' || true
hipcc --version 2>&1 | head -5 | sed 's/^/    /' || true
g++ --version 2>&1 | head -1 | sed 's/^/    /'

echo
echo "--- configure + build ---"
CXX=${CXX:-hipcc}
export CXX
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
  -DCMAKE_PREFIX_PATH="$KOKKOS_PREFIX" \
  -DCMAKE_CXX_COMPILER="$CXX"
cmake --build "$BUILD_DIR" -j16 --target bit_identity_test
rc=$?
if [ "$rc" -ne 0 ]; then
  echo "FATAL: build exited $rc" >&2
  exit "$rc"
fi

BIN=$BUILD_DIR/bit_identity_test
if [ ! -x "$BIN" ]; then
  echo "FATAL: missing $BIN" >&2
  exit 2
fi

echo
echo "--- run bit_identity_test ---"
"$BIN"
rc=$?
echo
echo "bit_identity_test exit: $rc"
echo "log: $LOG"
echo "COBALT_JOBID=${COBALT_JOBID:-unset}"
exit "$rc"
