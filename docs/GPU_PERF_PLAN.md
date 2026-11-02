# GPU performance plan for xpmath through Kokkos

Goal: make the xpmath arithmetic and functions faster on GPUs when they are
called inside Kokkos kernels, without breaking the correctness machinery
that already exists.

Each task below is sized for **one Claude session**. A session does exactly
one task, records the result, and stops.

---

## 0. How a session uses this document

0. Before anything else, run `git checkout gpu-perf && git pull` in
   **both** `~/xpmath-kokkos` and `~/kokkos-extended-precision-demo`. All
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
| P0.6 | Baseline runs, one session per GPU | xpmath-kokkos | that GPU | P0.2, P0.3, P0.4 |
| P0.7 | Per-GPU cost table generator | xpmath-kokkos | login node | P0.6 (all five) |
| P1.1 | HIP: replace `volatile` EFTs | xpmath | login + MI250 + MI300 | P0.7 |
| P1.2 | Type alignment | xpmath | login + all 5 GPUs | P1.1 |
| P2.1[b] | Outline rare-case paths, backend b | xpmath | login + all 5 GPUs | P1.2 (or prior P2.1) |
| P2.2[b] | Inline the hot small functions, backend b | xpmath | login + all 5 GPUs | P2.1[b] |
| P2.3[b] | Complex fast/slow split, backend b | xpmath | login + all 5 GPUs | P2.2[b] |
| P2.4 | Expansion arrays (needs user go-ahead) | xpmath | login + all 5 GPUs | P2.3[all] |
| P3.0 | Accuracy-vs-speed decision memo (STOP) | xpmath | login | P2.3[dd] |
| P3.1–P3.5[b] | Cheaper algorithms, one per task | xpmath | login + all 5 GPUs | P3.0 |
| P4.0 | Generic-core design (STOP) | xpmath | login | P3 done |
| R | Release, re-vendor, re-measure (once, at the end) | xpmath-kokkos | all 5 GPUs | user merged `gpu-perf` and tagged a release |

Backend order for every `[b]` task: **dd → ff → tf → qf**. Each backend is a
separate session.

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

The xpmath clone is `~/kokkos-extended-precision-demo`, on branch
`gpu-perf`. Use it for xpmath tasks and as `XPMATH_KOKKOS_PERF_XP_DIR`.
**Do not use `~/xpmath_perf_audit/upstream`, and never check out the local
branch `scrapped/tier1-792b17d`.** Both hold the scrapped attempt (§1.6).

### 1.2 Hard rules

1. **Host reasoning is never enough.** An argument that a change "must be
   bit-identical by algebra" is not validation. A previous attempt passed
   every host test and still silently corrupted results on a GPU through a
   compiler bug. Every library change is checked on the GPUs its task lists.
2. **One shared branch per repo: `gpu-perf`.** Every task commits onto
   `gpu-perf` (in xpmath-kokkos and/or the xpmath clone) and pushes it with
   a plain `git push`. The user merges `gpu-perf` into `main` once, at the
   end.
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
  `cmake --build build -j16 > /tmp/<task>-build.log 2>&1; tail -30 /tmp/<task>-build.log`
  `ctest ... > /tmp/<task>-ctest.log 2>&1; grep -E 'tests passed|Failed|\*\*\*' /tmp/<task>-ctest.log`
- Never print a sweep CSV (436k rows) or a raw device file. Compare them
  with `cmp`/`diff -q`, or count differences with `diff | wc -l`.
- Never print whole assembly files. Use the metric scripts from P0.1/P0.2.
- Long builds (ptxas can take 20–30 minutes): run them in the background
  with output to a log, and check the tail occasionally. Don't stream them.
- Don't re-read files you already have. Don't explore beyond the files
  your task lists unless a step fails.

### 1.4 Getting a GPU

The user allocates GPU nodes with an interactive `qsub`. When your task
needs a GPU:

1. Ask the user: *"Please allocate a `<GPU>` node and tell me the hostname."*
   Wait for the answer. Never submit jobs yourself.
2. Run commands with `ssh <host> 'bash -lc "<commands>"'`. `$HOME` is shared,
   so build trees and logs written on the node are visible from the login node.
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
   `RAW_OUT=/tmp/<task>-<arch>-before.csv`. Then run it on `wip/<task-id>`
   with `RAW_OUT=/tmp/<task>-<arch>-after.csv`. Use separate `BUILD_DIR`s.
2. Remove `#` header lines and timing lines from both (read the first 20
   lines of `validation/strip_timing.sh` for its usage), then `cmp` them.
3. Pass = identical. Any difference, even one row, is a failure. Report
   the count and the first 3 differing rows (op, backend, point), not the
   whole diff.

The scorer's exit code inside the script is a separate signal. Record it,
but bit identity is the pass criterion for bit-preserving tasks. GB300 uses
the script added by P0.5.

**CHECK-SANITIZER `<arch>`** (CUDA GPUs only: A100, B200, GB300). Needed by
any task that changes what gets inlined. A known nvcc defect (TD-4,
`~/kokkos-extended-precision-demo/docs/TOOLCHAIN_DEFECTS.md`) silently corrupts the stack frame of
large inlined code. Run
`compute-sanitizer --tool memcheck <build>/device/tests/sweep_device --out /tmp/<task>-san.csv > /tmp/<task>-san.log 2>&1`
and `grep 'ERROR SUMMARY' /tmp/<task>-san.log`. Pass = `0 errors`.

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
3. Run it on `gpu-perf`. Commit the raw file under `validation/gb300/logs/`. Do
   **not** add a `sweep_device_gate_gb300` ctest target. Note in the status block that
   the user decides whether GB300 becomes a gate.

**Done when:** a GB300 raw file exists and scores on the login node without
errors. It also makes CHECK-DEVICE-IDENTICAL gb300 possible.

### P0.6 — Baseline runs (one session per GPU)

**Repo** xpmath-kokkos · **Needs** the one GPU named · **Depends on** P0.2, P0.3, P0.4

Run as five separate sessions: **P0.6-a100, -b200, -gb300, -mi250, -mi300**.

**Steps:** build the harness against `vendor/xpmath` (v0.3.0), then run ISA
metrics (if not already committed for this arch) and the throughput runner,
including the science kernel. Commit the CSVs.

**Done when:** that arch has ISA and throughput CSVs for the vendored tag.

### P0.7 — Per-GPU cost table generator

**Repo** xpmath-kokkos · **Needs** login node · **Depends on** all P0.6 sessions

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

**Repo** xpmath · **Needs** login node, MI250, MI300 · **Depends on** P0.7

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

**Stop if:** either device comparison shows any difference. That would mean
hipcc really does reorder these operations without `volatile`. Report it;
don't add workarounds.

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

**Repo** xpmath · **Needs** login + all 5 GPUs · **Depends on** P2.1[b]

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

**Repo** xpmath · **Needs** login + all 5 GPUs · **Depends on** P2.2[b]

**Read first:** in `include/xp/<b>_complex.hpp`, only `operator*`,
`operator/`, and the recovery helpers they call (`mul_recover` and similar).

**Change:** the same as P2.1, for complex multiply and divide. Recovery
paths (e.g. KI-28's NaN recovery) become cold functions. Do not change the
`*_cross` / `*_cross_accum` code (see §1.6).

**Checks:** as P2.1.

### P2.4 — Expansion arrays (only with the user's go-ahead)

**Repo** xpmath · **Depends on** P2.3 for all four backends

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

**Repo** xpmath · **Needs** login node · **Depends on** P2.3[dd]

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

**Repo** xpmath · **Needs** login node · **Depends on** Phase 3 complete

Write `docs/GENERIC_CORE_DESIGN.md`: one implementation templated on limb
type (`float`, `double`) and limb count (2, 3, 4), replacing four
hand-maintained backends. It must keep every public name and every result
bit, migrate one operation family at a time, and list the order of
implementation tasks. Push `gpu-perf` and **stop**. Implementation tasks are
written from the approved design.

---

## Task R — Release, re-vendor, re-measure (once, at the end)

**Repo** xpmath-kokkos · **Needs** all 5 GPUs (one session may visit them
in turn, or split per GPU) · **Depends on** the user has merged xpmath's
`gpu-perf` into `main` and tagged a release

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
