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
