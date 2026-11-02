# perf/ — GPU perf harness

Measures how fast xpmath types run inside Kokkos kernels. It never judges
accuracy (no oracle, ulps or tolerances; that is xpmath's job) and it
**never registers ctest targets**: CI asserts exactly 13. It is off by
default (`XPMATH_KOKKOS_BUILD_PERF=OFF`).

## What is here

| file | role |
|---|---|
| `kernels.hpp` | one templated kernel per op, plus deterministic input generation |
| `unit.cpp` | compiled 12 times (real/complex x 6 types), one object per unit |
| `perf_main.cpp` | launches all 90 kernels once (no timing, no checks) |
| `isa_metrics.sh` | per-kernel ISA metrics CSV from a built tree |
| `isa_metrics_hip.py` | the gfx90a parser behind `isa_metrics.sh` |
| `results/<arch>/isa_<xpmath-tag>.csv` | committed baselines |

Types: real `double float DD FF TF QF`; complex `Kokkos::complex<double>`,
`Kokkos::complex<float>`, and the four xp complex structs.
Ops: real `add mul div sqrt exp log sin atan2`; complex `add mul div abs
sqrt log exp`. Every kernel uses `RangePolicy<LaunchBounds<256,1>>`.
Inputs are drawn per op from a safe domain (positive for sqrt/log, `|x| <= 700`
for exp on double-range types, `|x| <= 80` on float-range types, which would
otherwise overflow). All calls go through the `Kokkos::` forwards.

## Build (MI250, gfx90a; login node is enough)

```bash
module use /soft/modulefiles && module load gcc/13.3.0 cmake/3.28.3 rocm/7.0.2
export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:$LD_LIBRARY_PATH
cmake -S . -B build-perf-gfx90a -DCMAKE_CXX_COMPILER=hipcc \
      -DCMAKE_PREFIX_PATH=$HOME/xpm_device/kokkos-hip-gfx90a \
      -DXPMATH_KOKKOS_BUILD_PERF=ON
cmake --build build-perf-gfx90a --target xpk_perf_isa -j16 > /tmp/perf-build.log 2>&1
perf/isa_metrics.sh build-perf-gfx90a gfx90a
```

Build only `xpk_perf_isa`; the default target would also build the tests.
On HIP the units are compiled with `-save-temps=obj`, so each device
assembly file sits next to its object under `<build>/perf/`.

### Measuring an xpmath branch before release

```bash
cmake ... -DXPMATH_KOKKOS_PERF_XP_DIR=$HOME/kokkos-extended-precision-demo
```
uses that checkout's `include/` instead of `vendor/xpmath`. The result file
is named from `git describe --tags --always --dirty` of that checkout
(`v0.3.0` for the vendored tree). Do not mix the two: the harness never links
the `xpmath_kokkos` target, so exactly one `xp/` is on the include path.

## ISA metrics CSV

`type,op,regs,scratch_bytes,code_bytes,instructions,calls`, one row per
kernel. `type` is `double float DD FF TF QF` for real and `cdouble cfloat
cDD cFF cTF cQF` for complex.

| column | meaning |
|---|---|
| `regs` | kernel `.vgpr_count` from the metadata (includes callees; gfx90a unified VGPR+AGPR) |
| `scratch_bytes` | `.private_segment_fixed_size`: per-lane stack. Nonzero means the compiler spilled or kept an object in memory (for example `volatile`) |
| `code_bytes` | `codeLenInByte` of the kernel plus every function it can reach through direct calls |
| `instructions` | static instruction count over the same set of functions |
| `calls` | static count of `s_swappc_b64` (non-inlined call sites) over the same set |

Counts are static: a loop body counts once. Compare across tags, not across
architectures.
