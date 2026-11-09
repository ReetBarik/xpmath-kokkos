#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Compare two stripped xpmath raw device sweeps limb by limb.

Used by CHECK-DEVICE-IDENTICAL in docs/GPU_PERF_PLAN.md section 1.5. This is a
bit comparison, not an accuracy verdict.

A limb pair passes if its bits are identical, OR if both limbs are NaN
(IEEE 754 section 6.3 gives the sign of a NaN no meaning, and a compiler
may commute operands, which changes only which NaN payload propagates).
A NaN <-> non-NaN change, or any change to a non-NaN limb, fails.

Usage: compare_sweep_limbs.py BEFORE.stripped AFTER.stripped
Inputs: raw sweep files with '#' header lines and timing lines already
removed. Each data row is: backend,kind,op,point,limb,limb,...  where a
limb is 8 hex digits (float) or 16 hex digits (double), or empty.

Exit: 0 pass, 1 fail, 2 the files cannot be compared (row count or keys).
"""
import sys


def is_nan(h):
    v = int(h, 16)
    if len(h) == 8:
        return (v >> 23) & 0xFF == 0xFF and (v & 0x7FFFFF) != 0
    if len(h) == 16:
        return (v >> 52) & 0x7FF == 0x7FF and (v & ((1 << 52) - 1)) != 0
    raise ValueError(f"limb is not 8 or 16 hex digits: {h!r}")


def main(argv):
    if len(argv) != 3:
        print("usage: compare_sweep_limbs.py BEFORE.stripped AFTER.stripped",
              file=sys.stderr)
        return 2
    with open(argv[1]) as f:
        before = f.read().splitlines()
    with open(argv[2]) as f:
        after = f.read().splitlines()
    if len(before) != len(after):
        print(f"CANNOT COMPARE: {len(before)} vs {len(after)} rows")
        return 2

    identical = nan_only = 0
    failing = []
    for x, y in zip(before, after):
        if x == y:
            identical += 1
            continue
        fx, fy = x.split(","), y.split(",")
        if fx[:4] != fy[:4] or len(fx) != len(fy):
            print(f"CANNOT COMPARE: rows do not line up:\n  {x[:120]}\n  {y[:120]}")
            return 2
        ok = True
        for p, q in zip(fx[4:], fy[4:]):
            if p == q:
                continue
            if not (p and q and is_nan(p) and is_nan(q)):
                ok = False
                break
        if ok:
            nan_only += 1
        else:
            failing.append((x, y))

    print(f"rows: {len(before)}  identical: {identical}  "
          f"NaN-vs-NaN only: {nan_only}  failing: {len(failing)}")
    for x, y in failing[:3]:
        print(f"  before: {x[:160]}\n  after : {y[:160]}")
    print("RESULT: PASS" if not failing else "RESULT: FAIL")
    return 0 if not failing else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
