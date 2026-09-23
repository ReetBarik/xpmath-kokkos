# NOTICE

This repository is **multi-licensed**. All new code authored here is
`Apache-2.0 WITH LLVM-exception` (matching Kokkos). Under `vendor/xpmath/` lives
a byte-exact snapshot of [xpmath](https://github.com/ReetBarik/xpmath)
`include/xp/` and `LICENSES/` at a recorded tag; those files retain their
upstream SPDX identifiers and must not be hand-edited or relicensed.

The mapping below was verified against the SPDX headers in the vendored tree
(not assumed from any external checklist). Paths are relative to the repository
root.

## Per-path license mapping

| Path(s) | License (SPDX) | Notes |
|---|---|---|
| `vendor/xpmath/xp/dd_*.hpp` | `LicenseRef-DHB-License` | DDFUN v04 ports (real + complex). |
| `vendor/xpmath/xp/ff_*.hpp` | `LicenseRef-DHB-License` | DD→FF mechanical translation; inherits DHB, **not** LBNL-BSD. |
| `vendor/xpmath/xp/config.hpp` | `LicenseRef-DHB-License` | Verified SPDX in vendored header. |
| `vendor/xpmath/xp/trig_reduction.hpp`, `trig_reduction_data.hpp` | `LicenseRef-DHB-License` | Verified SPDX in vendored headers. |
| `vendor/xpmath/xp/qf_*.hpp` | `LicenseRef-LBNL-BSD-License` | QD 2.3.24–derived (4×FP32). |
| `vendor/xpmath/xp/tf_*.hpp` | `LicenseRef-LBNL-BSD-License` | Triple-float; SPDX is LBNL-BSD. |
| `vendor/xpmath/LICENSES/*` | as named | Verbatim upstream license texts. |
| Everything else in this repository | `Apache-2.0 WITH LLVM-exception` | Top-level `LICENSE`; wrappers, tests, scripts, docs. |

Full texts for the vendored identifiers live in
`vendor/xpmath/LICENSES/LicenseRef-DHB-License.txt` and
`vendor/xpmath/LICENSES/LicenseRef-LBNL-BSD-License.txt`.

## DHB-License §3 grant-back (quoted, not paraphrased)

From `vendor/xpmath/LICENSES/LicenseRef-DHB-License.txt`, clause 3:

> 3. You are under no obligation whatsoever to provide any modifications or enhancements of this software to anyone. However, if you choose to provide these modifications or enhancements to the author or make them publicly available, without enacting a separate written license agreement covering these modifications or enhancements, then you hereby grant to the author a non-exclusive, royalty-free perpetual license to install, use, modify, prepare derivative works, incorporate into other computer software, distribute, and sublicense such enhancements or derivative works thereof, in binary and source code form.

This clause applies to every file under `vendor/xpmath/xp/` whose SPDX line is
`LicenseRef-DHB-License`. It does **not** apply to
`LicenseRef-LBNL-BSD-License` files.

## Vendoring discipline

Refresh only via `scripts/sync_upstream.sh <tag>`. The ctest target
`vendor_fresh` (`scripts/check_vendor_fresh.sh`) re-fetches the tag in
`vendor/xpmath/UPSTREAM.txt` and fails on any drift. Never hand-edit
`vendor/xpmath/`.

## Contacts

- **DDFUN / DHB-License / commercial use:** David H. Bailey, <dhbailey@lbl.gov>.
- **QD / LBNL-BSD-License:** QD authors at LBNL; David H. Bailey is a co-author.
- **This repository:** https://github.com/ReetBarik/xpmath-kokkos
- **Upstream core:** https://github.com/ReetBarik/xpmath
