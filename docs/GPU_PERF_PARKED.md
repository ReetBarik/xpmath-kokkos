# GPU_PERF parked-ideas register

Rules: `docs/GPU_PERF_PLAN.md` §1.10. A parked idea is a change that measured
neutral (within ±3%) when tried. It is not landed, but a later task is
expected to change its value. Every session reads this file. When a row's
**Re-test at** names your task, you run that row's A/B at the end of your
task (§1.10) and update the row.

| # | Idea | Parked by | Change (where to get it) | Bit-preserving | Measured when parked | Re-test at | Status |
|---|---|---|---|---|---|---|---|
| 1 | Inline TF multiply/divide/sqrt (FULL) | P2.2[tf] (user exception: not neutral) | xpmath `wip/P2.2-tf` = ddd5701 (base 4d597fd): in tf_math.hpp multiply, divide, sqrt `XPMATH_NOINLINE_FUNCTION`->`XPMATH_INLINE_FUNCTION`; renorm() result-slot copies at acosh (log branch), xp_asin_imag_mag, xp_atan2_safe | yes | TF science after/before: A100 1.646, GB300 1.383, MI250 0.735, MI300 0.695; NVIDIA instruction-fetch bound, TF science code A100 695,808->1,469,312 B; logs $HOME/gpuperf-scratch/P2.2[tf]/ | P3.4[tf] | open |
