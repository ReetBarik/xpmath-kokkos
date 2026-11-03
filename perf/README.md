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
| `isa_metrics.sh` | per-kernel ISA metrics CSV from a built tree; arch `gfx90a gfx942 sm_80 sm_100` |
| `isa_metrics_hip.py` | the AMDGPU (gfx90a, gfx942) parser behind `isa_metrics.sh` |
| `isa_metrics_cuda.py` | the CUDA parser (`cuobjdump -res-usage`, `-sass`) |
| `results/<arch>/isa_<xpmath-tag>.csv` | committed baselines |

Types: real `double float DD FF TF QF`; complex `Kokkos::complex<double>`,
`Kokkos::complex<float>`, and the four xp complex structs.
Ops: real `add mul div sqrt exp log sin atan2`; complex `add mul div abs
sqrt log exp`. Every kernel uses `RangePolicy<LaunchBounds<256,1>>`.
Inputs are drawn per op from a safe domain (positive for sqrt/log, `|x| <= 700`
for exp on double-range types, `|x| <= 80` on float-range types, which would
otherwise overflow). All calls go through the `Kokkos::` forwards.

## Build (HIP gfx90a; login node is enough)

```bash
module use /soft/modulefiles && module load gcc/13.3.0 cmake/3.28.3 rocm/7.0.2
export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:$LD_LIBRARY_PATH
cmake -S . -B build-perf-gfx90a -DCMAKE_CXX_COMPILER=hipcc \
      -DCMAKE_PREFIX_PATH=$HOME/xpm_device/kokkos-hip-gfx90a \
      -DXPMATH_KOKKOS_BUILD_PERF=ON
cmake --build build-perf-gfx90a --target xpk_perf_isa -j16 > /tmp/perf-build.log 2>&1
perf/isa_metrics.sh build-perf-gfx90a gfx90a
```

gfx942 is the same with `rocm/7.0.2`, `-DCMAKE_PREFIX_PATH=$HOME/xpm_device/kokkos-hip-gfx942`
and `perf/isa_metrics.sh build-perf-gfx942 gfx942`.

## Build (CUDA sm_80, sm_100; login node is enough)

```bash
module use /soft/modulefiles && module load gcc/13.3.0 cmake/3.28.3 cuda/12.9.1
export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:$LD_LIBRARY_PATH
K=$HOME/kokkos-install-cuda-sm80-quadmath     # sm_100: $HOME/kokkos-install-cuda-sm100
cmake -S . -B build-perf-sm80 -DCMAKE_CXX_COMPILER=$K/bin/nvcc_wrapper \
      -DCMAKE_PREFIX_PATH=$K -DXPMATH_KOKKOS_BUILD_PERF=ON
nice -n 19 cmake --build build-perf-sm80 --target xpk_perf_isa -j4 > /tmp/perf-build.log 2>&1
perf/isa_metrics.sh build-perf-sm80 sm_80      # sm_100: build-perf-sm100 sm_100
```

Use a low `-j` on shared login nodes: each `nvcc` spawns several children
and `ptxas` is slow on the complex kernels (about 10 min per backend build).

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
| `regs` | HIP: kernel `.vgpr_count`+`.agpr_count` (includes callees). CUDA: `REG` from `cuobjdump -res-usage` |
| `scratch_bytes` | per-thread stack. HIP: `.private_segment_fixed_size`. CUDA: `max(STACK, LOCAL)`. Nonzero means the compiler spilled or kept an object in memory (for example `volatile`) |
| `code_bytes` | HIP: `codeLenInByte` of the kernel plus every function it can reach through direct calls. CUDA: 16 bytes x instructions in the kernel's SASS listing (callees are inside it) |
| `instructions` | static instruction count over the same set of functions |
| `calls` | static count of non-inlined call sites over the same set (HIP `s_swappc_b64`, CUDA `CALL*`) |

Counts are static: a loop body counts once. Compare across tags, not across
architectures.
