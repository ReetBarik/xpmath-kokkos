# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with
code in this repository.

## THE RULE THAT GOVERNS THIS ENTIRE REPOSITORY

**This repository never issues an accuracy verdict.**

- **xpmath owns** the MPFR/MPC oracle, the derived bound, the open-defect
  register, the host and device baselines, and every gate over them
  (`docs/CORRECTNESS.md` in xpmath).
- **xpmath-kokkos owns** one question only: *does a value computed through the
  Kokkos wrapper equal, bit for bit, the value the core computes directly?*

Bit-identity, not closeness. No ulps, no digits, no tolerances, no oracle.
There must be **no MPFR, no MPC, and no `__float128`** anywhere in this
repository outside `vendor/` (vendor is unmodified upstream). If an
integration bug makes a result *wrong rather than different*, the fix is in
xpmath and the evidence is a bit-identity failure here — not a second accuracy
measurement.

## Project Overview

**xpmath-kokkos** wraps the four xpmath backends (DD, FF, QF, TF) as
`Kokkos::Experimental` datatypes. It vendors a byte-exact snapshot of xpmath
`include/xp/` at a recorded tag under `vendor/xpmath/` — not `find_package`,
not FetchContent, not a submodule.

| path | role |
|---|---|
| `include/Kokkos_xpmath/` | Kokkos wrappers (filled by K1) |
| `vendor/xpmath/` | vendored xpmath `include/xp/` + `LICENSES/` at a tag |
| `tests/` | bit-identity tests only |
| `examples/` | usage examples / demos (later) |
| `scripts/sync_upstream.sh` | only legal way to refresh the vendor tree |
| `scripts/check_vendor_fresh.sh` | ctest `vendor_fresh` anti-rot guard |

Tracked progress: `docs/KOKKOS_LAYER_PLAN_STATUS.md`.

## Build

GCC 13.3.0, CMake 3.28.3 (JLSE). Project is **C++17**; Kokkos ≥5.1 may be
built at C++20 — keep that deliberate mismatch.

```bash
module use /soft/modulefiles && module load gcc/13.3.0 cmake/3.28.3
export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:$LD_LIBRARY_PATH

cmake -B build -DCMAKE_PREFIX_PATH=$HOME/kokkos-install-quadmath
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Other Kokkos installs on this host (for later device work):
`$HOME/kokkos-install-cuda-sm80-quadmath` (A100),
`~/xpm_device/kokkos-hip-gfx90a` (MI250).

## Vendoring

```bash
scripts/sync_upstream.sh v0.2.0   # only way to refresh vendor/
# never hand-edit vendor/xpmath/
ctest --test-dir build -R vendor_fresh
```

`vendor/xpmath/UPSTREAM.txt` records repository URL, tag, full commit SHA, and
UTC sync timestamp.

## Licensing

New code: `Apache-2.0 WITH LLVM-exception` (matches Kokkos). Vendored xpmath
is a license island — see `NOTICE.md`. Do not relicense vendored headers.

## Recovery note for later work

xpmath C10 deleted demos and `third_party/include/`. They last existed at
xpmath commit `158d618` (parent of C10). When porting wrappers (K1) or demos
(K6), recover from `git show 158d618:third_party/include/` (or equivalent),
**not** from the `v0.2.0` tree. K0 only vendors `include/xp/` at `v0.2.0`.
