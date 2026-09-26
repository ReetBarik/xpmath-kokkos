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

---

## K2 — `Kokkos::reduction_identity` specializations

**Branch:** `k2-reduction-identity`

**Outcome.** Green. All eight wrappers specialize `Kokkos::reduction_identity`
so they work as `parallel_reduce` accumulators. Real types expose
`sum` / `prod` / `max` / `min`; complex types expose `sum` / `prod` only
(Kokkos::complex convention). New ctest target `reduction_test` asserts
Serial-space bit-identity against a serial fold through the core `xp::`
type for every type × {sum, prod} × N ∈ {0, 1, 2, 10000}. Suite size is
exactly **10** (`vendor_fresh` + 8 smokes + `reduction_test`).

**What landed**

| path | change |
|---|---|
| `include/Kokkos_xpmath/{dd,ff,qf,tf}_math.hpp` | `reduction_identity` with sum/prod/max/min |
| `include/Kokkos_xpmath/{dd,ff,qf,tf}_complex.hpp` | `reduction_identity` with sum/prod only |
| `tests/reduction_test.cpp` | Serial bit-identity gate; device envelope noted in header |
| `CMakeLists.txt` | wire `reduction_test` ctest target |

**max/min extrema (format-derived, not leading-limb-only)**

Half-ulp non-overlapping cascade from the limb type's finite max:

| type | most-positive finite limbs |
|---|---|
| DD | `(DBL_MAX, 2^970)` via `from_bits(0x7FEF…FFFF, 0x7C9000…00)` |
| FF | `(FLT_MAX, 2^103)` via `from_bits(0x7F7FFFFF, 0x73000000)` |
| TF | `(FLT_MAX, 2^103, 2^79)` |
| QF | `(FLT_MAX, 2^103, 2^79, 2^55)` |

`max()` returns the negation (most-negative finite); `min()` returns the
most-positive. Constructors are not constexpr, so the methods are
`KOKKOS_FORCEINLINE_FUNCTION static` without `constexpr`.

**N=0 coverage.** Explicitly required and exercised for all eight types ×
both sum and prod (identity must equal zero / one). Also covered by the
direct `CHECK(reduction_identity<T>::sum() == T(0))` assertions.

**Gate**

```bash
module use /soft/modulefiles && module load gcc/13.3.0 cmake/3.28.3
export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:$LD_LIBRARY_PATH
cmake -B build -DCMAKE_PREFIX_PATH=$HOME/kokkos-install-quadmath
cmake --build build -j16
ctest --test-dir build --output-on-failure   # expect 10/10
test -d include && test -d tests || { echo "FAIL: wrong cwd"; exit 1; }
! grep -rn 'mpfr\|mpc_\|__float128' --include='*.cpp' --include='*.hpp' include/ tests/ \
  || { echo "FAIL: oracle machinery present; see the governing rule"; exit 1; }
```

**Measured (2026-09-24, JLSE gcc/13.3.0, Kokkos Serial quadmath install):**

```
 1/10 Test  #1: vendor_fresh .....................   Passed
 2/10 Test  #2: compile_smoke_dd_math ............   Passed
 3/10 Test  #3: compile_smoke_dd_complex .........   Passed
 4/10 Test  #4: compile_smoke_ff_math ............   Passed
 5/10 Test  #5: compile_smoke_ff_complex .........   Passed
 6/10 Test  #6: compile_smoke_qf_math ............   Passed
 7/10 Test  #7: compile_smoke_qf_complex .........   Passed
 8/10 Test  #8: compile_smoke_tf_math ............   Passed
 9/10 Test  #9: compile_smoke_tf_complex .........   Passed
10/10 Test #10: reduction_test ...................   Passed

100% tests passed, 0 tests failed out of 10
```

Oracle grep on `include/` and `tests/`: clean (`GATE_OK`).

**What K3 must know**

1. The eight types compile as `parallel_reduce` accumulators (Sum/Prod) on
   Serial. Max/Min identities exist for the four reals; they are not yet
   covered by a dedicated ctest (K2 gated sum/prod only).
2. K3 needs atomics — compare-and-swap over the multi-word expansion — so
   `Kokkos::atomic_add` (etc.) works on device Views of these types. Without
   that, scatter/atomic update patterns will not compile or will tear.
3. Still bit-identity only — no oracle, no ulps. Do not hand-edit `vendor/`.
4. K3 branch: `k3-atomics`. May develop in parallel with K4; serialise merges.

---

## K3 — Atomics

**Branch:** `k3-atomics` (from `main` @ `bab5dfa`).

**Outcome.** Green. `Kokkos::atomic_add` (and companion `atomic_fetch_add`) for
all eight wrappers via a compare-and-swap loop over a byte-equal integer view
of the whole expansion. Complex types are componentwise on `re`/`im`. New
ctest target `atomic_test` asserts Serial-space bit-identity against a serial
fold through the core `xp::` type, plus bit-exact exactly-representable-integer
sums (lost-update detector). Suite size is exactly **11** (`vendor_fresh` + 8
smokes + `reduction_test` + `atomic_test`).

**What landed**

| path | change |
|---|---|
| `include/Kokkos_xpmath/impl/atomic_cas_add.hpp` | CAS helper + prominent determinism caveat |
| `include/Kokkos_xpmath/{dd,ff,qf,tf}_math.hpp` | `atomic_add` / `atomic_fetch_add` overloads |
| `include/Kokkos_xpmath/{dd,ff,qf,tf}_complex.hpp` | componentwise `atomic_add` / `atomic_fetch_add` |
| `tests/atomic_test.cpp` | Serial bit-identity + exact-integer gate |
| `CMakeLists.txt` | wire `atomic_test` ctest target |

**CAS / strict aliasing.** Values move between `T` and an `AtomicWord<N>` POD
(`N/4` little-endian `uint32_t` limbs, `sizeof` matched to the expansion) only
via `std::memcpy` — never by reading a float/double glvalue as an integer.
The CAS itself is `Kokkos::atomic_compare_exchange` on that POD overlay of the
same storage bytes (pointer cast through `void*`). Hardware CAS when desul
considers the width lock-free; otherwise desul's address-locked CAS. Sizes:
DD/QF = 16 B, FF = 8 B, TF = 12 B (no native CAS width — always lock-based).
Complex = componentwise real CAS (not one CAS over the whole complex).

**Determinism documentation.** Lead comment block in
`include/Kokkos_xpmath/impl/atomic_cas_add.hpp` (and short pointers on each
public overload): atomic accumulation into a non-associative multi-word type
is order-dependent; last-limb differences across runs are a property of the
operation, not a defect.

**Exactly-representable-integer coverage.** `atomic_test` accumulates `1`
exactly `N` times for N ∈ {1, 2, 128, 1024} on every type (complex: `(1,0)`);
expects bit-identical `N` (or `(N,0)`). Separates lost updates from rounding
reorder. General non-integer Serial path still requires bit-identity against
the serial left-fold (Serial order is deterministic).

**Gate**

```bash
module use /soft/modulefiles && module load gcc/13.3.0 cmake/3.28.3
export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:$LD_LIBRARY_PATH
cmake -B build -DCMAKE_PREFIX_PATH=$HOME/kokkos-install-quadmath
cmake --build build -j16
ctest --test-dir build --output-on-failure   # expect 11/11
test -d include && test -d tests || { echo "FAIL: wrong cwd"; exit 1; }
! grep -rn 'mpfr\|mpc_\|__float128' --include='*.cpp' --include='*.hpp' include/ tests/ \
  || { echo "FAIL: oracle machinery present; see the governing rule"; exit 1; }
```

**Measured (2026-09-24, JLSE gcc/13.3.0, Kokkos Serial quadmath install):**

```
 1/11 Test  #1: vendor_fresh .....................   Passed
 2/11 Test  #2: compile_smoke_dd_math ............   Passed
 3/11 Test  #3: compile_smoke_dd_complex .........   Passed
 4/11 Test  #4: compile_smoke_ff_math ............   Passed
 5/11 Test  #5: compile_smoke_ff_complex .........   Passed
 6/11 Test  #6: compile_smoke_qf_math ............   Passed
 7/11 Test  #7: compile_smoke_qf_complex .........   Passed
 8/11 Test  #8: compile_smoke_tf_math ............   Passed
 9/11 Test  #9: compile_smoke_tf_complex .........   Passed
10/11 Test #10: reduction_test ...................   Passed
11/11 Test #11: atomic_test ......................   Passed

100% tests passed, 0 tests failed out of 11
```

Oracle grep on `include/` and `tests/`: clean (`GATE_OK`).

**What K4 must know**

1. Scatter-add / `Kokkos::atomic_add` works on Views of all eight types.
   Complex is componentwise (re then im), not a single CAS over the pair.
2. K4 decides `Kokkos::complex<T>` interop — implement conversions both ways
   with bit-identity round-trip, or document a measured negative in
   `docs/COMPLEX_INTEROP.md` and update the four `*_complex.hpp` notes in
   `include/` (not `vendor/`). Branch: `k4-complex-interop`.
3. Still bit-identity only — no oracle, no ulps. Do not hand-edit `vendor/`.
4. K3 may merge in parallel with K4 development; serialise the merges.

---

## K4 — Kokkos::complex interop: decide, implement or document

**Branch:** `k4-complex-interop` (from `main` @ `210cb4f`).

**Outcome.** Closed, negative. `Kokkos::complex<T>` does not instantiate for
`T` in {DoubleDouble, FloatFloat, QuadFloat, TripleFloat}, including the
`Kokkos::Experimental` aliases (they name the same `xp::` types). The
supported spelling is the standalone struct
(`Kokkos::Experimental::*Complex`). No conversion API. Evidence and the
compiler diagnostic are in `docs/COMPLEX_INTEROP.md`. The four wrapper
notes in `include/Kokkos_xpmath/{dd,ff,qf,tf}_complex.hpp` point at that
file. Suite size stays **11** (no new ctest target: the probe TU is
ill-formed by design and is not registered).

**Decision.** Do not force `Kokkos::complex<xp::Real>`. Kokkos 5.1.0
(`KOKKOS_VERSION 50100`) rejects the specialization at

```cpp
static_assert(std::is_floating_point_v<RealType> && ...);
```

before `real()` / `imag()`, arithmetic, or `Kokkos::abs` on that
specialization are usable. `std::is_floating_point_v<T>` is false for all
four class types. GCC 13.3.0 reports that note for each of them.
`TripleFloat` also fails `alignas(24)` under `KOKKOS_ENABLE_COMPLEX_ALIGN`
because `sizeof` is 12 and 24 is not a power of two.

**What landed**

| path | change |
|---|---|
| `docs/COMPLEX_INTEROP.md` | measured negative, diagnostic, supported spelling |
| `include/Kokkos_xpmath/{dd,ff,qf,tf}_complex.hpp` | note pointing at that decision |

**Gate**

```bash
module use /soft/modulefiles && module load gcc/13.3.0 cmake/3.28.3
export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:$LD_LIBRARY_PATH
cmake -B build -DCMAKE_PREFIX_PATH=$HOME/kokkos-install-quadmath
cmake --build build -j16
ctest --test-dir build --output-on-failure   # expect 11/11
test -d include && test -d tests || { echo "FAIL: wrong cwd"; exit 1; }
! grep -rn 'mpfr\|mpc_\|__float128' --include='*.cpp' --include='*.hpp' include/ tests/ \
  || { echo "FAIL: oracle machinery present; see the governing rule"; exit 1; }
```

**Measured (2026-09-24, JLSE gcc/13.3.0, Kokkos 5.1.0 Serial quadmath install):**

```
 1/11 Test  #1: vendor_fresh .....................   Passed    2.46 sec
 2/11 Test  #2: compile_smoke_dd_math ............   Passed    0.01 sec
 3/11 Test  #3: compile_smoke_dd_complex .........   Passed    0.00 sec
 4/11 Test  #4: compile_smoke_ff_math ............   Passed    0.00 sec
 5/11 Test  #5: compile_smoke_ff_complex .........   Passed    0.00 sec
 6/11 Test  #6: compile_smoke_qf_math ............   Passed    0.00 sec
 7/11 Test  #7: compile_smoke_qf_complex .........   Passed    0.00 sec
 8/11 Test  #8: compile_smoke_tf_math ............   Passed    0.00 sec
 9/11 Test  #9: compile_smoke_tf_complex .........   Passed    0.00 sec
10/11 Test #10: reduction_test ...................   Passed    0.51 sec
11/11 Test #11: atomic_test ......................   Passed    0.03 sec

100% tests passed, 0 tests failed out of 11
```

Oracle grep on `include/` and `tests/`: clean (`GATE_OK`).

**What K5 must know**

1. Complex values in the bit-identity campaign are the standalone
   `xp::*Complex` / `Kokkos::Experimental::*Complex` structs. Compare
   wrapper vs core in the same execution space. There is no
   `Kokkos::complex<T>` path to test.
2. K5 is the bit-identity campaign (63 ops × four backends). A device
   result is wrapper-on-device vs core-on-device, never device-wrapper
   vs host-core.
3. Still bit-identity only. Do not hand-edit `vendor/`.
4. Branch: `k5-bit-identity`, after this PR is on `main`.

---

## K5 — Bit-identity: the wrapper changes nothing

**Branch:** `k5-bit-identity` (from `main` @ `0272dbe`).

**Outcome.** Green. Zero differing cells on all three execution spaces.
`tests/bit_identity_test.cpp` compares raw limb bit patterns (including NaN
payloads) for all four backends × 63 operations over the grid copied from
xpmath `v0.2.0`. Suite size is exactly **12** (`vendor_fresh` + 8 smokes +
`reduction_test` + `atomic_test` + `bit_identity_test`). Cuda and HIP are
Cobalt jobs, not extra login-node ctest rows.

**What landed**

| path | change |
|---|---|
| `tests/data/sweep_grid.csv` + `PROVENANCE.txt` | byte copy of xpmath `v0.2.0` grid |
| `tests/bit_identity/sweep_{ops_core,ops_wrap,inputs}.hpp` | op inventory + evaluators |
| `tests/bit_identity_test.cpp` | Serial / Cuda / HIP bit-identity gate |
| `CMakeLists.txt` | `bit_identity_test` ctest target (TIMEOUT 7200) |
| `validation/a100/run_a100_bit_identity.sh` | Cobalt A100 job script |
| `validation/mi250/run_mi250_bit_identity.sh` | Cobalt MI250 job script |
| `validation/{a100,mi250}/logs/*_bit_identity_*.log` | committed device run logs |

**Pairing (stated in the test header).** Serial: wrapper inside
`Kokkos::parallel_for` versus core on the host. Cuda / HIP: both calls inside
the same kernel (wrapper-on-device vs core-on-device). Never device-wrapper vs
host-core. Complex values are the standalone `xp::*Complex` /
`Kokkos::Experimental::*Complex` structs.

**Device job IDs**

| arch | queue | node | job ID | exit |
|---|---|---|---|---|
| A100 (sm_80) | `gpu_a100` | gpu07 | **1004485** | 0 |
| MI250 (gfx90a) | `gpu_amd_mi250` | amdgpu04 | **1004486** | 0 |

**Gate**

```bash
module use /soft/modulefiles && module load gcc/13.3.0 cmake/3.28.3
export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:$LD_LIBRARY_PATH
cmake -B build -DCMAKE_PREFIX_PATH=$HOME/kokkos-install-quadmath
cmake --build build -j16
ctest --test-dir build --output-on-failure   # expect 12/12
test -d include && test -d tests || { echo "FAIL: wrong cwd"; exit 1; }
! grep -rn 'mpfr\|mpc_\|__float128' --include='*.cpp' --include='*.hpp' include/ tests/ \
  || { echo "FAIL: oracle machinery present; see the governing rule"; exit 1; }
```

**Measured (2026-09-26, JLSE gcc/13.3.0)**

Serial login-node ctest (`kokkos-install-quadmath`):

```
 1/12 Test  #1: vendor_fresh .....................   Passed
 2/12 Test  #2: compile_smoke_dd_math ............   Passed
 3/12 Test  #3: compile_smoke_dd_complex .........   Passed
 4/12 Test  #4: compile_smoke_ff_math ............   Passed
 5/12 Test  #5: compile_smoke_ff_complex .........   Passed
 6/12 Test  #6: compile_smoke_qf_math ............   Passed
 7/12 Test  #7: compile_smoke_qf_complex .........   Passed
 8/12 Test  #8: compile_smoke_tf_math ............   Passed
 9/12 Test  #9: compile_smoke_tf_complex .........   Passed
10/12 Test #10: reduction_test ...................   Passed
11/12 Test #11: atomic_test ......................   Passed
12/12 Test #12: bit_identity_test ................   Passed   53.18 sec

100% tests passed, 0 tests failed out of 12
```

Device runs (both `both_in_kernel=yes`, zero differing cells):

```
# A100 job 1004485
bit_identity_test: exec_space=Cuda both_in_kernel=yes
bit_identity_test: OK (4 backends × 63 ops × grid, zero differing cells)

# MI250 job 1004486
bit_identity_test: exec_space=HIP both_in_kernel=yes
bit_identity_test: OK (4 backends × 63 ops × grid, zero differing cells)
```

Oracle grep on `include/` and `tests/`: clean (`GATE_OK`).

**What K6 must know**

1. Bit-identity holds on Serial, A100, and MI250 for the v0.2.0 grid. The
   wrapper is a pure forward on that evidence.
2. K6 is the type-swap adoption example only — `examples/type_swap_kernel.cpp`
   plus a fast `type_swap_smoke` ctest. Do **not** recover the eight xpmath
   demos. Show the complex alias switch
   (`Kokkos::Experimental::DoubleDoubleComplex`, not `Kokkos::complex<DoubleDouble>`),
   including the three call-site differences recorded in the K6 plan section.
   Suite size becomes **13**.
3. Still bit-identity only. Do not hand-edit `vendor/`. Do not start K6 until
   this PR is on `main`. Branch: `k6-type-swap`.


---

## K6 — Adoption example: type swap

**Branch:** `k6-type-swap` (from `main` @ `ac7b30d`).

**Outcome.** Green. One adoption example shows the type-swap pattern: change
two aliases, keep calling `Kokkos::exp` / `sqrt` / … the same way, and use
the standalone complex struct (K4). New ctest target `type_swap_smoke` runs
the example. Suite size is exactly **13** (`12` from K5 + `type_swap_smoke`).
No timing demos, no accuracy columns, no demo suite.

**What landed**

| path | change |
|---|---|
| `examples/type_swap_kernel.cpp` | small Serial kernel; aliases + header comment |
| `examples/README.md` | two sentences pointing at the header and COMPLEX_INTEROP |
| `CMakeLists.txt` | wire `type_swap_smoke` ctest target (TIMEOUT 60) |
| `examples/.gitkeep` | removed (directory now has real content) |

**Aliases (supported spelling).** Precision is selected only by:

```cpp
using real_t    = Kokkos::Experimental::DoubleDouble;
using complex_t = Kokkos::Experimental::DoubleDoubleComplex;
```

The header comment shows the `Kokkos::complex<double>` spelling this
replaces and the analogous host-quadmath spelling side by side, and states
the three call-site differences that do not survive the swap: (1) no
conversion to/from `std::complex<double>`; (2) `real()` / `imag()` return a
copy — writes go to `z.re` / `z.im`; (3) `DoubleDoubleComplex` is 32 bytes,
not the 16 of `Kokkos::complex<double>`. The `__float128` token appears only
in that `examples/` comment (oracle gate greps `include/` and `tests/` only).

**libquadmath.** The smoke TU does not include quadmath headers and has no
quadmath symbols. `ldd` still shows `libquadmath.so.0` because the login-node
Kokkos install (`kokkos-install-quadmath`) was built with
`Kokkos_ENABLE_LIBQUADMATH` and exports `Kokkos::LIBQUADMATH` — every binary
in this tree that links `Kokkos::kokkos` inherits it. K7 CI builds Kokkos
without that TPL.

**Gate**

```bash
module use /soft/modulefiles && module load gcc/13.3.0 cmake/3.28.3
export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:$LD_LIBRARY_PATH
cmake -B build -DCMAKE_PREFIX_PATH=$HOME/kokkos-install-quadmath
cmake --build build -j16
ctest --test-dir build --output-on-failure   # expect 13/13
test -d include && test -d tests || { echo "FAIL: wrong cwd"; exit 1; }
! grep -rn 'mpfr\|mpc_\|__float128' --include='*.cpp' --include='*.hpp' include/ tests/ \
  || { echo "FAIL: oracle machinery present; see the governing rule"; exit 1; }
```

**Measured (2026-09-26, JLSE gcc/13.3.0, Kokkos Serial quadmath install):**

```
 1/13 Test  #1: vendor_fresh .....................   Passed    1.86 sec
 2/13 Test  #2: compile_smoke_dd_math ............   Passed    0.01 sec
 3/13 Test  #3: compile_smoke_dd_complex .........   Passed    0.00 sec
 4/13 Test  #4: compile_smoke_ff_math ............   Passed    0.00 sec
 5/13 Test  #5: compile_smoke_ff_complex .........   Passed    0.00 sec
 6/13 Test  #6: compile_smoke_qf_math ............   Passed    0.00 sec
 7/13 Test  #7: compile_smoke_qf_complex .........   Passed    0.00 sec
 8/13 Test  #8: compile_smoke_tf_math ............   Passed    0.00 sec
 9/13 Test  #9: compile_smoke_tf_complex .........   Passed    0.00 sec
10/13 Test #10: reduction_test ...................   Passed    0.52 sec
11/13 Test #11: atomic_test ......................   Passed    0.02 sec
12/13 Test #12: bit_identity_test ................   Passed   52.96 sec
13/13 Test #13: type_swap_smoke ..................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 13
```

Oracle grep on `include/` and `tests/`: clean (`GATE_OK`).

**What K7 must know**

1. Suite size is exactly **13**. K7's build-and-test lane must assert that
   count so a silently unregistered target fails CI.
2. K7 adds GitHub Actions (`vendor-fresh`, `no-oracle-guard`,
   `build-and-test`, `device-nvcc`, `device-hip`). Build Kokkos without
   `Kokkos_ENABLE_LIBQUADMATH`. Do not start K7 until this PR is on `main`.
3. Still bit-identity only. Do not hand-edit `vendor/`. Branch: `k7-ci`.
