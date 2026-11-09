# GPU_PERF deferral ledger

Rules: `docs/GPU_PERF_PLAN.md` §1.9. A task adds one row per GPU it could
not run on. Catch-up task C-<arch> settles rows. The ledger must have no
open rows before the user merges into `main`.

P0.5's acos fix (xpmath 4f60bc4..0bd8b95) is owed to A100 by P0.5b itself
and is not tracked here.

| # | Task | GPU | xpmath before..after | Checks owed | Bit-preserving | Status |
|---|---|---|---|---|---|---|
| 1 | P0.5b | b200 | 4f60bc4..0bd8b95 | CHECK-DEVICE-IDENTICAL b200, CHECK-SANITIZER b200 | yes | open; gpu-perf was already fast-forwarded past this row by P0.5b (accepted exception) (A100 passed as same-vendor substitute; $HOME/gpuperf-scratch/P0.5b/ worktrees remain). C-b200 2026-11-09 (blackwell00): FAILED. B200 sweep at 4f60bc4 is clean (exit 0, 0 above bound) but at 0bd8b95 DD:c:acos faults (error 700) and CHECK-SANITIZER shows 68 `Invalid __local__ write` errors; logs $HOME/gpuperf-scratch/C-b200/b200/{run-before,run-0bd8b95,san-0bd8b95}.log. Fault is inside this row; row stays open, needs the user's decision. D-b200-acos 2026-11-09: first fault = adf9ace; mechanism (UR7 result pointer clobbered; not TD-4 frame size) in GPU_PERF_PLAN_STATUS.md block "D-b200-acos" |
| 2 | P0.6 | b200 | 0bd8b95..0bd8b95 (no xpmath change) | P0.6 baseline runs on B200 (throughput, science, science_isa CSVs under perf/results/sm_100/); then re-run P0.7 (perf/gen_cost_table.py) | n/a (measurement only) | open: P0.6 part settled 2026-11-09 on blackwell00 (logs $HOME/gpuperf-scratch/P0.6-b200/); P0.7 re-run (perf/gen_cost_table.py) still owed, close this row then |
| 3 | P1.2 | b200 | 258e529..464fd97 | CHECK-DEVICE-IDENTICAL b200, CHECK-PERF b200 (same-session before/after) | yes | open; user said B200 unavailable 2026-11-09; A100 and GB300 passed both checks (same-vendor, §1.9). C-b200 2026-11-09: not run; B200 already faults at 0bd8b95 (row 1), so P1.2 could not be judged in isolation. 258e529 and 464fd97 show the same DD:c:acos fault (sanitizer 98 local-write errors each); logs $HOME/gpuperf-scratch/C-b200/b200/{run-258e529,run-after,san-258e529,san}.log. Row stays open |
