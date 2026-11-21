# GPU perf: fast-op chains

Tracks, for each fast op accepted in Phase 3, which other fast-tier
functions still reach the **strict** version of that op. A fast
override (tier.hpp pattern 1) speeds up only direct calls of the op
in the fast type. Every other fast function is a one-line forwarder
`to_fast(f(to_strict(x)))`, so its strict body calls strict exp,
strict log, and so on. Wiring a caller to the fast op ("chaining")
changes result bits, so each chain is its own gated task (one change
per gate run). The user picks which chains to run, after the
backend's current task closes.

How to fill a row: grep `include/xp/<b>_fast.hpp` for overrides and
forwarders, and the strict bodies (`<b>_math.hpp`,
`<b>_complex.hpp`) for calls to the op. Not from memory.

Perf rows that exist (perf harness, per backend): real add, atan2,
div, exp, log, mul, sin, sqrt; complex abs, add, div, exp, log, mul,
sqrt; science kernel. A chain can be gated only through ops that
have a perf row (gate `--ops` must list them).

**Fact that limits every chain (checked 2026-11-20 at xpmath
bbaa8fe):** the complex fast types forward the whole complex
function to strict (`log(DoubleDoubleComplexFast)` ->
strict complex `log` -> strict real `log`). So `cDD_fast` rows and
the science kernel (complex mul/add/div/sqrt/log, real atan2) see
**no** fast real op today, neither P3.1[dd] fast log nor P3.2[dd]
fast exp. Only the real `DD_fast` row of the op itself measures it.

## DD

| Fast op | Accepted at (staging) | Strict callers still reached from the fast tier | Callers with a perf row | Status |
|---|---|---|---|---|
| log (2 Newton steps) | c9e342e (P3.1[dd]); chained ff25642 (P3.2[dd]-chain, trimmed) | real: log2, log10, log1p, asinh, acosh, atanh; pow via dd_log_ext. complex: log10, pow, atan, asinh, acosh, atanh | cDD log now calls the fast real log (fast complex log = strict body + fast real log, NOINLINE) | chained: complex log (ff25642) |
| exp (mix, 12-term series) | bbaa8fe (P3.2[dd]) | real: strict log, exp2, exp10, expm1 (-> tanh), sinhcosh (-> sinh, cosh); real pow uses its own dd_exp_ext, not exp. complex: exp (still strict body + strict real exp), tan, tanh, pow, sin/cos/sinh/cosh via sinhcosh | cDD exp (not chained: 1fce03c measured, MI250 1.064, best 0.927 misses the fast rule) | chained: fast log Newton steps (b72fcab) |

Correction (2026-11-20, read at bbaa8fe dd_complex.hpp): strict
complex log calls real log ONCE (`log(abs(z))`, plus atan2), and
strict complex exp calls real exp ONCE (plus sincos). The table
above said "twice".

**User decision 2026-11-20:** chains 1 and 2 run as ONE task,
P3.2[dd]-chain (plan amendment), tested whole; chain 1 alone is
pre-screened for blame only.

**Update 2026-11-21 (P3.2[dd]-chain CLOSED, trimmed, user
exception):** staging ff25642 = chain 1 (fast log's Newton steps
call fast exp) + fast complex log calling the fast real log. The
science kernel now reaches a fast real op (complex log -> fast real
log -> fast exp). Fast complex exp stays a forwarder to strict.

Candidate chains (DD), smallest first (historical):
1. fast exp into fast log: 2 lines in `dd_log_fast_impl`. Gated via
   `DD_fast log` only (`--ops log`). Open user decision after
   P3.2[dd] closes.
2. complex exp/log overrides that call the fast real ops: changes
   `cDD_fast exp` / `cDD_fast log` and, for log, the science kernel.
   Not yet designed.

## FF, TF, QF

No fast op accepted yet (P3.1[ff], [tf], [qf] rejected).
