# GPU performance plan for xpmath through Kokkos

Goal: make the xpmath arithmetic and functions faster on GPUs when they are
called inside Kokkos kernels, without breaking the correctness machinery
that already exists.

Each task below is sized for **one Claude session**. A session does exactly
one task, records the result, and stops.

---

## 0. How a session uses this document

0. Before anything else, run `git checkout <branch> && git pull` in
   **both** repos. In `~/xpmath-kokkos` the branch is `gpu-perf`. In
   `~/kokkos-extended-precision-demo` it is `gpu-perf` for P0.5b, P0.5c and
   C-* tasks, and `gpu-perf-staging` for every other xpmath task from P1.1
   on (§1.9). All
   tasks commit onto that one shared branch (§1.2 rule 2), so it holds all
   earlier work and all earlier status blocks. Then record the starting
   commit of each repo (`git rev-parse HEAD`); some checks compare against
   it. If a status block for a task you depend on is missing, stop and tell
   the user.
1. Read **§1 Rules** (all of it), then **only your task's section**. Do not
   read the other tasks or this file's history. Do not read the whole repo.
2. Read `docs/GPU_PERF_PLAN_STATUS.md` and look only at the blocks for the
   tasks listed under your task's **Depends on**. They tell you what earlier
   sessions found.
   Also read `docs/GPU_PERF_PARKED.md` (short). If any open row's
   **Re-test at** names your task, you owe that row's A/B (§1.10) at the end
   of your task, on the GPUs your task already uses. Include those GPUs'
   needs in your one GPU request.
3. Start your session with its working directory set to the repo your task
   changes, so that repo's own `CLAUDE.md` loads:
   - xpmath tasks: `~/kokkos-extended-precision-demo` (this is the xpmath
     clone; the directory name is historical)
   - xpmath-kokkos tasks: `~/xpmath-kokkos`
4. Do the task. When done, or when stopped, append a status block (§1.8)
   and end the session.

### Task index

| ID | Title | Repo | Needs | Depends on |
|---|---|---|---|---|
| P0.1 | Perf harness: kernels + build + ISA metrics on MI250 | xpmath-kokkos | login node | — |
| P0.2 | ISA metrics for sm_80, sm_100, gfx942 | xpmath-kokkos | login node | P0.1 |
| P0.3 | Throughput runner + science-shaped kernel | xpmath-kokkos | 1 GPU (any) | P0.1 |
| P0.4 | GB300 lanes: harness build + bit-identity script | xpmath-kokkos | GB300 | P0.3 |
| P0.5 | GB300 device-sweep lane in xpmath | xpmath | GB300 + login | — |
| P0.5b | Validate P0.5's DD acos fix on the other GPUs; fast-forward xpmath `gpu-perf` | xpmath | A100, B200, MI250, MI300 | P0.5 |
| P0.5c | GB300 as an accuracy gate (user approved 2026-11-03) | xpmath | login (GB300 only if step 1 is not done) | P0.5b |
| P0.6 | Baseline runs, one session per GPU | xpmath-kokkos | that GPU | P0.2, P0.3, P0.4 |
| P0.7 | Per-GPU cost table generator | xpmath-kokkos | login node | P0.6 for every GPU available so far (re-run when a missing one lands) |
| P1.1 | HIP: replace `volatile` EFTs | xpmath (staging) | login + MI250 + MI300 | P0.7, P0.5 |
| P1.2 | Type alignment | xpmath | login + all 5 GPUs | P1.1 |
| P2.1[b] | Outline rare-case paths, backend b (**dd only; dropped for ff/tf/qf**) | xpmath | login + all 5 GPUs | P1.2 (or prior P2.1) |
| P2.2[b] | Inline the hot small functions, backend b (ff first, GB300 probe; see section) | xpmath | login + all 5 GPUs (probe: GB300) | P1.2 |
| ~~P2.3[b]~~ | DROPPED 2026-11-11 (user): complex fast/slow split | — | — | — |
| P2.4 | Expansion arrays (needs user go-ahead) | xpmath | login + all 5 GPUs | P2.2[all] |
| P3.0 | Accuracy-vs-speed decision memo (STOP) | xpmath | login | P2.2[dd] |
| P3.1–P3.5[b] | Cheaper algorithms, one per task | xpmath | login + all 5 GPUs | P3.0 |
| P4.0 | Generic-core design (STOP) | xpmath | login | R (v0.4.0 released) |
| C-&lt;arch&gt; | Catch-up for a deferred GPU (e.g. C-a100) | xpmath | that GPU | open rows in the deferral ledger; for C-a100 also P0.5b |
| R | Release v0.4.0, re-vendor, re-measure (after Phase 3, before Phase 4) | xpmath-kokkos | all 5 GPUs | Phase 3 done, ledger empty, xpmath `gpu-perf` == `gpu-perf-staging`, user merged and tagged |

Backend order for every `[b]` task: **dd → ff → tf → qf**. Each backend is a
separate session.

"All 5 GPUs" in the Needs column means: all five, **or** the ones available
plus deferral rows for the rest, under the rules of §1.9.

---

## 1. Rules (every session reads all of this)

### 1.1 The two repositories and who owns what

- **xpmath** (`github.com/ReetBarik/xpmath`, local clone
  `~/kokkos-extended-precision-demo`) owns
  the numerics and every accuracy verdict (MPFR/MPC oracle, derived bounds,
  `sweep_*_gate` targets, device baselines). See its `docs/CORRECTNESS.md`.
  All library changes in this plan are made there.
- **xpmath-kokkos** (`~/xpmath-kokkos`) owns two things: bit-identity of the
  Kokkos wrapper, and (from this plan on) **performance measurement through
  Kokkos**. It never issues an accuracy verdict. No MPFR, MPC or
  `__float128` outside `vendor/`. Never hand-edit `vendor/`; the only way
  to change it is `scripts/sync_upstream.sh <tag>` (task R).

The xpmath clone is `~/kokkos-extended-precision-demo`. Use it for xpmath
tasks and as `XPMATH_KOKKOS_PERF_XP_DIR`, checked out on the branch §0
step 0 names.
**Do not use `~/xpmath_perf_audit/upstream`, and never check out the local
branch `scrapped/tier1-792b17d`.** Both hold the scrapped attempt (§1.6).

### 1.2 Hard rules

1. **Host reasoning is never enough.** An argument that a change "must be
   bit-identical by algebra" is not validation. A previous attempt passed
   every host test and still silently corrupted results on a GPU through a
   compiler bug. Every library change is checked on the GPUs its task lists.
2. **One shared branch per repo: `gpu-perf`.** Every task commits onto
   `gpu-perf` (in xpmath-kokkos and/or the xpmath clone) and pushes it with
   a plain `git push`. The user merges into `main` at a release (v0.4.0 after Phase 3).
   **Exception (§1.9):** in the xpmath clone, every task from P1.1 on uses
   `gpu-perf-staging`. Wherever this document says `gpu-perf` for such a
   task's xpmath work, read `gpu-perf-staging`.
   - Prefix every commit subject with the task ID, e.g. `P2.1[dd]: ...`.
   - **Never force-push, rebase, amend, or rewrite commits already on
     `gpu-perf`.** Never merge, never tag, never push to `main`.
   - **Only kept work goes on `gpu-perf`.** Do the work on a local branch
     `wip/<task-id>`. Fast-forward `gpu-perf` to it only when every check
     the task lists has passed and the task says to keep the change. If you
     stop, or the task says not to keep it: push `wip/<task-id>` for
     reference, and put only your status block on `gpu-perf`.
   End commit messages with the attribution line the session's system
   prompt gives.
3. **Do not change what a test means to make it pass.** If a gate fails,
   you stop and report (§1.7). Do not edit baselines, registers, expected
   counts or tolerances unless your task explicitly says to.
4. **Do not add `#ifdef` special cases to dodge a compiler bug.** If a
   change only works on some architectures, stop and report. The user
   prefers no change to a pile of per-arch workarounds.
5. **xpmath-kokkos CI asserts exactly 13 ctest targets.** The perf harness
   must not register ctest targets. It lives under `perf/` behind a CMake
   option that defaults to OFF.

### 1.3 Keeping your context small (you will run out otherwise)

- Never `cat` a large file. The xp headers are 2,000+ lines each. Use
  `grep -n` to find a function, then read only that line range.
- Send every build and test log to a file. Show yourself only the tail or
  the failures:
  `nice -n 10 cmake --build build -j8 > /tmp/<task>-build.log 2>&1; tail -30 /tmp/<task>-build.log`
  `ctest ... > /tmp/<task>-ctest.log 2>&1; grep -E 'tests passed|Failed|\*\*\*' /tmp/<task>-ctest.log`
- **Shared nodes (user, 2026-11-11).** The login node and the GPU nodes are
  shared with other users. Never overwhelm them: build and test under
  `nice -n 10` with at most `-j8` (ctest at most `-j8`); never `-j` without a
  number or `-j$(nproc)`; build one tree at a time per node (no parallel
  before/after/variant builds); never run two perf pipelines on one node
  at once.
- Never print a sweep CSV (436k rows) or a raw device file. Compare them
  with `cmp`/`diff -q`, or count differences with `diff | wc -l`.
- Never print whole assembly files. Use the metric scripts from P0.1/P0.2.
- Long builds (ptxas can take 20–30 minutes): run them in the background
  with output to a log, and check the tail occasionally. Don't stream them.
- Don't re-read files you already have. Don't explore beyond the files
  your task lists unless a step fails.
- Long jobs on a GPU node: start them with
  `ssh -n <host> 'setsid nohup bash -lc "<cmd>" > $HOME/gpuperf-scratch/<task>/<name>.log 2>&1 &'`
  and poll the log. A plain `ssh ... &` inside a background shell makes the
  task look finished immediately.
- When you write a wait loop with `pgrep -f`, match a string that is not in
  the loop's own command line, or the loop will match itself and never end.
- Inside `ssh <host> 'bash -lc "..."'`, a `$var` is expanded by the outer
  shell. Escape it (`\$var`) when it must be expanded on the node.

### 1.4 Getting a GPU

The user allocates GPU nodes with an interactive `qsub`. When your task
needs a GPU:

1. Ask for **all** the GPUs your task lists in one message: *"This task
   needs <list>. Please give me hostnames for the ones you can allocate
   now, and tell me which are unavailable."* Wait for the answer. Never
   submit jobs yourself. Availability changes day to day, and any GPU can
   be the scarce one. Proceed with what you get, and defer the rest under
   §1.9. The minimum to land a library change is CHECK-HOST plus at least
   one GPU per vendor the change can affect (one NVIDIA and one AMD in
   general; one AMD for a HIP-only change like P1.1).
2. Run commands with `ssh <host> 'bash -lc "<commands>"'`. **Only `$HOME` is
   shared** with the login node; a node's `/tmp` is local to that node.
   Anything produced on a node that you need later (raw sweep files, logs,
   sanitizer output) goes under `$HOME/gpuperf-scratch/<task>/`.
3. Allocations can expire. Keep every step restartable: write outputs to
   files under the repo or `$HOME`, not only to the terminal.
4. Release the node when you're done by telling the user you've finished
   with it.

| GPU | Arch | Where to compile | Modules | Kokkos install |
|---|---|---|---|---|
| A100 | sm_80 | login node or node | gcc/13.3.0 cmake/3.28.3 cuda/12.9.1 | `$HOME/kokkos-install-cuda-sm80-quadmath` |
| B200 | sm_100 | login node or node | gcc/13.3.0 cmake/3.28.3 cuda/12.9.1 | `$HOME/kokkos-install-cuda-sm100` |
| GB300 | sm_103, **aarch64** | **on the node only** | gcc/11.2.0 cmake/3.22.1 cuda/13.3.1 | `$HOME/qcdl_gb300/kokkos/install/kokkos-5.1.0-gb300/Release` |
| MI250 | gfx90a | login node or node | gcc/13.3.0 cmake/3.28.3 rocm/7.0.2 | `$HOME/xpm_device/kokkos-hip-gfx90a` |
| MI300 | gfx942 | login node or node | gcc/13.3.0 cmake/3.28.3 rocm/7.0.2 | `$HOME/xpm_device/kokkos-hip-gfx942` |
| host | x86_64 Serial | login node | gcc/13.3.0 cmake/3.28.3 | `$HOME/kokkos-install-quadmath` |

Always `module use /soft/modulefiles` first, load gcc first, and on x86 also
`export LD_LIBRARY_PATH=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/lib64:$LD_LIBRARY_PATH`.
GB300 has no `__float128`, so xpmath's host scoring cannot run there. Produce
raw device output on GB300 and score it on the login node.

### 1.5 The standard checks (tasks refer to these by name)

**CHECK-HOST** (xpmath, login node). Full host suite, including both
accuracy gates:
```bash
cd ~/kokkos-extended-precision-demo
scripts/xpm_build.sh --arch host --build-dir /tmp/xpm-<task> > /tmp/<task>-build.log 2>&1; tail -5 /tmp/<task>-build.log
ctest --test-dir /tmp/xpm-<task>/host -j8 --timeout 1800 > /tmp/<task>-host.log 2>&1
grep -E 'tests passed|Failed|Not Run' /tmp/<task>-host.log
```
Pass = 100% passed.

**CHECK-DEVICE-IDENTICAL `<arch>`** (xpmath, on that GPU). Proves a
"bit-preserving" change really preserved every bit on that device:
1. At your session's starting commit (§0 step 0; use a separate
   `git worktree` for it), run `validation/<arch>/run_<arch>_sweep.sh` with
   `RAW_OUT=$HOME/gpuperf-scratch/<task>/<arch>-before.csv`. Then run it on
   `wip/<task-id>` with `RAW_OUT=$HOME/gpuperf-scratch/<task>/<arch>-after.csv`.
   Use separate `BUILD_DIR`s.
2. Remove `#` header lines and timing lines from both (read the first 20
   lines of `validation/strip_timing.sh` for its usage), then run
   `python3 -I ~/xpmath-kokkos/perf/compare_sweep_limbs.py <before> <after>`.
3. Pass = exit 0. The rule: every limb must be bit-identical, **except** that
   a limb that is NaN in both runs may differ in sign or payload bits. IEEE
   754 gives a NaN's sign no meaning, and a compiler may swap operands, which
   changes only which NaN propagates. A NaN↔non-NaN change, or any change to
   a non-NaN limb, is a failure. Report the script's summary line and its
   first 3 failing rows, not the whole diff. (Decided by the user
   2026-11-09, after P1.1: all 2,098 NaN-only rows were overflow points in
   QF/TF that the scorer leaves unscored.)

The scorer's exit code inside the script is a separate signal. Record it,
but bit identity is the pass criterion for bit-preserving tasks. GB300 uses
the script added by P0.5.

**CHECK-SANITIZER `<arch>`** (CUDA GPUs only: A100, B200, GB300). Needed by
any task that changes what gets inlined. A known nvcc defect (TD-4,
`~/kokkos-extended-precision-demo/docs/TOOLCHAIN_DEFECTS.md`) silently corrupts the stack frame of
large inlined code. Run
`compute-sanitizer --tool memcheck <build>/device/tests/sweep_device --out $HOME/gpuperf-scratch/<task>/<arch>-san.csv > $HOME/gpuperf-scratch/<task>/<arch>-san.log 2>&1`
and `grep 'ERROR SUMMARY'` on that log. Pass = `0 errors`.

**CHECK-GFX90A-LINT** (login node). Any task that changes inlining on HIP
must run `scripts/xpm_lint_device_asm.sh <dir-with-gfx90a-.s-files>`. It
guards the AMD branch-reach bug (TD-1). Pass = clean exit.

**CHECK-PERF `<arch>`** (xpmath-kokkos harness from P0.1–P0.3). Build the
harness against a local xpmath tree (P0.1 documents how), record ISA
metrics and throughput, and compare with the latest committed baseline in
`perf/results/<arch>/`. Report the per-op change as a ratio. A change that
makes anything slower by more than 3% must say why in the status block.

### 1.6 Known history you must not repeat

- **cross_accum early-return removal was scrapped.** Removing the
  data-dependent `return` in `{dd,ff,qf,tf}_cross_accum` passed host tests
  but silently corrupted results on a GPU through a compiler bug. The only
  workaround was per-arch `#ifdef`s, so it was dropped. Do not retry that
  diff. P2.4 handles this area, and only with the user's go-ahead.
- **Forced `noinline` on CUDA exists because of TD-4.** Several functions
  also carry a `td4_workaround` function-pointer indirection that you won't
  find by blaming the declaration line (QF `divide`, `log`,
  `xp_asin_imag_mag`). Grep for `td4_workaround` before touching any
  function's inlining.
- **The MI250 QCDLoop hang was launch bounds, not code size.** It was fixed
  in QCDLoop. Don't use this plan to chase it.
- **Sweep operands must be platform-independent.** Raw device files are
  scored on the x86 login node, so the operands a producer derives must be
  bit-identical on every host CPU. libm calls are not: `std::hypot` differs
  between x86 and aarch64 glibc 2.38, which made 15 GB300 complex-pow
  operands differ and looked like a 1e18-ulp library defect (P0.5). Derive
  operands only from IEEE-exact operations; `scripts/sweep_inputs.hpp`'s
  `hypot_cr` is the model. If a non-x86 producer fails a gate, first check
  whether its operands match x86's.
- **TD-4 also bites on sm_103 (GB300).** P0.5 hit it in DD complex `acos`
  (error 700, 61 `Invalid __local__ write`). The fix was structural: call
  `xp_asin_imag_mag` directly instead of the whole `asin`, with no `#ifdef`.
  Prefer that kind of fix.

### 1.7 Stop and ask the user when

- a check fails and you can't explain why within two attempts;
- a fix would need an `#ifdef` for specific architectures;
- your task would require editing `vendor/`, a baseline, a register, or a
  test's expectation;
- the task's steps don't match what you find in the code (stale plan);
- a GPU you need isn't available.

When you stop, write the status block anyway (outcome "Stopped") so the
next session doesn't repeat your work.

### 1.8 The status block

Append to `~/xpmath-kokkos/docs/GPU_PERF_PLAN_STATUS.md`, newest last, and
commit it onto `gpu-perf` in xpmath-kokkos and push. This applies even
when the task's code lives in xpmath, and even when you stopped.

```
## <task-id> — <title>
Date: YYYY-MM-DD   Outcome: Done | Stopped | Partial
Commits: <repo>@<sha> for each repo you pushed to gpu-perf  (or: kept off gpu-perf, see wip/<task-id>: <reason>)
Changed: <files, one line each>
Checks: <each check run: name, arch, pass/fail, log path>
Measured: <the key numbers, before → after>
Next session must know: <1–5 bullets>
```

Keep it under 40 lines. It's the only memory the next session gets.

### 1.9 Two xpmath branches, and what to do when a GPU is unavailable

**Branches in the xpmath clone:**
- `gpu-perf`: checked on **every** GPU. Only P0.5b, P0.5c and catch-up
  tasks (C-<arch>) move it.
- `gpu-perf-staging`: every xpmath task from P1.1 on commits here. It
  starts at 0bd8b95, P0.5's acos fix, which was checked on B200, GB300,
  MI250 and MI300. Its A100 check is owed by P0.5b itself.

xpmath-kokkos keeps one branch, `gpu-perf`.

**Keeping staging current:** if xpmath `gpu-perf` has commits that staging
lacks (for example P0.5c's gate), the next xpmath session merges it first.
It runs `git merge origin/gpu-perf` (a normal merge, never a rebase), then
CHECK-HOST, then pushes.

**Deferring a GPU.** A task may land on staging without one of the GPUs it
lists only if all of these hold:
1. the user has said that GPU is unavailable (any GPU, not only A100);
2. at least one other GPU of the **same vendor** passed the same checks in
   this task (NVIDIA: A100, B200, GB300; AMD: MI250, MI300);
3. CHECK-HOST passed (it can never be deferred).

**B200 standing deferral (user, 2026-11-11).** Through the end of Phase 2,
B200 counts as unavailable for every task: condition 1 above holds without
asking. Do not request a B200; defer it with a ledger row (conditions 2 and
3 still apply). B200 rows are settled in batches by `C-b200` when the user
gets a B200 window (planned: end of Phase 2, after Phase 3, before
release). C-b200's step 1 groups consecutive bit-preserving rows into one
run, so one window settles all of them.

Each deferral is a row in `~/xpmath-kokkos/docs/GPU_PERF_DEFERRED.md`:
task, GPU, xpmath `before-SHA..after-SHA`, checks owed, and whether the
task is bit-preserving. Commit it with your status block.

**Before the user merges into `main`:** xpmath `gpu-perf` and
`gpu-perf-staging` point at the same commit, and the ledger has no open
rows.

### 1.10 Parked ideas: when your task must re-test one

Some changes measured neutral when tried, but a later task is expected to
make them worth more (for example, inlining `multiply` once P3.4 has made it
small). They are rows in `docs/GPU_PERF_PARKED.md`. You do not decide whether
a row is relevant. Its **Re-test at** column names the task, and if that is
your task, the A/B is part of your task.

**Parking (the task that measured it):** if a task's own section allows a
PARK verdict, record a row: the idea; the task; where the change lives
(a pushed `wip/` branch and SHA, plus a one-line description so it can be
re-applied by hand if it no longer applies cleanly); whether it is
bit-preserving; the key ratios; and **Re-test at**, a list of task IDs.

**Re-testing (the task named in Re-test at):**
1. Finish your own task first, including its checks. Your result is "A".
2. On your own `wip/<task-id>-parked<N>` branch from A, re-apply the parked
   change (cherry-pick onto your own wip branch, or re-apply by hand).
   That is "B". Never re-apply onto a shared branch.
3. Run on B, on the GPUs your task already uses: CHECK-HOST; the full
   device sweep (exit 0); CHECK-DEVICE-IDENTICAL A vs B if the row is
   bit-preserving (otherwise score it under the P3.0 policy); CHECK-SANITIZER
   on the CUDA GPUs among them if the change affects inlining; perf A vs B
   by ratio, 3 interleaved repeats.
4. Verdict, by the same rule the parking task used: **GO** (at least 3%
   faster on the backend's median or its science-shaped kernel, no
   unexplained slowdown above 3%), **PARK** (within ±3%), **NO-GO**
   (slowdown above 3%, or any check failed).
5. GO: do **not** land it yourself. Push `wip/<task-id>-parked<N>`, write
   the numbers in your status block, and stop for the user. The user decides
   whether it becomes its own task. PARK: update the row (new ratios, keep or
   extend Re-test at). NO-GO: mark the row retired, with the reason.
6. Your own task's result (A) lands or stops exactly as your section says,
   whatever B shows.

If a row's Re-test at names a task that was dropped, the orchestrator moves
it. P4.0 reads every row (open or retired) as design input.

---

## C-&lt;arch&gt; — Catch-up for a deferred GPU (e.g. C-a100)

**Repo** xpmath (ledger and status in xpmath-kokkos) · **Needs** that GPU ·
**Depends on** at least one open ledger row for that GPU; for C-a100, also
P0.5b done

Run it whenever the GPU frees up. For A100, do P0.5b, P0.6-a100 and C-a100
back to back in one allocation (three sessions).

**Steps:**
1. Take this GPU's open rows, oldest first. Group consecutive
   bit-preserving rows into one run. A bit-changing row (Phase 3) is a run
   on its own.
2. For a bit-preserving run: CHECK-DEVICE-IDENTICAL with before = the first
   row's before-SHA and after = the last row's after-SHA. If it differs,
   test row boundaries (each row's after-SHA) to find the first failing
   row, then stop.
3. For a bit-changing row: run the device sweep at its after-SHA and score
   it under the P3.0 policy, the same way the task did on the other GPUs.
4. On CUDA GPUs: CHECK-SANITIZER at the newest after-SHA. If it fails, test
   row boundaries to find the first failing row, then stop.
5. CHECK-PERF at the newest after-SHA. Per-task attribution on this GPU is
   lost; that's accepted.
6. Mark the rows settled in the ledger, with log paths.
7. Let X be the newest staging commit with no open ledger rows for any GPU
   at or before it. Fast-forward xpmath `gpu-perf` to X and push.
   Fast-forward only; X must already contain every commit on `gpu-perf`.

**Stop if:** anything fails. Report the failing row and leave both branches
as they are. A fix goes on staging as a new commit (a fix or a
`git revert`), never by rewriting history, and the user decides which.

---

## Phase 0 — Measure (no library changes)

Why first: none of the later phases can show a win without a baseline, and
the user's question "does FF beat double on GB300?" is answered here.

### P0.1 — Perf harness: kernels, build, ISA metrics on MI250

**Repo** xpmath-kokkos · **Needs** login node · **Depends on** —

**Read first:** `CMakeLists.txt` (top 60 lines), `tests/bit_identity_test.cpp`
(lines 1–60, for the include and type style), `include/Kokkos_xpmath/Kokkos_xpmath.hpp`.

**Build this:**
1. `perf/CMakeLists.txt`, included from the top-level `CMakeLists.txt` only
   when `-DXPMATH_KOKKOS_BUILD_PERF=ON` (default OFF). It adds no ctest
   targets. Option `XPMATH_KOKKOS_PERF_XP_DIR` lets the harness use a local
   xpmath checkout's `include/` in place of `vendor/xpmath`. Later phases
   need this to measure a branch before it's released.
2. `perf/kernels.hpp`: one templated kernel per op, instantiated for six
   types. The same source serves every type:
   - real: `double`, `float`, DD, FF, TF, QF
   - complex: `Kokkos::complex<double>`, `Kokkos::complex<float>`, and the
     four xp complex types
3. Fixed op list (do not grow it in this task):
   - real: `add mul div sqrt exp log sin atan2`
   - complex: `add mul div abs sqrt log exp`
4. Every kernel uses `Kokkos::RangePolicy<Kokkos::LaunchBounds<256,1>>`
   (QCDLoop's validated launch policy). Inputs come from a deterministic
   generator in each op's safe domain (positive for log/sqrt, |x| ≤ 700
   for exp). Results are written to a View so nothing is dead-code
   eliminated.
5. `perf/isa_metrics.sh <build-dir> <arch>`: from the compiled device code,
   write `perf/results/<arch>/isa_<xpmath-tag-or-sha>.csv` with columns
   `type,op,regs,scratch_bytes,code_bytes,instructions,calls`.
   - HIP: build with `-save-temps` and read the `.s` metadata
     (`.vgpr_count`, `.private_segment_fixed_size`, `codeLenInByte`). Count
     `s_swappc_b64` for calls.
   - Do only gfx90a in this task. Keep the script structured so P0.2 can
     add CUDA.
6. `perf/README.md`: how to build, how to run, column meanings, and the
   rule that perf builds never register ctest targets.

**Done when:** the harness builds for gfx90a against the MI250 Kokkos install,
`isa_metrics.sh` writes a CSV with all 6 types × 15 ops where they apply,
`ctest -N` in a normal build still says 13, and the CSV is committed.
Expect DD `mul` to show nonzero `scratch_bytes`; that's the `volatile`
overhead P1.1 removes.

### P0.2 — ISA metrics for sm_80, sm_100, gfx942

**Repo** xpmath-kokkos · **Needs** login node · **Depends on** P0.1

**Read first:** `perf/README.md`, `perf/isa_metrics.sh`.

**Steps:**
1. Add CUDA support to `isa_metrics.sh`. Use `cuobjdump -res-usage` for
   `REG`, `STACK` and `LOCAL`, and `cuobjdump -sass` to count instructions
   and `CALL`.
2. Run it for sm_80, sm_100 and gfx942 using the Kokkos installs in §1.4.
   CUDA builds can take long: run them in the background.
3. Commit the three CSVs.

**Done when:** four arch CSVs exist (with P0.1's gfx90a). GB300 is P0.4.

### P0.3 — Throughput runner and science-shaped kernel

**Repo** xpmath-kokkos · **Needs** one GPU (any; MI250 or A100 preferred) · **Depends on** P0.1

**Read first:** `perf/README.md`, `perf/kernels.hpp`.

**Steps:**
1. `perf/throughput.cpp`, one executable. For each type × op:
   - 2 warm-up runs, then the median of 5 timed runs with `Kokkos::fence()`
     and `Kokkos::Timer`.
   - Report ns per op per element and the ratio to the same op in native
     `double` (and `float`).
   - Arithmetic ops run a dependent chain of 64 operations per element
     with operands held in registers, chosen so values stay bounded (e.g.
     multiply by a value near 1). Functions are evaluated on 4 inputs per
     element.
   - Write `perf/results/<arch>/throughput_<xpmath-tag-or-sha>.csv`.
2. The **science-shaped kernel** (`perf/science_kernel.hpp`) is a synthetic
   stand-in for a loop-integral leaf. It does **not** depend on QCDLoop.
   - Per element, it keeps at least 24 complex values live at once and
     runs a fixed chain of about 200 operations: complex mul/add/div,
     complex `log` and `sqrt`, real `atan2`, and a few data-dependent
     branches like a branch-cut sign choice.
   - Templated on (real, complex) types, so it runs for double, float and
     the four backends.
   - Report time per element, plus registers and stack bytes (use P0.1/P0.2
     scripts). Its purpose is to show occupancy effects that single-op
     kernels hide.
3. Document both in `perf/README.md`.

**Done when:** the runner works on the allocated GPU and its CSV is
committed. Only one GPU is needed here; P0.6 covers the rest.

### P0.4 — GB300 lanes: harness build and bit-identity script

**Repo** xpmath-kokkos · **Needs** GB300 · **Depends on** P0.3

**Read first:** `validation/b200/run_b200_bit_identity.sh`, `perf/README.md`.

**Steps:**
1. Create `validation/gb300/run_gb300_bit_identity.sh`. Copy the B200 script
   and change only the modules and Kokkos prefix to the GB300 row of §1.4.
   Run it on GB300 and commit its log. Pass = zero differing cells.
2. Build the perf harness on the GB300 node. Add `sm_103` to
   `isa_metrics.sh` if needed. Commit the GB300 ISA CSV.

**Done when:** GB300 bit-identity passes and the GB300 ISA CSV is committed.

### P0.5 — GB300 device-sweep lane in xpmath

**Repo** xpmath · **Needs** GB300, then login node · **Depends on** —

**Read first:** `validation/b200/run_b200_sweep.sh`, and the `--arch` handling
in `scripts/xpm_build.sh` (`grep -n arch scripts/xpm_build.sh`).

**Steps:**
1. Add `--arch gb300` to `scripts/xpm_build.sh`: GB300 modules from §1.4,
   device tree only.
2. Create `validation/gb300/run_gb300_sweep.sh`. On the node, it builds and
   runs `sweep_device` and writes the raw file. It does **not** score;
   scoring runs on the login node with
   `sweep_accuracy --ulp --score-results <raw> --where gb300`.
   `scripts/sweep_accuracy.cpp` accepts only a closed list of `--where`
   tokens, so add `gb300` to it. That touches no baseline, register or
   expectation.
3. Run it on `gpu-perf`. Commit the raw file under `validation/gb300/logs/`. Do
   **not** add a `sweep_device_gate_gb300` ctest target. Note in the status block that
   the user decides whether GB300 becomes a gate.

**Done when:** a GB300 raw file exists and scores on the login node without
errors. It also makes CHECK-DEVICE-IDENTICAL gb300 possible.

**Outcome (2026-11-03):** done on `wip/P0.5-dd-acos` (0bd8b95). It needed one
library change (DD complex `acos`, see §1.6) and one harness change
(`hypot_cr`). The library change still needs the other GPUs: that is P0.5b.

### P0.5b — Validate the DD acos fix on the other GPUs

**Repo** xpmath · **Needs** A100, B200, MI250, MI300 · **Depends on** P0.5

**Read first:** the "P0.5 (final)" status block only.

**Steps:**
1. Before = xpmath `gpu-perf` (4f60bc4). After = `wip/P0.5-dd-acos`
   (0bd8b95). Use these in place of the §0 starting commit and
   `wip/<task-id>`.
2. CHECK-DEVICE-IDENTICAL on a100, b200, mi250 and mi300.
3. CHECK-SANITIZER on a100 and b200, on the after tree.
4. If every check passes: fast-forward xpmath `gpu-perf` to
   `wip/P0.5-dd-acos` and push.

**Stop if:** any device shows a difference. If the only differing rows are
DD complex `acos`, report them with the scorer's ulps on that device. The
user decides whether a bit change there is acceptable. Do not fast-forward.

**Done when:** all four devices are identical, both sanitizer runs report 0
errors, and xpmath `gpu-perf` is at 0bd8b95.

### P0.5c — GB300 as an accuracy gate (approved by the user 2026-11-03)

**Repo** xpmath · **Needs** GB300, then login node · **Depends on** P0.5b

The user approved this on 2026-11-03. It adds a ctest target, which is an
approved exception to §1.2 rule 3.

**Steps:**
1. Two GB300 sweeps of the tree `gpu-perf` now points at must be
   bit-identical (headers and timing removed). P0.5 may already have done
   this on 0bd8b95; check its status blocks. That result counts only if
   `gpu-perf` is at 0bd8b95. Otherwise, run the two sweeps on GB300. Stop
   if they differ.
2. On the login node, score the raw file, commit
   `validation/sweep/sweep_baseline_gb300.csv.gz` and the raw file of
   record, and add a `sweep_device_gate_gb300` target modelled on
   `sweep_device_gate_b200`.
3. Update the ctest count that xpmath's CI asserts (67 → 68).
4. Regenerate `docs/DEVICE_PRECISION.md` with a GB300 column.
5. Run CHECK-HOST.

### P0.6 — Baseline runs (one session per GPU)

**Repo** xpmath-kokkos · **Needs** the one GPU named · **Depends on** P0.2, P0.3, P0.4

Run as five separate sessions: **P0.6-a100, -b200, -gb300, -mi250, -mi300**.

**Steps:** build the harness against `vendor/xpmath` (v0.3.0), then run ISA
metrics (if not already committed for this arch) and the throughput runner,
including the science kernel. Commit the CSVs.

**Done when:** that arch has ISA and throughput CSVs for the vendored tag.

### P0.7 — Per-GPU cost table generator

**Repo** xpmath-kokkos · **Needs** login node · **Depends on** the P0.6 sessions done so far

A GPU with no P0.6 data yet gets a "NO DATA yet" column. Re-run P0.7 (a
short session) after that GPU's P0.6 lands.

**Steps:** write `perf/gen_cost_table.py`. It reads the latest CSVs per arch
and generates `docs/PERF_TABLE.md`:
- one table per GPU, rows = type × op;
- columns = ns/op, ratio vs double, ratio vs float, regs, stack bytes;
- one summary row per type for the science kernel.

State at the top: which xpmath tag, which date, and "performance only, no
accuracy statement" (accuracy lives in xpmath's `DOMAINS.md`). Commit the
generated file.

**Done when:** `docs/PERF_TABLE.md` exists. This is the table users choose
a type from, and the baseline every later task compares against.

---

## Phase 1 — Stop hiding operations from the compiler (bit-preserving)

### P1.1 — HIP: replace `volatile` in the EFT helpers

**Repo** xpmath (on `gpu-perf-staging`, §1.9) · **Needs** login node, MI250, MI300 · **Depends on** P0.7, P0.5

**Background:** `include/xp/config.hpp` §5 (`eft_add/sub/mul/fma`) copies
operands into `volatile` locals on every non-CUDA path. On gfx90a that
turns each floating-point operation into a stack store and reload: DD
`multiply` became 208 instructions (70 stack loads/stores, 76 waits)
against 62 for the same arithmetic without `volatile`. CUDA uses inline PTX
and is fine.

**Read first:** `grep -n 'eft_' include/xp/config.hpp`, then that section
only (about lines 300–470).

**Change** (HIP device pass only, under `__HIP_DEVICE_COMPILE__`; host and
CUDA untouched):
- `eft_add/sub/mul` become plain operations inside a block that starts with
  `#pragma clang fp contract(off)` and `#pragma clang fp reassociate(off)`.
- `eft_fma` becomes `__builtin_fma(a, b, c)` with no `volatile` (an FMA
  cannot be split, so it needs no protection).
- Update the §5 comment: what changed and why. Keep it short.

**Checks:** CHECK-HOST; CHECK-DEVICE-IDENTICAL mi250 and mi300; CHECK-PERF
mi250 (expect DD `mul` stack bytes → 0).

**Stop if:** either device fails CHECK-DEVICE-IDENTICAL (NaN-vs-NaN
differences are allowed, §1.5). A failure would mean hipcc really does
reorder these operations without `volatile`. Report it; don't add
workarounds.

### P1.2 — Type alignment

**Repo** xpmath · **Needs** login + all 5 GPUs · **Depends on** P1.1

**Change:** `alignas(16)` on `DoubleDouble` and `QuadFloat`, `alignas(8)` on
`FloatFloat`. These leave `sizeof` unchanged. Leave `TripleFloat` alone:
padding it would change its size. Grep for `sizeof(` in the xp headers and
in xpmath-kokkos's `impl/atomic_cas_add.hpp` to confirm nothing assumes the
old alignment.

**Checks:** CHECK-HOST; CHECK-DEVICE-IDENTICAL on all 5; CHECK-PERF on all 5.

**Keep it only if** throughput is not worse anywhere by more than 3%.
Otherwise keep it off `gpu-perf` (§1.2 rule 2) and record why.

---

## Phase 2 — Fast path inline, rare path out of line (bit-preserving)

The rule: ordinary inputs run a short, inlined sequence. Overflow rescue,
NaN recovery, rescaling loops and similar rare cases move into separate
`noinline` functions that the fast path calls only when a cheap check fails.
The operations on each path stay exactly the same, so results must stay
bit-identical, and the checks prove it.

Each `[b]` task is done once per backend, in order dd → ff → tf → qf, one
session each.

### P2.1[b] — Outline the rare-case paths in real arithmetic

> **Amended 2026-11-10 (user decision): dropped after dd.** P2.1[dd] landed
> only its log1p result-slot change; outlining slowed DD and FF (GB300 FF/cFF
> median 1.077) and re-triggered the ptxas UR7 fault on B200 and sm_103. Do
> not run P2.1 for ff, tf or qf. P2.2[b] now depends on P1.2 instead.

**Repo** xpmath · **Needs** login + all 5 GPUs · **Depends on** P1.2 (dd) or P2.1 of the previous backend

**Read first:** in `include/xp/<b>_math.hpp`, only `two_prod`, `multiply`,
`divide` (and its core/lift helpers), `multiply_scalar`, `divide_scalar`.
Find them with `grep -n`.

**Change:**
1. If not already present, add `XPMATH_COLD_FUNCTION` to `config.hpp`:
   `noinline`, plus `__attribute__((cold))` where supported. Document the
   fast/slow rule there in a few lines.
2. In each function above, move every rare-case branch body into a
   `detail::<name>_slow` cold function with identical operations. Examples:
   non-finite/zero product, Zone C, the KI-41 rescale loop.
3. **Leave existing `XPMATH_NOINLINE_FUNCTION` marks as they are.**
   Inlining changes are P2.2, kept separate so a failure points to one cause.

**Checks:** CHECK-HOST; CHECK-DEVICE-IDENTICAL on all 5; CHECK-SANITIZER on
A100, B200, GB300; CHECK-PERF on all 5.

### P2.2[b] — Inline the hot small functions

> **Amended 2026-11-11, late (user decisions on dd).**
> - **dd lands as 4d597fd.** xpmath `wip/P2.2-dd` = 4d597fd passed identity
>   on all five GPUs, full memcheck on B200, A100, GB300, gfx90a lint and
>   host (status: P2.2[dd] block, addenda 1-2, C-b200 (2), C-a100 (2)).
>   Accepted regression: DD science kernel 1.165 on GB300. Landing session
>   (no GPU): fast-forward `gpu-perf-staging` 6f4fb5a..4d597fd. No new
>   ledger rows: B200 and A100 at 4d597fd are already settled (rows 6, 7).
> - **Ledger rows 4-7 closed by the user.** So the landing session also runs
>   C-<arch> step 7: fast-forward xpmath `gpu-perf` to the newest staging
>   commit with no open ledger rows at or before it (expected 4d597fd), which
>   counts as this section explicitly saying to fast-forward it.
> - **Lesson for tf/qf:** the GB300 fault needed all three inline marks
>   together and was fixed by making `angle()`'s pi/2 a bit constant (no
>   multiply on that path). Check for the same shape in the tf/qf
>   forwarding audit.

> **Amended 2026-11-11 (user decisions after the probe).**
> - **ff lands now, B200 deferred.** The kept shape is xpmath
>   `wip/P2.2-ff` = 6f4fb5a (sqrt inlined). It passed GB300, MI250, MI300,
>   A100 and host (status addenda 1-4). Accepted regressions: FF sqrt
>   +10.7% on GB300; cFF div +4.8% on A100. Landing = fast-forward
>   `gpu-perf-staging` to 6f4fb5a plus ledger row for B200 (6e6a989..6f4fb5a,
>   CHECK-DEVICE-IDENTICAL, CHECK-SANITIZER, CHECK-PERF, bit-preserving) under
>   the §1.9 standing deferral. No GPU needed.
> - **Fan-out approved** to dd, tf, qf (in that order), full scope as below
>   on A100, GB300, MI250, MI300; B200 is deferred by the §1.9 standing
>   deferral (ledger row). Each backend does the forwarding
>   audit first, as in the ff probe. Lessons: never renormalise a result
>   that can be an overflow inf (quick_two_sum gives lo = NaN; ff exp10
>   failed identity this way); complex-returning forwards cannot be
>   renormalised; perf runs one pipeline per node at a time.

> **Amended 2026-11-10 (user decision): P2.2[ff] runs first, as a GB300-only
> go/no-go probe.** Order for P2.2 is ff first; dd, tf, qf wait for the
> user's call after the probe.
> - **Probe scope:** GB300 only. No other GPU, no ledger rows, nothing on
>   `gpu-perf-staging`. Push the result to `wip/P2.2-ff` only.
> - **Forwarding audit first (deliverable before editing):** in
>   `ff_math.hpp`/`ff_complex*.hpp`, list every function that returns a
>   callee's result directly (result-slot forwarding) where the callee is a
>   real call or is inlined into a large device function, or becomes one
>   after this task's change. log1p is known; do not assume it is the only
>   one. For each candidate, apply the result-slot renormalise
>   (`eft_quick_two_sum` on an already-normalised pair, as in xpmath 6e6a989
>   and local b9b4428) only if the sweep proves it bit-identical. Watch the
>   quick_two_sum edge: an inf hi gives lo = NaN.
> - **Checks on GB300:** CHECK-HOST; full 252-kernel sweep exit 0;
>   CHECK-DEVICE-IDENTICAL vs `gpu-perf-staging`; full-sweep memcheck
>   0 errors (an `--only` run is not a clearance); perf by ratio vs before,
>   3 interleaved repeats.
> - **Verdict (three outcomes):** GO if the FF/cFF median or the FF
>   science-shaped kernel is at least 3% faster and no row is more than 3%
>   slower without a stated reason. PARK if everything is within ±3%: add a
>   row to `docs/GPU_PERF_PARKED.md` (§1.10) with Re-test at = `P3.4[ff]`
>   (P3.4 makes `multiply` small, which makes inlining it more likely to
>   pay; P3.1/P3.2 also stress multiply/divide). NO-GO if anything is more
>   than 3% slower without a reason, or a check fails. Either way, write the
>   status block and stop; the user decides whether to fan out.

**Repo** xpmath · **Needs** login + all 5 GPUs · **Depends on** P1.2 (amended 2026-11-10)

**Read first:** `docs/TOOLCHAIN_DEFECTS.md` TD-1 and TD-4 entries only;
`grep -n 'XPMATH_NOINLINE_FUNCTION\|td4_workaround' include/xp/<b>_*.hpp`.

**Change:** remove `XPMATH_NOINLINE_FUNCTION` from `multiply`, `divide` and
`sqrt` of backend b (real type only), making them `XPMATH_INLINE_FUNCTION`.
Don't touch `exp`, `log`, trig, or anything with a `td4_workaround`.

**Checks:** CHECK-HOST; CHECK-DEVICE-IDENTICAL on all 5; CHECK-SANITIZER on
A100, B200, GB300; CHECK-GFX90A-LINT; CHECK-PERF on all 5.

**Stop if:** any sanitizer error, lint failure or device difference
appears, even on one architecture. Record which function and arch. Do not
put the mark back only for that arch (§1.2 rule 4).

### P2.3[b] — Complex fast/slow split

> **DROPPED 2026-11-11 (user).** It is P2.1's outlining applied to complex
> multiply/divide, and P2.1 lost on every GPU and re-triggered the ptxas UR7
> fault. Do not run it.

**Repo** xpmath · **Needs** login + all 5 GPUs · **Depends on** P2.2[b]

**Read first:** in `include/xp/<b>_complex.hpp`, only `operator*`,
`operator/`, and the recovery helpers they call (`mul_recover` and similar).

**Change:** the same as P2.1, for complex multiply and divide. Recovery
paths (e.g. KI-28's NaN recovery) become cold functions. Do not change the
`*_cross` / `*_cross_accum` code (see §1.6).

**Checks:** as P2.1.

### P2.4 — Expansion arrays (only with the user's go-ahead)

**Repo** xpmath · **Depends on** P2.2 for all four backends (amended 2026-11-11; P2.3 dropped)

Per-thread arrays with runtime indexes (`*_cross_accum`, `*_expansion_push`,
`float e[36]` in QF `fma`) end up in slow local memory. This is where the
scrapped attempt failed. **Before doing anything, ask the user for the
details of that failure** and wait for an explicit go-ahead. Then write a
one-page proposal in the status block and stop. Implementation tasks will be
defined from that proposal.

---

## Phase 3 — Cheaper algorithms (changes result bits)

These tasks trade some accuracy for speed. xpmath's gates decide whether
each one is acceptable. xpmath-kokkos only measures speed.

### P3.0 — Accuracy-vs-speed decision memo (STOP after writing)

**Repo** xpmath · **Needs** login node · **Depends on** P2.2[dd] (amended 2026-11-11; P2.3 dropped)

**Read first:** `docs/CORRECTNESS.md` §2–3 (bounds, monotone gate, exit codes).

xpmath's monotone gate fails if any point gets worse by more than 10^0.1.
A faster, slightly less accurate algorithm will trip it even when every
point stays below its derived bound. Write `docs/PERF_ACCURACY_POLICY.md`
(at most one page) that lays out the options for the user to choose from.
For example:
- (a) never accept a monotone regression;
- (b) accept regressions that stay below the derived bound, re-baselined in
  their own commit with the measured speedup quoted;
- (c) an opt-in "fast" tier kept separate from the default.

Push `gpu-perf` and **stop**. No P3.1+ task starts until the user has chosen.

### P3.1–P3.5[b] — One algorithm change per task

**Repo** xpmath · **Needs** login + all 5 GPUs · **Depends on** P3.0 decided, and the same task for the previous backend

Each task is one row below, applied to one backend (dd first). For each:
1. Read only the function named.
2. Make the change.
3. Run CHECK-HOST. Handle any monotone or absolute gate result exactly as
   the policy from P3.0 says, re-baselining per `docs/CORRECTNESS.md` §5 in
   its own commit if the policy allows.
4. Run the device sweeps on all 5 (`run_<arch>_sweep.sh`) and record the
   scorer's results.
5. Run CHECK-PERF on all 5. Report the speedup per GPU.

| Task | Function | Change |
|---|---|---|
| P3.1[b] | `log` | DD does 3 Newton steps, each calling full `exp` plus a divide. Starting from a double seed, test 1 and 2 steps. FF from a float seed: test 1 and 2. TF/QF: measure whether 3 is still needed. |
| P3.2[b] | `exp` | Get the reduction integer from one native multiply in place of the full `divide(a, ln2)`. Replace the per-term `divide_scalar` in the Taylor loop with multiplication by precomputed reciprocal-factorial constants, using a fixed term count with no data-dependent exit. |
| P3.3[b] | `sincos` | Same series change as P3.2. Add a cheap multi-part pi/2 reduction for moderate arguments; keep Payne-Hanek only above a threshold the session measures. |
| P3.4[b] | `multiply` | FMA-based double-word multiply (Joldes–Muller–Popescu) in place of the current full accumulation. |
| P3.5[b] | complex `operator/` | Replace the 5-word expansion in `*_cross` with Kahan's FMA determinant for `a*b − c*d`. |

---

## Phase 4 — One generic core

### P4.0 — Design (STOP after writing)

**Repo** xpmath · **Needs** login node · **Depends on** R (v0.4.0 released) (amended 2026-11-11)

Write `docs/GENERIC_CORE_DESIGN.md`: one implementation templated on limb
type (`float`, `double`) and limb count (2, 3, 4), replacing four
hand-maintained backends. It must keep every public name and every result
bit, migrate one operation family at a time, and list the order of
implementation tasks. Push `gpu-perf` and **stop**. Implementation tasks are
written from the approved design.

---

## Task R — Release, re-vendor, re-measure (once, at the end)

> **Amended 2026-11-11 (user): release v0.4.0 after Phase 3, before
> Phase 4.** The user is preparing a paper; settling the math comes first.
> Order: finish Phase 2 (P2.2[tf], P2.2[qf]) -> P3.0 policy -> P3.1-P3.5
> for every backend -> C-<arch> for any open rows -> user merges xpmath
> `gpu-perf` into `main` and tags v0.4.0 -> R. Phase 4 (P4.0 onward) starts
> after the release and gets its own release and R later if it changes
> anything.

**Repo** xpmath-kokkos · **Needs** all 5 GPUs (one session may visit them
in turn, or split per GPU) · **Depends on** the deferral ledger is empty,
xpmath `gpu-perf` equals `gpu-perf-staging`, and the user has merged
xpmath's `gpu-perf` into `main` and tagged a release

Before R, every perf measurement uses the harness built against the xpmath
clone's `gpu-perf` via `XPMATH_KOKKOS_PERF_XP_DIR` (CHECK-PERF), so no
intermediate releases are needed.

**Steps:**
1. `scripts/sync_upstream.sh <tag>`, rebuild, run ctest. Pass = 13/13.
2. Run `validation/<arch>/run_<arch>_bit_identity.sh` on all 5. Pass = zero
   differing cells on each.
3. Re-run the perf harness on all 5 (as in P0.6) and regenerate
   `docs/PERF_TABLE.md` (P0.7 script).
4. In the status block, give the before/after summary per GPU (per-type
   science-kernel time, and DD/FF `mul`/`div`/`log` ns/op).

**Done when:** all three steps are green and committed on `gpu-perf`.

---

## Out of scope

- Host (CPU) performance. The host `volatile` stays, since GCC has no
  reliable per-function contraction control. Revisit only if asked.
- Choosing a type for the user. The library publishes the cost table; the
  user picks the type.
- Per-architecture implementations of the same type.
- QCDLoop. It's an application consumer, not a gate for this plan.
