# GPU_PERF_PLAN status

One block per completed (or stopped) task from `docs/GPU_PERF_PLAN.md`,
appended newest last, using the template in that file's §1.8. A session
reads only the blocks for the tasks its own task depends on.

---

## P0.1 — Perf harness: kernels, build, ISA metrics on MI250
Date: 2026-11-02   Outcome: Done
PR: not opened (`gh` is not installed on this host). Branch pushed: https://github.com/ReetBarik/xpmath-kokkos/compare/main...gpuperf/p0.1-perf-harness?expand=1
Changed:
- CMakeLists.txt: option XPMATH_KOKKOS_BUILD_PERF (OFF) + add_subdirectory(perf)
- perf/{CMakeLists.txt,kernels.hpp,unit.cpp,perf_main.cpp}: 90 kernels, 12 per-(kind,type) object units, target xpk_perf_isa
- perf/{isa_metrics.sh,isa_metrics_hip.py}: gfx90a metrics from -save-temps device .s
- perf/README.md; perf/results/gfx90a/isa_v0.3.0.csv (90 rows)
Checks: ctest -N = 13 with perf OFF and ON (pass); vendor_fresh (pass); HIP gfx90a build on login node (pass, /tmp/p01-gfx90a-build.log); Serial build + run of all 90 kernels (pass, /tmp/p01-serial-run.log). Not run on a GPU (not needed).
Measured (MI250, xpmath v0.3.0; scratch bytes, calls): double mul 0 B, 0; DD add 24 B; DD mul 96 B, 1 call; DD log 416 B, 9 calls; cDD log 1488 B, 51 calls. double/float/cdouble/cfloat: all 0 B, 0 calls.
Next session must know:
- Build only `--target xpk_perf_isa` (default target also builds the tests). Recipe is in perf/README.md; configure needs -DCMAKE_CXX_COMPILER=hipcc and rocm/7.0.2.
- Metrics are keyed off `<build>/perf/xp_version.txt` (tag from vendor/UPSTREAM.txt, or `git describe` of XPMATH_KOKKOS_PERF_XP_DIR). XP_DIR=~/kokkos-extended-precision-demo is the xpmath clone (plan §1.1). Update 2026-11-02: its main was reset to origin/main (v0.3.0); the 8 scrapped cross_accum commits now live only on the local branch scrapped/tier1-792b17d. Work happens on its `gpu-perf` branch.
- `regs` = .vgpr_count + .agpr_count; `code_bytes`/`instructions`/`calls` are summed over the kernel and every function reachable by direct call (static counts). The parser exits nonzero if the kernel set is not exactly the expected 90, or if codeLenInByte contradicts the instruction count.
- exp domain is type-dependent: |x| <= 700 for double/DD, <= 80 for float-range types (they overflow past ~88). Complex abs/etc. use the same Kokkos:: forwards as the wrappers.
- P0.2 adds CUDA (cuobjdump/resource usage) and gfx942 by extending the `case` in isa_metrics.sh; the harness sources need no change.

## P0.2 — ISA metrics for sm_80, sm_100, gfx942
Date: 2026-11-03   Outcome: Done
Commits: xpmath-kokkos@2ff6916 (code + CSVs); status block in the following commit. xpmath clone untouched.
Changed:
- perf/isa_metrics.sh: archs gfx90a gfx942 sm_80 sm_100 (CUDA arch names carry the underscore)
- perf/isa_metrics_cuda.py (new): cuobjdump -res-usage/-sass parser, reuses tables from isa_metrics_hip.py
- perf/README.md: gfx942 and CUDA build recipes, per-arch column meanings
- perf/results/{gfx942,sm_80,sm_100}/isa_v0.3.0.csv (90 rows each)
Checks: all three harness builds rc=0 (logs ~/p02/*-build.log, sm_80/sm_100 at -j4); parsers exit 0 with exactly 90 kernels and consistent code-size checks. ctest -N (13) not re-run: no CMake or test files changed.
Measured (scratch B, calls; xpmath v0.3.0): DD mul gfx90a 96/1, gfx942 96/1, sm_80 16/1, sm_100 16/1. DD log 416/9, 416/9, 160/32, 96/38. cDD log 1488/51, 1488/51, 944/106, 944/137. double mul 0 B everywhere.
Next session must know:
- CUDA `scratch_bytes` = max(STACK, LOCAL); `code_bytes` = 16 x instructions. Without RDC, callees sit inside the kernel's SASS listing, so there is no call-graph walk. Static counts: compare across tags, not archs.
- Build dirs are build-perf-{gfx942,sm80,sm100} in the repo (untracked, safe to reuse). Build CUDA at -j4 under nice: an earlier -j32 x3 attempt overloaded the login node and froze sessions. /tmp is per login node; keep logs under $HOME.
- Results are keyed by xpmath tag; a build with XPMATH_KOKKOS_PERF_XP_DIR writes isa_<git describe>.csv beside the baseline.

## P0.3 — Throughput runner and science-shaped kernel
Date: 2026-11-03   Outcome: Done
Commits: xpmath-kokkos@576aabc (code + MI250 CSVs) on gpu-perf; status block follows in the next commit. xpmath: no changes (start 4f60bc4).
Changed:
- perf/{throughput.cpp,throughput_kernels.hpp,throughput_common.hpp,tp_unit.cpp,limbs.hpp}: runner, target xpk_perf_throughput (12 per-type units, separate from the 90-kernel ISA set)
- perf/science_kernel.hpp: ScienceKernel<R,Z>, 24 live complex values, 200 ops (mul/sqrt/add/div/log/atan2 + data-dependent sign flip)
- perf/{isa_metrics.sh,isa_metrics_hip.py,isa_metrics_cuda.py}: new `science` mode -> science_isa_<tag>.csv
- perf/CMakeLists.txt, perf/README.md; perf/results/gfx90a/{throughput,science,science_isa}_v0.3.0.csv
Checks: ctest -N = 13 with perf ON and OFF (pass); vendor_fresh (pass); isa_v0.3.0.csv regenerates byte-identical on gfx90a, and sm_80 CUDA parser output identical to committed (pass); host Serial run, all 90+6 rows finite (/tmp/p03-host-run.log); MI250 run on amdgpu04, nonfinite=0 in every row, subset re-run within 3.5% (/tmp/p03-gfx90a-run2.log is node-local). CUDA --science path NOT run (no CUDA tp objects built yet).
Measured (MI250 GCD, v0.3.0, ratio to native double): add DD 803x, cDD 957x; mul DD 1440x, cDD 2551x; div DD 223x, cDD 1736x; log DD 3291x, cDD 5056x.
Science kernel, ratio to double / regs / scratch B: DD 6095x / 354 / 2560; FF 3750x / 294 / 1408; TF 28934x / 480 / 1584; QF 85139x / 642 / 2080; double 186 / 0; float 110 / 0. Time per element: double 1.2 ns, QF 104 us.
Next session must know:
- Run: build-perf-gfx90a/perf/xpk_perf_throughput --arch gfx90a on the GPU node (~45 min total; QF science alone is ~7 x 219 s). Output goes to the shared perf/results/<arch>/; stdout log goes to the node's local /tmp.
- Native double/float/cdouble/cfloat run 8x more elements (--native-mult) because at n=2M a fixed ~launch cost inflated their per-op time up to 1.8x and made ratios too small. The `n` column says which; ratios compare ns_per_op. Add/mul ratios vs native are good to ~20%.
- Classification: add/mul/div = 64-op dependent chain; sqrt/exp/log/sin/atan2/abs = 4 evals per element. Inputs come from a 4096-entry table (i & mask) so natives are not memory-bound.
- Science `regs` for QF (642) exceeds 512 = vgpr_count+agpr_count as P0.1 defines it; compare across tags, do not read as occupancy directly. The science kernel needs no CUDA changes in the sources; for CUDA, build target xpk_perf_throughput then `perf/isa_metrics.sh <build> sm_80 science` (globs xpk_tp_c*.dir/tp_unit.cpp.o).
- Build dir build-perf-gfx90a (untracked) holds both targets; build a throughput tree with -j8 (~25 min on HIP); ptxas will be much slower for CUDA.

## P0.4 — GB300 lanes: harness build and bit-identity script
Date: 2026-11-03   Outcome: Done
Commits: xpmath-kokkos@46a8808 (code, log, CSV) on gpu-perf; status block follows in the next commit. xpmath: no changes (start 4f60bc4).
Changed:
- validation/gb300/run_gb300_bit_identity.sh: B200 script with GB300 modules (gcc/11.2.0 cmake/3.22.1 cuda/13.3.1) and Kokkos prefix. Also forced: log/build-dir names, fallback Kokkos arch BLACKWELL103, x86 LD_LIBRARY_PATH line removed, B200 qsub header replaced by a "run on an allocated node" note
- validation/gb300/logs/gb300_bit_identity_20261103_064625.log: passing run
- perf/isa_metrics.sh, perf/README.md: accept sm_103
- perf/results/sm_103/isa_v0.3.0.csv: 90 rows
Checks: GB300 bit_identity_test (graceblackwell01, GB300 cc 10.3, CUDA 13.3.73): pass, 4 backends x 63 ops, 1700 real + 1780 complex points, zero differing cells, exit 0. Perf harness xpk_perf_isa built on the node, isa_metrics.sh sm_103 wrote 90 rows (/tmp/p04-perf-build.log, /tmp/p04-isa.log are node-local). ctest -N count not re-run (no CMake changes).
Measured (sm_103 v0.3.0 regs/scratch B, vs sm_100): double add 25/0 (25/0); QF add 32/0 (32/0); QF log 114/432 (114/424); cQF log 167/1256 (146/920). Compilers differ (13.3 vs 12.9.1), so code size differs somewhat (QF add 4736 B vs 3584 B).
Next session must know:
- GB300 is aarch64 and must be built on the node (graceblackwell01 for this session). Perf tree: build-perf-gb300 (untracked, on shared $HOME), configured with nvcc_wrapper from the GB300 Kokkos prefix, -DXPMATH_KOKKOS_BUILD_PERF=ON; xpk_perf_throughput not built yet (P0.6 must build it, ~long ptxas).
- Run isa_metrics.sh on the node with cuda/13.3.1 loaded (cuobjdump); the login node is x86 and has no sm_103 toolchain match.
- Launch node jobs with `ssh -n host 'setsid nohup bash -lc "..." > log 2>&1 < /dev/null &'`; without -n/setsid the ssh call hangs until the job ends.
- python3 -I of the ISA scripts leaves perf/__pycache__/ behind; delete it before committing.

## P0.5 — GB300 device-sweep lane in xpmath
Date: 2026-11-03   Outcome: Stopped
Commits: kept off gpu-perf, see wip/P0.5 (xpmath@1cfa793, b790730, pushed): done-when not met (no complete raw file; TD-4-style fault on sm_103)
Changed (all on xpmath wip/P0.5 only):
- scripts/xpm_build.sh: `--arch gb300` (gcc/11.2.0 cmake/3.22.1 cuda/13.3.1, GB300 install nvcc_wrapper, -arch=sm_103 pinned, device tree only; --only host/both refused)
- validation/gb300/run_gb300_sweep.sh: build + sweep_device + per-kernel isolation on fault; does NOT score
- scripts/sweep_accuracy.cpp: added "gb300" to the closed --where token list (plan step was stale: scorer rejected it)
- validation/gb300/logs/: run log, sweep_device .out, sanitizer_DD_c_acos.log
Checks: GB300 build on graceblackwell01: pass (sm_103, cuda 13.3.1). Full sweep_device: FAIL error=700 first fault DD:c:acos (log: validation/gb300/logs/gb300_sweep_20261103_144732.log). Isolation: only DD:c:acos faults; the 39 later kernels pass alone. Not scored (no valid raw file). No sweep_device_gate_gb300 target added.
Measured: compute-sanitizer memcheck on `--only DD:c:acos`: 61 errors, all "Invalid __local__ write of size 8" (the TD-4 signature, TD-4 spill-frame mis-size); host frame complex_backend<BeDD,14>.
Next session must know:
- Kernels 0..211 of 252 passed in the full run, but the full-sweep raw file stops at the fault and is not a result; it is untracked (20M) at xpmath validation/gb300/logs/sweep_gb300_raw.csv, not committed.
- User decides: fix DD complex acos for sm_103 (library change, likely TD-4 mitigation; per-arch #ifdef is disallowed by 1.2 rule 4), or accept a stitched file with this one kernel excluded. Untried: nothing beyond one memcheck run.
- If gpu-perf should carry the lane, fast-forward gpu-perf to wip/P0.5 after a clean device run; the user also decides whether GB300 becomes a gate.
- Scorer on login node is built at /tmp/xpm-P0.5/host/tests/sweep_accuracy (may be gone).
- Backgrounded ssh inside run_in_background returns immediately; use `ssh -n host 'setsid nohup ...'` (see P0.4 block).
