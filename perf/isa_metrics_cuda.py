#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
"""Per-kernel ISA metrics from CUDA objects (cuobjdump -res-usage / -sass).

usage: isa_metrics_cuda.py <build-dir> <out.csv> <sm_XX>

Reads every perf/CMakeFiles/xpk_unit_*.dir/unit.cpp.o under <build-dir>. Writes
the same columns as the HIP script (see perf/README.md). Without relocatable
device code, called device functions live inside the kernel's own SASS
listing, so summing that listing covers everything the kernel can reach.
Exits nonzero unless exactly the 90 expected (type, op) rows are found.
"""
import glob
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import isa_metrics_hip as common  # noqa: E402  (shared type/op tables and KERNEL_RE)

RES_FN_RE = re.compile(r"^\s+Function (\S+):\s*$")
RES_VAL_RE = re.compile(r"^\s+REG:(\d+) STACK:(\d+) SHARED:\d+ LOCAL:(\d+)")
SASS_FN_RE = re.compile(r"^\s+Function : (\S+)\s*$")
INSN_RE = re.compile(r"^\s+/\*([0-9a-f]{4,})\*/\s+(?:@!?U?P\w+\s+)?([A-Z][A-Z0-9_.]*)")
INSN_BYTES = 16  # sm_70 and later


def run(cmd):
    return subprocess.run(cmd, stdout=subprocess.PIPE, universal_newlines=True,
                          check=True).stdout.splitlines()


def parse_res(lines):
    res, cur = {}, None
    for line in lines:
        m = RES_FN_RE.match(line)
        if m:
            cur = m.group(1)
            continue
        m = RES_VAL_RE.match(line)
        if m and cur:
            reg, stack, local = (int(x) for x in m.groups())
            res[cur] = (reg, stack, local)
            cur = None
    return res


def parse_sass(lines):
    """Return {name: (code_bytes, instructions, calls)}."""
    out, cur = {}, None
    for line in lines:
        m = SASS_FN_RE.match(line)
        if m:
            cur = m.group(1)
            out[cur] = [0, 0, 0]  # last_addr_end, insns, calls
            continue
        if cur is None:
            continue
        m = INSN_RE.match(line)
        if m:
            fn = out[cur]
            fn[0] = int(m.group(1), 16) + INSN_BYTES
            fn[1] += 1
            if m.group(2).startswith("CALL"):
                fn[2] += 1
    for name, (end, insns, _) in out.items():
        if end != insns * INSN_BYTES:
            sys.exit("%s: code end %d != %d instructions x %d bytes"
                     % (name[:60], end, insns, INSN_BYTES))
    return {n: tuple(v) for n, v in out.items()}


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    build, out, arch = sys.argv[1:]
    objs = sorted(glob.glob(os.path.join(build, "perf", "CMakeFiles", "xpk_unit_*.dir", "unit.cpp.o")))
    if not objs:
        sys.exit("no unit objects under %s/perf" % build)

    entries = []  # (mangled, res, sass)
    for obj in objs:
        res = parse_res(run(["cuobjdump", "-arch", arch, "-res-usage", obj]))
        sass = parse_sass(run(["cuobjdump", "-arch", arch, "-sass", obj]))
        if set(res) != set(sass):
            sys.exit("%s: res-usage and sass list different functions" % obj)
        for name in res:
            entries.append((name, res[name], sass[name]))

    demangled = subprocess.run(["c++filt"], input="\n".join(e[0] for e in entries) + "\n",
                               stdout=subprocess.PIPE, universal_newlines=True,
                               check=True).stdout.splitlines()

    rows = {}
    for (name, (reg, stack, local), (code, insns, calls)), dem in zip(entries, demangled):
        m = common.KERNEL_RE.search(dem)
        if not m:
            continue  # Kokkos housekeeping kernel
        _, tname, opname = m.groups()
        if tname not in common.TYPE_NAMES:
            sys.exit("unknown kernel type %r in %s" % (tname, dem))
        key = (common.TYPE_NAMES[tname], opname.lower())
        if key in rows:
            sys.exit("duplicate kernel for %s" % (key,))
        rows[key] = (reg, max(stack, local), code, insns, calls)

    expected = [(t, o.lower()) for _, t in common.REAL_TYPES for o in common.REAL_OPS] + \
               [(t, o.lower()) for _, t in common.COMPLEX_TYPES for o in common.COMPLEX_OPS]
    missing = [k for k in expected if k not in rows]
    extra = [k for k in rows if k not in expected]
    if missing or extra:
        sys.exit("kernel set mismatch: missing=%s extra=%s" % (missing, extra))

    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    with open(out, "w") as f:
        f.write("type,op,regs,scratch_bytes,code_bytes,instructions,calls\n")
        for k in expected:
            f.write("%s,%s,%s\n" % (k[0], k[1], ",".join(str(v) for v in rows[k])))
    print("wrote %s (%d rows)" % (out, len(expected)))


if __name__ == "__main__":
    main()
