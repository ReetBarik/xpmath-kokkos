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

## Current state (updated 2026-11-12)

Verify these against the files and branches before relying on them.

- **Branches.** xpmath `gpu-perf` = `gpu-perf-staging` = 4d597fd (P2.2[dd]
  landed 2026-11-11, block "P2.2[dd] (final)"). Nothing has landed since.
  xpmath-kokkos `gpu-perf` holds every plan/status/ledger commit.
- **Ledger.** Rows 1-7 all CLOSED. No open rows.
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
- **In flight.**
  - `P3.0` (Opus, interactive with the user; no GPU). Worktree
    $HOME/gpuperf-scratch/P3.0/wt on xpmath `wip/P3.0` (local only, still at
    4d597fd with no commits as of 2026-11-12; not on origin). Pushes only
    wip/P3.0 + a status block; checklist line "Policy decided:
    (a)/(b)/(c)/other/Undecided".
- **After P3.0 reports:** if the user chose a policy, write its gate rule
  into the P3.1-P3.5 section (`Plan: ...`). P3.1[dd]'s first commit copies
  docs/PERF_ACCURACY_POLICY.md onto staging from origin/wip/P3.0. P3.4[tf]
  (and P3.4[qf] if parked) owes the parked A/B (§1.10); mention it in
  those prompts.
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

## Open questions for the user (raise them; don't decide them)

- (2026-11-12, unanswered) Can the Cursor / open-model workers ssh into GPU
  nodes and keep a session alive through multi-hour runs? If not, P3.x
  prompts must have the user launch sweeps and paste back log paths. Ask
  before writing the P3.1[dd] prompt if the user hasn't answered.

Settled: 2026-11-11 land 4d597fd; rows 4-7 closed. 2026-11-12 tf NO-GO, qf
probe NO-GO.

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

Concise, plain English. Tables with a copy-pasteable session-ID column.
Worker prompts always go in the chat reply, fully expanded from the
canonical template (RESUME NOTE on top), ready to paste into a Sonnet
session; never store them in files (user, 2026-11-11).
Ask clarifying questions when a request is ambiguous. Verify claims against
files before relaying them.
