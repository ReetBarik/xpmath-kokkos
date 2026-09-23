# xpmath-kokkos

Kokkos datatype wrappers for
[xpmath](https://github.com/ReetBarik/xpmath) — the four extended-precision
backends (DD, FF, QF, TF) exposed as `Kokkos::Experimental` types.

## What this repository owns

One question only: **does a value through the Kokkos wrapper equal, bit for bit,
the value the core computes directly?**

Accuracy (MPFR/MPC oracle, ulp gates, baselines) lives in xpmath
(`docs/CORRECTNESS.md`). This repository has no oracle, no ulps, and no
`__float128` outside unmodified `vendor/`.

## Layout

```
include/Kokkos_xpmath/   # wrappers (K1+)
vendor/xpmath/           # xpmath include/xp/ + LICENSES/ at a recorded tag
tests/                   # bit-identity only
examples/
scripts/sync_upstream.sh
scripts/check_vendor_fresh.sh
```

## Build

Requires Kokkos ≥5.1. This project builds as **C++17** even when Kokkos was
configured as C++20.

```bash
module use /soft/modulefiles && module load gcc/13.3.0 cmake/3.28.3
export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:$LD_LIBRARY_PATH

cmake -B build -DCMAKE_PREFIX_PATH=$HOME/kokkos-install-quadmath
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

## Vendoring

```bash
scripts/sync_upstream.sh v0.2.0
ctest --test-dir build -R vendor_fresh
```

See `NOTICE.md` for the license island under `vendor/xpmath/`.

## License

`Apache-2.0 WITH LLVM-exception` for all new code. Vendored xpmath retains
`LicenseRef-DHB-License` / `LicenseRef-LBNL-BSD-License` — see `NOTICE.md`.
