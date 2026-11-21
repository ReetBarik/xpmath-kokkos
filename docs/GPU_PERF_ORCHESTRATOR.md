# GPU perf plan: orchestrator notes

For the session that owns `docs/GPU_PERF_PLAN.md`. It runs no plan tasks
itself. It hands the user prompts for worker sessions, reviews their
reports, rules on questions, and amends the plan. The files in this repo
hold all the state; this page tells you how to use them.

## What to read when you start

1. This file.
2. `docs/GPU_PERF_PLAN.md` §0, §1 (all of it, including §1.9) and the task
   index. Read task sections only when a report concerns them.
3. The last 3–4 blocks of `docs/GPU_PERF_PLAN_STATUS.md` (`grep -n '^## '`
   first; don't read the whole file).
4. `docs/GPU_PERF_DEFERRED.md` (the ledger of GPU checks still owed).
5. `docs/GPU_PERF_PARKED.md` (neutral ideas parked for a later re-test, §1.10).
   When you drop or reorder a task, move any Re-test at entries that name it.

Check where the branches are:
```bash
cd ~/xpmath-kokkos && git checkout gpu-perf && git pull
cd ~/kokkos-extended-precision-demo && git fetch && \
  for b in gpu-perf gpu-perf-staging; do git log --oneline -1 origin/$b; done
```

## Fixed facts and user decisions (don't re-litigate)

- **Repos.** xpmath-kokkos is `~/xpmath-kokkos`, branch `gpu-perf` (plan,
  status, ledger, perf harness). The xpmath clone is
  `~/kokkos-extended-precision-demo`.
  - xpmath `gpu-perf` means checked on every GPU.
  - xpmath `gpu-perf-staging` is where tasks from P1.1 on land.
  - The local branch `scrapped/tier1-792b17d` is off-limits.
- **No `gh`.** Workers push branches. The user merges `gpu-perf` into
  `main` at a release (v0.4.0 after Phase 3). Never merge, tag, push to `main`, or force-push.
- **GPUs.** A100, B200, GB300 (aarch64; build on the node), MI250, MI300.
  The user allocates them with an interactive qsub; workers ssh in.
  Availability varies day to day. A missing GPU is deferred via the ledger
  and settled later by `C-<arch>`.
- **Backend type choice is the user's**, not the library's. The library
  publishes the cost table (P0.7).
- **QCDLoop is out of scope** as a gate. Micro-benchmarks plus the synthetic
  science kernel are the metric.
- **No per-architecture `#ifdef` workarounds.** The user scrapped an earlier
  cross_accum fix for exactly that reason.
- **CHECK-DEVICE-IDENTICAL allows NaN-vs-NaN differences** (decided
  2026-11-09). The comparator is `perf/compare_sweep_limbs.py`.
- **GB300 accuracy gate (P0.5c) is approved.**
- **P0.5b and P0.5c stay as written.**
- **Known accepted exception:** P0.5b fast-forwarded xpmath `gpu-perf` with
  B200 still owed (ledger row 1).
- **Side finding, outside this plan:** QF/TF return NaN instead of ±inf on
  float-range overflow, because the EFT computes inf−inf. It predates this
  plan and xpmath leaves it unscored (states U/N). It's a candidate xpmath
  correctness issue.

## Current state (updated 2026-11-21, P3.2[ff] stage A verified)

Verify these against the files and branches before relying on them.

- **Branches.** xpmath `gpu-perf` = 4d597fd. `gpu-perf-staging` =
  c9e342e (P3.1[dd] FINAL, fast-forwarded from 9219ffa by B4; 9219ffa =
  P3.0c on top of e3ee1f1 = P3.0b: tiers, gate, fast baselines, frozen
  Reference; landed with NO GPU checks, by user decision). xpmath-kokkos `gpu-perf` holds every
  plan/status/ledger commit plus the P3.0b perf-harness fast types.
- **P3.0 done:** policy (c) strict + fast tiers (wip/P3.0 = 19b6d8f).
  **P3.0b done** (not originally in the plan; index row added). Gate rule
  is in the plan as a `Plan:` amendment under P3.1–P3.5 (36137a6), with
  no P3.0b device validation (user: no math changed, assume bit-identical). B200 standing deferral
  extended through Phase 3.
- **P3.1[dd] done: REJECT** (host pre-screen; no GPU). DD log 1-step:
  384 points Worse, worst 3.04 digits; 2-step: 22 Worse, worst 1.23 digits.
  3 Newton steps stay. wip/P3.1-dd = 25f0411 (2-step override). Its
  fresh/host.exits monotone=2 is a misuse of `--score-results` on a host
  CSV, not a result; prompts now say to use the plan's commands verbatim.
  Branch names use `-<b>` (git refs can't contain `[`).
- **Policy observation (raised with user 2026-11-12):** fast "not Worse vs
  fast Current" with fast Current == Reference means a first change gets
  no more accuracy room in fast than in strict; the 4x cap is reachable
  only cumulatively (P3.0b noted exit 1 is unreachable for now).
- **Precision-ladder policy (user, 2026-11-12; don't re-litigate).** The
  backends fill the precision ladder between native types (float 24, FF ~46,
  double 53, TF ~72, QF ~96, DD ~103-106, fp128 113 host). A fast tier is
  its own rung: close to strict, above the next rung, cheaper. Rules in
  ulps (plan P3.0c): strict f <= max(1, r+0.5) and <= max(1, c+0.5); fast
  f <= max(4, r+1) vs Reference only; bound rule for both; speed rules
  unchanged; K = 4 uniform. Dominance column in a P0.7 re-run after
  P3.5[qf], reported only.
- **P3.0c done** (Muse-spark-bf16): staging -> 9219ffa, gate code and
  selftest (66/0, re-run by orchestrator) accepted. Its CHECK-HOST was NOT
  clean (host ctest killed at 47/49; #26, #32 never finished). The P3.1[dd]
  resume runs CHECK-HOST at 9219ffa first. Sanity: DD log 2-step on host is
  exit 1 (fast only), worst point 0.573 -> 3.18 ulps.
- **Prompts must be mechanical (user, 2026-11-12).** The open model
  struggled through P3.0c. Every prompt: literal copy-paste commands (no
  variables carried between steps), an expected-output check after each
  step ("else STOP and paste X"), decision tables, a pre-filled status
  block. Split tasks into stages ending in a hard STOP; the orchestrator
  writes the next stage. Note: xpm_build.sh builds with -j$(nproc); run it
  under `taskset -c 0-7` so nproc = 8.
- **P3.1[dd] resume stage A done (Muse-spark-bf16), reviewed: PASS 2-step,
  fast only** (host pre-screen exit 1; 1-step exit 2). CHECK-HOST at 9219ffa
  clean ($HOME/gpuperf-scratch/P3.1[dd]/r2/checkhost.log, 49/49 + 24/24).
  origin/wip/P3.1-dd-2 = 3ab255a (62c1865 1-step, 3ab255a 2-step; only
  dd_fast.hpp). Fresh host files: $HOME/gpuperf-scratch/P3.1[dd]/r2/fresh/.
- **Stage B1 never ran (checked 2026-11-18):** no worktrees, no
  run-node.sh, no a100/mi250/perf dirs in r2; no branch moved; r2/fresh
  still holds only stage A's host files (Nov 12 03:36). User was on a break.
  **B1 re-issued 2026-11-18 for Sonnet 5.5 / Claude Code** (same content as
  below; adds pre-launch file checks and a 4 h wait limit).
- **P3.1[dd] stage B1 DONE, verified 2026-11-19 (Ran on: Sonnet 5.5 /
  Claude Code).** gpu07 (A100) + amdgpu04 (MI250), released. Both TIER=fast
  sweeps: sweep rc=1 (expected: monotone vs old fast baseline), FATAL 0, no
  last_error, 436080 rows, exits absolute=0 monotone=1 (match sweep.log),
  fresh/{a100,mi250}.{csv,exits} byte-equal to scored_fast. Perf: 4 builds +
  12 runs rc=0. Orchestrator re-ran perf_ratios: byte-identical to
  r2/perf/ratios_{sm_80,gfx90a}.csv. Preview (gate in B3 decides): DD_fast
  log 0.667 A100, 0.668 MI250 (fast needs <= 0.909); cDD_fast log 1.000 /
  1.001; DD_fast science 1.000 / 1.002; no DD_fast/cDD_fast row > 1.03.
  A100 rows > 1.03 are strict cDD add/mul chain 1.14 and native types
  (bimodal; strict code unchanged; the gate's slow check reads only the
  tier's types). Branches unmoved; host.* untouched. Deviations, harmless:
  (1) perf_ratios.py globs, so the `[dd]` in the path breaks it; the worker
  made a 1.3G copy ~/gpuperf-scratch/P3_1_dd_r2_perf_copy (user to rm).
  Fix from B2 on: run perf_ratios through a bracket-free symlink.
  p3_tier_gate.py has no glob. (2) amdgpu04 has no `nproc` on PATH in the
  ssh shell, so node.txt lacks the count and xpm_build.sh ran `-j` (unbounded,
  but pinned to 8 cores by taskset). B2 runner shims nproc. (3) A stray
  `rm -rf /dev/null` in the worker's copy command (no effect).
- **P3.1[dd] stage B2 DONE, verified 2026-11-19 (Ran on: Sonnet 5.5 /
  Claude Code).** graceblackwell01 (GB300) + amdgpu00 (MI300; nproc shim
  used), released. Runner r2/run-node-b2.sh md5 7c25603c... (= orch copy).
  Both status.log START..ALLDONE, 4 builds + 12 runs rc=0, FATAL 0,
  last_error 0, 436080 rows, sweep built sweep_device_fast in wt-after.
  MI300 sweep rc=1, fresh/mi300.exits absolute=0 monotone=1 (= sweep.log),
  fresh/mi300.csv byte-equal to scored_fast. GB300 scored on login:
  score.done 0 = gb300.exits absolute=0, monotone=-1, RESULT: PASS.
  host/a100/mi250 fresh files untouched. Branches unmoved. Orchestrator
  re-ran perf_ratios: byte-identical to r2/perf/ratios_{gfx942,sm_103}.csv.
  Preview: DD_fast log 0.629 MI300, 0.668 GB300; cDD_fast log 1.001 /
  1.000; DD_fast science 0.997 / 0.999. **Problem: MI300 DD_fast div chain
  1.0347 (> 1.03, tier type, gated GPU)**, so the gate's slow check will
  give exit 2 (reject) at B3. Per-pair ratios 1.023 / 1.035 / 1.051
  (median 1.035). Div code is unchanged (3ab255a's only hunk is DD fast
  log in dd_fast.hpp); unrelated unchanged rows drifted too on MI300 (FF
  log 1.043, double log 1.117). This is the B3 table's "STOP, user decides"
  case; raised with the user before issuing B3.
- **User decision 2026-11-19 (MI300 div): pooled re-run, rule fixed in
  advance.** Stage B2b: one MI300 node runs 3 more interleaved pairs
  (before4..after6) with the SAME B2 binaries (r2/perf/mi300/build-*),
  runner r2/run-node-b2b.sh md5 81a7c1116a442b9ddaea371925dff5c4.
  perf_ratios over all 6 pairs -> r2/perf6/ratios_gfx942.csv; r2/perf6
  also gets byte copies of ratios_{sm_80,gfx90a,sm_103}.csv. Original
  r2/perf/ratios_gfx942.csv is kept as evidence. B3's gate uses
  --perf r2/perf6 and its verdict stands, whatever it is. No more
  re-runs.
- **P3.1[dd] stage B2b DONE, verified 2026-11-19 (Ran on: Sonnet 5.5 /
  Claude Code).** amdgpu00 (MI300), released. Runner md5 81a7c111... (=
  orch copy; worker overwrote the orchestrator's leftover copy, same
  bytes). status-b2b.log: one START, 6 runs rc=0, ALLDONE (01:33-01:36Z).
  r2/perf6 = 4 files, 3 copies cmp-equal; orchestrator re-ran the 6-pair
  perf_ratios: byte-identical. B1/B2 evidence untouched; branches unmoved.
  Pooled MI300: DD_fast div 1.029 (now under 1.03), DD_fast log 0.636,
  cDD_fast log 1.000, DD_fast science 1.000. **But DD_fast sqrt 1.0365**
  (B2 alone: 0.990; per pair 0.990 0.875 1.032 1.057 1.041 1.052; sqrt
  code unchanged), so the gate's slow check will very likely give exit 2
  (reject) at B3. The user's verdict-stands rule applies. Policy thought
  (3% check catches MI300 drift on unchanged ops) has now recurred:
  raised with the user for P3.1[ff] onward, not for this verdict.
- **B3 (first version) WITHDRAWN before it ran.** User decision
  2026-11-19 (option A): narrow the 3% slow check to changed ops (real +
  complex) + science; other rows report only. Applied to P3.1[dd] too,
  disclosed in its status block (rule changed after MI300 numbers were
  known). Plan amendment "slow-check scope". Orchestrator could not test
  the patch itself (writes to /vast and /tmp blocked by auto mode).
  Stage **B3a** issued: worker edits scripts/p3_tier_gate.py, selftest
  (66 -> 67 cases) and docs/PERF_ACCURACY_POLICY.md on wip/P3.1-dd-2
  (one commit after 3ab255a), pushes, STOPs. Orchestrator reviews the
  diff, then issues revised B3 (gate run in the main clone at the B3a
  commit, --perf r2/perf6; expected exit 1 under the new rule).
- **B3a DONE, verified 2026-11-19 (Ran on: Sonnet 5.5 / Claude Code).**
  origin/wip/P3.1-dd-2 = 3f517c3 (parent 3ab255a; previous tip 3ab255a,
  no force-push). 3 files, 24+/7-; patch script r2/slowscope.py md5
  b1341d8c... (= orchestrator's); selftest 66 -> 67 cases, 0 failed
  (r2/selftest-b3a.log). staging/gpu-perf unmoved.
- **Revised B3 issued 2026-11-19:** gate in the main clone at 3f517c3,
  --perf r2/perf6; exit 1 -> re-baseline fast + CHECK-HOST + lint;
  exit 2 -> reject status block (discloses the rule change).
- **B3 STOPPED 2026-11-19 (verified; Ran on: Sonnet 5.5 / Claude Code):**
  gate run once, r2/gate.log (01:59Z) = one line, exit 3: "ratio 1.029
  is not after/before (0.0026/0.0026) for DD_fast/div" in
  perf6/ratios_gfx942.csv. Nothing committed/pushed; wip 3f517c3,
  staging 9219ffa, gpu-perf 4d597fd; wt-final absent. Cause = GATE BUG
  since P3.0b (464cdd2): load_ratios requires |ratio - after/before|
  <= 1%, but perf_ratios.py defines ratio = median of per-pair ratios
  and before/after = medians, printed %.4f. Selftest fixtures write
  ratio = a/b exactly, so never caught. Fails on 27 rows of perf6
  gfx942 (4 DD_fast/cDD_fast) and would have failed on B2's 3-pair
  perf/ratios_gfx942 too (cDD_fast/mul). Data is fine. Recommended fix
  (needs user OK, gate change disclosed in status block): B3b patch
  replacing the check with "before, after, ratio finite and > 0",
  selftest case updated, then re-issue B3 unchanged.
- **User 2026-11-19: option 1** (replace the ratio==after/before check
  with finite > 0; disclose in final status block). **B3b+B3 issued** as
  one worker prompt: patch r2/ratiocheck.py md5
  9c37c0e0e5edfe21918b9e19dc1d8eed (8 edits: gate, selftest 67 -> 69
  incl. set_ratio cases, policy Speed line; diff 3 files +38/-9),
  r2/selftest-b3b.log, commit on wip (parent 3f517c3), gate ONCE ->
  r2/gate2.log (r2/gate.log = exit-3 evidence, keep). Exit 1 -> 4 .gz
  (gzip -9 -n), gen_domains no diff, re-baseline commit, wt-final,
  runner r2/run-b3-final.sh md5 6e2be1ef411a4dd499a1e8e6b2a6fcbc
  (CHECK-HOST r2/checkhost-final + lint r2/lint/{strict,fast} from
  scripts/sweep_device.cpp, status r2/status-b3.log). Exit 2/other ->
  STOP. Orchestrator in-memory prediction with the patched check: exit
  1; speed log a100 0.667, mi250 0.668, mi300 0.636, gb300 0.668
  (report only); slower 0; report-only drift 1 (mi300 DD_fast sqrt
  1.036; div 1.029 is under 3%).
- **B3b + B3 DONE, verified 2026-11-19 (Ran on: Sonnet 5.5 / Claude
  Code).** origin/wip/P3.1-dd-2 reflog 3ab255a -> 3f517c3 -> a8c95e1
  (GATEFIX, 3 files +38/-9, diff matches the 8 edits; ratiocheck.py md5
  OK; selftest-b3b.log 69 cases, 0 failed) -> c9e342e (FINAL, "re-baseline
  fast after log", 4 fast .gz only, each zcat == r2/fresh/<loc>.csv; body
  = the extract; DOMAINS/DEVICE_PRECISION/b200 untouched). No force-push.
  r2/gate2.log (02:12Z, 29 lines) == orchestrator prediction exactly:
  exit 1, fast only; speed log a100 0.667, mi250 0.668, mi300 0.636,
  gb300 0.668 (report only); slower 0; report-only 1 (mi300 DD_fast
  sqrt 1.036). r2/gate.log (exit-3) untouched. r2/wt-final = c9e342e.
  run-b3-final.sh md5 OK; status-b3.log START c9e342e..ALLDONE
  (02:14-02:34Z): CHECK-HOST done=0, one PASS line, 49/49 + 24/24, no
  Terminated (log read by orchestrator); lint strict + fast hipcc_rc=0,
  TIER2-4 ok, 252 kernels, lint_rc=0 (1 amdgcn .s each). Staging
  9219ffa, gpu-perf 4d597fd unmoved. B1/B2/B2b evidence timestamps
  intact. 1.3G perf copy already gone. **B4 issued** (A100 + GB300
  sanitizer on wt-final, runner r2/run-b4.sh; then staging -> c9e342e,
  ledger row 8, final status block via md5-checked r2/b4docs.py).
- **B4 DONE, verified 2026-11-19 (Ran on: Sonnet 5.5 / Claude Code).
  P3.1[dd] CLOSED: accepted into the FAST tier only.** Sanitizer clean
  on gpu06 (A100) + graceblackwell01 (GB300), both binaries, 436080
  rows each; san-fast data == B1/B2 fast sweeps. Staging 9219ffa ->
  c9e342e (ff). Worker docs commit eb80ad1 (ledger row 8 = B200, OPEN;
  status block "P3.1[dd] (final)"), reproduced by re-running b4docs.py.
  Cleanup lines given to the user. **Next: P3.1[ff] stage A** (host
  pre-screen, branch wip/P3.1-ff from c9e342e).
- **P3.1[ff] stage A issued 2026-11-19** (user: 1 Newton step only;
  strict FF log already does 2; `Plan:` b0388b9). Worktree
  "$HOME/gpuperf-scratch/P3.1[ff]/wt" on new branch wip/P3.1-ff from
  c9e342e (clone checkout untouched). Patch ffpatch.py md5
  92935c224c597ca392bcb4bfd87e3e71 (input ff_fast.hpp md5 8a36d99d...,
  +23/-1, test-compiled by orchestrator). Runner run-a.sh md5
  0d4c7f3c5113570bfd67e6ac4fa5fa18 (status-a.log, build_1step,
  fresh_1step). Pre-screen `--host-only --backend FF --ops log --base
  c9e342e` -> prescreen_1step.log. Status block via adocs.py md5
  1b2b2bd49c4f2d0b26948795f4c95d48 (16 lines). Base CHECK-HOST reused
  from P3.1[dd] B3 (same commit). Link p31ff-link for stage B.
- **P3.1[ff] stage A verified 2026-11-19: REJECT, P3.1[ff] CLOSED.**
  Candidate xpmath f6c3a4f (parent c9e342e) on wip/P3.1-ff only; staging
  c9e342e and gpu-perf 4d597fd unmoved. Script md5s, status-a.log (one
  START, build rc=0), host.exits absolute=0 monotone=1, 436080 rows,
  prescreen exit 2 all checked from scratch by orchestrator. Real log
  1 step: 488 strict / 72 fast-rule failures, worst 9.0 bits lost vs
  Ref, ulp-bound fails 4 (gate rule f>bound && Ref<=bound; the
  scorer's absolute gate still passed). Complex log unchanged. FF log
  stays at 2 steps. Status block 02dea4b. No GPU used, no ledger row.
  Scratch P3.1[ff] (wt, build_1step) can be removed once wip/P3.1-ff
  is no longer needed. **Next: P3.1[tf]** (plan row: measure whether
  TF log still needs 3 steps; read tf log first).
- **P3.1[tf] stage A issued 2026-11-19** (orchestrator: 2 Newton steps
  only; strict TF log does 3; 1 step not tested, ~48 of ~72 bits and FF
  1 step was rejected). Worktree "$HOME/gpuperf-scratch/P3.1[tf]/wt" on
  new branch wip/P3.1-tf from c9e342e. tfpatch.py md5
  910b8618c51fa208f73e153bb8ad5b9a (input tf_fast.hpp md5
  c80bc2fb46c412b248aed68044bc13a8; prints +20 lines; diff +21/-1;
  INLINE like dd_log_fast_impl; KI-23 branch -54.1f / 88.722839f as
  strict). Test-compiled by orchestrator: g++ 13.3 OK, 6/7 sample
  points bit-identical to strict, 7.25 differs at 1.3e-23 rel.
  run-a.sh md5 b9792df37f1f6f54d4f40c3755503eac (= P3.1[ff] runner with
  [ff]->[tf], 1step->2step). adocs.py md5
  41f4cf7546269ae5d967e73d931dbf5b (16 lines; reject -> Next P3.1[qf]).
  Pre-screen `--host-only --backend TF --ops log --base c9e342e` ->
  prescreen_2step.log. Link p31tf-link for stage B.
- **P3.1[tf] stage A verified 2026-11-19: REJECT, P3.1[tf] CLOSED.**
  (Ran on: Sonnet 5.5 / Claude Code.) Candidate xpmath 92b4580 (parent
  c9e342e, tf_fast.hpp +21/-1) on wip/P3.1-tf only; staging c9e342e and
  gpu-perf 4d597fd unmoved. Script md5s, status-a.log (one START, build
  rc=0), host.exits absolute=0 monotone=1, 436080 rows, nohup-a.out
  empty, prescreen exit 2 all checked from scratch by orchestrator.
  Real log 2 steps: 20 strict / 18 fast-rule failures, worst 24.6 bits
  lost vs Ref (row 1618: 606446 ulps vs Ref 0.024, bound 6.4e9, i.e.
  ill-conditioned inputs near 1); ulp-bound fails 0. Complex log
  unchanged. TF log stays at 3 steps. Status block e003cc8. No GPU, no
  ledger row. Cleanup given to user: remove P3.1[tf]/wt worktree and
  build_2step (logs, scripts, fresh_2step kept).
  Orchestrator probe for QF (sample points, scratch-free, deleted after):
  QF 2 steps vs strict 3 differs by up to 3.2e-23 relative at 1+1e-7
  (~20 bits of a ~2^-96 ulp), and by 1.9e-27 at 3e7; TF 2 vs 3 at
  1+1e-6 differs 5.3e-17 (~18 bits), matching the gate's TF finding.
  So a QF 2-step pre-screen is expected to reject. User asked whether to
  run P3.1[qf] stage A anyway or close it on this evidence.
- **P3.1[qf] CLOSED without a run (user, 2026-11-19):** rejection judged
  certain from the TF gate result + the probe above. `Plan:` amendment and
  an orchestrator-written status block. QF log stays at 3 steps. P3.1 is
  done for all backends. **Next: P3.2[dd]** (exp: native-multiply
  reduction integer, reciprocal-factorial Taylor constants, fixed term
  count). Not yet designed; read dd exp and dd_fast.hpp first.
- **P3.2[dd] stage A issued 2026-11-19** (user: variant `mix` only;
  exp only, `--ops exp`; log chaining revisited after fast exp is
  accepted; `Plan:` amendment "P3.2[dd] variant and scope"). Worktree
  "$HOME/gpuperf-scratch/P3.2[dd]/wt" on new branch wip/P3.2-dd from
  c9e342e. ddexppatch.py md5 f4d31efc62d94be16ef441bb26ef25ae (input
  dd_fast.hpp md5 a8fa36c25177fcf56bf78508d5e43280; prints +46 lines;
  diff +47/-1). Test-compiled by orchestrator (g++ 13.3, -O2
  -ffp-contract=off, -Wall -Wextra: no warnings from dd_fast.hpp):
  patched fast exp bit-identical to the prototype on 1e6 points, equal
  to strict on 86.5%, max 2.96 units of 2^-106 from strict (400k-point
  prototype run); specials (0, +-inf, overflow/underflow edges, ln2/2
  multiples) match strict; NaN -> NaN (strict 0). Host timing proxy:
  mix 0.62x of strict exp. run-a.sh md5
  7bca64c9c220efe10c694298b3be9fd6 (= P3.1[tf] runner with
  [tf]->[dd]/P3.2, 2step->mix). adocs.py md5
  4040d98cc547b398b38968f76a1e9fae (16 lines; reject -> Next
  P3.2[ff]). Pre-screen `--host-only --backend DD --ops exp --base
  c9e342e` -> prescreen_mix.log. Link p32dd-link for stage B.
- **P3.2[dd] stage A STOPPED at step 4 (2026-11-19), correctly.** The
  pasted prompt was hard-wrapped at ~72 columns inside the heredocs, so
  no md5 could match; the worker wrote nothing, committed nothing (only
  the wt worktree at c9e342e and its own /tmp/p32gen/). Orchestrator
  recovered all three scripts byte-exact (md5s above) from the issuing
  session's transcript into ~/xpmath-kokkos/.p32dd-scripts/ (untracked)
  and re-tested the patch on c9e342e dd_fast.hpp: +46 lines, numstat
  47 1. Resume: worker copies + md5-checks the files, then continues.
- **P3.2[dd] stage A (resume) verified 2026-11-19: PASS, fast only
  (pre-screen exit 1). (Ran on: Sonnet 5.5 / Claude Code.)** Candidate
  xpmath 7429482 (parent c9e342e, dd_fast.hpp +47/-1) on wip/P3.2-dd;
  staging c9e342e, gpu-perf 4d597fd unmoved. dd_fast.hpp cmp-equal to
  the orchestrator's test copy; script md5s OK; status-a.log clean;
  436080 rows; pre-screen re-run identical; adocs.py --dry reproduces
  block 07ca432. Real exp: strict-Ref 6, fast-Ref 0, worst 6.0 bits
  lost vs Ref; complex exp unchanged. Strict-rule count 28 = 6 exp + 22
  log (P3.1[dd] fast log). Review note appended to the block. Lesson:
  host.csv has 12 comment lines + header (436093 lines); prompts should
  expect 436093 or count data rows. **Next: P3.2[dd] B1** (A100 + MI250,
  TIER=fast sweep + perf; before = c9e342e, after = 7429482).
- **P3.2[dd] B1 issued 2026-11-19.** Scripts as files in untracked
  ~/xpmath-kokkos/.p32dd-b1/, worker cp's them into
  "$HOME/gpuperf-scratch/P3.2[dd]/" and md5-checks: run-b1.sh
  18ceaedb1ca9abd9de7c6d770ef9e77c (= P3.1 B1 runner, paths changed,
  nproc shim, uname -m; sweep in wt, perf before wt-before = c9e342e
  detached / after wt = 7429482), launch-b1.sh
  3d566b2ffbd99faf53459b807231aa25 (setsid nohup taskset 0-7; refuses
  if status.log exists), check-b1.sh fa8a011f03618075d30fe7f7bc6dc8be
  (read-only evidence dump), ratios-b1.sh
  59893d229f8336b63f26efd1771d3656 (perf_ratios via bracket-free link
  ~/gpuperf-scratch/p32dd-link -> P3.2[dd]; refuses to overwrite).
  check/ratios dry-tested on P3.1[dd] r2 data: ratios byte-identical
  to r2/perf/ratios_{sm_80,gfx90a}.csv. No commits in B1.
- **P3.2[dd] B1 DONE, verified 2026-11-19 (Ran on: Sonnet 5.5 /
  Claude Code).** gpu06 (A100) + amdgpu04 (MI250; nproc shim used),
  released. Script md5s OK in scratch; wt-before c9e342e, wt 7429482;
  p32dd-link resolves. Both status.log START..ALLDONE: sweep rc=1
  (monotone vs old fast baseline, expected), copy rc=0, 4 builds + 12
  runs rc=0; node.txt hostname, x86_64, 8; FATAL/last_error 0; 436080
  rows; fresh/{a100,mi250}.exits absolute=0 monotone=1 (= sweep.log),
  fresh csv cmp-equal to scored_fast. host.* and fresh_mix/ untouched
  (17:21). Branches unmoved (gpu-perf 4d597fd, staging c9e342e,
  wip/P3.2-dd 7429482); no worker commit. Orchestrator re-ran
  perf_ratios (via ~/xpmath-kokkos/.p32dd-rat): byte-identical to
  perf/ratios_{sm_80,gfx90a}.csv. Preview (B3 gate decides): DD_fast
  exp 0.431 A100, 0.368 MI250 (fast needs <= 0.909); cDD_fast exp
  1.000 / 1.001; DD_fast log 1.001 / 1.000; DD_fast science 1.000 /
  0.993. No row > 1.03 on either GPU (A100 strict cDD chain bimodality
  did not show this time). **Next: B2** (GB300 + MI300).
- **P3.2[dd] B2 issued 2026-11-19.** Scripts in untracked
  ~/xpmath-kokkos/.p32dd-b2/, worker cp's into P3.2[dd]/ and
  md5-checks: run-b2.sh 3b72689584f5bb3cc53f212f1c3eef1c (= P3.1 r2
  run-node-b2.sh with paths changed; diff is cosmetic + wt/wt-before),
  launch-b2.sh 8bf3e4e50755a4c4536e2ea5519ba940, score-b2.sh
  175e0239f1cec4807011fb04160599ed (GB300 raw scored on login with
  build_mix scorer; writes fresh/gb300.{csv,exits}, gb300/score.done),
  check-b2.sh 866652233cce6baa886146206865838b, ratios-b2.sh
  cfa1663eccead29412645119e4618b0f. check/ratios dry-tested on P3.1
  r2 gb300/mi300 data: ratios byte-identical. score-b2.sh untested
  (writes /vast). No commits in B2.
- **P3.2[dd] B2 DONE, verified 2026-11-20 (Ran on: Sonnet 5.5 /
  Claude Code).** graceblackwell01 (GB300) + amdgpu01 (MI300; nproc
  shim used), released. Script md5s OK in scratch. Both status.log
  START..ALLDONE; gb300 sweep rc=0 (copy skipped), mi300 sweep rc=1
  (expected), copy rc=0; 4 builds + 12 runs rc=0; node.txt aarch64 8 /
  x86_64 8; FATAL/last_error/isolating 0; 436080 rows; mi300.exits
  absolute=0 monotone=1 (= sweep.log), CMP_OK. GB300 score.done 0,
  gb300.exits absolute=0 monotone=-1, RESULT: PASS. host/a100/mi250
  fresh files untouched (Nov 19). Branches unmoved. Orchestrator re-ran
  perf_ratios (.p32dd-rat2): byte-identical. after/before: DD_fast exp
  0.432 GB300, 0.394 MI300; cDD_fast exp 1.000 / 1.014; DD_fast
  science 1.000 / 1.002. MI300 > 1.03: DD_fast sqrt 1.073, FF sqrt
  1.037 (not gated). Orchestrator ran the gate read-only
  (.p32dd-rat2/gate-pred.log): exit 1, fast only; speed a100 0.431,
  mi250 0.367, mi300 0.394, gb300 0.432; strict-rule rows 112 = (6 exp
  + 22 log) x 4 locations. **B3 issued**: scripts in untracked
  ~/xpmath-kokkos/.p32dd-b3/: gate-b3.sh 52c944d4..., rebase-b3.sh
  35b251da..., run-b3-final.sh c595b733... (= P3.1 r2 copy, paths),
  launch-b3.sh 3c00034d..., check-b3.sh 2df72f2e....
- **User 2026-11-20: one allocation per GPU** ("get a GPU, max use,
  release; don't re-allocate it later"). Plan amendment: from P3.2[ff]
  on, pairs are MI250+MI300 (sweep, perf), then A100+GB300 (sweep,
  perf, sanitizer both binaries on the candidate commit), then login.
  Keep it simple; the user rejected batching/option questions as
  over-engineering.
- **P3.2[dd] B3 DONE, verified 2026-11-20 (Ran on: Sonnet 5.5 /
  Claude Code).** Script md5s OK in scratch. gate.log cmp-equal to
  .p32dd-rat2/gate-pred.log, gate.exit 1 (fast only). origin/
  wip/P3.2-dd 7429482 -> bbaa8fe (FINAL, reflog one push, ancestor):
  4 fast .gz only, each zcat == fresh/<loc>.csv, message ==
  rebaseline-msg.txt. wt-final = bbaa8fe; status-b3.log START
  bbaa8fe..ALLDONE (17:40-17:59Z); CHECK-HOST rc=0, 49/49 + 24/24,
  RESULT PASS, Terminated 0; lint strict + fast hipcc_rc=0, 252
  kernels, lint_rc=0. fresh/ and perf/ timestamps unchanged. gpu-perf
  4d597fd, staging c9e342e unmoved. **B4 issued** (A100 + GB300
  sanitizer on wt-final; scripts in untracked .p32dd-b4/): run-b4.sh
  eca630dd... (= P3.1 r2 copy, path only), launch-b4.sh ad462676...,
  check-b4.sh 5023866a... (dry-tested on P3.1 r2 data), ff-b4.sh
  616a18de... (staging c9e342e -> bbaa8fe), b4docs.py a109bf61...
  (ledger row 9 + "P3.2[dd] (final)" block; --dry OK, 64 lines).
- **P3.2[dd] B4 DONE, verified 2026-11-20. P3.2[dd] CLOSED:
  accepted into the FAST tier only.** Sanitizer clean on gpu07 (A100)
  + graceblackwell01 (GB300), both binaries, 436080 rows each;
  san-fast data == B1/B2 fast sweeps. Staging c9e342e -> bbaa8fe
  (ff). Docs commit ff49e71 (ledger row 9 = B200, OPEN; block
  "P3.2[dd] (final)"), reproduced by re-running b4docs.py. Cleanup
  lines given to the user. **Next:** user decides open question
  "chain fast exp into fast log"; then P3.2[ff] stage A (new GPU
  order; stronger orchestrator model for the design).
- **Cleanup 2026-11-20:** user ran a throwaway script (worktrees
  P3.0/P3.0b/P3.1[dd]r2/P3.1[tf]/P3.2[dd], build trees, links,
  .p32dd-*): scratch 15G -> 12G. Untracked sweep logs of the two
  dirty worktrees saved to `P3.1[dd]/r2/wt-after-logs/` and
  `P3.2[dd]/wt-logs/`. On /vast `git worktree remove` can leave files
  ("Directory not empty"): follow it with rm -rf, then prune.
- **P3.2[dd]-chain designed + stage A/B1 issued 2026-11-20 (Opus
  orchestrator).** User: chains 1+2 as one task (plan amendment).
  Candidate in dd_fast.hpp only, branch wip/P3.2-dd-chain from
  bbaa8fe; commit 1 = chain1 (fast log -> fast exp, +3/-2), commit 2
  = full (+15/-2: fast complex exp / log overrides; log NOINLINE).
  Scripts in untracked ~/xpmath-kokkos/.p32ddc/: ddchainpatch.py
  7ee5e0ae... (chain1 in af8779ac -> out 54a5cc6b; full out
  1b0464c2), run-a.sh 47761981..., run-gpu.sh dd3a2139...,
  launch-gpu.sh 9fb40162..., score-gb300.sh 4d8ee1db...,
  check-gpu.sh 7cbc1ea0..., ratios.sh 89cb65a2... (dry-tested on
  P3.2[dd] a100/gb300 data: byte-identical ratios). Orchestrator host
  test (g++ 13.3, -O2 -ffp-contract=off, 200k points, test dir
  .p32ddc-test): real log fast/strict host time 0.669 -> 0.443;
  full vs strict: cplx log max 13.8 units 2^-106 rel (74% identical),
  cplx exp max 4.97 (82% identical); no new warnings. Order this
  task: A100 (gpu07) + GB300 (graceblackwell01) first (user held
  them): host pre-screen, then sweep + perf + sanitizer on commit 2.
  Then MI250 + MI300 (B2), then login (B3: gate, re-baseline,
  CHECK-HOST, lint, landing). Scratch "P3.2[dd]-chain", link
  p32ddc-link.
- **P3.2[dd]-chain A + B1 DONE, verified 2026-11-20 (Ran on:
  Sonnet 5.5 / Claude Code).** origin wip/P3.2-dd-chain = 1fce03c
  (new, = candidate.sha; parent b72fcab = chain1; numstat 3/2 +
  15/2); dd_fast.hpp md5 at b72fcab/1fce03c = 54a5cc6b/1b0464c2.
  gpu-perf 4d597fd, staging bbaa8fe unmoved. Script md5s OK in
  scratch. Pre-screen full + chain1 both exit 1 (strict failed, fast
  passed; 0 fast-rule failures; strict rows 53 / 32). gpu07 (A100) +
  graceblackwell01 (GB300) ALLDONE, released: A100 absolute=0,
  monotone=1, CMP_OK; GB300 absolute=0, PASS; sanitizer both
  binaries 436080 rows, 0 errors each; FAST_DATA_EQUAL both. Ratios
  re-run into .p32ddc-rat/, cmp-equal. after/before sm_80 | sm_103:
  DD log 0.485 | 0.481, DD exp 1.000 | 1.000, cDD exp 0.999 | 0.858,
  cDD log 0.999 | 0.795, DD science 0.985 | 0.834; 0 rows > 1.03.
  A100 complex flat (real log is 0.24 of 31.9 ns cDD log). **Next:
  B2** on amdgpu04 (MI250) + amdgpu00 (MI300), TIER=fast sweep +
  perf, no sanitizer.
- **P3.2[dd]-chain B2 issued 2026-11-20.** New scripts in
  .p32ddc/ (list in amd.md5): run-amd.sh de4663b9... (from run-gpu.sh
  + P3.2[dd] run-b1/b2 AMD parts; gcc 13.3 lib path both; nproc
  shim), launch-amd.sh 4a68a7a5..., check-amd.sh 2b0abe6c... (dry
  run on P3.2[dd] mi250/mi300 data OK). ratios.sh reused (dry:
  gfx90a/gfx942 ratios byte-equal to P3.2[dd] via a temp link
  .p32ddc-rat/olddd; the bracket path breaks perf_ratios' glob).
- **P3.2[dd]-chain B2 DONE, verified 2026-11-21 (Ran on: Sonnet
  5.5 / Claude Code).**
  amdgpu04 (MI250) + amdgpu00 (MI300), released. Branches unmoved
  (gpu-perf 4d597fd, staging bbaa8fe, wip 1fce03c). Scratch script
  md5s OK vs amd.md5 (e1355e15...). status.log both START 1fce03c ..
  ALLDONE, sweep rc=1, every other rc=0. check-amd.log: absolute=0,
  monotone=1, CMP_OK, 436080 rows both. Ratios re-run into
  .p32ddc-rat/, cmp-equal. after/before gfx90a | gfx942: DD log
  0.424 | 0.465, DD exp 0.999 | 1.010, cDD exp 1.064 | 0.927, cDD
  log 0.970 | 0.914, DD science 0.853 | 0.900. Rows > 1.03: gfx90a 1
  (cDD_fast exp), gfx942 9 (unchanged ops; DD_fast sqrt 1.056).
  **Gate predicted read-only (.p32ddc-test/gate-pred.log): exit 2.**
  0 fast-rule accuracy rows, but (a) MI250 cDD_fast exp 1.064 is a
  changed op > 3% slower, and (b) best exp = 0.927 (MI300 cDD) misses
  the fast 0.9091 rule; GB300 cDD exp 0.858 is report only. (b) alone
  rejects, so a re-run would not change it. Log alone is a clean win
  (best 0.424; cDD log <= 0.970 everywhere). Raised with the user:
  B3 = gate + reject block, then a log-only chain variant.
- **User decision 2026-11-21: land the trimmed variant without GPU
  validation.** Trimmed = chain1 (b72fcab) + the complex-log hunk of
  1fce03c; complex exp stays as in bbaa8fe. Its parts were each
  measured above; the user accepts whatever the combined speed is.
  No GPU sweep/perf/sanitizer and no speed gate on the trimmed
  commit (disclose as a user exception in its status block). Still
  run login-only: host pre-screen (--ops log), CHECK-HOST, gfx90a
  lint strict+fast. 1fce03c is not landed; its predicted exit 2 is
  recorded in the same status block instead of a separate B3 run.
- **P3.2[dd]-chain trimmed (stage L) issued 2026-11-21.** Login
  only. Scripts in untracked .p32ddl/ (l.md5): ddchainlogpatch.py
  (chain1 54a5cc6b -> fcdfe718 = 1fce03c's file minus the cexp
  hunk; local t.cpp: cexp bit-identical to strict, logs = 1fce03c),
  splice.py (device fast CSVs = 1fce03c fresh rows, DD,c,exp rows
  from the bbaa8fe baseline; dry run OK on all 4), prep-l.sh (new
  branch wip/P3.2-dd-chainlog in wt-c1 + commit), run-l.sh (host
  sweep, prescreen --ops log,exp expect exit 1, SPLICE_HOST_EQUAL
  required, 4 gz, gen_domains, re-baseline commit, CHECK-HOST, lint
  strict+fast), launch-l.sh, check-l.sh, push-l.sh (wip push + ff
  staging bbaa8fe -> final). 1fce03c vs bbaa8fe baseline differs
  only in DD r log / c log / c exp rows on host and all 3 GPUs.
  After review: orchestrator writes status block "P3.2[dd]-chain
  (final)" (user exception + predicted 1fce03c exit 2), ledger row
  10 (B200), chains doc row, plan amendment.
- **P3.2[dd]-chain stage L DONE, verified 2026-11-21 (Ran on:
  Sonnet 5.5 / Claude Code). Task CLOSED.** Step 2 stopped on the
  rev-parse prompt bug; resumed as 2b. Candidate 11c139c, final
  ff25642; origin staging bbaa8fe -> ff25642 (ff),
  wip/P3.2-dd-chainlog ff25642, gpu-perf 4d597fd unmoved. All
  status-l lines present incl. SPLICE_HOST_EQUAL; gz cmp-equal x4;
  prescreen exit 1; CHECK-HOST PASS; lint 0 x2. Status block
  "P3.2[dd]-chain (final)", ledger row 10, chains doc, plan
  amendment written. Next: cleanup, then P3.2[ff] stage A on a
  stronger model, base = staging ff25642.
- **P3.2[ff] designed + stage A issued 2026-11-21 (Opus
  orchestrator).** Variant mix8 (plan amendment "P3.2[ff] variant
  and scope"). Scripts in untracked ~/xpmath-kokkos/.p32ff/ (a.md5):
  ffexppatch.py 5e7530c0... (input ff_fast.hpp 8a36d99d, out
  df425676, +45 lines, numstat 46/1), run-a.sh a96acae0... (build,
  host sweep, prescreen --backend FF --ops exp --base ff25642, copy
  to fresh/ on exit 0/1), adocs.py 79f217a9... (18-line block; dry
  OK). Worktree "$HOME/gpuperf-scratch/P3.2[ff]/wt", new branch
  wip/P3.2-ff from ff25642. Base CHECK-HOST reused from stage L
  (checkhost-l.log, ff25642). Test dir .p32ff-test (prototype t.cpp).
  Next after a pass: B1 = MI250 + MI300 (sweep + perf), B2 = A100 +
  GB300 (sweep + perf + sanitizer both binaries on the candidate),
  B3 = login (gate, re-baseline, CHECK-HOST, lint, staging ff, B200
  ledger row 11). Link p32ff-link for perf_ratios.
- **P3.2[ff] stage A verified 2026-11-21: PASS, fast only (exit 1).
  (Ran on: Claude Sonnet 5.5 / Claude Code CLI.)** Candidate xpmath
  5c1511d on wip/P3.2-ff (parent ff25642, ff_fast.hpp 46/1, md5
  df425676). Strict-rule row 1 (FF/r/exp/477, 1.178 vs Ref 0.327
  ulps), fast-rule rows 0. Status block b76d7da + review note.
  **Next: B1** (MI250 + MI300, TIER=fast sweep + perf; check also
  confirms all non-FF/r/exp device rows equal the spliced ff25642
  fast baselines).
- **P3.2[ff] B1 issued 2026-11-21.** Scripts in untracked
  ~/xpmath-kokkos/.p32ff-b1/ (b1.md5): run-b1.sh 0d41b3e4... (=
  P3.2[dd]-chain run-amd.sh, paths only), launch-b1.sh b47a15ff...
  (wt = 5c1511d, wt-before = ff25642), splicecheck.py 717e32dc...
  (fresh device CSV vs ff25642 fast baseline; only FF/r/exp may
  differ; dry: 1fce03c fresh -> SPLICE_BAD DD/c/exp 82, baseline
  itself -> OK), check-b1.sh f4925c45..., ratios-b1.sh 59691d50...
  (dry: chain gfx90a/gfx942 ratios byte-identical). No commits.
- **Stage B1 prompt first given 2026-11-12** (A100 + MI250 TIER=fast sweep + perf, no
  commits; node runner "$HOME/gpuperf-scratch/P3.1[dd]/r2/run-node.sh",
  worktrees r2/wt-before = 9219ffa, r2/wt-after = 3ab255a). Gate exit 0 is
  impossible for this candidate (host strict already fails); B4 needs only
  A100 + GB300 (sanitizer is CUDA-only).
- **Next:** P3.1[dd] stage B (GPU flow, prompt in chat), split B1 (A100 +
  MI250 TIER=fast sweep + perf), B2 (GB300 + MI300), B3 (login: gate +
  re-baseline), B4 (sanitizer / TIER=both pairs, final checks, staging
  fast-forward, B200 ledger row). Expected path: gate exit 1 if the fast
  speed rule (>= 10% on one gated GPU, nothing > 3% slower) holds, else
  exit 2. Then P3.1[ff] stage A.
- **Workers (user, 2026-11-12):** open models in Claude Code from now on;
  workflow unchanged (they ssh to GPU nodes themselves). Prompts still
  spell out every rule (no reliance on CLAUDE.md/memory), nohup + log +
  done-marker for long runs, mandatory checklist. Review harder.
- **Ledger.** Rows 1-7 CLOSED. Rows 8-10 OPEN (P3.1[dd], P3.2[dd],
  P3.2[dd]-chain trimmed; b200, §1.9).
- **Parked register:** row 2 = qf FULL (5e59302), Re-test at `P3.4[qf]`.
  Row 1 = tf FULL (xpmath `wip/P2.2-tf` = ddd5701),
  Re-test at `P3.4[tf]`. Parked by explicit user exception to §1.10's
  "neutral" rule (it is not neutral; see below).
- **Phase 2 results.**
  - ff landed 6f4fb5a (accepted FF sqrt +10.7% GB300, cFF div +4.8% A100).
  - dd landed 4d597fd (accepted GB300 DD science 1.165; AMD gained 2-3x).
  - **tf: NO-GO (user, 2026-11-12).** FULL (TF mul/div/sqrt inline + 3
    renorm copies) correct on all GPUs run, but TF science after/before:
    A100 1.646, GB300 1.383, MI250 0.735, MI300 0.695. NVIDIA is
    instruction-fetch bound (science-kernel code ~2x; ncu in
    $HOME/gpuperf-scratch/P2.2[tf]/a100/perf/diag/DIAGNOSIS.md). Variants
    retired: MKEEP (fa013f2, multiply NOINLINE; A100 1.492, GB300 1.242) and
    SQRT (b3b6fcd, sqrt only; A100 1.085, GB300 0.968 but GB300 cTF abs
    2.149 unexplained, AMD ~0.995). All three wip branches are on origin.
    Plan amendment 13c73d7; status block "P2.2[tf] — ... (perf go/no-go
    data)" + orchestrator review + user-decision line. No B200 row.
- **qf: NO-GO by probe rule (2026-11-12).** FULL wip/P2.2-qf = 5e59302:
    A100 QF science 1.353 (code 1,080,576->2,408,576 B), MI250 0.830;
    gfx90a lint FAILS (xp_atan_im 137,847 B > 131,068 B reach). GB300/MI300
    not run. Parked row 2, Re-test at P3.4[qf] (must pass lint too).
    Status block "P2.2[qf] ... (probe)" + orchestrator review. Phase 2 is
    finished.
- **Order to v0.4.0 (user, 2026-11-11):** Phase 2 (done) -> P3.0 ->
  P3.1-P3.5 [dd,ff,tf,qf] -> C-<arch> for open rows (one C-b200 needs a
  B200 window) -> user merges and tags v0.4.0 -> R. Then Phase 4. Reason:
  the user is preparing a paper; settled math first. The tf vendor split
  (AMD wins, NVIDIA icache loss) is a paper-worthy result and P4.0 input.
- **User's GPU limit:** at most 2 nodes allocated at once. Prompts must
  sequence GPUs (ask for the next pair only at release points).
- **Cleanup:** done 2026-11-12 for P2.2[tf] and P2.2[qf] (worktrees and 52
  build trees removed; 4.1G->1009M, 2.1G->528M; evidence kept). Home quota
  is tight; build trees go under scratch.
- **A100 cfloat div bimodal** (native type). Not the library. Note only.
- **Worker tooling from P3.1 on (user, 2026-11-12):** P3.0 runs on Opus in
  Claude Code. P3.1 onward may run in Cursor (Grok, sometimes Sonnet) or in
  Claude Code on Muse or another open model. Prompts must work in any of
  these harnesses: don't count on CLAUDE.md or
  memory being loaded; long GPU runs go under nohup with a log file and a
  done-marker (the agent's terminal can time out); no `!` shell prefix; the
  FINISH checklist is required, and so is quoting evidence paths. Ask the
  user to put tasks that move xpmath `gpu-perf` or settle the ledger (C-*)
  on Sonnet, not Grok (earlier rule: Sonnet or better). Review harder: an
  open model has fast-forwarded gpu-perf and force-pushed before.
- **Model/harness tag:** the user tells the orchestrator which model and
  harness ran each report; record it in the review note ("Ran on: Grok /
  Cursor") so later sessions can see which models needed corrections.
- **P3.0 scope note (given to the user 2026-11-12 to paste into P3.0):**
  Phase 2 finished, base 4d597fd still current; tf/qf parked (bit-identical,
  no accuracy effect); write the policy as a mechanical gate rule for weaker
  models (gate result -> action table, exact thresholds, what to quote,
  re-baseline commit subject format; judgment -> "stop and ask the user");
  push only wip/P3.0 + status block with "Policy decided:" line. If the memo
  lacks a mechanical rule, add one in the `Plan:` amendment and confirm it
  with the user.
- **P3.1-P3.5 "Needs all 5 GPUs"** must be written into prompts as
  sequenced pairs (2-node limit) with B200 deferred via ledger (§1.9).
- **The user does not want prompts stored in files.** Prompts go in chat.
- **P2.1 dropped after dd; P2.3, P2.4 dropped; B200 deferred (§1.9).**
- **Not done yet:** P0.5c (login node; can slot in any time).
- **Chain tracking (user, 2026-11-20: yes).** docs/GPU_PERF_CHAINS.md
  created; update a row when a fast op is accepted or chained.
- **Orchestrator cost (user, 2026-11-20: yes).** Run the
  orchestrator on Sonnet for routine stages (review, runner
  adaptation, next prompt); stronger model for new-candidate design
  (e.g. P3.2[ff] stage A), gate exit 3, policy questions. Lighter
  reviews: branches, md5s, status logs, cited numbers, perf_ratios
  re-run; no re-running pre-screens or docs generators unless
  something looks off.

## Open questions for the user (raise them; don't decide them)

- **Chain timing (2026-11-19).** User asked whether to test exp
  callers with fast exp while A100/MI250 were held. Orchestrator
  recommended finishing fast exp first, chaining later as its own
  task (one change per gate run; stacked log+exp error could fail
  log's fast rule with unclear blame). User then asked for B2, so
  the chain run was not done. Facts: only fast log's two strict exp
  calls (wt dd_fast.hpp:197,200) are a 2-line chain; pow, sinh/cosh,
  tanh, expm1, exp2/exp10 have no perf row; the science kernel
  (perf/science_kernel.hpp) uses complex mul/add/div/sqrt/log and
  real atan2, no exp, so it measures fast exp only after chaining
  into log.
  **Correction (2026-11-20):** complex fast types forward the
  whole complex function to strict, so cDD_fast log and science see
  no fast real op (not fast log, not fast exp). Chaining exp into
  fast log is measured by DD_fast log only. See
  docs/GPU_PERF_CHAINS.md.
  **SETTLED 2026-11-20 (user):** yes, and with chain 2 in the same
  task (a fast op must reach the science kernel). P3.2[dd]-chain.

Settled: 2026-11-11 land 4d597fd; rows 4-7 closed. 2026-11-12 tf NO-GO, qf
probe NO-GO; workers = open models in Claude Code, ssh as before; no P3.0b
GPU validation.

## Lessons from Phase 2 (put them in prompts when relevant)

- **ptxas UR7 defect** (B200 sm_100 and GB300 sm_103): a function that
  returns a callee's result directly ("result-slot forwarding") can have its
  result pointer clobbered when code shape changes, for example when a real
  call (noinline/cold) is added on a path inlined into log1p. The symptom is
  a DD/FF complex acos/asin crash (error 700), or silent wrong rows. The
  workaround shape: the caller renormalises through `eft_quick_two_sum` so it
  owns its result slot (xpmath 6e6a989 for DD).
- **Clearance means the full 252-kernel sweep + identity + full memcheck.**
  An isolated `--only` run has given false clearances.
- **Shared nodes:** builds must not overwhelm shared nodes (user,
  2026-11-11): nice, at most -j8, one build per node at a time. Plan §1
  and the canonical prompt's SHARED NODES section say so. The two prompts
  handed out 2026-11-11 predate this; the user was told to prepend it.
- **Perf noise:** A100 timing is bimodal (use 3 interleaved repeats, not
  best-of-2); MI250/MI300 drift uniformly (compare against native rows);
  GB300 repeat spread is under 3%.
- **Deletions:** the auto-mode classifier blocks the orchestrator's
  `rm`/`worktree remove`, even with the user's approval. Give the user
  `! <command>` lines instead, one short command per line (long lines get
  wrapped on paste and break bash).
- **Worker handoffs can be wrong about what was run.** The P2.2[dd] fix
  worker said perf and lint weren't re-run at 4d597fd; the scratch dirs
  showed both had been. Check scratch before relaying gaps.
- **Inlining can lose on NVIDIA by instruction fetch** (tf, 2026-11-11):
  the science kernel's code roughly doubled and stall_no_instruction rose
  to 86-90% while executed instructions fell. AMD gained 26-30% from the
  same shape. Bigger backends (qf) are at higher risk; probe first.
- **The ancestor check fails for a new branch** (no remote ref). Prompts
  now say: for a new branch, confirm `git ls-remote origin <branch>` is
  empty, then push.
- **Pasted prompts get hard-wrapped at ~72 columns** (P3.2[dd],
  2026-11-19), including inside heredocs, which breaks the md5 check.
  Keep every heredoc line <= 72 columns (check with
  `awk 'length>72'`), or put the scripts in an untracked folder in
  ~/xpmath-kokkos and have the worker `cp` + md5sum them.
- Workers sometimes return a free-form HANDOFF instead of the checklist, or
  stop without writing a status block. Verify the evidence and write a
  Stopped block yourself, as was done for P2.1[ff].

## Reviewing a worker's report

The user pastes the report. Verify cheaply before accepting it:
- **Status block.** Is it on `origin/gpu-perf`, and do its cited SHAs exist
  on origin?
- **Ledger.** Does every skipped GPU have a ledger row?
- **Branches.** Did anything move that shouldn't have? Only P0.5b, P0.5c
  and `C-*` may move xpmath `gpu-perf`.
- **Force-push.** For each pushed branch, is the previous remote tip an
  ancestor of the new one? Check with
  `git reflog refs/remotes/origin/<branch>` plus
  `git merge-base --is-ancestor`.
- **Evidence.** Spot-check under `$HOME/gpuperf-scratch/<task>/`. For
  example, re-run `compare_sweep_limbs.py` on the before/after files.

Problems seen so far:
- An open model fast-forwarded xpmath `gpu-perf` with B200 still owed.
- It amended and force-pushed a status commit.
- It quoted a local SHA that never reached origin.
- B200 was reported as validated, but no evidence existed.

Record review findings as a short "Orchestrator review" note appended to
the task's status block, then commit and push.

## When a worker stops and asks a question

Decide it yourself if the plan's rules settle it. Otherwise ask the user
with a recommendation, verifying the facts in the scratch files first.
Then:
1. amend the plan (commit subject `Plan: ...`);
2. give the user a one-time resume prompt: the worker prompt below with a
   short RESUME NOTE at the top, saying what changed, which existing results
   to reuse, and what remains. P1.1's resume is the model.

## Session order (amended 2026-11-12)

P2.2[qf] probe done (NO-GO, parked at P3.4[qf]; tf parked at P3.4[tf]); `P3.0` on
Opus with the user, in parallel (pushes wip/P3.0 only);
`P3.1[dd..qf]` … `P3.5[dd..qf]` (P3.4[tf] owes parked row 1's A/B, P3.4[qf] row 2's);
C-<arch> for open rows; the user merges and tags **v0.4.0**; `R`. Then
Phase 4 (`P4.0`). The user is preparing a paper, so settled math (Phase 3)
comes before the release. Parked rows are re-tested by the task their
Re-test at names (§1.10). When you drop or reorder a task, move its Re-test
at entries.

Slot in whenever a GPU frees up:
- `C-<arch>` for open ledger rows. B200 rows are batched (§1.9 standing
  deferral); run C-b200 only when the user has a B200 window.
- `P0.5c` (login node).
- A re-run of `P0.7` when a missing GPU's P0.6 lands.

Release v0.4.0 (after Phase 3): the ledger is empty and xpmath `gpu-perf` ==
`gpu-perf-staging`; the user merges and tags; then `R for xpmath tag v0.4.0`.

Start the worker in `~/kokkos-extended-precision-demo` for xpmath tasks and
in `~/xpmath-kokkos` for xpmath-kokkos tasks. Tasks that move `gpu-perf` or
settle the ledger (`C-*`, end of plan) should run on Sonnet or better.

## Worker prompt (canonical; replace `<ID>`)

```
Your task is <ID> from ~/xpmath-kokkos/docs/GPU_PERF_PLAN.md. Do ONLY that task.

== SETUP ==
1. cd ~/xpmath-kokkos && git checkout gpu-perf && git pull
2. Read §0 and §1 of the plan in full (including §1.9). Then read ONLY the
   section for <ID>. Do not read other task sections.
3. In ~/kokkos-extended-precision-demo (the xpmath clone), check out and pull
   the branch §0 step 0 names for <ID>:
   - `gpu-perf` for P0.5b, P0.5c, and C-* tasks
   - `gpu-perf-staging` for every other xpmath task
4. Record the starting commit of both repos: `git rev-parse HEAD`.
5. In docs/GPU_PERF_PLAN_STATUS.md, read ONLY the blocks for the tasks under
   <ID>'s "Depends on". Always read docs/GPU_PERF_DEFERRED.md and
   docs/GPU_PERF_PARKED.md. If a parked row's "Re-test at" names <ID>, you
   owe its A/B (plan §1.10) after your own task; include its GPU needs in
   your one GPU request.

== FORBIDDEN COMMANDS (no exceptions, even to fix your own mistake) ==
- git commit --amend
- git push --force, git push -f, git push --force-with-lease
- git rebase, git reset --hard, git reset <commit>, git cherry-pick onto a
  shared branch
- any push to `main`, any `git merge` into main, any `git tag`
- submitting GPU jobs (qsub, etc.)
If you made a mistake in something already committed, fix it with a NEW
commit. Never rewrite history.

== BEFORE EVERY git push ==
Run: git fetch origin && git merge-base --is-ancestor origin/<branch> HEAD
If that command fails, STOP and tell me. Do not push.

== MOVING xpmath `gpu-perf` ==
You may move xpmath `gpu-perf` ONLY if ALL of these are true:
(a) your task section explicitly says to fast-forward it;
(b) every check your task lists has PASSED on every GPU it lists, with
    nothing deferred;
(c) GPU_PERF_DEFERRED.md has no open rows at or before the target commit.
If any of these is false, do NOT move it. Write in your status block why
not.

== GPUs ==
- Ask me for ALL the GPUs your task needs in ONE message, then wait.
- Use only the hostnames I give you.
- Defer a GPU only if I said it is unavailable AND §1.9's conditions hold.
  Every deferral gets a row in GPU_PERF_DEFERRED.md.
- Anything produced on a GPU node goes under $HOME/gpuperf-scratch/<ID>/,
  never /tmp.

== EVIDENCE ==
- Every PASS you report must cite a log or CSV path under
  $HOME/gpuperf-scratch/<ID>/ that exists. No path, no PASS.
- Never report a GPU as checked unless you ran the check on that GPU in
  this session. A GPU you did not run is DEFERRED (ledger row) or NOT RUN.
- Quote numbers from command output; do not estimate or round to "about".

== SHARED NODES ==
The login node and GPU nodes are shared with other users; don't overwhelm
them. Build and test under `nice -n 10` with at most -j8 (never bare -j or
-j$(nproc)). Build one tree at a time per node, never before/after/variant
builds in parallel. Run one perf pipeline per node at a time.

== WHEN TO STOP AND ASK ME ==
A check fails twice. The plan doesn't match the code. A rule above would be
broken. You are unsure whether something is allowed. Stopping is always
acceptable; breaking a rule is not.

== FINISH ==
1. Append your status block (§1.8) and push.
2. After pushing, get SHAs with `git rev-parse origin/<branch>` for each
   branch you pushed. Quote those SHAs, never local ones from before a push.
3. Reply with this checklist, answering each line YES or NO:
   - Did only <ID>'s work:
   - Never amended, force-pushed, rebased, or reset:
   - Ran the ancestor check before every push:
   - Every listed check passed, or was deferred with a ledger row:
   - Parked rows naming <ID>: none / A/B done (verdict per row):
   - Moved xpmath gpu-perf: YES/NO (if YES, confirm (a), (b), (c) all held):
   - Status block pushed; SHAs quoted from origin:
   - GPUs I can release now: <hostnames or "none">
   Then give a 3-line summary.
```

## How the user likes answers

**Less ceremony (user, 2026-11-19):** "You are spending way too many
tokens on ceremony. Just give me the plain facts." Short answers, no
recaps of what was verified unless asked, no long option menus.
Concise, plain English. Tables with a copy-pasteable session-ID column.
Worker prompts always go in the chat reply, fully expanded from the
canonical template (RESUME NOTE on top), ready to paste into a Sonnet
session; never store them in files (user, 2026-11-11).
Ask clarifying questions when a request is ambiguous. Verify claims against
files before relaying them.
