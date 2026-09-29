#!/usr/bin/env bash
# ===========================================================================
# validation/mi300/run_mi300_bit_identity.sh — MI300 (gfx942) bit-identity
# ===========================================================================
# Builds Kokkos 5.1.0 HIP for gfx942 if the prefix is missing, then builds
# and runs bit_identity_test on the allocated GPU. Wrapper and core are both
# evaluated inside the same HIP kernel (see tests/bit_identity_test.cpp).
# This is not an accuracy sweep.
#
# SUBMIT:
#     qsub -A pepper_hep -n 1 -t 360 -q gpu_amd_mi300x --mode script \
#          validation/mi300/run_mi300_bit_identity.sh
#
#COBALT -A pepper_hep
#COBALT -n 1
#COBALT -t 360
#COBALT -q gpu_amd_mi300x

set -uo pipefail

_self=$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)
REPO_ROOT=${REPO_ROOT:-$(cd "$_self/../.." && pwd)}
BUILD_DIR=${BUILD_DIR:-$REPO_ROOT/build-mi300-bit-identity}
KOKKOS_PREFIX=${KOKKOS_PREFIX:-$HOME/xpm_device/kokkos-hip-gfx942}
KOKKOS_SRC=${KOKKOS_SRC:-$HOME/xpm_device/kokkos-5.1.0-hip-src}
KOKKOS_BUILD=${KOKKOS_BUILD:-$HOME/xpm_device/kokkos-hip-gfx942-build}

if [ ! -f "$REPO_ROOT/include/Kokkos_xpmath/Kokkos_xpmath.hpp" ]; then
  echo "FATAL: REPO_ROOT=$REPO_ROOT does not look like xpmath-kokkos." >&2
  exit 2
fi

LOGDIR="$REPO_ROOT/validation/mi300/logs"
mkdir -p "$LOGDIR"
STAMP=$(date +%Y%m%d_%H%M%S)
LOG="$LOGDIR/mi300_bit_identity_${STAMP}.log"

exec > >(tee -a "$LOG") 2>&1

echo "==========================================================="
echo " MI300 bit-identity"
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

if [ ! -f "$KOKKOS_PREFIX/lib64/cmake/Kokkos/KokkosConfig.cmake" ] && \
   [ ! -f "$KOKKOS_PREFIX/lib/cmake/Kokkos/KokkosConfig.cmake" ]; then
  echo
  echo "--- build Kokkos 5.1.0 HIP gfx942 ---"
  if [ ! -d "$KOKKOS_SRC/.git" ]; then
    rm -rf "$KOKKOS_SRC"
    git clone --depth 1 --branch 5.1.0 https://github.com/kokkos/kokkos.git "$KOKKOS_SRC"
  fi
  export CXX=hipcc
  cmake -S "$KOKKOS_SRC" -B "$KOKKOS_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$KOKKOS_PREFIX" \
    -DCMAKE_CXX_STANDARD=20 \
    -DCMAKE_CXX_COMPILER=hipcc \
    -DKokkos_ENABLE_SERIAL=ON \
    -DKokkos_ENABLE_HIP=ON \
    -DKokkos_ARCH_AMD_GFX942=ON \
    -DKokkos_ENABLE_OPENMP=OFF \
    -DKokkos_ENABLE_LIBQUADMATH=OFF \
    -DKokkos_ENABLE_TESTS=OFF \
    -DKokkos_ENABLE_EXAMPLES=OFF \
    -DKokkos_ENABLE_BENCHMARKS=OFF \
    -DBUILD_TESTING=OFF
  cmake --build "$KOKKOS_BUILD" -j16
  rc=$?
  if [ "$rc" -ne 0 ]; then
    echo "FATAL: Kokkos build exited $rc" >&2
    exit "$rc"
  fi
  cmake --install "$KOKKOS_BUILD"
  rc=$?
  if [ "$rc" -ne 0 ]; then
    echo "FATAL: Kokkos install exited $rc" >&2
    exit "$rc"
  fi
else
  echo "Kokkos prefix present: $KOKKOS_PREFIX"
fi

echo
echo "--- configure + build ---"
CXX=${CXX_TEST:-hipcc}
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
