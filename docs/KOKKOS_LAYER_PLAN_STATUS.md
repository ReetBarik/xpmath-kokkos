# KOKKOS_LAYER_PLAN_STATUS.md

This file records the outcome of each section of the Kokkos-layer arc — the
repository that wraps xpmath backends as Kokkos datatypes and asks only
bit-identity questions. One STATUS block per completed section, appended in
completion order, newest last. It follows the convention of xpmath's
`docs/CORE_PLAN_STATUS.md`: each block states what was done, what was
measured, and what the next section must know.

---

## K0 — Create the repository, vendor v0.2.0, establish the licensing island

**Branch:** `main` (empty-repo bootstrap; no prior history to branch from).

**Outcome.** Green. Repository layout, Apache-2.0 WITH LLVM-exception top-level
license, `NOTICE.md` license island (SPDX-verified), governing rule in
`CLAUDE.md`, vendored xpmath at tag `v0.2.0`, and ctest `vendor_fresh` all
landed on `main`.

**Vendored pin**

- repository: `https://github.com/ReetBarik/xpmath.git`
- tag: `v0.2.0`
- commit: `669aa6905f0d4900950a1308e6efa02a1d1d4257` (merge of PR #37 / C10)
- sync via `scripts/sync_upstream.sh v0.2.0`; freshness via
  `scripts/check_vendor_fresh.sh` / ctest `vendor_fresh`

**SPDX correction vs naive path list.** Verified against vendored headers:

| path | SPDX |
|---|---|
| `vendor/xpmath/xp/dd_*.hpp` | `LicenseRef-DHB-License` |
| `vendor/xpmath/xp/ff_*.hpp` | `LicenseRef-DHB-License` (not LBNL-BSD) |
| `vendor/xpmath/xp/config.hpp`, `trig_reduction*.hpp` | `LicenseRef-DHB-License` |
| `vendor/xpmath/xp/qf_*.hpp`, `tf_*.hpp` | `LicenseRef-LBNL-BSD-License` |
| everything else in this repo | `Apache-2.0 WITH LLVM-exception` |

DHB §3 grant-back is quoted verbatim in `NOTICE.md`.

**Gate**

```bash
module use /soft/modulefiles && module load gcc/13.3.0 cmake/3.28.3
export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:$LD_LIBRARY_PATH
cmake -B build -DCMAKE_PREFIX_PATH=$HOME/kokkos-install-quadmath
cmake --build build -j16
ctest --test-dir build --output-on-failure   # expect 1/1: vendor_fresh
test -d include && test -d tests || { echo "FAIL: wrong cwd"; exit 1; }
! grep -rn 'mpfr\|mpc_\|__float128' --include='*.cpp' --include='*.hpp' include/ tests/ \
  || { echo "FAIL: oracle machinery present; see the governing rule"; exit 1; }
```

**Measured (2026-09-23, JLSE gcc/13.3.0, Kokkos quadmath install):**

```
1/1 Test #1: vendor_fresh .....................   Passed    2.50 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   2.51 sec
```

Oracle grep on `include/` and `tests/`: clean (`GATE_OK`).

**What K1 must know**

1. Wrappers do **not** exist at `v0.2.0` under `third_party/include/` — C10
   deleted that tree. Recover from xpmath commit `158d618`
   (`git show 158d618:third_party/include/` or equivalent), then place under
   `include/Kokkos_xpmath/`.
2. Bit-identity only. No MPFR, MPC, `__float128` in `include/` or `tests/`.
3. Project C++17; Kokkos install may be C++20 — keep the mismatch.
4. Branch from updated `main` as `k1-wrapper-headers`.
5. Do not start K1 until this K0 commit is on `main` and the gate above is green.

---

## K1 — Port the wrapper headers and the Kokkos::Experimental surface

**Branch:** `k1-wrapper-headers` (from `main` @ `def9bd1`).

**Outcome.** Green. Eight Kokkos::Experimental wrappers recovered from xpmath
`158d618`, TF `Kokkos::` forwards completed to the dd/ff/qf one-line pattern,
`Kokkos::real` / `Kokkos::imag` added for all four complex types, umbrella
header, installable `xpmath_kokkos::xpmath_kokkos` package-config, and eight
per-header compile smokes. Suite size is exactly **9** (`vendor_fresh` + 8
smokes). Gate extras compile as build-only TUs (not ctest).

**Recovery.** Wrappers are **not** at `v0.2.0` under `third_party/include/`
(C10 deleted that tree). Recovered with
`git show 158d618:third_party/include/<file>` from the xpmath clone and placed
under `include/Kokkos_xpmath/`. Includes remain `#include <xp/...>` against the
K0 INTERFACE path `vendor/xpmath`. Each `*_complex.hpp` also includes its
matching `*_math.hpp` wrapper so a lone complex include can name
`Experimental::<Real>` return types in the `Kokkos::` forwards.

**TF completeness finding.** Against `vendor/xpmath/xp/tf_math.hpp` at
`v0.2.0`, the `Kokkos::Experimental` using-list was already complete for the
public surface (internals like `tf_divide_core` / `tf_three_sum` stay unexported,
matching QF). The size asymmetry vs `qf_math.hpp` was the **`Kokkos::`
re-exposure block**: at `158d618` TF used bare `using xp::fn;` inside
`namespace Kokkos` instead of the sibling `KOKKOS_INLINE_FUNCTION` one-line
forwards. K1 replaced that block with the dd/ff/qf pattern. No public TF math
symbols were missing from `Experimental`.

**Adoption ergonomics.** Historical one-line `Kokkos::` math forwards preserved
(and completed for TF). Free `Kokkos::real` / `Kokkos::imag` overloads added on
all four complex types, forwarding to `.real()` / `.imag()`. No host
quad-precision type overloads; `Kokkos_ENABLE_LIBQUADMATH` is not required.

**Install.** `find_package(xpmath_kokkos)` works via exported
`xpmath_kokkos::xpmath_kokkos` (wrappers under `include/Kokkos_xpmath/`,
vendored core under `include/xp/`). Optional DESTDIR consumer smoke against the
install prefix: green.

**Gate**

```bash
module use /soft/modulefiles && module load gcc/13.3.0 cmake/3.28.3
export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:$LD_LIBRARY_PATH
cmake -B build -DCMAKE_PREFIX_PATH=$HOME/kokkos-install-quadmath
cmake --build build -j16
ctest --test-dir build --output-on-failure   # expect 9/9
test -d include && test -d tests || { echo "FAIL: wrong cwd"; exit 1; }
! grep -rn 'mpfr\|mpc_\|__float128' --include='*.cpp' --include='*.hpp' include/ tests/ \
  || { echo "FAIL: oracle machinery present; see the governing rule"; exit 1; }
```

Build-only (not ctest): `compile_gate_all_forwards`, `compile_gate_real_imag`.

**Measured (2026-09-24, JLSE gcc/13.3.0, Kokkos Serial quadmath install):**

```
1/9 Test #1: vendor_fresh .....................   Passed
2/9 Test #2: compile_smoke_dd_math ............   Passed
3/9 Test #3: compile_smoke_dd_complex .........   Passed
4/9 Test #4: compile_smoke_ff_math ............   Passed
5/9 Test #5: compile_smoke_ff_complex .........   Passed
6/9 Test #6: compile_smoke_qf_math ............   Passed
7/9 Test #7: compile_smoke_qf_complex .........   Passed
8/9 Test #8: compile_smoke_tf_math ............   Passed
9/9 Test #9: compile_smoke_tf_complex .........   Passed

100% tests passed, 0 tests failed out of 9
```

Oracle grep on `include/` and `tests/`: clean (`GATE_OK`).
`find_package` DESTDIR smoke: green.

**What K2 must know**

1. Wrappers are usable as `Kokkos::Experimental` types; `Kokkos::exp` / `sin` /
   … and `Kokkos::real` / `imag` spellings work. No `reduction_identity` yet —
   `parallel_reduce` with a DD/FF/QF/TF accumulator will not compile until K2.
2. Specialize `Kokkos::reduction_identity<T>` for all eight types; complex omits
   `max()`/`min()` per Kokkos `complex` convention. Bit-identity vs serial core
   in Serial space; device spaces use the re-association envelope (see plan K2).
3. Do not start K2 until this K1 PR is on `main` (or ready for human merge) and
   the 9/9 gate above is green. Branch: `k2-reduction-identity`.
4. Still bit-identity only — no oracle, no ulps, no host quad-precision types
   outside `vendor/`.
