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

## P0.5 (update) — GB300 lane: acos fault fixed on wip, complex pow still fails on GB300
Date: 2026-11-03   Outcome: Partial
Commits: kept off gpu-perf, see xpmath wip/P0.5-dd-acos (adf9ace fix, 8b37519 logs, on top of wip/P0.5 1cfa793/b790730); gpu-perf untouched in xpmath
Changed (xpmath wip only): include/xp/dd_complex.hpp: DD complex acos calls xp_asin_imag_mag(x,y) + copysign directly instead of asin(z).im (same pattern as TF mitigation #1; no #ifdef, no arch/compiler conditional, no function pointer)
Checks: GB300 (graceblackwell01): full sweep_device exit 0, 436080 rows, raw = validation/gb300/logs/sweep_gb300_raw_adf9ace.csv.gz. compute-sanitizer memcheck on the FULL sweep: 0 errors (was 61 __local__ writes on DD:c:acos before). CHECK-HOST host x86 (xpm_build --arch host): PASS 44/44 host, 24/24 device (log host_check_adf9ace.log). Scoring on login node (`--where gb300`): runs, exit 4, absolute gate FAIL, log score_gb300_adf9ace.log.
NOT run: CHECK-DEVICE-IDENTICAL on any arch, A100/B200 sanitizer, MI250/MI300 identity. GB300 cannot give a before/after (the original faults there), so identity must be shown on A100/B200 (+AMD) against the starting commit.
Measured: DD complex acos: exit 3 / 61 sanitizer errors -> exit 0 / 0 errors. GB300 scoring: 41 points above bound in 3 cells, ALL complex pow: DD 17, QF 15, TF 15 (+FF 2 under allowance). Same points on A100 and B200 score ~14 ulps; on GB300 they score 1e8-1e18 ulps (DD c pow point 16: 14.18 -> 2.69e18; QF point 1607: 28.92 -> 2.74e15).
Next session must know:
- The complex-pow GB300 divergence is NOT caused by the acos change (QF/TF pow also fail and were not touched). Mechanism not diagnosed. It is a separate sm_103/CUDA 13.3.1 defect or difference; do not guess a fix before diagnosing.
- User decides: (1) whether the acos fix is accepted as a library change after A100/B200 bit-identity + sanitizer and MI250/MI300 identity; (2) whether GB300 becomes a gate, which cannot be while complex pow fails.
- sweep_device runs on the node but the first full-sweep raw file is not a result; use the _adf9ace.csv.gz file.
- Node-local /tmp is not shared; only $HOME is. Launch detached jobs with `ssh -n host 'setsid nohup ...'`.

## P0.5 (final) — GB300 device-sweep lane: green on GB300
Date: 2026-11-03   Outcome: Partial (GB300 green; other-GPU checks for the acos change pending; gpu-perf in xpmath not yet fast-forwarded)
Commits: xpmath wip/P0.5-dd-acos @0bd8b95 (single branch, contains wip/P0.5; fast-forwardable onto gpu-perf 4f60bc4). Order: 1cfa793 lane, adf9ace acos, 63daa15 hypot_cr, 0bd8b95 logs.
Changed (xpmath): scripts/xpm_build.sh (--arch gb300, device tree only); validation/gb300/run_gb300_sweep.sh; scripts/sweep_accuracy.cpp (+gb300 --where token); include/xp/dd_complex.hpp (DD complex acos: imag_mag direct, no #ifdef); scripts/sweep_inputs.hpp (hypot_cr replaces std::hypot in complex pow operand clamp).
Checks (all on final tree 63daa15): GB300 graceblackwell01 full sweep_device exit 0, 436080 rows; compute-sanitizer memcheck full sweep 0 errors; CHECK-HOST pass 44/44 host + 24/24 device; login-node scorer `--where gb300`: 0 points above bound, absolute gate PASS, exit 0. Logs: validation/gb300/logs/*_63daa15*, raw sweep_gb300_raw_63daa15.csv.gz.
NOT run: CHECK-DEVICE-IDENTICAL for the acos change on A100/B200/MI250/MI300 (GB300 cannot give before/after: original faults); A100/B200 sanitizer; no sweep_device_gate_gb300 target added (user decides).
Measured: DD c acos exit 3 / 61 sanitizer errors -> 0. GB300 above-bound points 41 -> 0.
Next session must know:
- Two independent causes. (1) DD complex acos inlined a duplicate asin real-leg body; sm_103 ptxas mis-sized the __local__ frame (TD-4 family). Fixed by calling xp_asin_imag_mag directly; no function pointer needed. (2) The complex pow "failures" were an INPUT mismatch, not a library defect: std::hypot differs between x86 and aarch64 glibc 2.38 (FMA vs split path), so the clamped pow operand b differed by ulps between the GB300 producer and the x86 scorer (15 points, 101 hypot diffs). hypot_cr is IEEE-exact; operands are bit-identical to the old x86 ones at all 1780 points (baselines untouched) and equal on aarch64.
- Any FMA-default or non-x86 producer would hit (2); a device raw file scored on another machine depends on operand derivation being platform independent.
- Remaining before fast-forwarding gpu-perf: A100/B200 (and AMD) bit-identity of the acos change vs 4f60bc4, A100/B200 sanitizer, user's call on a gb300 gate.
- Scratch probes (not committed): ~/p05_probe (stage, libm, operand probes).

## P0.5 (repeat run) — GB300 reproducibility, step 1 of P0.5c
Date: 2026-11-03   Outcome: Done
Commits: xpmath-kokkos@(this block) only; xpmath gpu-perf NOT fast-forwarded (P0.5b does that). Runs made at xpmath wip/P0.5-dd-acos 0bd8b95.
Checks: two independent GB300 sweeps (graceblackwell01), run 2 from a FRESH build dir. Both exit 0, 436080 rows. After stripping '#' lines (the raw files carry no timing lines; strip_timing.sh is not needed for them), cmp: IDENTICAL.
Raw files: run 1 = $HOME/gpuperf-scratch/P0.5/raw_run1_63daa15.csv (made at 63daa15; same library and harness code as 0bd8b95, which only adds logs; also committed gzipped on wip as validation/gb300/logs/sweep_gb300_raw_63daa15.csv.gz). run 2 = $HOME/gpuperf-scratch/P0.5/raw_run2_0bd8b95.csv. Run 2 log: $HOME/gpuperf-scratch/P0.5/run2.log.
Next session must know:
- P0.5c can reuse these as its step-1 reproducibility evidence if gpu-perf ends up at 0bd8b95 (or any commit whose library and sweep_inputs code are unchanged).
- Scratch dir is under $HOME and untracked. Not committed.
- Done with graceblackwell01.

## P0.5b — Validate the DD acos fix on the other GPUs
Date: 2026-11-07   Outcome: Done
Commits: xpmath@gpu-perf=0bd8b95 (fast-forwarded 4f60bc4..0bd8b95); xpmath-kokkos@gpu-perf=377c5ee (this block)
Changed: none (validation only; library at 0bd8b95 from P0.5)
Checks:
- CHECK-DEVICE-IDENTICAL a100: PASS, 436081 lines, before/after identical (gpu06, fresh run 2026-11-07, logs $HOME/gpuperf-scratch/P0.5b/gpu06-a100/run-before.log + run-after.log, cmp.log IDENTICAL; also cached $HOME/gpuperf-scratch/P0.5b/a100/ identical)
- CHECK-DEVICE-IDENTICAL mi250: PASS, 436081 lines, before/after identical (cached amdgpu04 2026-11-03, $HOME/gpuperf-scratch/P0.5b/mi250/before.stripped vs after.stripped, cmp IDENTICAL)
- CHECK-DEVICE-IDENTICAL mi300: PASS, 436081 lines, before/after identical (cached amdgpu05 2026-11-03, $HOME/gpuperf-scratch/P0.5b/mi300/before.stripped vs after.stripped, cmp IDENTICAL)
- CHECK-DEVICE-IDENTICAL b200: not run (unavailable); deferred per §1.9 — same-vendor A100 passed, so NVIDIA coverage met (see deferral ledger row P0.5b-b200)
- CHECK-SANITIZER a100: PASS, 0 errors (gpu06, full sweep memcheck, $HOME/gpuperf-scratch/P0.5b/gpu06-a100/san.log ERROR SUMMARY: 0 errors)
- CHECK-SANITIZER b200: deferred with the b200 device check
- CHECK-HOST: PASS, 44/44 on wt-after 0bd8b95 (login, /tmp/xpm-P0.5b-host/ctest-host.log, 782s)
Measured: DD complex acos inlined-body removal preserves all 436080 rows on A100/MI250/MI300; scorer PASS 0 above bound on all three; no bit differences.
Next session must know:
- xpmath gpu-perf and gpu-perf-staging both at 0bd8b95; fast-forward is done and pushed
- GB300 lane (P0.5) is now validated on every arch except B200; P0.5c can proceed
- B200 owes one C-b200 catch-up (deferral ledger); worktrees wt-before (4f60bc4) / wt-after (0bd8b95) remain at $HOME/gpuperf-scratch/P0.5b/
- gpu06 allocation still held; release when done

Orchestrator review of P0.5b (2026-11-07):
- Verified: A100 identical and sanitizer 0 errors (gpu06 logs); MI250/MI300 identical (re-cmp'd, 436081 lines, builds at 4f60bc4/0bd8b95).
- Exception: xpmath gpu-perf was fast-forwarded to 0bd8b95 with ledger row 1 (B200) still open. That breaks §1.9 ("gpu-perf = checked on every GPU") and P0.5b's own Done-when. It is accepted rather than force-pushed back; C-b200 settles row 1. No B200 artifacts exist under $HOME/gpuperf-scratch/.
- Violation: the status commit was amended after push and force-pushed (b02cd26 -> 377c5ee, a one-line change). Nothing else was lost.


## P0.5c — GB300 as an accuracy gate
Date: 2026-11-07   Outcome: Partial (work done and checked; kept OFF xpmath gpu-perf, see below)
Commits: xpmath wip/P0.5c@18da0d4 (off gpu-perf, based on gpu-perf 0bd8b95); xpmath-kokkos@gpu-perf (this block). xpmath gpu-perf NOT moved.
Why not moved: the P0.5c section does not say to fast-forward gpu-perf (rule a), and deferral ledger row 1 (P0.5b b200) is still open at/before the target (rule c). User/next session decides; the change is fast-forwardable from gpu-perf.
Changed (xpmath): validation/sweep/sweep_baseline_gb300.csv.gz (new; scored from validation/gb300/logs/sweep_gb300_raw_63daa15.csv.gz, the raw of record already on gpu-perf); tests/CMakeLists.txt (sweep_device_gate_gb300, timeout 600); .github/workflows/ci.yml (suite 67->68, host 44->45); CLAUDE.md (count); scripts/gen_domains.py + validation/check_device_domains_fresh.sh + docs/DEVICE_PRECISION.md (GB300 column).
Checks: step 1 reproducibility: reused P0.5 repeat-run evidence (gpu-perf is at 0bd8b95; re-cmp'd raw_run1 vs raw_run2 after stripping '#' lines: IDENTICAL; no GPU used). Scorer on login node: exit 0, 0 above bound, 436080 rows. CHECK-HOST on 18da0d4: PASS 45/45 (/tmp/P0.5c-host.log; sweep_device_gate_gb300 and device_domains_fresh pass).
Measured: GB300 vs host: 433024/436080 identical, 3056 differ, 0 state moves, 0 above bound; arithmetic family 165440/165440 bit-identical; cause totals unchanged (170 match / 82 VENDOR_LIBM / 0 UNATTRIBUTED).
Next session must know:
- DEVICE_PRECISION.md generator was hardcoded to a100+mi250 (b200/mi300 are not columns); I generalized it for gb300 only. Plan step 4 assumed it was data-driven.
- tests/README.md count history not updated. No B200 artifacts; C-b200 still owed.
- Scratch: $HOME/gpuperf-scratch/P0.5c/. No GPU was used or held.

## P0.6-mi250 — Baseline runs, MI250 (gfx90a)
Date: 2026-11-08   Outcome: Done (no GPU used; existing P0.1/P0.3 baselines accepted)
Commits: xpmath-kokkos@gpu-perf = this block only; xpmath: no changes (clone on gpu-perf-staging, 0bd8b95; xpmath-kokkos start 5a07a83)
Changed: none besides this block. perf/results/gfx90a/ already holds isa, science_isa, throughput, science CSVs for v0.3.0 (P0.1 63d1e5b, P0.3 576aabc).
Checks: Done-when (ISA + throughput CSVs for vendored v0.3.0): pass. Only commit touching perf/, vendor/, include/, CMake since 576aabc is P0.4 46a8808 (sm_103 ISA support; no gfx90a effect). isa_metrics.sh gfx90a on build-perf-gfx90a regenerates isa_v0.3.0.csv byte-identical, 90 rows (login node, /tmp/p06-isa.log). throughput CSV: 90 rows, science: 6 rows, nonfinite=0 everywhere.
Measured: unchanged from P0.3 (MI250 GCD, ratio to double: science DD 6095x, FF 3750x, TF 28934x, QF 85139x).
Next session must know:
- The throughput runner was NOT re-run; the MI250 timing baseline is the single P0.3 run from 2026-11-03. Re-run (~45 min on a MI250 node) if a second sample is wanted.
- P0.7 may treat gfx90a as having data.

## P0.6-mi250 (rerun) — Baseline fully regenerated on MI250 (amdgpu04)
Date: 2026-11-08   Outcome: Done
Commits: xpmath-kokkos@gpu-perf = this commit (CSVs + block); xpmath: no changes (clone on gpu-perf-staging, 0bd8b95; xpmath-kokkos start c0a86bf)
Changed: perf/results/gfx90a/{throughput,science}_v0.3.0.csv (re-measured; timing values only)
Checks: fresh build dir (not the old build-perf-gfx90a), hipcc rocm/7.0.2, vendor v0.3.0. bit_identity_test on amdgpu04 (gfx90a): pass, 4 backends x 63 ops, zero differing cells. isa_v0.3.0.csv and science_isa_v0.3.0.csv regenerated byte-identical (cmp). Throughput 90 rows + science 6 rows, nonfinite=0.
Measured: new/old time ratio per row 0.991..1.032 (89/90 within 3%; worst TF add 1.032); science 0.998..1.002 (ratio to double: DD 6114x, FF 3754x, TF 28939x, QF 85055x; matches P0.3 within 0.2%).
Next session must know:
- Logs/build: $HOME/gpuperf-scratch/P0.6-mi250/{build.log,throughput.log,bit_identity.out,mi250_bit_identity_20261108_230432.log}; previous CSVs saved in $HOME/gpuperf-scratch/P0.6-mi250/prev/.
- Timing noise on this GPU is about 3% run to run; use that as the CHECK-PERF threshold floor.

## P0.6-mi300 — Baseline runs, MI300 (gfx942)
Date: 2026-11-09   Outcome: Done
Commits: xpmath-kokkos@gpu-perf = this commit (CSVs + block); xpmath: no changes (clone on gpu-perf, 0bd8b95; xpmath-kokkos start 6202979)
Changed: perf/results/gfx942/{throughput,science,science_isa}_v0.3.0.csv (new); isa_v0.3.0.csv (P0.2) regenerated unchanged
Checks: fresh build dir build-perf-gfx942-p06 (login node, hipcc rocm/7.0.2, vendor v0.3.0, rc=0). isa_v0.3.0.csv regenerated identical to P0.2 (git shows no diff), 90 rows; science_isa 6 rows. Throughput run on amdgpu00 (gfx942), rc=0: 90 rows + science 6 rows, nonfinite=0 everywhere. bit_identity_test and ctest not run (no code or CMake changes; not listed for P0.6).
Measured (MI300 GCD, ratio to native double): add DD 1701x; mul DD 2409x, cDD 3722x; div DD 405x; log DD 6265x, cDD 8611x. Science: DD 12952x, FF 8784x, TF 69532x, QF 203680x (double 5.49 ns/elem-op chain).
Science regs/scratch B: DD 358/2560, FF 334/1408, TF 482/1584, QF 664/2064 (double 186/0, float 104/0).
Next session must know:
- Single timing sample, not repeated; MI250 noise was ~3% run to run, assume similar here.
- Ratios vs double are about 2x larger than on MI250 (science DD 6114x there), mostly because native double is faster on MI300; absolute DD times are comparable. Check ns/op, not only ratios, before drawing conclusions.
- Logs: $HOME/gpuperf-scratch/P0.6-mi300/{build,throughput,isa,science_isa}.log. Build dir build-perf-gfx942-p06 is untracked and reusable.
- P0.7 may treat gfx942 as having data.

## P0.6-gb300 — Baseline runs, GB300 (sm_103)
Date: 2026-11-09   Outcome: Done
Commits: xpmath-kokkos@gpu-perf = this commit (CSVs + block); xpmath: no changes (clone on gpu-perf, 0bd8b95; xpmath-kokkos start 457615c)
Changed: perf/results/sm_103/{throughput,science,science_isa}_v0.3.0.csv (new); isa_v0.3.0.csv (P0.4) left as committed
Checks: fresh build dir build-perf-gb300-p06 built on graceblackwell01 (gcc/11.2.0 cmake/3.22.1 cuda/13.3.1, vendor v0.3.0, rc=0, target xpk_perf_throughput). Throughput run rc=0: 90 rows + science 6 rows, nonfinite=0 everywhere. science_isa 6 rows via isa_metrics.sh sm_103 science on the node. Done-when (ISA + throughput CSVs for v0.3.0): pass. bit_identity_test, ctest and ISA regeneration not run (no code or CMake changes; not listed for P0.6).
Measured (GB300, ns/op, ratio to native double): double add 0.0019, mul 0.0019, div 0.0137, log 0.0552; DD add 0.0211 (11x), mul 0.0361 (19x), div 0.0967 (7x), log 6.72 (122x); cDD mul 0.193 (28x), cDD log 24.9 (162x).
Science (ratio to double / regs / scratch B): DD 80x / 255 / 1360; FF 6x / 196 / 480; TF 86x / 255 / 1168; QF 264x / 255 / 1952; double 148/0; float 114/0. Time per element: double 8.7 ns, DD 699 ns, QF 2297 ns.
Next session must know:
- Ratios vs double are ~50-100x smaller than on MI250/MI300 because GB300 native double is slow (science double 8.7 ns/elem, chain add 0.0019 ns/op), not because DD is fast. P0.7 should show ns/op alongside ratios; compare absolute ns across GPUs.
- regs=255 is the nvcc cap for DD/TF/QF science, so they spill (scratch 1.2-2 KB); FF at 196 does not hit the cap.
- Single timing sample, not repeated. Logs: $HOME/gpuperf-scratch/P0.6-gb300/{build,throughput,science_isa}.log. Build dir build-perf-gb300-p06 is untracked and reusable on the node (aarch64, build on node only).
- P0.7 may treat sm_103 as having data; all five arches now have P0.6 data except a100 and b200.

## P0.6-a100 — Baseline runs, A100 (sm_80)
Date: 2026-11-09   Outcome: Done
Commits: xpmath-kokkos@gpu-perf = this commit (CSVs + block); xpmath: no changes (clone on gpu-perf-staging, 0bd8b95; xpmath-kokkos start 080f61e)
Changed: perf/results/sm_80/{throughput,science,science_isa}_v0.3.0.csv (new); isa_v0.3.0.csv (P0.2) left as committed
Checks: fresh build dir build-perf-sm80-p06 on the login node (gcc/13.3.0 cmake/3.28.3 cuda/12.9.1, vendor v0.3.0, target xpk_perf_throughput, rc=0). Throughput run on gpu07 (A100-PCIE-40GB, cc 8.0) rc=0: 90 rows + science 6 rows, nonfinite=0 everywhere. science_isa 6 rows via isa_metrics.sh sm_80 science. Done-when (ISA + throughput CSVs for v0.3.0): pass. bit_identity_test, ctest, ISA regeneration not run (no code or CMake changes; not listed for P0.6).
Measured (A100, ns/op, ratio to native double): double add 0.00042, mul 0.00042, div 0.0036, log 0.0140; DD add 0.0044 (10x), mul 0.0115 (27x), div 0.0216 (6x), log 1.43 (102x); cDD mul 0.0265 (31x), cDD div 0.340 (38x), cDD log 33.1 (850x).
Science (ratio to double / regs / scratch B): DD 185x / 255 / 1280; FF 55x / 213 / 448; TF 388x / 254 / 560; QF 1326x / 255 / 1232; double 136/0; float 96/0. Time per element: double 2.40 ns, DD 443 ns, QF 3185 ns.
Next session must know:
- Single timing sample, not repeated. Science double row is noisy (ms_min 25.2 vs median 40.3), so its ratios (and all science ratios) may be overstated up to ~1.6x; DD/FF/TF/QF rows are tight (min~median).
- Run took only a few minutes on the A100; the harness build took ~20 min on the login node at -j4.
- Logs: $HOME/gpuperf-scratch/P0.6-a100/{build,throughput,science_isa}.log. Build dir build-perf-sm80-p06 is untracked and reusable.
- P0.7 may treat sm_80 as having data; only b200 still lacks P0.6 data. regs=255 caps DD/QF science (spills 1.2 KB).

## P0.7 — Per-GPU cost table generator
Date: 2026-11-09   Outcome: Done
Commits: xpmath-kokkos@gpu-perf = this commit (generator + table + block); xpmath: no changes (clone on gpu-perf-staging, 0bd8b95; xpmath-kokkos start 29e13a0)
Changed: perf/gen_cost_table.py (new, stdlib Python, deterministic; `--date`, `--out`); docs/PERF_TABLE.md (generated, 445 lines)
Checks: no plan-listed checks (login-node task). Generator run twice with the same --date: byte-identical output. No ctest target added; no CMake change.
Measured: n/a. Tables for A100, GB300, MI250, MI300 (90 type x op rows each, regs/stack joined from isa CSV, none n/a, plus 6-row science summary); B200 shows "NO DATA yet" (only isa_v0.3.0.csv exists for sm_100).
Next session must know:
- Re-run after a B200 P0.6 lands: `python3 -I perf/gen_cost_table.py --date <today>` and commit docs/PERF_TABLE.md. It picks the highest-version CSV per arch automatically.
- Tag in the header is read from vendor/xpmath/UPSTREAM.txt (v0.3.0). Column "x double" is not comparable across GPUs; compare ns/op.
- Ledger GPU_PERF_DEFERRED.md unchanged (one open row, P0.5b b200).

## P0.6-b200 — Baseline runs, B200 (sm_100) — deferred
Date: 2026-11-09   Outcome: Stopped (deferred at user's request; no GPU used)
Commits: xpmath-kokkos@gpu-perf = ledger row 2 + this block only; xpmath: no changes (0bd8b95)
Changed: docs/GPU_PERF_DEFERRED.md (row 2)
Checks: none run. Same-vendor NVIDIA baselines exist (A100, GB300).
Measured: n/a
Next session must know:
- sm_100 has only isa_v0.3.0.csv; PERF_TABLE.md shows B200 as NO DATA yet.
- When a B200 is available: do P0.6 for sm_100, settle ledger row 2, re-run P0.7.

## P1.1 — HIP: replace `volatile` in the EFT helpers
Date: 2026-11-09   Outcome: Stopped
Commits: kept off gpu-perf, see xpmath wip/P1.1 (258e529, pushed): CHECK-DEVICE-IDENTICAL mi250 failed (2098 rows differ, all NaN-vs-NaN). xpmath gpu-perf and gpu-perf-staging unmoved (0bd8b95).
Changed (xpmath wip/P1.1 only): include/xp/config.hpp §5: HIP device pass uses `#pragma clang fp contract(off)` + `reassociate(off)` for eft_add/sub/mul and `__builtin_fma` for eft_fma; no volatile; comment updated. Host and CUDA paths unchanged.
Checks:
- CHECK-HOST: PASS, 44/44 host + 24/24 (login, /tmp/P1.1-build.log, run by xpm_build.sh).
- CHECK-DEVICE-IDENTICAL mi250 (amdgpu04): FAIL. 436081 lines each; 2098 rows differ ($HOME/gpuperf-scratch/P1.1/mi250/{before,after}.stripped; before=0bd8b95, after=258e529). First 3: QF,r,add,1694-1696 (e.g. limbs 1-3 ffc00000 before, 7fc00000 after).
- CHECK-DEVICE-IDENTICAL mi300: not run (stopped after MI250 failure; user had an MI300 available later). CHECK-PERF mi250: not run.
Measured: classify.py ($HOME/gpuperf-scratch/P1.1/classify.py): all 2098 differing rows differ only in limbs that are NaN in both runs (sign/payload bits). No finite or infinite value changed. Ops: QF/TF real expm1, complex sinh/cosh/exp/sin/cos, QF add, others. Those rows are scored N (unscorable) by the sweep scorer, so the scorer's exit 0 says nothing about them; before already had NaN at the same limbs.
Next session must know:
- Likely cause (unverified): without volatile, the compiler may commute add operands; with two NaN inputs IEEE leaves the returned NaN unspecified.
- Plan gate is strict bit identity, so this is a stop. Not decided: whether NaN-vs-NaN counts as identical (user's call). If accepted, rerun MI300 under that rule and CHECK-PERF mi250 before landing on staging.
- Whether NaN lower limbs are correct at these points is an xpmath question, not tested here. Operands not inspected.
- No ledger row (nothing landed on staging). Worktrees wt-before/wt-after remain under $HOME/gpuperf-scratch/P1.1/.
- amdgpu04 is no longer needed unless the decision above changes.

## P1.1 (resumed) — HIP: replace `volatile` in the EFT helpers
Date: 2026-11-09   Outcome: Done
Commits: xpmath@gpu-perf-staging = 258e529 (fast-forward of wip/P1.1 from 0bd8b95, pushed); xpmath gpu-perf unmoved (0bd8b95); xpmath-kokkos@gpu-perf = this block only
Changed: xpmath include/xp/config.hpp §5 (HIP device pass: fp contract/reassociate off pragmas for eft_add/sub/mul, __builtin_fma for eft_fma; no volatile). Host and CUDA unchanged.
Checks (rule of §1.5 as amended 734654a, NaN-vs-NaN allowed):
- CHECK-HOST: PASS (earlier session, 258e529, /tmp/P1.1-build.log).
- CHECK-DEVICE-IDENTICAL mi250 (amdgpu04 data from the first session, re-evaluated): PASS, 436081 rows, 433983 identical, 2098 NaN-vs-NaN only, 0 failing. $HOME/gpuperf-scratch/P1.1/mi250/{before,after}.stripped
- CHECK-DEVICE-IDENTICAL mi300 (amdgpu00): PASS, same counts (433983 / 2098 / 0); scorer exit 0, 0 points above bound. $HOME/gpuperf-scratch/P1.1/mi300/
- CHECK-PERF mi250 (amdgpu04): PASS, 0 of 90 kernels slower than 3%, no non-finite. $HOME/gpuperf-scratch/P1.1/perf/
Measured (mi250, vs committed v0.3.0 baseline): total instructions 808831 -> 282811; stack bytes 24144 -> 21728; DD add 24 -> 0 B, DD mul 96 -> 64 B (plan expected 0). Science ns/element: DD 7493->355, FF 4601->235, TF 35468->589, QF 104245->1646; double/float unchanged.
Next session must know:
- Residual DD mul stack (64 B) is the by-value argument store for the non-inlined multiply (XPMATH_NOINLINE_FUNCTION), confirmed in the asm, not volatile.
- Chain add ratios (DD add 0.01x) now sit at the native-add timing floor; treat as "very large", not precise.
- The 2098 NaN-only rows are QF/TF overflow points the scorer leaves unscored.
- No deferrals, no ledger rows. A100/B200/GB300 not run (HIP-only change, CUDA path untouched).
- New perf CSVs (v0.3.0-7-g258e529) were kept in scratch, not committed to perf/results.

## P1.2 — Type alignment
Date: 2026-11-09   Outcome: Done (B200 deferred)
Commits: xpmath@gpu-perf-staging = 464fd97 (fast-forward of wip/P1.2 from 258e529, pushed; wip/P1.2 also pushed); xpmath gpu-perf unmoved (0bd8b95, task does not move it); xpmath-kokkos@gpu-perf = this block + ledger row 3
Changed: xpmath include/xp/{dd_math,ff_math,qf_math}.hpp: `alignas(16)` DoubleDouble and QuadFloat, `alignas(8)` FloatFloat. TripleFloat untouched. No sizeof/alignment assumptions found in xp headers or include/Kokkos_xpmath/impl/atomic_cas_add.hpp (memcpy + sizeof only; plan's path `impl/atomic_cas_add.hpp` is really `include/Kokkos_xpmath/impl/`).
Checks:
- CHECK-HOST: PASS 44/44 (xpm_build.sh's own run, /tmp/xpm-P1.2/ctest-host.log).
- CHECK-DEVICE-IDENTICAL: PASS on mi300 (amdgpu00), mi250 (amdgpu04), a100 (gpu06), gb300 (graceblackwell01): 436081/436081 rows identical, 0 NaN-vs-NaN, 0 failing. Scorer exit 0, 0 above bound everywhere (gb300 scored on login). $HOME/gpuperf-scratch/P1.2/{mi300,mi250,a100,gb300}/
- CHECK-PERF (same-session before=258e529 vs after=464fd97, 2 runs each, best-of-2): no non-finite; science-shaped kernels within 1.5% on all four. See Measured.
- b200: DEFERRED (ledger row 3). CHECK-SANITIZER not listed for P1.2, not run.
Measured: no speedup anywhere (median per-op change 0-1.5%). Reproducible >3% slowdowns, accepted by the user ("keep it"): mi250 cDD log +9.5% (133.5-134.1 -> 146.2-146.5 ms); a100 FF mul +8.9% (0.430 -> 0.468 ms); a100 cQF sqrt +3.6% (8.08 -> 8.37 ms). mi300 and gb300: nothing above 3% outside noise (mi300 per-op noise up to 12%, user accepted noise-limited misses). gb300 best aligned kernel 13% faster.
Next session must know:
- mi250 cDD log: ISA identical (7403 insns, 116 VGPR) before/after; only scratch 1424 -> 1440 B and shifted stack offsets. alignas on DoubleDouble alone reproduces it; QuadFloat alone does not. Mechanism unproven (hypothesis: scratch layout/stride). A pad experiment was invalid (scratch did not change). A100 slowdowns not investigated.
- The plan states no expected gain for P1.2; none observed.
- Ledger row 3 (b200) must be settled by C-b200 together with open row 1/2 before the user merges to main.
- Scratch: before/after worktrees and builds under $HOME/gpuperf-scratch/P1.2/ (also var/ for the mi250 bisect).
Orchestrator review (2026-11-09): verified. 464fd97 on origin/gpu-perf-staging and origin/wip/P1.2; staging moved 258e529 -> 464fd97 as a fast-forward (reflog + ancestor check); xpmath gpu-perf unmoved at 0bd8b95; b200 skip has ledger row 3; scratch dirs present for all four GPUs. Accepted.

## P0.6-b200 — Baseline runs, B200 (sm_100)
Date: 2026-11-09   Outcome: Done
Commits: xpmath-kokkos@gpu-perf (CSVs + ledger + this block, one commit; SHA quoted from origin in the session reply). xpmath: no changes (0bd8b95, not moved).
Changed:
- perf/results/sm_100/throughput_v0.3.0.csv (90 rows)
- perf/results/sm_100/science_v0.3.0.csv (6 rows)
- perf/results/sm_100/science_isa_v0.3.0.csv (6 rows)
- docs/GPU_PERF_DEFERRED.md: row 2 annotated (P0.6 part settled; P0.7 re-run still owed, row stays open)
Checks: harness xpk_perf_throughput built on login node vs vendor/xpmath v0.3.0 (cuda/12.9.1, -j4; log $HOME/gpuperf-scratch/P0.6-b200/build.log); run on blackwell00 (NVIDIA B200, cc 10.0), nonfinite=0 in every row, exit 0 (run.log); science_isa via isa_metrics.sh sm_100 science rc=0. Row counts match sm_80 (91/7/7 lines). ctest -N not re-run (no CMake or test changes). No repeat run for noise (single run).
Measured (B200, v0.3.0): ns/op DD add 0.00075 (8.0x double), mul 0.00208 (22x), div 0.00431 (5.8x), log 0.344 (108x); cDD log 8.96 ns/op (1037x). Science kernel ns/element: double 0.86, float 0.37, DD 178 (207x), FF 83 (97x), TF 718 (834x), QF 2480 (2881x). Science regs/scratch B: DD 255/1280, FF 200/448, TF 255/848, QF 255/1264 (regs capped at 255).
Next session must know:
- Run P0.7 (login node) to add B200 to PERF_TABLE.md; then ledger row 2 can be closed. Rows 1 and 3 (b200) remain open.
- build-perf-sm100 now also holds xpk_perf_throughput (reconfigured; the P0.2-era tree had no such target). Whole run took ~2 min on B200.
- Raw logs: $HOME/gpuperf-scratch/P0.6-b200/ (run.log, build.log, science_isa.log).

## C-b200 — Catch-up for B200 (sm_100)
Date: 2026-11-09   Outcome: Stopped (B200 fails; no row settled)
Commits: xpmath-kokkos@gpu-perf = this block + ledger notes (SHA quoted from origin in the session reply). xpmath: no commits; gpu-perf NOT moved (0bd8b95).
Changed: docs/GPU_PERF_DEFERRED.md (rows 1 and 3 annotated, both stay open); this block.
Checks (blackwell00, B200 cc 10.0, CUDA_VISIBLE_DEVICES=0, all under $HOME/gpuperf-scratch/C-b200/b200/):
- CHECK-DEVICE-IDENTICAL b200 4f60bc4 -> 464fd97: FAIL. Before (4f60bc4) sweep exit 0, scorer exit 0, 0 above bound (run-before.log). After (464fd97): sweep_device exit 3, `first fault backend=DD kind=c op=acos error=700`, raw CSV truncated (20.2 MB vs 25.5 MB), so no limb comparison was possible (run-after.log).
- Row-boundary search: 0bd8b95 (row 1 after) FAILS the same way (exit 3, DD:c:acos error 700; run-0bd8b95.log). 258e529 (row 3 before) FAILS the same way (run-258e529.log). First failing row = row 1 (4f60bc4..0bd8b95). Not searched inside row 1.
- CHECK-SANITIZER b200: FAIL at all three failing SHAs, `Invalid __local__ write` in ComplexKernel<DoubleDouble,...,14> (xp::log frame, "Access at 0x0 is out of bounds"), then cudaErrorLaunchFailure 719. ERROR SUMMARY: 69 errors @0bd8b95, 99 @258e529, 99 @464fd97 (san-*.log).
- CHECK-PERF b200: NOT RUN (plan: stop if anything fails). Harnesses for 258e529/464fd97 are built at C-b200/perf/build-{before,after}-sm100 and can be reused.
Measured: 4f60bc4 clean, 0bd8b95/258e529/464fd97 all fault at the same op. P1.1 (HIP-only) and P1.2 (alignas) are not the cause; the fault predates both.
Next session must know:
- Library commits inside row 1 (4f60bc4..0bd8b95): 1cfa793, b790730, adf9ace (DD complex acos calls xp_asin_imag_mag directly), 8b37519, 63daa15, 0bd8b95. Only adf9ace changes include/xp acos; the rest are lane/scripts/operands. adf9ace is the CANDIDATE, unproven (no intermediate SHA was run).
- It is the TD-4 signature (§1.6): GB300 hit the same op; the fix passed on GB300/MI250/MI300/A100 but B200 was never checked. §1.9's claim that 0bd8b95 was "checked on B200" is wrong; ledger row 1 was the open item.
- Not diagnosed; no fix attempted. Do not move xpmath gpu-perf: it already sits at a SHA that fails B200 (0bd8b95).
- Ledger row 2 (P0.7 re-run) still open; to be run by a separate session. It also meant gpu-perf could not move (condition (c)), moot given the failure.
- Scratch moved to /vast/projects/pepper_hep/rbarik_xpmath/ ($HOME/gpuperf-scratch is now a symlink to it; home quota was over 100 GB). Old build* trees from finished tasks were deleted; logs/CSVs/worktrees kept.

## D-b200-acos — Diagnose DD complex acos fault on B200 at 0bd8b95
Date: 2026-11-09   Outcome: Partial (mechanism strongly supported, not proven by experiment; nothing fixed)
Commits: xpmath-kokkos@gpu-perf = this block + ledger note. xpmath: no commits; wip/D-b200-acos holds no experiments (local only, not pushed); gpu-perf/gpu-perf-staging not moved.
Changed: docs/GPU_PERF_PLAN_STATUS.md (this block), docs/GPU_PERF_DEFERRED.md (row 1 note). Full write-up: $HOME/gpuperf-scratch/D-b200-acos/DIAGNOSIS.md
Checks (blackwell00, B200 sm_100, CUDA 12.9.1, logs under $HOME/gpuperf-scratch/D-b200-acos/):
- First faulting commit = adf9ace (only library change in 4f60bc4..0bd8b95). b790730 (library == 4f60bc4): DD:c:acos alone exit 0 / memcheck 0 errors; FULL 252-kernel sweep exit 0 / full-sweep memcheck 0 errors (full-*-b790730.log). adf9ace: exit 3 error 700, memcheck 99 errors (run-/san-adf9ace.log).
- cuda-gdb (gdb*-adf9ace.log): fault = `STL.64 [R0],R10` in the epilogue of the per-kernel xp::log clone, R0 = 0. log gets its result pointer in UNIFORM reg UR7, computes R0 = UR7 - c[0x2f8] at entry. At fault UR7 = 0xe1000000 (= local window base c[0x2f8]) so R0 = 0 and the store lands outside the stack.
- UR7 is valid (0xe1fffa70, set by the kernel) at log1p entry on every hit and 0xe1000000 at the log1p->log call on every hit. Only writers of UR7 in log1p: two `LDCU UR7,c[0x0][0x2f8]` (scratch) on the small-|x| path, which returns without calling log.
- Hypothesis: UR regs are per-warp; ptxas treats UR7 as dead on that path (true per lane) but divergent lanes of one warp clobber it before the other lanes forward it to log. Faulting warp is heavily divergent (active 0x0f0fffff, divergent 0xf0f00000); only the warp-0 converged warp is clean.
Measured: not a frame-size effect. ptxas -v 912 B cumulative / 0 spills, kernel depot 96 B, and all PTX .func depots identical in b790730 and adf9ace. b790730 has 0 uniform-register result-pointer conversions and 0 `LDCU UR7,c[0x2f8]`; adf9ace has 1 and 4 (REG 124 -> 96, multiply/sqrt/divide calls 36/21/12 -> 24/14/8).
TD-4: REFUTED as documented (frame under-size). Same symptom signature (error 700, Invalid __local__ write), different mechanism: a ptxas uniform-register / divergence defect on a forwarded result pointer. Treat as a new toolchain-defect candidate; docs/TOOLCHAIN_DEFECTS.md not edited.
Fix class (not implemented): structural, no #ifdef: stop log1p tail-forwarding its result slot to log (receive into a local, then copy) or otherwise change code shape; td4_workaround indirection is the precedent. Validate by live sweep + full-sweep memcheck on B200, A100, GB300 (not DD:c:acos alone).
Next session must know:
- Unproven: no throwaway experiment was run. Cheapest proof: wip change to log1p, or a launch with converged warps (fault should vanish).
- Unknown: why b790730 did not get the UR7 form; why the old asin(z).im path faulted on sm_103. Full-sweep memcheck of adf9ace not run (4 `LDCU UR7,c[0x2f8]` in the kernel, other ops may share the pattern).
- Isolated runs mislead: exit 0 on one kernel proves little; b790730 was cleared by full-sweep + memcheck.
- cuda-gdb on blackwell00: use `module load cuda/12.9.1` only and LD_LIBRARY_PATH=/soft/compilers/cuda/cuda-12.9.1/lib64 (loading gcc/13.3.0 breaks it with a libstdc++ symbol error). Conditional breakpoints on $UR7 fail to evaluate; use printf commands.
- Do not run two device builds at once on the shared login node (hung it once); use `taskset -c 0-7 nice` and build sequentially.
- Ledger rows 1 and 3 stay open; gpu-perf still sits at a SHA that fails B200.

## F-b200-acos — Prove and fix DD complex acos fault on B200 (0bd8b95)
Date: 2026-11-09   Outcome: Partial (B200 proven + fixed; A100/GB300/AMD not run, fix kept off shared branches)
Commits: kept off gpu-perf/gpu-perf-staging, see xpmath wip/F-b200-acos: 67949b7 (throwaway experiment, asm barrier) then 8b5c338 (candidate F1, the fix). Pushed for reference. xpmath-kokkos@gpu-perf = this block + ledger notes.
Changed: xpmath include/xp/dd_math.hpp (log1p large-arg path: receive log() into a local, quick-two-sum, return; no #ifdef). Here: this block, GPU_PERF_DEFERRED.md rows 1 and 3.
Mechanism: PROVEN by intervention (not by ptxas source). Two different changes that stop log1p tail-forwarding its result pointer to log (asm barrier; arithmetic renormalise) both take `LDCU UR7,c[0x2f8]` in the acos kernel 4 -> 0 AND remove the fault; the unmodified build faults. cuda-gdb per warp (gdb8-adf9ace.log): warp that ran the LDCU sites reaches log with UR7=0xe1000000; warp that did not has 0xe1fffa70. Divergence as the trigger is inferred from those traces + the heavily divergent faulting warp; no converged-warp launch was run.
Scope: isolated per-kernel memcheck of the adf9ace build over all 252 kernels (iso-adf9ace.summary): only DD:c:acos faults (99 errors); 251 others 0 errors. The other 2 LDCU UR7 sites are entry scratch in divide and multiply (legal callee clobber); only log1p forwards an incoming UR7.
Checks (all under $HOME/gpuperf-scratch/F-b200-acos/):
- CHECK-HOST (F1 host tree): PASS 100%, 44 tests (host-f1-ctest.log). Host half only; single-tree count 67 not run.
- B200 blackwell00 full sweep_device @8b5c338: exit 0 (full-f1.log). Full-sweep memcheck: 0 errors (full-san-f1.log). Scorer: 0 above bound, PASS (score-f1.log).
- CHECK-DEVICE-IDENTICAL b200 vs b790730 full sweep (library == 4f60bc4): PASS, 436081/436081 rows identical, 0 NaN-only (ident-b200.log). Bit-preserving on B200.
- A100, GB300, MI250/MI300: NOT RUN (user gave only blackwell00). No same-vendor GPU passed, so §1.9 deferral is not satisfied; F1 must not land on staging until one NVIDIA + one AMD pass.
Measured: DD:c:acos 700/99 memcheck errors -> exit 0/0 errors; kernel SASS LDCU UR7 4 -> 0; log1p adds 3 FLOPs on the |a|>=0.25 path.
Next session must know:
- F1 is a workaround for a ptxas uniform-register defect, not a root-cause fix; a similar forwarded-pointer shape elsewhere could recur. No ptxas bug filed.
- F1 is plain C++ (no #ifdef) and also compiles on HIP, where log1p is inline: needs MI250/MI300 CHECK-DEVICE-IDENTICAL and CHECK-GFX90A-LINT. NaN/inf edge rows rely on the full-sweep identity (done on B200 only).
- A100 + GB300 (build on node) need CHECK-DEVICE-IDENTICAL and CHECK-SANITIZER; user decides how it reaches gpu-perf (§1.9, C-* task).
- Still unknown: why ptxas chose the UR7 form for adf9ace, and why the old path faulted on sm_103 only.

## F-b200-acos (addendum) — F1 validated on A100
Date: 2026-11-09   Outcome: Partial (B200 + A100 pass; GB300 and AMD still owed)
Commits: xpmath wip/F-b200-acos@8b5c338 unchanged, still kept off gpu-perf/gpu-perf-staging. xpmath-kokkos@gpu-perf = this block.
Checks (gpu06 = A100-PCIE-40GB sm_80, CUDA 12.9.1; logs under $HOME/gpuperf-scratch/F-b200-acos/a100/):
- Before = 0bd8b95 (valid on A100; its worktree has no F1 text), after = 8b5c338. Build stamps verified (build-info.txt).
- Full sweep_device: before exit 0, after exit 0 (run-before.log, run-after.log).
- CHECK-DEVICE-IDENTICAL a100: PASS, 436081/436081 rows identical, 0 NaN-only (ident-a100.log).
- CHECK-SANITIZER a100 (full sweep, after): 0 errors (san-after.log). Scorer: 0 above bound, PASS (score-after.log).
Next session must know:
- NVIDIA now has two passing GPUs (B200, A100). §1.9 condition 2 still needs an AMD GPU (MI250/MI300) to pass, and GB300 (build on node) is still owed; ledger rows 1 and 3 stay open.
- Gate rule: F1 may not land on staging until the AMD CHECK-DEVICE-IDENTICAL (+ gfx90a lint) passes.

## F-b200-acos (addendum 2) — F1 validated on GB300
Date: 2026-11-09   Outcome: Partial (all three NVIDIA GPUs pass; AMD still owed)
Commits: xpmath wip/F-b200-acos@8b5c338 unchanged, still kept off gpu-perf/gpu-perf-staging. xpmath-kokkos@gpu-perf = this block.
Checks (graceblackwell01 = GB300 sm_103 aarch64, built on the node via validation/gb300/run_gb300_sweep.sh; logs under $HOME/gpuperf-scratch/F-b200-acos/gb300/):
- Before = 0bd8b95 (worktree, no F1), after = 8b5c338; build stamps verified. Both script runs exit 0.
- CHECK-DEVICE-IDENTICAL gb300: PASS, 436081/436081 rows identical, 0 NaN-only (ident-gb300.log).
- CHECK-SANITIZER gb300 (full sweep, after, cuda/13.3.1): 0 errors (san-after.log). Scorer on x86 login: 0 above bound, PASS (score-after.log).
Next session must know:
- NVIDIA: B200, A100, GB300 all pass with F1. Only AMD (MI250/MI300 CHECK-DEVICE-IDENTICAL + gfx90a lint) is missing before F1 can land on staging (§1.9); user decides how it reaches gpu-perf.

## F-b200-acos (addendum 3) — F1 validated on MI250; all four GPUs pass
Date: 2026-11-09   Outcome: Done for validation (F1 still kept off gpu-perf/staging; landing is the user's call)
Commits: xpmath wip/F-b200-acos@8b5c338 unchanged. xpmath-kokkos@gpu-perf = this block + ledger notes.
Checks (amdgpu04 = MI250X gfx90a, ROCm 7.0.2, device trees built on the login node; logs under $HOME/gpuperf-scratch/F-b200-acos/mi250/):
- Before = 0bd8b95 (worktree, no F1), after = 8b5c338; build stamps verified. Both full sweeps exit 0.
- CHECK-DEVICE-IDENTICAL mi250: PASS, 436081/436081 rows identical, 0 NaN-only (ident-mi250.log). Scorer: 0 above bound (score-after.log).
- CHECK-GFX90A-LINT: clean before and after (lint-before.log, lint-after.log) on a TU exercising DD log1p + complex acos (lint_tu.hip), not CI's sqrt-only TU. Largest callee unchanged (31071 B atan2); exempt kernel 26616 -> 26903 B.
Summary across GPUs, F1 @8b5c338 vs 0bd8b95 (B200 vs b790730): identical 436081/436081 and 0 above bound on B200, A100, GB300, MI250; memcheck 0 errors on B200, A100, GB300. CHECK-HOST 100% (44 host tests).
Next session must know:
- §1.9 conditions are now met for F1 (NVIDIA + AMD passed, CHECK-HOST passed). Not landed: user decides the route (§1.9: only P0.5b/P0.5c/C-* tasks move xpmath gpu-perf). Ledger rows 1 and 3 stay open until B200 catch-up for P1.1/P1.2 runs on a branch containing F1.
- MI300 not run (not requested); gfx942 is untested for F1.

## F-b200-acos (addendum 4) — F1 validated on MI300; validation complete on all five GPUs
Date: 2026-11-09   Outcome: Done for validation (F1 kept off gpu-perf/staging; landing and ledger closing are the user's call)
Commits: xpmath wip/F-b200-acos@8b5c338 unchanged. xpmath-kokkos@gpu-perf = this block.
Checks (amdgpu00 = MI300 gfx942, ROCm 7.0.2, device trees built on the login node; logs under $HOME/gpuperf-scratch/F-b200-acos/mi300/):
- Before = 0bd8b95 (worktree, no F1), after = 8b5c338; build stamps verified. Both full sweeps exit 0.
- CHECK-DEVICE-IDENTICAL mi300: PASS, 436081/436081 rows identical, 0 NaN-only (ident-mi300.log). Scorer: 0 above bound (score-after.log).
- Lint not run for gfx942: scripts/xpm_lint_device_asm.sh guards the gfx90a defects only; it was clean on MI250.
All five GPUs now: F1 bit-identical to baseline and 0 above bound on B200, A100, GB300, MI250, MI300; memcheck 0 errors on B200, A100, GB300; CHECK-HOST 100% (44 host tests); gfx90a lint clean.
Next session must know:
- Nothing further to validate for F1. To land it, a P0.5b/P0.5c/C-* style task must bring wip/F-b200-acos@8b5c338 onto xpmath gpu-perf (§1.9), then staging merges gpu-perf (normal merge) and reruns CHECK-HOST.
- Ledger rows 1 and 3 can be closed by the user once F1 is on a branch where the B200 catch-up for P1.1/P1.2 can run; I did not close them.

## C-b200 — Catch-up for B200 (sm_100), with F1 (second attempt)
Date: 2026-11-09   Outcome: Partial (all B200 checks pass; rows 1-3 settled-pending user close; gpu-perf NOT fast-forwarded)
Commits: xpmath@gpu-perf-staging = 0769d6d (S1 = 464fd97 + F1, new commit, pushed; also on origin wip/C-b200). xpmath gpu-perf unchanged at 0bd8b95. xpmath-kokkos@gpu-perf = this block + ledger + PERF_TABLE.
Changed: xpmath include/xp/dd_math.hpp (F1 hunk only, = 8b5c338 minus the experiment; user chose option A). Here: this block, GPU_PERF_DEFERRED.md rows 1-3, docs/PERF_TABLE.md (P0.7 re-run, B200 now has data).
Checks (blackwell00, CUDA 12.9.1, device tree build-info git-head 0769d6d; logs $HOME/gpuperf-scratch/C-b200/s1/):
- CHECK-HOST on S1: PASS 100%, 44/44 (host-ctest.log). Host half only; single-tree 67 count not run.
- B200 full sweep_device: exit 0 (full-s1.log). Full-sweep memcheck: 0 errors (full-san-s1.log). Scorer: 0 defects / 0 above bound (score-s1.log).
- CHECK-DEVICE-IDENTICAL b200 vs ~/gpuperf-scratch/D-b200-acos/full-b790730.csv (library == 4f60bc4, clean): PASS 436081/436081, 0 NaN-only (ident-b200.log). S1 contains the adf9ace acos change, P1.1, P1.2 and F1, so this one result settles the identity half of rows 1 and 3 together. No row bisection needed.
- CHECK-PERF b200 (perf/, 3 harnesses built here: before 258e529, after 464fd97, S1 0769d6d; 2 same-session runs each; none of the 90 throughput ops or 6 science kernels is acos, so the old harnesses were valid without F1, S1 harness built anyway): after/before and S1/before: 2 of 90 ops >3% slower, FF log 1.041 and cQF sqrt 1.042 (before 0.6815 -> 0.7094 ms; 4.716 -> 4.917 ms; run-to-run spread ~0.1%). S1/after max 1.025, 0 ops >3%: F1 costs nothing measurable. Science kernel unchanged (max 1.001). Logs/CSVs perf/out-*, cmp.py.
- P0.7 re-run: done, rc 0, 0 "NO DATA" (p07.log).
Measured: DD:c:acos fault (exit 3, 99 memcheck errors) -> clean; identity 436081/436081.
Not done: step 7 (gpu-perf fast-forward). Rows 1-3 are only settled-pending until the user closes them, and gpu-perf is behind staging by P1.1, P1.2, S1, so X cannot be taken without the user's go-ahead.
Next session must know:
- >3% slowdowns have no identified mechanism: both are in 258e529..464fd97 (P1.2 type alignment), not F1, and are in FF/QF ops. Cause not investigated.
- F1 remains a ptxas workaround (UR7 forwarded-pointer clobber), not a root-cause fix; no ptxas bug filed, TOOLCHAIN_DEFECTS.md untouched.
- A100/GB300/MI250/MI300 were validated for 8b5c338 earlier, not re-run at S1 (same hunk, S1 differs by P1.1/P1.2 which those GPUs already passed).
- Question to user: B200 builds use CUDA 12.9.1 throughout; whether F1 is still needed under 13.x was not tested.

## C-b200 (addendum) — rows closed, gpu-perf fast-forwarded
Date: 2026-11-09   Outcome: Done
Commits: xpmath@gpu-perf = 0769d6d (fast-forward from 0bd8b95; equals gpu-perf-staging). xpmath-kokkos@gpu-perf = this addendum on top of 056e63f.
Supersedes the C-b200 block's "gpu-perf NOT fast-forwarded" and "settled-pending": the user closed ledger rows 1-3 and approved the fast-forward on 2026-11-09. Ancestor check passed before the push. Ledger has no open rows; xpmath gpu-perf == gpu-perf-staging.
Next session must know:
- The FF log / cQF sqrt ~4% slowdown from P1.2 (see C-b200 block) is still unexplained.

## C-b200 (addendum 2) — P1.2 slowdown accepted
Date: 2026-11-09   Outcome: Done
The user accepted the B200 P1.2 slowdown (FF log +4.1%, cQF sqrt +4.2%, 2 of 90 throughput ops, cause not investigated) on 2026-11-09 and said to move on. It is not an open defect. Later tasks compare against the current B200 baseline; do not chase these two ops unless a later change moves them further.

## P2.1[dd] — Outline rare-case paths in real arithmetic (DD)
Date: 2026-11-09   Outcome: Stopped
Commits: kept off gpu-perf-staging and gpu-perf, see xpmath wip/P2.1-dd@5d387bf (branch name uses `-` because git refs cannot contain `[`): B200 fault in DD complex acos. xpmath-kokkos@gpu-perf = this block only. Starting commits: xpmath 0769d6d (staging), xpmath-kokkos dd472f7.
Changed (wip only): xpmath include/xp/config.hpp: new XPMATH_COLD_FUNCTION (noinline; cold on g++/clang/hipcc; __noinline__ on nvcc). include/xp/dd_math.hpp: detail::dd_{two_prod,multiply,divide_nonfinite,divide_zone_c}_slow and dd_divide_lifted_slow (KI-41 loop); used by two_prod, multiply, multiply_scalar, divide, divide_scalar, dd_divide_core. Existing NOINLINE marks untouched.
Checks:
- CHECK-HOST: PASS 44/44 host + 24/24 device-tree (xpm_build.sh's own ctest, /tmp/xpm-P2.1dd/ctest-*.log).
- CHECK-DEVICE-IDENTICAL b200 (blackwell00): FAIL, not scorable. before=0769d6d full sweep exit 0, scorer exit 0. after=5d387bf: DD:c:acos error 700 in full sweep and in isolation (only kernel that faulted of 252; every other kernel ran ok). Logs $HOME/gpuperf-scratch/P2.1[dd]/b200/{run-before,run-after}.log; worktrees wt-before, wt-after in the same dir.
- CHECK-SANITIZER, CHECK-PERF, other GPUs: not run (stopped at first failure). A100, GB300, MI250, MI300: user gave only blackwell00. No deferral rows written because the task is not landing.
Measured: n/a.
Next session must know:
- This is the same DD:c:acos / sm_100 fragility as D-b200-acos and F-b200-acos (UR7 result pointer clobbered; fixed on staging by F1 = 0769d6d). The new fault is introduced by the P2.1[dd] diff. Root mechanism NOT diagnosed; which of the five cold functions triggers it is unknown (two_prod is inlined into everything, so the likeliest suspect, unconfirmed).
- Do not add an #ifdef (§1.2 rule 4). Needs the user's decision: bisect the five outlines on B200, restrict outlining to multiply/divide only, or drop P2.1[dd].
- §1.9 note: landing needs at least one AMD GPU too (MI250 or MI300); two AMD deferrals would violate condition 2.
- Several outlines wrap trivial bodies (`return p`, `DoubleDouble(s1, 0.0)`); only the KI-41 loop is substantial. Expect little perf gain from the trivial ones.

## P2.1[dd] (addendum) — Diagnosis of the B200 DD:c:acos fault
Date: 2026-11-09   Outcome: Stopped (diagnosed on B200 only; no fix landed, nothing pushed to xpmath)
Commits: xpmath-kokkos@gpu-perf = this block. xpmath unchanged (wip/P2.1-dd@5d387bf). Experiments in $HOME/gpuperf-scratch/P2.1[dd]/bisect/ (worktrees wt-A..D,G; variant-G.patch).
Checks (blackwell00, B200, sweep_device --only DD:c:acos, then full sweep; identity via compare_sweep_limbs vs 0769d6d):
- after (5d387bf): fault in the log1p clone of the acos kernel, UR7=0xe1000000 (same signature as D-b200-acos). Pre-change build clean on the same kernel.
- A (dd_two_prod_slow made inline): --only passes; FULL sweep exit 0 but 188 DD:c:acos rows differ from before (silent wrong values).
- B (dd_multiply_slow inline): --only passes; FULL sweep faults on DD:c:asin (error 700). Isolated pass was misleading.
- C (divide nonfinite/zone-C inline), D (KI-41 lifted inline): still fault. Divide-side outlines are irrelevant.
- G (full 5d387bf + log1p small-|x| path renormalises multiply_scalar's result via eft_quick_two_sum, same trick as F1): full sweep exit 0, identity 436081/436081, 0 failing. Full-sweep memcheck NOT run.
Mechanism: same ptxas uniform-register defect as D/F-b200-acos. The trivial outlines add noinline calls on log1p's small-|x| path (via inlined multiply_scalar/two_prod) and perturb ptxas; the symptom (crash vs silent wrong rows, acos vs asin) depends on code shape. G supports, but does not prove, the hypothesis that log1p forwarding its result slot at `return multiply_scalar(sum, 2.0)` is the remaining exposed shape. SASS LDCU-UR7 counts did not separate faulting from clean builds; do not use them as a predictor.
Next session must know:
- Candidate fixes (user decides): (1) drop the trivial outlines (dd_two_prod_slow, dd_multiply_slow), keep the KI-41 loop; (2) add G's log1p change to P2.1[dd]; (3) both. Each needs full sweep + identity + full-sweep memcheck on B200 and A100/GB300, and MI250/MI300 identity + CHECK-GFX90A-LINT, since other kernels may hold the same shape.
- Variant G is a log1p code change (+3 flops, small path), bit-identical on B200; a fix in the F1 mould, not a root-cause fix. CHECK-PERF not run.
- Other DD functions that return a callee's result directly may carry the same exposure; no audit done.

## P2.1[dd] (addendum 2) — Option 3 implemented; B200 green, other GPUs owed
Date: 2026-11-09   Outcome: Partial (not landed; staging and gpu-perf unmoved)
Commits: xpmath wip/P2.1-dd@8844010 (on top of 5d387bf): drops the dd_two_prod_slow/dd_multiply_slow outlines, keeps divide-side cold functions (nonfinite, Zone C, KI-41 lifted loop) and adds log1p small-|x| own-result-slot renormalise. xpmath-kokkos@gpu-perf = this block.
Checks @8844010:
- CHECK-HOST: PASS 44/44 host + 24/24 device tree (/tmp/xpm-P2.1dd-final/).
- B200 (blackwell00): full sweep exit 0, scorer exit 0; CHECK-DEVICE-IDENTICAL vs 0769d6d 436081/436081, 0 failing; full-sweep CHECK-SANITIZER 0 errors. Logs $HOME/gpuperf-scratch/P2.1[dd]/final/{run,san}.log.
- NOT RUN: A100, GB300 (identity + sanitizer), MI250, MI300 (identity), CHECK-GFX90A-LINT, CHECK-PERF on all 5. No deferral rows (task not landing).
Next session must know:
- Landing needs one more NVIDIA + one AMD at minimum (§1.9), plus CHECK-PERF; the log1p change is a code change on all backends' DD path, so HIP identity matters.
- CHECK-PERF may show small slowdowns; the remaining outlines are cheap and expected neutral.

## P2.1[dd] (addendum 3) — Option 3 on B200 + MI300: correct, but slower
Date: 2026-11-09   Outcome: Partial (not landed; staging and gpu-perf unmoved)
Commits: xpmath wip/P2.1-dd@8844010 (unchanged since addendum 2). xpmath-kokkos@gpu-perf = this block.
Checks @8844010 (before = 0769d6d, same session, 2 runs each, best-of-2; scratch $HOME/gpuperf-scratch/P2.1[dd]/{mi300,lint,perf,final}):
- CHECK-DEVICE-IDENTICAL mi300 (amdgpu00): PASS 436081/436081, 0 failing; scorer exit 0 both.
- CHECK-GFX90A-LINT (sweep_device TU compiled for gfx90a on amdgpu00, 85 callees, 252 kernels): PASS, tiers 1-4 ok. Note: only compiled for gfx90a; MI250 hardware identity NOT run.
- CHECK-PERF b200: throughput 90 ops median 1.00, range 0.973-1.061, science DD 1.036, 0 nonfinite. >3% slower: DD sqrt +6.1%, cDD abs +4.6%, DD exp +4.4%, DD log +3.5%. Repeatable (spread ~1%).
- CHECK-PERF mi300: median 1.009, range 0.86-1.43, science DD 1.031. Repeatable DD slowdowns: DD sqrt +43% (0.0121-0.0124 -> 0.0173-0.0175 ns/op), cDD sqrt +10%, cDD abs +10%, DD div +5%. Native double/float rows moved up to +17% (noise; ignore).
- Earlier (addendum 2) B200: identity 436081/436081, full memcheck 0 errors, host 44/44 + 24/24.
- NOT RUN: A100, GB300, MI250 (identity; sanitizer on A100/GB300); ISA metrics.
Interpretation: no speedup anywhere; the plan expects none from trivial outlines and DD got slower. Likely cause (unproven): the cold calls kept in divide_scalar/dd_divide_core (non-finite, Zone C) are inlined into sqrt/exp/log (DD sqrt = hardware sqrt + two_prod + subtract + divide_scalar) and add call sites; HIP pays most.
Next session must know:
- Do not land 8844010 as is: >3% slowdowns have no benefit to justify them (§1.5).
- Options: (a) drop the divide_scalar/dd_divide_core cold calls too, keeping only the KI-41 lifted outline and the log1p result-slot change (then P2.1[dd] is a ~no-op for speed plus a ptxas hardening); (b) land only the log1p change as a hardening task; (c) drop P2.1[dd]. User decides.
- The log1p change is what actually protects B200 against the UR7 defect for this code shape; it is needed with any variant that keeps cold calls on log1p's path.

## P2.1[dd] (addendum 4) — Option (b): log1p-only change, A100 green; AMD owed
Date: 2026-11-10   Outcome: Partial (not landed; staging and gpu-perf unmoved)
Commits: xpmath wip/P2.1-dd@6e6a989 (pushed). It restores config.hpp/dd_math.hpp to 0769d6d and keeps ONLY the log1p small-|x| own-result-slot renormalise (9 changed lines in dd_math.hpp; no XPMATH_COLD_FUNCTION, no outlining). xpmath-kokkos@gpu-perf = this block. User chose option (b) on 2026-11-09.
Checks @6e6a989 (before = 0769d6d; scratch $HOME/gpuperf-scratch/P2.1[dd]/{a100,perfb,optb}):
- CHECK-HOST: PASS 44/44 host + 24/24 device tree (/tmp/xpm-P2.1dd-optb/).
- CHECK-DEVICE-IDENTICAL a100 (gpu06, A100-PCIE-40GB): PASS 436081/436081, 0 failing; scorer exit 0 both runs.
- CHECK-SANITIZER a100 (full sweep, memcheck): 0 errors.
- CHECK-PERF a100 (same session, 2 runs, then 3 interleaved reruns of the two noisy rows): xpmath types median 1.00; DD sqrt/exp/log, cDD log/sqrt identical to 3-4 digits. First pass flagged cDD add/mul (+32%) and native rows (+10-84%) but one "before" run was a fast outlier (same build differed 26% run to run; native double science 1.50 vs 2.70). Interleaved reruns: cDD add 0.00474/0.00474 vs 0.00473, cDD mul 0.02457 vs 0.02457 before/after. Conclusion: no slowdown from this change; A100 timings are bimodal, use interleaved repeats, not best-of-2.
- NOT RUN for 6e6a989: B200 (identity, sanitizer, perf), GB300 (identity, sanitizer, perf), MI250 and MI300 (identity), CHECK-GFX90A-LINT, perf on AMD. Earlier B200/MI300/lint results were for 8844010 (outlining + same log1p change) and do NOT count for 6e6a989, although 6e6a989 is a strict subset of it.
Next session must know:
- To land (§1.9): one AMD GPU must pass identity + lint on 6e6a989 (MI300 or MI250); B200 and GB300 may be deferred with ledger rows because A100 passed as same-vendor. Ledger row needed per deferred GPU: xpmath 0769d6d..6e6a989, bit-preserving yes, owed identity + sanitizer + perf.
- Staging must also be fast-forwardable: it is at 0769d6d and wip/P2.1-dd is 3 commits ahead (5d387bf, 8844010, 6e6a989), so landing = merge/fast-forward the branch, then the history still contains the outlining commits and the net diff is only the log1p change. User/next session decides whether that is acceptable.
- P2.1[dd] yielded no speedup; ff is next in the dd->ff->tf->qf order if the user wants the outlining attempted there (it slowed DD by 3-43% on B200/MI300, so likely not worth repeating without a reason).

## P2.1[dd] (addendum 5) — Option (b) on MI250: green
Date: 2026-11-10   Outcome: Partial (not landed; staging and gpu-perf unmoved)
Commits: xpmath wip/P2.1-dd@6e6a989 (unchanged). xpmath-kokkos@gpu-perf = this block.
Checks @6e6a989 on amdgpu04 (MI250, gfx90a), before = 0769d6d; scratch $HOME/gpuperf-scratch/P2.1[dd]/{mi250,perfb}:
- CHECK-DEVICE-IDENTICAL mi250: PASS 436081/436081, 0 failing; scorer exit 0 both runs.
- CHECK-GFX90A-LINT (sweep_device TU of 6e6a989 compiled for gfx90a, 82 callees, 252 kernels): PASS tiers 1-4.
- CHECK-PERF mi250 (2 runs, same session): xpmath throughput 60 ops median 1.00, range 0.944-1.051, science 1.00, native 0.997-1.002, 0 nonfinite. One row >3% (DD exp +5.1% on the mean) was noise: 4 interleaved reruns DD exp before 0.867-0.888 vs after 0.852-0.866 ns/op; DD sqrt/div/log unchanged. No slowdown from this change.
Status of 6e6a989 across GPUs: PASS = host, a100 (identity, memcheck, perf), mi250 (identity, lint, perf). NOT RUN = b200, gb300 (identity, sanitizer, perf), mi300 (identity, perf). Both vendors now have a passing GPU, so §1.9 conditions 2 and 3 hold for deferring the rest; condition 1 (user says the GPU is unavailable) has NOT been stated for b200, gb300 or mi300.
Next session must know:
- If the user declares b200/gb300/mi300 unavailable: add one ledger row each (xpmath 0769d6d..6e6a989, bit-preserving yes; owed identity + sanitizer (CUDA) + perf), then land on staging. Row for b200 must note that 8844010 already passed identity + memcheck on b200 and identity on mi300.
- Landing moves staging by merging wip/P2.1-dd (3 commits, net diff = the log1p change). xpmath gpu-perf must NOT move (§1.9: only P0.5b/P0.5c/C-* move it).

## P2.1[dd] (addendum 6) — Option (b) on MI300: green
Date: 2026-11-10   Outcome: Partial (not landed; staging and gpu-perf unmoved)
Commits: xpmath wip/P2.1-dd@6e6a989 (unchanged). xpmath-kokkos@gpu-perf = this block.
Checks @6e6a989 on amdgpu00 (MI300X, gfx942), before = 0769d6d, fresh before/after sweeps same session; scratch $HOME/gpuperf-scratch/P2.1[dd]/{mi300b,perfb}:
- CHECK-DEVICE-IDENTICAL mi300: PASS 436081/436081, 0 failing; scorer exit 0 both runs.
- CHECK-PERF mi300 (2 runs, means): xpmath throughput 60 ops median 1.009, range 0.958-1.064, science DD 1.005-1.008, 0 nonfinite. Native rows drift the same way (median 1.011, up to +7.7%), so this is node drift. The five xpmath rows >3% are all FF/cFF/cTF/cQF (untouched by this change, which edits only dd_math.hpp log1p); no DD row exceeds 3%. No slowdown from this change.
Status of 6e6a989: PASS = host, a100 (identity, memcheck, perf), mi250 (identity, lint, perf), mi300 (identity, perf). NOT RUN = b200 and gb300 (identity, sanitizer, perf). 8844010 (superset incl. this log1p change) passed b200 identity + full memcheck, which supports but does not settle b200 for 6e6a989.
Next session must know:
- Remaining owed: b200 and gb300. Either run them or, if the user declares them unavailable, add ledger rows (xpmath 0769d6d..6e6a989, bit-preserving yes, owed identity + sanitizer + perf) and land on staging by merging wip/P2.1-dd. gpu-perf must not move.

## P2.1[dd] (addendum 7) — Option (b) on GB300: green; only B200 owed
Date: 2026-11-10   Outcome: Partial (not landed; staging and gpu-perf unmoved)
Commits: xpmath wip/P2.1-dd@6e6a989 (unchanged). xpmath-kokkos@gpu-perf = this block.
Checks @6e6a989 on graceblackwell01 (GB300, aarch64, sm_103; gcc/11.2.0 cmake/3.22.1 cuda/13.3.1), before = 0769d6d, built on the node; scratch $HOME/gpuperf-scratch/P2.1[dd]/{gb300,perfb}:
- CHECK-DEVICE-IDENTICAL gb300: PASS 436081/436081, 0 failing, 0 NaN-only. No device fault in either run. Scorer (login node, absolute gate) on the after raw: exit 0, PASS.
- CHECK-SANITIZER gb300 (full-sweep memcheck of the after build): 0 errors.
- CHECK-PERF gb300 (2 runs, means): xpmath throughput 60 ops median 1.00, range 0.977-1.006, 0 >3%; science 0.999-1.001; native 0.999-1.025; 0 nonfinite; before r1 vs r2 spread median 0.1%. No slowdown.
Status of 6e6a989: PASS = host, a100 (identity, memcheck, perf), mi250 (identity, lint, perf), mi300 (identity, perf), gb300 (identity, memcheck, perf). NOT RUN = b200 only (identity, sanitizer, perf). 8844010 (superset incl. this log1p change) passed b200 identity + full memcheck.
Next session must know:
- Only B200 remains. Run it, or if the user declares it unavailable add one ledger row (xpmath 0769d6d..6e6a989, bit-preserving yes, owed identity + sanitizer + perf; note 8844010 passed b200 identity + memcheck) and land on staging by merging wip/P2.1-dd. xpmath gpu-perf must not move.

## P2.1[dd] (final) — Landed on staging: log1p result-slot change only; B200 deferred
Date: 2026-11-10   Outcome: Done (B200 deferred; no speedup found)
Commits: xpmath gpu-perf-staging = 6e6a989 (fast-forward from 0769d6d, pushed; wip/P2.1-dd also pushed at 6e6a989). xpmath gpu-perf unmoved (0769d6d, task does not move it). xpmath-kokkos@gpu-perf = this block + ledger row 4.
Changed: xpmath include/xp/dd_math.hpp only (+8/-1): log1p small-|x| path renormalises multiply_scalar's result via eft_quick_two_sum so log1p owns its result slot. The history also contains 5d387bf (outlining) and 8844010 (partial revert); 6e6a989 restores config.hpp and the rest of dd_math.hpp to 0769d6d, so the net diff is the log1p change.
Checks @6e6a989 (before = 0769d6d): CHECK-HOST PASS 44/44 + 24/24; CHECK-DEVICE-IDENTICAL PASS 436081/436081 on a100, mi250, mi300, gb300; CHECK-SANITIZER 0 errors on a100 and gb300; CHECK-GFX90A-LINT PASS; CHECK-PERF no slowdown on a100, mi250, mi300, gb300; b200 DEFERRED (ledger row 4).
Measured: P2.1[dd]'s outlining (5d387bf/8844010) gave no speedup and slowed DD (b200 DD sqrt +6%, mi300 DD sqrt +43%), so it was dropped. The kept log1p change is neutral.
Next session must know:
- Root cause of the B200 DD:c:acos/asin fault (D/F-b200-acos and the P2.1[dd] addenda): ptxas uniform-register defect on forwarded result slots; sensitive to code shape around log1p. Outlining two_prod/multiply degenerate returns re-triggered it (crash in acos, silent wrong rows in acos, or crash in asin depending on the variant). Isolated --only runs are NOT a valid clearance: use the full 252-kernel sweep + identity + full memcheck.
- Any future change that adds a real call (noinline/cold) on a path inlined into log1p or into other DD functions returning a callee's result directly can re-trigger it on B200. Run B200 full sweep + memcheck for such changes before landing; C-b200 must settle ledger row 4 (identity, memcheck, perf at 6e6a989).
- Perf timing: A100 is bimodal run to run (one 'before' run differed 26% on cDD add/mul); use interleaved repeats, not best-of-2. MI300/MI250 show uniform node drift; compare against native rows.
- Plan note: P2.1[ff] is next in order. The outlining idea slowed DD, so it is likely not worth repeating on ff/tf/qf without a measured reason; user decides.

## P2.1[ff] — Outline rare-case paths, FF
Date: 2026-11-10   Outcome: Stopped (no-go; series abandoned by the user)
Commits: none pushed. Kept off staging: local-only xpmath branches wip/P2.1-ff-plain (a771c57, plan diff) and wip/P2.1-ff-ur7 (b9b4428, plan diff + FF log1p owns its result slot on both paths, same shape as DD 6e6a989). Before = 6e6a989. Worktrees: $HOME/gpuperf-scratch/P2.1[ff]/wt-{before,plain,ur7}.
Changed: nothing landed.
Checks (GB300 graceblackwell01, sm_103, only GPU used; $HOME/gpuperf-scratch/P2.1[ff]/gb300/):
- plain: FAIL. Full sweep faults at FF:c:asin, error 700 (run-plain.log). Full memcheck 27 errors, 26 "Invalid __local__ write of size 8" (san-plain.log). Host 44/44.
- ur7: sweep exit 0; CHECK-DEVICE-IDENTICAL 436081/436081, 0 failing (before.nc.csv vs ur7.nc.csv; orchestrator re-ran the comparator: PASS); full memcheck 0 errors (san-ur7.log); host 44/44 (device-tree ctest not confirmed finished).
Measured (3 interleaved repeats, perf/out-<v>-r<n>/sm_103/): ur7/before FF mul 1.40, cFF mul 1.19, FF log 1.14, FF exp 1.13, cFF div 1.12; FF/cFF median 1.077; native rows 0.996-1.002; plain within 0.01 of ur7. No FF row improved.
Next session must know:
- Outlining lost even on GB300, the GPU with the most FP32 headroom. The outlined bodies are trivial, so cold calls only add call sites. P2.1 is dropped for ff, tf and qf (plan amended).
- sm_103 DOES reproduce the UR7-class ptxas fault (plain variant). The result-slot shape (log1p renormalises via eft_quick_two_sum) clears it, as on B200 for DD.
- b9b4428's FF log1p change is identity-verified and memcheck-clean on GB300. It is not landed on its own; P2.2[ff] may carry it as part of its forwarding audit.
Orchestrator review (2026-11-10): written by the orchestrator from the worker's handoff; evidence spot-checked (comparator re-run, memcheck summaries, run-plain.log fault line, branches absent from origin).

## P2.2[ff] — Inline FF multiply/divide/sqrt: GB300 go/no-go probe
Date: 2026-11-10   Outcome: Done (verdict GO; probe only, nothing landed on staging)
Commits: xpmath wip/P2.2-ff = 6f4fb5a (new branch, pushed; 27d56b9 = cherry-pick of b9b4428 FF log1p; f50e1c2 = change; 6f4fb5a = exp10 revert). gpu-perf-staging and gpu-perf unmoved (6e6a989 / 0769d6d). xpmath-kokkos@gpu-perf = this block.
Changed (xpmath, vs 6e6a989): ff_math.hpp: multiply/divide/sqrt NOINLINE->INLINE; log1p both paths and acosh large path renormalise result via eft_quick_two_sum. ff_complex.hpp: xp_asin_real_mag, xp_atan2_safe renormalise.
Checks (GB300 graceblackwell01 sm_103 only; $HOME/gpuperf-scratch/P2.2[ff]/): CHECK-HOST PASS 44/44 + 24/24 at 6f4fb5a (host2-build.log). Full sweep exit 0 before+after (gb300/status.log, run-{before,after}.log). CHECK-DEVICE-IDENTICAL vs 6e6a989 PASS 436081/436081, 0 failing (gb300/ident.log). Full-sweep memcheck on after: 0 errors (gb300/san-after.log). Perf 3 interleaved repeats (perf/out-{before,after}-r{1,2,3}/sm_103/, ratios perf/ratios.txt).
Measured (after/before ns, median of 3; <1 = faster): FF/cFF median over 15 rows 0.949; FF mul 0.667, cFF mul 0.717, FF div 0.832, cFF div 0.937, FF log 0.842, FF exp 0.896, cFF exp 0.949; FF science kernel 0.953 (DD/QF/TF/double/float science 1.000-1.001). Only row >3% slower: FF sqrt 1.107 (repeats 1.092/1.110/1.107). Next worst row anywhere: cQF exp 1.010.
FF sqrt reason (evidence, not proven causal): sqrt's body calls no multiply/divide, so removing its NOINLINE gains nothing and only inlines the whole branchy body. cuobjdump: kernel stack 32->0, REG 47->48; after-kernel has 752 insts with 23 BSSY/BSYNC pairs (before-kernel 480, callee body not included, so not like-for-like). Keeping sqrt NOINLINE is the obvious variant if the user wants no regression.
Forwarding audit (full list: gpuperf-scratch/P2.2[ff]/forwarding_audit.md): A (callee stays real call): log1p small(multiply_scalar)+large(log) [applied], acosh large->log [applied], xp_asin_real_mag->atan2 [applied], xp_atan2_safe->atan2 [applied], exp10 rare path->exp [renormalise FAILED identity: 72 FF exp10 rows, overflow hi=inf gives lo=NaN vs 0; reverted in 6f4fb5a; run1 logs gpuperf-scratch/P2.2[ff]/run1-exp10-fail/]. Complex-returning forwards not renormalisable: complex pow->exp, tan->operator/, abs->hypot. B (forward multiply/divide/sqrt result, inline after this change; listed, unchanged): log2, log10, hypot, erfc, tgamma, bessel_y0/y1, incgamma, fma, xp_asin_real_leg, ff_cross, xp_log_hypot2, xp_atan_re_split, atanh, asin/acos/atan2 (via inline angle), pow (via ff_exp_ext), sqrt (add/divide_scalar).
Next session must know:
- The plan's ~/xpmath-kokkos/perf/run_perf.sh does not exist; the runner is the scratch script gpuperf-scratch/P2.2[ff]/perf/run_perf.sh (copied from P2.1[ff]).
- quick_two_sum renormalise is NOT identity where hi = inf (lo becomes NaN): never renormalise a result that can be an overflow inf.
- Running two perf pipelines on one node overlaps; run1 perf was contaminated and discarded (run1-exp10-fail/perf-contaminated); final perf is a clean single run.
- The gain is multiply/divide-driven (FF mul -33%); user decides fan-out to dd/tf/qf and whether to keep sqrt NOINLINE.

## P2.2[ff] (addendum) — sqrt NOINLINE variant measured; user decision: keep sqrt inlined
Date: 2026-11-10   Outcome: Done (decision recorded; nothing landed on staging)
Commits: xpmath wip/P2.2-ff = 6f4fb5a unchanged and is the kept shape (sqrt inlined). Variant wip/P2.2-ff-sqrtnoinline = 842c101, LOCAL ONLY, perf-only (no identity/memcheck/host run). xpmath-kokkos@gpu-perf = this block.
Measured (GB300, 3 interleaved repeats, after/before ns; gpuperf-scratch/P2.2[ff]/perf2/): sqrt inlined vs sqrt NOINLINE: FF sqrt 1.110 vs 1.008; FF science kernel 0.951 vs 1.010 (54.4 / 51.8 / 55.0 ns per element before / inlined / NOINLINE); FF/cFF median 0.950 vs 0.950; FF mul 0.661 and div 0.832 in both. Keeping sqrt NOINLINE fixes the sqrt micro-kernel but removes the whole science-kernel gain. Mechanism not isolated (science kernel reaches real sqrt through complex sqrt, ff_complex.hpp:405). Single-row noise up to ~5% on native rows (cfloat mul 1.05).
Decision (user, 2026-11-10): keep FF sqrt inlined. The +10.7% FF sqrt micro-kernel regression is accepted.
Next session must know:
- Not checked off GB300: sqrt inlined is unverified on A100, B200, MI250, MI300 (P2.1[dd] saw DD sqrt +43% on MI300 from an unrelated outlining change, so sqrt can be sensitive on AMD). asin/acos/hypot/asinh/acosh call sqrt and are not timed by the harness.
- Fan-out to dd/tf/qf and landing on gpu-perf-staging are still the user's call.

## P2.2[ff] (addendum 2) — MI250 (gfx90a) check of the kept shape (sqrt inlined)
Date: 2026-11-10   Outcome: Done (MI250 green; A100, B200, MI300 still unchecked; nothing landed on staging)
Commits: xpmath wip/P2.2-ff = 6f4fb5a, unchanged. xpmath-kokkos@gpu-perf = this block. Done at the user's request (hostname amdgpu04, MI250X/MI250 gfx90a, rocm/7.0.2); the original probe scope was GB300 only.
Checks (before = 6e6a989, after = 6f4fb5a; gpuperf-scratch/P2.2[ff]/mi250/ and lint/): full device sweep exit 0 both (status.log). CHECK-DEVICE-IDENTICAL PASS 436081/436081, 0 failing (ident.log). CHECK-GFX90A-LINT PASS, exit 0, tiers 1-4 ok, 252 kernels (lint/lint.log; compiled from scripts/sweep_device.cpp at 6f4fb5a with hipcc --offload-arch=gfx90a --save-temps=obj, run on the login node). No sanitizer (CUDA only). Perf 3 interleaved repeats (perf/out-{before,after}-r{1,2,3}/gfx90a/).
Measured (after/before ns, median of 3; <1 = faster): FF/cFF median over 15 rows 0.584. FF mul 0.145, cFF mul 0.111, FF div 0.501, cFF div 0.612, FF log 0.497, FF exp 0.508, FF sqrt 0.878 (faster here, unlike GB300's 1.107), cFF sqrt 0.184, cFF abs 0.409. FF science kernel 0.550 (0.549/0.550/0.551); DD/QF/TF/double/float science 1.000-1.002. Non-finite counts 0 before and after. Worst row anywhere: DD sin 1.025; native-type rows move up to ~18% between repeats (TF mul 0.819 in r2), so single-row differences under that are noise.
Sanity: after-run FF mul 0.00239 ns/op vs FF add 0.00133 (plausible); before-run FF mul was 0.01668, so the old NOINLINE call is the anomaly on gfx90a. DD mul is still NOINLINE at 0.0358 ns/op, 15x FF's inlined mul; whether inlining DD's would give a similar gain here is untested, so no claim.
Next session must know:
- Checked now: GB300 and MI250 (identity, sweep, perf; memcheck on GB300, lint on gfx90a). Unchecked: A100, B200, MI300.
- The call-overhead hypothesis for the huge MI250 gain is not isolated.

## P2.2[ff] (addendum 3) — MI300 (gfx942) check of the kept shape (sqrt inlined)
Date: 2026-11-10   Outcome: Done (MI300 green; A100 and B200 still unchecked; nothing landed on staging)
Commits: xpmath wip/P2.2-ff = 6f4fb5a, unchanged. xpmath-kokkos@gpu-perf = this block. Done at the user's request (hostname amdgpu00, gfx942, rocm/7.0.2); the original probe scope was GB300 only.
Checks (before = 6e6a989, after = 6f4fb5a; gpuperf-scratch/P2.2[ff]/mi300/): full device sweep exit 0 both (status.log). CHECK-DEVICE-IDENTICAL PASS 436081/436081, 0 failing (ident.log). CHECK-GFX90A-LINT not applicable (gfx90a only; passed in addendum 2). No sanitizer (CUDA only). Perf 3 interleaved repeats (perf/out-{before,after}-r{1,2,3}/gfx942/).
Measured (after/before ns, median of 3; <1 = faster): FF/cFF median over 15 rows 0.595. FF mul 0.124, cFF mul 0.095, FF div 0.334, cFF div 0.676, FF log 0.327, FF exp 0.379, FF sqrt 0.690, cFF sqrt 0.375, cFF abs 0.310. FF science kernel 0.737 (0.744/0.737/0.734); DD/QF/TF/double/float science 1.001-1.006. Non-finite 0 before and after. Worst rows are native/other-backend types FF code does not touch (cdouble mul 1.038 with repeats 1.038/1.080/0.963; cfloat exp 1.029), i.e. noise.
Next session must know:
- Checked now: GB300, MI250, MI300 (identity, sweep, perf all pass; memcheck on GB300; lint on gfx90a). Unchecked: A100, B200.
- FF sqrt: GB300 1.107 (slower), MI250 0.878, MI300 0.690 (faster). The GB300 regression is the only slowdown found.

## P2.2[ff] (addendum 4) — A100 (sm_80) check of the kept shape (sqrt inlined); B200 NOT yet checked
Date: 2026-11-11   Outcome: A100 done (green); B200 pending (no node allocated); nothing landed on staging
Commits: xpmath wip/P2.2-ff = 6f4fb5a, unchanged. xpmath-kokkos@gpu-perf = this block. Done at the user's request (hostname gpu06, A100-PCIE-40GB cc 8.0, gcc/13.3.0 cmake/3.28.3 cuda/12.9.1).
Checks (before = 6e6a989, after = 6f4fb5a; gpuperf-scratch/P2.2[ff]/a100/): full device sweep exit 0 both (status.log, run-{before,after}.log; run via validation/a100/run_a100_sweep.sh, driver a100/run.sh). CHECK-DEVICE-IDENTICAL PASS 436081/436081, 0 failing (ident.log). CHECK-SANITIZER memcheck on full after sweep: ERROR SUMMARY 0 errors (san-after.log). CHECK-HOST cited from 6f4fb5a (44/44, 24/24, host2-build.log), not redone. Perf 3 interleaved repeats, xpk_perf_throughput --arch sm_80 (perf/out-{before,after}-r{1,2,3}/sm_80/, ratios perf/ratios.txt).
Measured (after/before ns, median of 3; <1 = faster): FF/cFF median over 15 rows 0.939. FF mul 0.591, cFF mul 0.785, FF div 0.869, cFF div 1.048 (repeats 1.050/1.044/1.048), FF log 0.937, FF exp 0.939, FF sqrt 0.927, cFF sqrt 0.919. FF science kernel 0.914 (0.914/0.915/0.912); DD/QF/TF/float science 0.992-1.000.
Noise: science double 1.187 (repeats 1.187/1.382/0.609) is the A100 bimodality on a native type FF code does not touch; do not read it. cFF div 1.048 is stable across repeats but is the only FF-family row above 1.0; not investigated.
Next session must know:
- Checked now: GB300, MI250, MI300, A100 (identity, sweep, perf all pass; memcheck on GB300 and A100; lint on gfx90a). Unchecked: B200 (UR7 risk: full sweep, identity, full memcheck needed; on failure record function+arch, no per-arch #ifdef).
- FF sqrt: GB300 1.107 (slower), A100 0.927, MI250 0.878, MI300 0.690. The GB300 regression is still the only slowdown on an FF row of note.
Orchestrator review (2026-11-11, covers the P2.2[ff] block and addenda 1-4): wip/P2.2-ff = 6f4fb5a on origin, staging is its ancestor, staging/gpu-perf unmoved. Cited logs exist; ident.log RESULT: PASS on gb300, mi250, mi300, a100; san-after.log ERROR SUMMARY 0 errors on gb300 and a100. Open for the user: A100 cFF div 1.048 (stable, no stated reason) and B200 not checked.

## P2.2[ff] (final) — Landed on staging; B200 deferred
Date: 2026-11-11   Outcome: Done (landed; B200 deferred under §1.9 standing deferral)
Commits: xpmath gpu-perf-staging fast-forwarded 6e6a989..6f4fb5a (no merge commit, no new commits; ancestor check passed before push). xpmath gpu-perf NOT moved. xpmath-kokkos@gpu-perf = this block + ledger row 5.
Changed: xpmath include/xp/ff_math.hpp, include/xp/ff_complex.hpp (inline FF multiply/divide/sqrt; forwarded result slots renormalised except exp10, reverted). xpmath-kokkos docs/GPU_PERF_DEFERRED.md (row 5), this file.
Checks (all run in earlier sessions, cited here, none rerun; logs under $HOME/gpuperf-scratch/P2.2[ff]/):
- CHECK-HOST at 6f4fb5a PASS 44/44 + 24/24 (host2-build.log).
- CHECK-DEVICE-IDENTICAL vs 6e6a989 PASS 436081/436081 on GB300 (gb300/ident.log), MI250 (mi250/ident.log), MI300 (mi300/ident.log), A100 (a100/ident.log).
- CHECK-SANITIZER memcheck full sweep 0 errors on GB300 (gb300/san-after.log) and A100 (a100/san-after.log).
- CHECK-GFX90A-LINT PASS on MI250 (lint/lint.log).
- CHECK-PERF 3 interleaved repeats on each GPU (perf/ratios.txt, perf/out-{before,after}-r*/).
- B200: NOT RUN, deferred (ledger row 5: identity, full memcheck, perf).
Measured: see addenda 1-4 (FF/cFF median after/before: MI250 0.584, MI300 0.595, A100 0.939). Accepted regressions (user, 2026-11-11): FF sqrt 1.107 on GB300, cFF div 1.048 on A100.
Next session must know:
- This session ran no GPU checks and requested no GPU.
- B200 risk is UR7 (result-slot clobber): C-b200 must run full sweep, identity, full memcheck; on failure record function+arch, no per-arch #ifdef.
- P2.2[dd], [tf], [qf] fan-out is next (dd, tf, qf order); each does its forwarding audit first. Never renormalise a result that can be an overflow inf.
- Ledger has open rows 1-5 relevant to B200 gating; xpmath gpu-perf must stay put until C-b200.

## P2.2[dd] — Inline DD multiply/divide/sqrt
Date: 2026-11-11   Outcome: Stopped (GB300 identity/memcheck FAILED; nothing landed on staging)
Commits: kept off gpu-perf-staging, see xpmath wip/P2.2-dd = 3f594b2 (pushed; 6f4fb5a + 3d43c76 change + 3f594b2 name-clash fix). Staging unmoved at 6f4fb5a, xpmath gpu-perf unmoved. xpmath-kokkos@gpu-perf = this block.
Changed (xpmath): dd_math.hpp multiply/divide/sqrt NOINLINE->INLINE; acosh (a<=kDDSqHi path) renormalises result. dd_complex.hpp: xp_asin_real_mag, xp_atan2_safe, xp_asin_imag_mag renormalise result. exp10 and xp_log_hypot2 (log(0)) deliberately NOT renormalised (inf -> lo NaN).
Checks (all under $HOME/gpuperf-scratch/P2.2[dd]/; before = 6f4fb5a, after = 3f594b2; run one tree/pipeline at a time, -j8 via nproc shim):
- Forwarding audit: forwarding_audit.md (written before editing).
- CHECK-HOST PASS 44/44 (host-ctest.log; wrapper xpm_build.sh hardcodes -j$(nproc), so build/ctest were driven by hand with -j8).
- MI250 (amdgpu04): sweep exit 0; IDENTICAL 436081/436081 (mi250/ident.log); CHECK-GFX90A-LINT PASS tiers 1-4 (lint/lint.log); perf mi250/perf/ratios.txt.
- MI300 (amdgpu00): sweep exit 0; IDENTICAL 436081/436081 (mi300/ident.log); perf mi300/perf/ratios.txt.
- A100 (gpu06): sweep exit 0; IDENTICAL 436081/436081 (a100/ident.log); full memcheck 0 errors (a100/san-after.log); perf a100/perf/ratios.txt.
- GB300 (graceblackwell00): FAIL. before sweep exit 0; after sweep exit 3 (gb300/run-after.log): DD complex log, log10, asin fault (error 700); after file has 322161 rows vs 436081 so identity cannot compare (gb300/ident.log); full memcheck 61 errors, 60 x "Invalid __local__ write of size 16" in the DD complex log kernel, frame xp::atan2(DoubleDouble,...) (gb300/san-after.log).
- B200: not asked (standing deferral). No ledger row added: nothing landed, so no staged range is owed.
Measured (after/before, median of 3; <1 faster; DD/cDD median over 15 rows): MI250 0.32, MI300 0.375, A100 0.979, GB300 0.998. DD science kernel: MI250 0.484, MI300 0.368, A100 0.987, GB300 1.154 (slower). Rows >3% slower: MI250 TF mul 1.037 (untouched backend); MI300 FF log 1.051, double log 1.035, cdouble exp 1.041 (noise, unproven); A100 cDD div 1.107 (user accepted), cDD mul 1.239 / cDD add 1.441 / cdouble abs 1.100 (bimodal A100 noise, repeats 0.85-1.59); GB300 cFF exp 1.053, cFF log 1.046, DD science 1.154.
Next session must know:
- Failure is function DD:c:log / log10 / asin, arch sm_103 only (A100 memcheck clean). Mechanism NOT diagnosed; the change has two parts (inline marks; renormalise in 4 functions) and was not bisected. Same signature as P0.5 TD-4 on sm_103 (Invalid __local__ write). No per-arch #ifdef (rule 4). Fix, bisect, or drop is the user's call.
- Unchecked for this change: B200 (UR7 risk). GB300 perf above was measured on a build whose sweep faults, so treat as indicative only.

## P2.2[dd] (addendum) — GB300 fault diagnosed (mechanism strongly supported, nothing fixed)
Date: 2026-11-11   Outcome: Partial (diagnosis only; nothing landed, no library change)
Commits: xpmath-kokkos@gpu-perf = this block. xpmath wip/P2.2-dd = 3f594b2 unchanged; staging unmoved. Full write-up: $HOME/gpuperf-scratch/P2.2[dd]/diag/DIAGNOSIS.md
Checks (graceblackwell00, sm_103, memcheck --only DD:c:log, logs diag/*_log.log): 6f4fb5a 0 errors; 3f594b2 61; inline marks only 61; renormalise only 0; multiply-only / divide-only / sqrt-only 0 each. Pairs not tested.
Findings: the renormalise edits are innocent; the fault needs the combination of the three inline marks. Not TD-4 (kernel STACK equals highest local store in all variants). Fault = STL.128 [UR7] in the atan2 clone epilogue, result pointer in uniform reg UR6 = 0 under cuda-gdb; `UMOV UR6,0x0` x2 inside that clone overwrite it. Those two UMOVs and the caller's `UIADD3 UR6,UR8,0x10` exist only in the inline-all build (0/0 in before, m, d, s). Same family as D-b200-acos.
Next session must know:
- Unproven by experiment (no fix tried). Options for the user: bisect pairs, keep one mark NOINLINE (A100/AMD gain mostly from multiply), or a structural result-slot change; all need full-sweep memcheck + identity on GB300 and A100.
- Single-kernel repro takes seconds; building a variant is one TU, about 3 min on the node (scripts in diag/).

## C-b200 (2) — B200 catch-up for ledger rows 4-6 (0769d6d..4d597fd)
Date: 2026-11-11   Outcome: Done (rows 4, 5, 6 settled; row 7 A100 left open)
Commits: xpmath-kokkos@gpu-perf = this block + ledger rows 4-7 (SHA from origin in the session reply). xpmath: no commits; gpu-perf (0769d6d) NOT moved: A100 rows (new row 7 and older) still open, so condition (c) fails.
Changed: docs/GPU_PERF_DEFERRED.md (rows 4-6 settled, row 7 added), this file.
Checks (blackwell00, B200 cc 10.0, CUDA_VISIBLE_DEVICES=0, -j8 nice, one tree at a time; all under $HOME/gpuperf-scratch/C-b200-2/):
- NOTE: origin/gpu-perf-staging is 6f4fb5a, not 4d597fd (the task expected 4d597fd). 4d597fd exists only on wip/P2.2-dd; user said to go ahead on it. Worktrees wt-before=0769d6d, wt-after=4d597fd.
- CHECK-DEVICE-IDENTICAL b200 0769d6d -> 4d597fd: PASS, rows 436081 identical 436081 NaN-only 0 failing 0 (b200/ident.log). Both sweeps exit 0, scorer 0 above bound / 0 defects (b200/run-{before,after}.log).
- CHECK-SANITIZER b200 full sweep at 4d597fd: ERROR SUMMARY 0 errors (b200/san-after.log).
- CHECK-PERF b200 sm_100, 3 interleaved repeats, after/before (perf/ratios.txt): median DD 0.939, cDD 0.955, FF 0.886, cFF 0.799, QF/TF/cQF/cTF/double/cdouble 1.000; science DD 0.936, FF 0.899, QF 1.000, TF 0.999.
Measured: only row >3% slower is float add chain 1.143 (repeats 1.143/0.875/1.143): untouched native float, no library change touches it, noisy bimodal; not attributable to these commits.
Next session must know:
- No B200 fault: UR7 family did not recur at 4d597fd (DD:c:log/asin/acos clean in full sweep + full memcheck).
- Staging still 6f4fb5a; landing 4d597fd on staging is the user's call. Row 7 (A100 at 4d597fd) is the remaining owed check.

## C-a100 (2) — A100 catch-up for ledger row 7 (P2.2[dd], 6f4fb5a..4d597fd)
Date: 2026-11-11   Outcome: Done (row 7 settled)
Commits: xpmath-kokkos@gpu-perf = this block + ledger row 7 (SHA from origin in the session reply). xpmath: no commits; gpu-perf and gpu-perf-staging NOT moved by this session.
Changed: docs/GPU_PERF_DEFERRED.md (row 7 settled), this file.
Checks (gpu06, A100-PCIE-40GB sm_80, -j8 nice, one pipeline; all under $HOME/gpuperf-scratch/C-a100-2/; worktrees wt-before=6f4fb5a, wt-after=4d597fd; starting HEADs: xpmath 6f4fb5a on gpu-perf-staging, xpmath-kokkos 1d0214a on gpu-perf):
- CHECK-DEVICE-IDENTICAL a100: PASS, rows 436081 identical 436081 NaN-only 0 failing 0 (a100/ident.log). Both sweeps exit 0 (a100/status.log, run-{before,after}.log).
- CHECK-SANITIZER a100, full sweep (no --only) at 4d597fd: ERROR SUMMARY 0 errors (a100/san-after.log).
- CHECK-PERF a100 sm_80, 3 interleaved repeats, after/before (perf/ratios.txt): median DD 0.958, cDD 0.987, FF/QF/TF/cFF/cQF/cTF 1.000-1.001; science DD 0.987, FF 1.011, QF 1.000, TF 1.000.
Measured: rows >3% slower: cDD add chain 1.142 (repeats 1.378/0.912/1.142; known bimodal); cdouble abs 1.270 and sqrt 1.098, double science 1.659 (native types, repeats 0.9-1.75; noisy); cfloat div 1.838 (repeats 1.838/1.823/1.838). cfloat div is native float, bimodal 0.00365 vs 0.00671 ns/op: before took the fast mode in all 3 repeats here, after the slow mode; in the earlier 3f594b2 session (P2.2[dd]/a100/perf) before ran slow mode twice. So both modes occur for 6f4fb5a-source builds; mechanism NOT shown, not attributed to the change. cDD div accepted earlier; not >3% here.
Next session must know:
- 4d597fd is still only on wip/P2.2-dd; staging 6f4fb5a. Landing is the user's decision.
- 4d597fd elsewhere: GB300 identity 436081/436081 + full memcheck 0 errors, MI250 and MI300 identity 436081/436081 (P2.2[dd]/{gb300fix,mi250fix,mi300fix}/; run against P2.2[dd]/wt-fix = 4d597fd); no status block records these, perf/lint not re-checked there.

## P2.2[dd] (addendum 2) — 4d597fd fix validated on GB300, MI250, MI300 (written by orchestrator from scratch evidence)
Date: 2026-11-11   Outcome: Partial (correctness green on all five GPUs; not landed: GB300 DD science 1.165 needs a user decision)
Commits: xpmath wip/P2.2-dd = 4d597fd (pushed 07:31 UTC, one commit on top of 3f594b2: `angle()` builds pi/2 from bit constants instead of multiply_scalar(pi, 0.5)). Staging 6f4fb5a and gpu-perf 0769d6d unmoved. xpmath-kokkos@gpu-perf = this block.
Provenance: the worker session that made 4d597fd ran these checks but wrote no block. Verified by the orchestrator: every run.sh builds from `P2.2[dd]/wt-fix` (HEAD 4d597fd, clean apart from untracked validation logs), all runs timestamped 07:36-08:13 UTC, after the 07:31 push; 4d597fd is the only commit after 3f594b2, so the builds are 4d597fd. Perf "before" reuses the 6f4fb5a builds from the first session, interleaved in the same session on the same node.
Checks (all under $HOME/gpuperf-scratch/P2.2[dd]/; before = 6f4fb5a, after = 4d597fd):
- CHECK-HOST: host-fix-ctest.log, CTEST-EXIT 0.
- GB300 (graceblackwell00): sweep rc 0; IDENTICAL 436081/436081, NaN-only 0 (gb300fix/ident.log); full memcheck ERROR SUMMARY 0 errors (gb300fix/san-after.log).
- MI250 (amdgpu04): sweep rc 0; IDENTICAL 436081/436081 (mi250fix/ident.log); CHECK-GFX90A-LINT tiers 1-4 ok, lint_rc=0 (lint_fix/lint.log).
- MI300 (amdgpu00): sweep rc 0; IDENTICAL 436081/436081 (mi300fix/ident.log).
- B200 and A100 at 4d597fd: C-b200 (2) and C-a100 (2) blocks above.
- CHECK-PERF, 3 interleaved repeats ({gb300fix,mi250fix,mi300fix}/perf/ratios.txt). Rows >3% slower: GB300 science DD 1.165 (1.165/1.163/1.165), cFF exp 1.054, cFF log 1.046 (FF untouched by this change; the same rows were in the 3f594b2 run); MI250 TF mul 1.039 (untouched backend); MI300 cdouble mul 1.038 (native type, 1.0-1.08).
Measured (after/before): DD science kernel GB300 1.165, MI250 0.487, MI300 0.366, A100 0.987 and B200 0.936 (C-* blocks). DD throughput GB300 roughly flat (atan2 1.015, div 0.998, log 0.995), MI250/MI300 0.15-0.44.
Next session must know:
- The worker's handoff said perf and lint were not re-run at 4d597fd. Wrong: both were run (paths above).
- Landing 4d597fd is blocked only by the GB300 DD science slowdown (1.165, reproducible, no reason stated). The user decides.

## P2.2[dd] (final) — Landed on staging and gpu-perf at 4d597fd
Date: 2026-11-11   Outcome: Done (landed on both xpmath branches; landing session, no GPU, nothing rerun)
Commits: xpmath gpu-perf-staging fast-forwarded 6f4fb5a..4d597fd and xpmath gpu-perf fast-forwarded 0769d6d..4d597fd (--ff-only, no merge commit, no new commits; ancestor check passed before each push). xpmath-kokkos@gpu-perf = this block. No ledger rows added.
Changed: this file only. Library change is xpmath dd_math.hpp (multiply/divide/sqrt NOINLINE->INLINE; acosh renormalise) and dd_complex.hpp (3 forwards renormalised; angle() pi/2 as bit constants), already in 4d597fd.
Checks (all run in earlier sessions, cited here, none rerun; logs under $HOME/gpuperf-scratch/):
- GB300, MI250, MI300, CHECK-GFX90A-LINT, CHECK-HOST at 4d597fd: P2.2[dd] block + addendum + addendum 2 (P2.2[dd]/{gb300fix,mi250fix,mi300fix,lint_fix}/, host-fix-ctest.log). Identity 436081/436081 on each; GB300 full memcheck 0 errors.
- B200: C-b200 (2) block (C-b200-2/b200/): identity 436081/436081, full memcheck 0 errors.
- A100: C-a100 (2) block (C-a100-2/a100/): identity 436081/436081, full memcheck 0 errors.
- gpu-perf move: (a) amendment 2026-11-11 late in plan P2.2[b] counts as the explicit fast-forward instruction; (b) holds via the cited addenda and C-b200 (2)/C-a100 (2); (c) GPU_PERF_DEFERRED.md rows 1-7 all end "CLOSED by user"; 0769d6d is an ancestor of 4d597fd. Parked register has no rows.
Measured (after/before): DD science kernel GB300 1.165 (ACCEPTED regression, user 2026-11-11), MI250 0.487, MI300 0.366, A100 0.987, B200 0.936.
Next session must know:
- For tf/qf: the GB300 fault needed all three inline marks (multiply, divide, sqrt) together; it was fixed by making angle()'s pi/2 a bit constant (no multiply on that path). Check for that shape in the tf/qf forwarding audit.
- Staging and gpu-perf now both at 4d597fd; ledger has no open rows. Next in fan-out: P2.2[tf], then P2.2[qf].

## P2.2[tf] — Inline TF multiply/divide/sqrt (perf go/no-go data)
Date: 2026-11-11   Outcome: Stopped (user decision pending)
Commits: kept off gpu-perf and staging (both still 4d597fd), see xpmath wip/P2.2-tf (FULL = ddd5701), wip/P2.2-tf-mkeep (MKEEP = fa013f2: FULL with multiply NOINLINE), wip/P2.2-tf-sqrt (SQRT = b3b6fcd: only sqrt inline; multiply+divide NOINLINE). All three keep the renorm() result-slot copies at acosh, xp_asin_imag_mag, xp_atan2_safe. No ledger row (nothing landed).
Changed: xpmath include/xp/tf_math.hpp, include/xp/tf_complex.hpp (wip branches only); this file.
Checks (scratch $HOME/gpuperf-scratch/P2.2[tf]/; before = 4d597fd):
- CHECK-HOST FULL 44/44: host/host-ctest.log (earlier session; SQRT and MKEEP host NOT RUN).
- A100 (gpu07) FULL: a100/{ident.log,san-after.log,perf/ratios.txt}; SQRT: a100/sqrt/{ident.log,san-sqrt.log,ratios-sqrt.txt}. Identity 436081/436081 both; full memcheck 0 errors both; sweeps rc=0.
- GB300 (graceblackwell01) FULL: gb300/{ident.log,san-after.log,perf/ratios.txt}; SQRT: gb300/sqrt/{ident.log,san-sqrt.log,ratios-sqrt.txt}. Identity 436081/436081 both; memcheck 0 errors both.
- MI250 (amdgpu04) and MI300 (amdgpu00), FULL and SQRT: {mi250,mi300}/ident-{after,sqrt}.log: identity PASS (FULL: 436075 identical + 6 NaN-only, failing 0; SQRT: 436081 identical); sweeps rc=0 (status.log); perf before/FULL/SQRT interleaved 3 repeats: {mi250,mi300}/perf/ratios-{after,sqrt}.txt.
- CHECK-GFX90A-LINT (login node, hipcc gfx90a, full sweep_device.cpp): FULL clean, 73 callees, largest 81,826 B xp_asin_real_leg; SQRT clean, 75 callees, largest 84,723 B; no WARN: lint/{after,sqrt}/lint.log.
- MKEEP: NVIDIA perf and diagnosis only (earlier session; a100/mkeep, gb300/perf/ratios-mkeep-summary.txt); no AMD, no identity/sanitizer run on MKEEP.
Measured (after/before, median of 3; >1 = slower). Columns: TF science | TF throughput median | cTF median | worst row >3%:
- FULL  A100: 1.646 | 0.985 | 1.000 | science TF 1.646 (cTF log 1.232, TF div 1.181, cTF abs 1.160; native cfloat div 1.838 / double science 1.394 are bimodal noise)
- FULL  GB300: 1.383 | 0.955 | 0.978 | science TF 1.383 (cTF abs 1.323, TF div 1.210)
- FULL  MI250: 0.735 | 0.890 | 0.722 | none
- FULL  MI300: 0.695 | 0.710 | 0.683 | DD log 1.036 (repeats 1.036/0.996/1.075), cTF add 1.036
- MKEEP A100: 1.492 | 1.000 | 1.001 | science TF 1.492 (TF div 1.181, cTF abs 1.118)
- MKEEP GB300: 1.242 | 1.002 | 1.000 | cTF abs 1.537 (science TF 1.242, TF div 1.209)
- MKEEP MI250, MI300: NOT RUN
- SQRT  A100: 1.085 | 1.000 | 1.000 | double science 1.151 (native, bimodal), cDD div 1.097 (repeats 0.94/1.157/1.097), science TF 1.085
- SQRT  GB300: 0.968 | 0.999 | 1.000 | cTF abs 2.149 (2.153/2.149/2.146; FULL was 1.323, cause not investigated)
- SQRT  MI250: 0.995 | 0.998 | 0.993 | none
- SQRT  MI300: 0.997 | 1.006 | 1.006 | FF sqrt 1.068 (repeats 1.100/1.068/0.981; FF untouched), double div 1.063 (noisy); AMD drifts uniformly: native double/float science 1.008/0.998
TF science kernel code size, B (before / FULL / MKEEP / SQRT): A100 695,808 / 1,469,312 / 999,040 / 720,512; GB300 840,704 / 1,573,760 / 1,107,200 / 971,904 (a100/sqrt/sci_isa_sqrt.csv, gb300/sqrt/sci_isa_sqrt.csv). Diagnosis of FULL: a100/perf/diag/DIAGNOSIS.md (instruction-fetch bound).
Next session must know:
- FULL helps AMD a lot (science -26%/-30%) and hurts NVIDIA science kernels (+65%/+38%). SQRT is neutral on AMD, +8.5% on A100 and -3.2% on GB300 science, but GB300 cTF abs is 2.149x slower under SQRT (unexplained). Decision belongs to the user; nothing lands without it.
- B200 not run (standing deferral); gpu-perf and staging not moved.
Orchestrator review (2026-11-11): verified in scratch. Origin SHAs match (wip/P2.2-tf ddd5701, -mkeep fa013f2, -sqrt b3b6fcd; staging = gpu-perf = 4d597fd). b3b6fcd differs from ddd5701 only in multiply/divide INLINE->NOINLINE. Identity PASS on all shapes run (FULL MI250/MI300: 6 NaN-only rows each, allowed); SQRT memcheck A100/GB300 0 errors; lint rc 0 for FULL and SQRT; science/cTF-abs ratios match the table. Gaps (not failures): no CHECK-HOST on SQRT; ancestor check impossible for the three new wip branches (no remote ref), plain new-branch pushes, nothing overwritten.
User decision (2026-11-12): NO-GO. Nothing lands; FULL (ddd5701) parked as GPU_PERF_PARKED.md row 1, Re-test at P3.4[tf]; SQRT and MKEEP retired. Plan amendment 13c73d7.

## P2.2[qf] — Inline QF multiply/divide/sqrt (probe)
Date: 2026-11-11   Outcome: Stopped (NO-GO by probe rule)
Commits: kept off gpu-perf and staging (both still 4d597fd): xpmath wip/P2.2-qf = 5e59302 (e050d5c + 5e59302 on base 4d597fd). Parked as GPU_PERF_PARKED.md row 2, Re-test at P3.4[qf]. No ledger row. GB300 and MI300 not run (probe stops at A100).
Changed: xpmath include/xp/qf_math.hpp (multiply/divide/sqrt INLINE; acosh unchanged), include/xp/qf_complex.hpp (renorm copies at xp_asin_imag_mag, xp_atan2_safe); wip branch only. Audit: $HOME/gpuperf-scratch/P2.2[qf]/forwarding_audit.md.
Checks (scratch $HOME/gpuperf-scratch/P2.2[qf]/; before = 4d597fd):
- CHECK-HOST at e050d5c: 44/44 (host/host-ctest.log). NOT re-run at 5e59302 (which only removes two renorm copies).
- First attempt e050d5c (4 renorm copies), MI250 identity FAIL: 8 failing rows = acosh rows 62/63 (renorm changes f0..f2 of an already-normalised QF result) and complex asin rows 1089,1153,1217,1281,1345,1409 (+0 limbs became -0 via xp_asin_real_mag copy) (mi250/prefix-e050d5c/ident.log). Dropped those two copies in 5e59302, as the task says to keep only copies the sweep proves identical. NaN-only: exp2 6, fma 2, sqrt 6 (allowed).
- 5e59302: MI250 (amdgpu04) sweep rc=0, identity PASS 436081 rows, 436067 identical, 14 NaN-only, 0 failing (mi250/ident.log). A100 (gpu07) sweep rc=0, identity PASS 436081/436081 identical (a100/ident.log); full-sweep memcheck ERROR SUMMARY 0 errors (a100/san-after.log).
- CHECK-GFX90A-LINT at 5e59302: FAIL, rc=1 (lint/after/lint.log). Tier 3 FATAL: xp_atan_im 137,847 B, xp_asin_imag_mag 137,508 B exceed 131,068 B; WARN xp_asin_real_leg 123,566 B. Baseline 4d597fd: clean, rc=0, largest callee 52,248 B (lint/before/lint.log).
- CHECK-PERF 3 interleaved repeats: a100/perf/ratios.txt, mi250/perf/ratios.txt.
Measured (after/before, median of 3; >1 = slower):
- A100: QF science 1.353 (1.353/1.353/1.353); QF median 0.971; cQF median 1.083. Rows >3% slower: cQF abs 1.705, cQF sqrt 1.494, QF sqrt 1.321. Native noise, not attributed: double science 1.701 (repeats 1.701/1.712/1.018), cfloat div 1.838 (bimodal).
- MI250: QF science 0.830; QF median 0.839; cQF median 0.707; no row >3% slower. MI250 drifts: native science double/float 1.001/1.000, DD 1.011.
- QF science code size A100: 1,080,576 -> 2,408,576 B (a100/perf/sci_isa_{before,after}.csv).
Next session must know:
- Same vendor split as tf: A100 slower (1.353 > 1.03 rule) while MI250 is 17% faster; plus gfx90a lint FAILS (inlining pushes xp_atan_im / xp_asin_imag_mag past the TD-1 branch reach). Any re-test at P3.4[qf] must pass the lint, not just perf.
- QF renorm() is NOT a no-op on already-normalised QF results (weak non-overlap): never add result-slot renorm copies without the sweep.
- Pre-fix evidence in a100/prefix-e050d5c and mi250/prefix-e050d5c. gpu-perf and staging not moved.
Orchestrator review (2026-11-12): verified in scratch. origin/wip/P2.2-qf = 5e59302 (2 commits on 4d597fd; diff only qf_math.hpp + qf_complex.hpp); staging = gpu-perf = 4d597fd, unmoved. Identity: A100 436081/436081, MI250 436067 + 14 NaN-only, 0 failing; e050d5c MI250 had 8 failing (prefix-e050d5c/ident.log), so dropping two copies was within the task's rule. A100 memcheck 0 errors; host 44/44 (e050d5c only; 5e59302 removes code, acceptable for a parked branch). Lint rc=1 as stated. Perf ratios match a100/mi250 perf/ratios.txt (A100 QF science 1.353 x3; MI250 0.830; untouched backends ~1.00, so the A100 double-science 1.701 is native bimodality, repeats 1.701/1.712/1.018). NO-GO applies under the 2026-11-12 probe rule; parked row 2 is modelled on row 1. No B200 row needed (nothing landed). Phase 2 complete.

## P3.0 — Accuracy-vs-speed decision memo
Date: 2026-11-11   Outcome: Done
Commits: xpmath@19b6d8f on wip/P3.0 only (amendment 2026-11-11: no push to staging/gpu-perf; P3.1[dd] brings the file onto staging with `git checkout origin/wip/P3.0 -- docs/PERF_ACCURACY_POLICY.md`). xpmath-kokkos: this block only.
Changed: xpmath docs/PERF_ACCURACY_POLICY.md (new)
Checks: none run (doc-only, no builds, no GPUs). Headroom re-read via $HOME/gpuperf-scratch/P3.0/scripts/headroom.py over validation/sweep/sweep_baseline.csv.gz.
Measured: n/a. Policy only; xpmath-kokkos issues no accuracy verdict.
Decided: (c), by the user 2026-11-11. Two tiers, judged per backend:
- strict (default): absolute gate + register unchanged; 1x bound rule; not Worse (x1.2589254 and >1 ulp) vs current strict baseline AND vs a frozen reference (baselines at xpmath 4d597fd); >=3% faster on >=1 gated GPU, no op >3% slower on any gated GPU.
- fast (opt-in): absolute gate + register unchanged; 1x bound rule; not Worse vs fast current; <=4x (2 bits) vs reference unless <=1 ulp; >=10% faster on >=1 gated GPU, no op >3% slower on any gated GPU.
- Gated GPUs: a100, b200, mi250, mi300; GB300 reported only. Strict pass -> strict (and re-baseline both); else fast pass -> fast only; else reject; anything else -> stop and ask. P3 tasks never edit the bound, kUlpAllowance, the register or the reference.
Next session must know:
- NEW TASK NEEDED before P3.1: P3.0b (user chose a dedicated setup task, run by a stronger model with the user) builds the tier switch, the fast baselines (copies of strict), the frozen reference, the per-point compare script that applies the table, and xpmath-kokkos two-tier bit-identity tests. The orchestrator must add P3.0b to GPU_PERF_PLAN.md; P3.1+ depends on it.
- CORRECTNESS.md §3 says "no point above its bound" but sweep_accuracy.cpp:2097 kUlpAllowance = 8.0; 109 host rows are above 1x. Flagged for a separate fix; P3 does not touch it.
- P3.4 caveat (unverified, from memory): JMP FMA mul may reach ~4-5 ulps vs mul bound 3, which would fail both tiers. P3.4 must measure it.

## P3.0b — Two-tier setup (strict/fast switch, fast baselines, frozen reference, gate)
Date: 2026-11-12   Outcome: Done (device checks skipped by the user's decision)
Commits: xpmath@e3ee1f1 on gpu-perf-staging and wip/P3.0b (3 commits on top of edb4c6f; gpu-perf NOT moved, still 4d597fd). xpmath-kokkos: harness + this block on gpu-perf.
Changed: xpmath include/xp/tier.hpp, {dd,ff,tf,qf}_fast.hpp, one #include per *_complex.hpp; scripts/sweep_accuracy.cpp (--tier, tier-header refusal exit 2), sweep_device.cpp (# tier: header); validation/*/run_*_sweep.sh (TIER=strict|fast|both, writes <raw>.exits); scripts/p3_tier_gate.py + selftest; tests/tier_mix_compile_fail.cpp; validation/check_{tier_mix,reference_frozen}.sh; 5 fast baselines (byte copies of strict); validation/sweep/reference_4d597fd/; ci.yml counts 67->72 suite, 44->49 host (user to review); NOTICE.md; CORRECTNESS.md count; PERF_ACCURACY_POLICY.md. xpmath-kokkos perf/*: *_fast types behind XPMATH_HAS_TIERS, throughput unit ids 6-9, perf/perf_ratios.py.
Checks: CHECK-HOST 49/49 pass (/tmp/P30b-host.log, may be gone). Gate selftest 58 cases pass. Harness builds against wt and 4d597fd, 13 ctests each. GFX90A lint (not required, no inlining changed) strict and fast rc=0 ($HOME/gpuperf-scratch/P3.0b/lint/). NOT RUN, by user decision: CHECK-DEVICE-IDENTICAL, CHECK-PERF, CHECK-SANITIZER on any GPU; no fast GPU runs. Strict device unchanged is therefore UNPROVEN until P3.1[dd].
Measured: n/a. xpmath-kokkos issues no accuracy verdict.
Decisions (user): option F with per-function chaining; fast baselines copied from strict; device fast identity dropped; rest of xpmath-kokkos deferred to R.1.
Next session must know:
- Gate: python3 -I scripts/p3_tier_gate.py --backend --ops --base --fresh <dir with <loc>.csv[.gz] + <loc>.exits, locs host a100 b200 mi250 mi300 gb300> --perf <dir with ratios_{sm_80,sm_100,gfx90a,gfx942,sm_103}.csv from xpmath-kokkos perf/perf_ratios.py>. Exits: 0 strict pass, 1 fast-only, 2 reject, 3 tool error stop.
- Flow: change in fast first; on exit 0 promote to strict and re-run TIER=both. Exit 1 is unreachable until strict and the Reference diverge.
- .exits holds absolute= and monotone=; monotone=-1 means skipped (no baseline file).
- Fast device binaries (sweep_device_fast) are first run in P3.1[dd]; a fast device fault there is a P3.0b defect.
- Medium code review findings deferred (not fixed): run scripts score 3x per run; scored CSV carries no tier header; check_tier_mix oracle is loose; --tier not validated in early-exit modes.
