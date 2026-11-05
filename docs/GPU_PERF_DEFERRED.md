# GPU_PERF deferral ledger

Rules: `docs/GPU_PERF_PLAN.md` §1.9. A task adds one row per GPU it could
not run on. Catch-up task C-<arch> settles rows. The ledger must have no
open rows before the user merges into `main`.

P0.5's acos fix (xpmath 4f60bc4..0bd8b95) is owed to A100 by P0.5b itself
and is not tracked here.

| # | Task | GPU | xpmath before..after | Checks owed | Bit-preserving | Status |
|---|---|---|---|---|---|---|
