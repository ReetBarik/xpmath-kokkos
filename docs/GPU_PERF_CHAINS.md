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

## FF

Checked 2026-11-21 at xpmath a6b5626 (grep of ff_math.hpp,
ff_complex.hpp for `exp(` calls).

| Fast op | Accepted at (staging) | Strict callers still reached from the fast tier | Callers with a perf row | Status |
|---|---|---|---|---|
| exp (mix8, 8-term series) | a6b5626 (P3.2[ff]; candidate 5c1511d) | real: strict log (Newton step, 2 calls), exp2, exp10, expm1, sinhcosh, erf, erfc, zeta, incgamma, tgamma (Lanczos). complex: exp (strict body + strict real exp), tan, tanh, pow (exp(w*log z)). Real pow: no direct exp( call found in ff_math.hpp, not traced further | cFF exp (not chained, measured 1.000 / 1.001 / 1.000 / 1.002); FF log now calls the fast exp | chained: fast log Newton steps (e3bd4cb, staging 2f68b93) |
| log (strict algorithm, 2 Newton steps, calling fast exp) | 2f68b93 (P3.2[ff]-chain; candidate d469318) | real: log2, log10, log1p, asinh, acosh, atanh, pow_int, ff_log_ext (pow), bessel_y0/y1, expint, incgamma, zeta. complex: log10, pow, atan, asinh, acosh, atanh, xp_log_hypot2 (grep of `log(` at 2f68b93) | cFF log calls the fast real log (fast complex log = strict body + fast real log, NOINLINE) | chained: complex log (d469318). Complex exp still strict body + strict real exp |

**Update 2026-11-22 (P3.2[ff]-chain CLOSED):** staging 2f68b93 =
fast real log (strict algorithm) calling fast exp + fast complex
log calling the fast real log. The FF science kernel now reaches a
fast real op. Fast complex exp stays a forwarder to strict.

## TF

Checked 2026-11-22 at xpmath f2d94cc (grep of tf_math.hpp,
tf_complex.hpp for `exp(` calls).

| Fast op | Accepted at (staging) | Strict callers still reached from the fast tier | Callers with a perf row | Status |
|---|---|---|---|---|
| exp (mix9, 9-term series) | f2d94cc (P3.2[tf]; candidate 8f72843; gate exit 0 landed fast-only) | real: strict log (Newton step, 3 calls), exp2, exp10, expm1, sinhcosh (-> sinh, cosh, tanh); real pow uses its own tf_exp_ext. complex: exp (strict body + strict real exp), tan, tanh, pow | TF log, cTF exp | not chained (P3.2[tf]-chain designed) |

## QF

No fast op accepted yet (P3.1[qf] rejected).
