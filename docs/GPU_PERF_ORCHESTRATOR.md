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

## Session order (after P1.2)

`P2.1[dd]`, `P2.2[dd]`, `P2.3[dd]`, then the same three for ff, tf and qf;
then `P2.4` (optional), `P3.0` (the user chooses a policy), `P3.1[dd..qf]` …
`P3.5[dd..qf]`, and `P4.0`.

Slot in whenever a GPU frees up:
- `C-<arch>` for open ledger rows. B200 currently owes rows 1–3, the
  P0.6-b200 baseline included.
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
   <ID>'s "Depends on". Always read docs/GPU_PERF_DEFERRED.md.

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
   - Moved xpmath gpu-perf: YES/NO (if YES, confirm (a), (b), (c) all held):
   - Status block pushed; SHAs quoted from origin:
   - GPUs I can release now: <hostnames or "none">
   Then give a 3-line summary.
```

## How the user likes answers

Concise, plain English. Tables with a copy-pasteable session-ID column.
Ask clarifying questions when a request is ambiguous. Verify claims against
files before relaying them.
