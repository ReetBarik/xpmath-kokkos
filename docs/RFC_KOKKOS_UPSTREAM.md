# Kokkos RFC draft — portable extended precision

**Status:** draft, ready for Reet to paste as a GitHub issue on `kokkos/kokkos`.
This file has not been posted. No Kokkos pull request is opened from this step.

## Motivation

Codes that need more than IEEE binary64 inside a Kokkos kernel either stay on
the host or depend on a compiler quad-precision scalar that has no device
path. [xpmath](https://github.com/ReetBarik/xpmath) tag `v0.2.1` (commit
`07540423aa56b1210d548acda943641c46516e94`) is a header-only C++17 library
with four shipped backends — DD, FF, QF, and TF — that compile with the host
compiler, `nvcc`, and `hipcc`. Format limits, the correctness argument, and
the device-versus-host record live there:

- [docs/CORRECTNESS.md](https://github.com/ReetBarik/xpmath/blob/v0.2.1/docs/CORRECTNESS.md)
- [docs/DOMAINS.md](https://github.com/ReetBarik/xpmath/blob/v0.2.1/docs/DOMAINS.md)
- [docs/DEVICE_PRECISION.md](https://github.com/ReetBarik/xpmath/blob/v0.2.1/docs/DEVICE_PRECISION.md)

This repository does not repeat those measurements. It asks whether a value
computed through the Kokkos wrapper equals, bit for bit, the value the core
computes in the same execution space. That holds on Serial, on CUDA (A100
`sm_80` job **1005372**, B200 `sm_100` job **1005384**), and on HIP (MI250
`gfx90a` job **1005373**, MI300 `gfx942` job **1005379**). Each device run
evaluates both calls inside one kernel (`both_in_kernel=yes`) and reports
zero differing cells. The logs are under `validation/` on
[xpmath-kokkos `36c672d`](https://github.com/ReetBarik/xpmath-kokkos/commit/36c672d69125).

## Proposed consumption

Vendor the core under `tpls/xpmath/` the way Kokkos 5.1.0 vendors
`tpls/mdspan` and `tpls/desul`, and place the `Kokkos::Experimental` wrappers
under `core/src/Kokkos_xpmath/`. `scripts/materialize_kokkos_pr.sh` does that
copy. Wrapper text and `xp` header text are not rewritten. The script also
appends an include-path and install registration to `core/src/CMakeLists.txt`:
`#include <xp/...>` is not on Kokkos's include path until `tpls/xpmath` is
registered, and the install tree has to receive those headers. That
registration is the same kind of CMake wiring `mdspan` and `desul` already
have in that file.

## (a) Vendored TPL, or core-native?

**Proposal: vendored TPL.**

Kokkos 5.1.0 already builds with bundled copies at `tpls/mdspan` and
`tpls/desul`, registered from `core/src/CMakeLists.txt`. The xpmath files are
not one license. [`NOTICE.md`](../NOTICE.md) maps `LicenseRef-DHB-License` and
`LicenseRef-LBNL-BSD-License` onto specific headers. New wrapper code is
Apache-2.0 WITH LLVM-exception. A byte copy into `tpls/` keeps that island.
Folding the algorithms into core sources under a single Kokkos license would
relicense files this repository does not relicense.

## (b) Naming and API

Keep `Kokkos::Experimental::` and the existing spelling: `DoubleDouble`,
`FloatFloat`, `QuadFloat`, `TripleFloat`, and the matching `*Complex` names.
`xp::` stays in the vendored headers. The wrappers are aliases and one-line
`Kokkos::` forwards.

These types are siblings of the optional host quad-precision math switch
`Kokkos_ENABLE_LIBQUADMATH`, not a fifth value of that switch. Where a wrapper
forwards, the call is the existing `Kokkos::` math spelling (`Kokkos::sqrt`,
`Kokkos::exp`, and the other forwarded functions). They are not a drop-in
replacement for that host scalar. Real and imaginary parts of the complex
types are written through the struct members, and `Kokkos::complex` of these
real types does not instantiate ([`COMPLEX_INTEROP.md`](COMPLEX_INTEROP.md)).

## (c) Licensing

New code in this repository, including the wrappers, is Apache-2.0 WITH
LLVM-exception (top-level [`LICENSE`](../LICENSE)).

Per-path map, from [`NOTICE.md`](../NOTICE.md), checked against the SPDX lines in
the vendored tree at xpmath `07540423aa56b1210d548acda943641c46516e94`:

| Path(s) under the vendored tree | SPDX |
|---|---|
| `xp/dd_*.hpp`, `xp/ff_*.hpp`, `xp/config.hpp`, `xp/trig_reduction.hpp`, `xp/trig_reduction_data.hpp` | `LicenseRef-DHB-License` |
| `xp/qf_*.hpp`, `xp/tf_*.hpp` | `LicenseRef-LBNL-BSD-License` |
| `LICENSES/*` | as named |

DHB-License §3, quoted from
`vendor/xpmath/LICENSES/LicenseRef-DHB-License.txt` (not paraphrased):

> 3. You are under no obligation whatsoever to provide any modifications or enhancements of this software to anyone. However, if you choose to provide these modifications or enhancements to the author or make them publicly available, without enacting a separate written license agreement covering these modifications or enhancements, then you hereby grant to the author a non-exclusive, royalty-free perpetual license to install, use, modify, prepare derivative works, incorporate into other computer software, distribute, and sublicense such enhancements or derivative works thereof, in binary and source code form.

That clause applies to files whose SPDX line is `LicenseRef-DHB-License`. It
does not apply to `LicenseRef-LBNL-BSD-License` files. Full texts travel with
the snapshot in `LICENSES/`.

## What already exists

- xpmath tag `v0.2.1`. CI:
  [`.github/workflows/ci.yml`](https://github.com/ReetBarik/xpmath/blob/v0.2.1/.github/workflows/ci.yml)
  with jobs `docs-fresh`, `standalone`, `no-kokkos-build`, `monotone-gate`,
  `build-and-test`, `device-nvcc`, and `device-hip`.
- This repository's CI, five gating lanes, none advisory:
  [`.github/workflows/ci.yml`](https://github.com/ReetBarik/xpmath-kokkos/blob/36c672d69125/.github/workflows/ci.yml)
  (`vendor-fresh`, `no-oracle-guard`, `build-and-test` with ctest count 13,
  `device-nvcc` for `sm_80` compile-only, `device-hip` for `gfx90a`
  compile-only).
- Snapshot refresh is `scripts/sync_upstream.sh <tag>`, checked by the
  `vendor_fresh` test. The materialized tree is produced by
  `scripts/materialize_kokkos_pr.sh`.
- That script was run on Kokkos 5.1.0 commit
  `3ec81abe1816109f6f62ac48cef41921f91a4d00`. The resulting Serial install
  (`Kokkos_ENABLE_LIBQUADMATH=OFF`) was the only Kokkos on the bit-identity
  compile line, plus this repo's `tests/bit_identity` headers. On
  2026-09-30 that test reported `exec_space=Serial both_in_kernel=no` and
  zero differing cells. Device jobs **1005372**, **1005384**, **1005373**,
  and **1005379** are the CUDA and HIP runs of the same test, recorded on
  `main` at `36c672d`.
