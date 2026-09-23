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
