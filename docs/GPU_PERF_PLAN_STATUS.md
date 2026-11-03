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
