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
| `isa_metrics.sh` | per-kernel ISA metrics CSV from a built tree; arch `gfx90a gfx942 sm_80 sm_100 sm_103` |
| `isa_metrics_hip.py` | the AMDGPU (gfx90a, gfx942) parser behind `isa_metrics.sh` |
| `isa_metrics_cuda.py` | the CUDA parser (`cuobjdump -res-usage`, `-sass`) |
| `throughput.cpp` | the throughput runner (driver, target `xpk_perf_throughput`) |
| `throughput_kernels.hpp`, `throughput_common.hpp`, `limbs.hpp` | chain and function kernels, timing, limb helpers |
| `tp_unit.cpp` | compiled 12 times (real/complex x 6 types), one object per unit |
| `science_kernel.hpp` | the science-shaped kernel |
| `results/<arch>/isa_<xpmath-tag>.csv` | committed ISA baselines |
| `results/<arch>/science_isa_<xpmath-tag>.csv` | registers and stack of the science kernel |
| `results/<arch>/throughput_<xpmath-tag>.csv` | committed throughput baselines |
| `results/<arch>/science_<xpmath-tag>.csv` | science kernel time per element |

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

## Throughput runner

Target `xpk_perf_throughput` (separate units, so the 90-kernel ISA set above is
unchanged). Build it like `xpk_perf_isa`, then run it on the GPU node:

```bash
cmake --build build-perf-gfx90a --target xpk_perf_throughput -j8 > /tmp/perf-build.log 2>&1
ssh <host> 'bash -lc "module use /soft/modulefiles && module load gcc/13.3.0 rocm/7.0.2 &&
  export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:\$LD_LIBRARY_PATH &&
  cd ~/xpmath-kokkos && build-perf-gfx90a/perf/xpk_perf_throughput --arch gfx90a"'
```

`--arch` names the results directory and is required. Other options: `--n`
(elements per launch, default 2097152), `--reps` (default 5), `--types
DD,cDD,...`, `--no-science`, `--native-mult` (default 8), `--out-dir`. The result file name carries the
xpmath tag (`v0.3.0` for the vendored tree). Exit status 3 means some kernel
produced non-finite output, so its timing is not meaningful; the CSVs are still
written.

For each type and op: 2 warm-up launches, then 5 timed launches, each bracketed
by `Kokkos::fence()` and `Kokkos::Timer`; the row reports the median (and min and
max). Launch policy is `RangePolicy<LaunchBounds<256,1>>`, as in the ISA kernels.

| kind | ops | shape |
|---|---|---|
| chain | add mul div (real and complex) | a dependent chain of 64 operations per element, operands held in registers, multiplier/divisor within 0.1% of 1 so values stay bounded. The chain is 4 trips of 16 explicit steps, so loop overhead is the same for every type |
| func | sqrt exp log sin atan2 (real); abs sqrt log exp (complex) | 4 evaluations per element on 4 inputs, from the same domains as the ISA kernels (atan2 sees pairs (v0,v1) (v1,v2) (v2,v3) (v3,v0)) |

Both kinds store one double per element: the sum of every limb of the result,
so the compiler cannot drop the low-order work. Inputs live in a 4096-entry
table indexed by `i & mask`, so the native `double`/`float` baselines are not
limited by memory bandwidth.

`throughput_<tag>.csv` columns: `type,op,kind,ops_per_elem,n,ms_median,ms_min,
ms_max,ns_per_op,ratio_double,ratio_float,nonfinite`.

- `ns_per_op` = `ms_median * 1e6 / (n * ops_per_elem)`: device-wide time per
  operation per element (an inverse throughput, not a latency).
- `ratio_double` / `ratio_float`: `ns_per_op` of this (type, op) divided by that of
  the same op in the native type of the same kind (`double`/`float` for real types,
  `cdouble`/`cfloat` for complex ones). Blank when that baseline was filtered out.
- The native types run `--native-mult` (8) times more elements than `n`; the `n`
  column says which. A native add chain finishes in about 40 us at n = 2M, so a
  fixed launch and tail cost inflated its time by up to 1.8x (measured on MI250:
  `double add` 0.00032 ns/op at 2M, 0.00018 at 16M) and made every ratio against
  it too small. The xpmath types run for milliseconds and are not affected.
  Even so, treat ratios on add/mul against native as good to about 20%.
- `nonfinite`: count of non-finite values in the last run's output (a sanity check
  that the timed path was the normal one, not an accuracy measurement).

### Science-shaped kernel

`science_kernel.hpp` is a synthetic stand-in for a loop-integral leaf. It does
not depend on QCDLoop. `ScienceKernel<R, Z>` is templated on (real, complex), so it
runs for `double`, `float` and the four backends. Per element it loads 24 complex
values (all live until the end) and runs 20 blocks of 10 operations, 200 in total:

```
t0 = z[a]*z[b]   t1 = sqrt(t0)   t2 = t1 + z[c]   den = z[d] + 4   t3 = t2/den
t4 = log(t3 + 2.8)   at = atan2(im t2, re t2) / 16   if (im t3 < 0) at = -at
z[x] = (re t4, im t4 + at)
```

The `if` is the data-dependent branch-cut-style sign choice; `sqrt`, `log` and
`atan2` have their own data-dependent branches inside. Indices are compile-time
constants (index sequences and forced inlining), so the 24 values are
registers, not an indexed array in memory. Every block maps modulus <= 1.6 to
modulus <= 1.6, so no type overflows.

`science_<tag>.csv`: `type,n,ops_per_elem,ms_median,ms_min,ms_max,
ns_per_element,ratio_double,ratio_float,nonfinite`, one row per real type.
Registers and stack come from the ISA scripts, from the throughput build:

```bash
perf/isa_metrics.sh build-perf-gfx90a gfx90a science    # -> science_isa_<tag>.csv
```

Its purpose is to show occupancy effects that single-op kernels hide: compare
`regs` and `scratch_bytes` here with the time per element.
