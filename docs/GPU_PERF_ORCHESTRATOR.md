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
  `main` once, at the end. Never merge, tag, push to `main`, or force-push.
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

## Current state (updated 2026-11-11)

Verify these against the files and branches before relying on them.

- **Branches.** xpmath `gpu-perf` = 0769d6d, `gpu-perf-staging` = 6e6a989
  (both unchanged since 2026-11-10). xpmath `wip/P2.2-ff` = 6f4fb5a (on
  origin; child of staging): FF multiply/divide/sqrt inlined + result-slot
  renormalise in log1p, acosh, xp_asin_real_mag, xp_atan2_safe.
- **Ledger.** Rows 1-3 CLOSED (read to the end of the Status cell). Only
  **row 4** open: P2.1[dd] on B200, 0769d6d..6e6a989.
- **Parked register:** no rows.
- **P2.2[ff]:** probe verdict GO on GB300 (2026-11-10), then at the user's
  request checked green on MI250, MI300, A100 (status addenda 2-4). Identity
  436081/436081 on all four, memcheck 0 on GB300 + A100, lint on gfx90a.
  User accepted FF sqrt +10.7% on GB300 (keep sqrt inlined). Not reviewed
  by the user yet: A100 cFF div 1.048, stable over 3 repeats, no reason
  stated. **B200 unchecked; nothing on staging.** Orchestrator review of
  the evidence done 2026-11-11 (paths exist; ident PASS x4; memcheck 0 x2).
- **User decisions 2026-11-11:** land P2.2[ff] (6f4fb5a) on staging once
  B200 is checked; cFF div +4.8% on A100 accepted; P2.2 fans out to dd, tf,
  qf; P2.3 dropped (plan amended). The user is trying to get a B200 on
  2026-11-12 morning.
- **Next (B200 window, run sequentially on the one node, never at the same
  time):** (a) `C-b200` for ledger row 4 (may fast-forward xpmath `gpu-perf`
  to 6e6a989 if it passes); (b) `P2.2[ff]` resume: B200 check of 6f4fb5a,
  then fast-forward staging to 6f4fb5a. Prompts: "Prompts for the next B200
  window" below. Then P2.2[dd].
- **P2.1 is dropped after dd** (user, 2026-11-10). Evidence in the P2.1[ff]
  Stopped block.
- **Not done yet:** P0.5c (login node; can slot in any time).

## Open questions for the user (raise them; don't decide them)

1. **B200 standing deferral.** Amend §1.9 so B200 is unavailable by default
   through Phase 2, settled in batched C-b200 windows? Orchestrator
   recommends yes. On 2026-11-11 the user agreed with the other
   recommendations but answered on B200 only "will try to get B200
   tomorrow"; confirm before writing it into the plan.

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
- **Perf noise:** A100 timing is bimodal (use 3 interleaved repeats, not
  best-of-2); MI250/MI300 drift uniformly (compare against native rows);
  GB300 repeat spread is under 3%.
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

## Session order (amended 2026-11-10)

`P2.2[ff]` GB300 probe (in flight), then the user's call (see open question
3). Then P2.2[dd], P2.2[tf], P2.2[qf] (P2.3 dropped 2026-11-11),
`P2.4` (optional, needs the user's go-ahead), `P3.0` (the user chooses a
policy), `P3.1[dd..qf]` … `P3.5[dd..qf]`, and `P4.0`. Parked rows are
re-tested automatically by the task their Re-test at names (§1.10). When you
drop or reorder a task, move its Re-test at entries.

Slot in whenever a GPU frees up:
- `C-<arch>` for open ledger rows. B200 currently owes row 4. Batch B200
  (open question 1).
- `P0.5c` (login node).
- A re-run of `P0.7` when a missing GPU's P0.6 lands.

At the end: the ledger is empty and xpmath `gpu-perf` == `gpu-perf-staging`;
the user merges and tags; then `R for xpmath tag <tag>`.

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

## Prompts for the next B200 window (written 2026-11-11)

Run (a) then (b) on the same B200 host, one after the other.

(a) C-b200 for ledger row 4: the canonical worker prompt with `<ID>` =
`C-b200`, plus on top:
```
RESUME NOTE: settle ledger row 4 only (P2.1[dd], 0769d6d..6e6a989, B200:
CHECK-DEVICE-IDENTICAL, CHECK-SANITIZER full sweep, CHECK-PERF same-session
before/after, 3 interleaved repeats). Rows 1-3 are already CLOSED. Do not
run anything for P2.2[ff]; another session uses this node after you, so
release it when done and say so.
```

(b) P2.2[ff] landing: the canonical worker prompt with `<ID>` = `P2.2[ff]`,
plus on top:
```
RESUME NOTE: the probe and its addenda 1-4 are done (read the P2.2[ff]
blocks in GPU_PERF_PLAN_STATUS.md and the 2026-11-11 amendment in the P2.2
section). Do NOT redo GB300, MI250, MI300, A100 or host; reuse
$HOME/gpuperf-scratch/P2.2[ff]/. Remaining, on B200 only, comparing
xpmath 6e6a989 (before) with wip/P2.2-ff = 6f4fb5a (after): full 252-kernel
sweep exit 0 both; CHECK-DEVICE-IDENTICAL; full-sweep memcheck 0 errors
(no --only); perf by ratio, 3 interleaved repeats, one pipeline at a time.
Logs under $HOME/gpuperf-scratch/P2.2[ff]/b200/. If all pass, fast-forward
gpu-perf-staging to 6f4fb5a (ancestor check first; no merge commit, no
new commits) and write a status block saying P2.2[ff] is landed. If
anything fails, stop and record function + arch; no per-arch #ifdef; do
not land. Do not move xpmath gpu-perf.
```

## How the user likes answers

Concise, plain English. Tables with a copy-pasteable session-ID column.
Ask clarifying questions when a request is ambiguous. Verify claims against
files before relaying them.
