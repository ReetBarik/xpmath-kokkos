#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
"""Per-kernel ISA metrics from -save-temps device assembly (HIP, AMDGPU).

usage: isa_metrics_hip.py <build-dir> <out.csv>

Reads every *-hip-amdgcn-amd-amdhsa-*.s under <build-dir>/perf. Writes
type,op,regs,scratch_bytes,code_bytes,instructions,calls (see perf/README.md).
Exits nonzero unless exactly the 90 expected (type, op) rows are found.
"""
import glob
import os
import re
import subprocess
import sys

REAL_TYPES = [("double", "double"), ("float", "float"), ("xp::DoubleDouble", "DD"),
              ("xp::FloatFloat", "FF"), ("xp::TripleFloat", "TF"), ("xp::QuadFloat", "QF")]
COMPLEX_TYPES = [("Kokkos::complex<double>", "cdouble"), ("Kokkos::complex<float>", "cfloat"),
                 ("xp::DoubleDoubleComplex", "cDD"), ("xp::FloatFloatComplex", "cFF"),
                 ("xp::TripleFloatComplex", "cTF"), ("xp::QuadFloatComplex", "cQF")]
REAL_OPS = ["Add", "Mul", "Div", "Sqrt", "Exp", "Log", "Sin", "Atan2"]
COMPLEX_OPS = ["Add", "Mul", "Div", "Abs", "Sqrt", "Log", "Exp"]

TYPE_NAMES = dict(REAL_TYPES + COMPLEX_TYPES)
KERNEL_RE = re.compile(r"xpk_perf::(RealKernel|ComplexKernel)<(.+?), xpk_perf::op::(\w+)>")
INSN_RE = re.compile(r"^\s+[a-z][a-z0-9_]*(\s|$)")
CALLEE_RE = re.compile(r"\s(_Z[A-Za-z0-9_]+)@rel32@lo")


def parse_asm(path):
    """Return (funcs, kernels). funcs[name] = dict(code, insns, calls, callees)."""
    funcs = {}
    cur = None            # function whose body we are inside
    last_closed = None    # function whose '; codeLenInByte' block comes next
    in_meta = False
    meta_lines = []
    with open(path) as f:
        for line in f:
            if line.startswith("\t.amdgpu_metadata"):
                in_meta = True
                continue
            if line.startswith("\t.end_amdgpu_metadata"):
                in_meta = False
                continue
            if in_meta:
                meta_lines.append(line)
                continue
            m = re.match(r"^(_Z[A-Za-z0-9_]+):", line)
            if m:
                cur = m.group(1)
                funcs[cur] = {"code": 0, "insns": 0, "calls": 0, "callees": set()}
                continue
            if cur is not None:
                if line.startswith(".Lfunc_end"):
                    last_closed = cur
                    cur = None
                    continue
                if INSN_RE.match(line):
                    funcs[cur]["insns"] += 1
                    if line.split()[0] == "s_swappc_b64":
                        funcs[cur]["calls"] += 1
                    c = CALLEE_RE.search(line)
                    if c:
                        funcs[cur]["callees"].add(c.group(1))
                continue
            m = re.match(r"^; codeLenInByte = (\d+)", line)
            if m and last_closed in funcs:
                funcs[last_closed]["code"] = int(m.group(1))
    for name, fn in funcs.items():
        # AMDGPU encodings are 4 or 8 bytes, so this catches a misattributed block.
        if not 4 * fn["insns"] <= fn["code"] <= 8 * fn["insns"]:
            sys.exit("%s: codeLenInByte=%d inconsistent with %d instructions in %s"
                     % (name[:60], fn["code"], fn["insns"], path))
    return funcs, parse_metadata(meta_lines)


def parse_metadata(lines):
    """Split the YAML-ish kernel list into {name: {field: int}} without a YAML parser."""
    kernels = {}
    cur = None
    for line in lines:
        m = re.match(r"^  - \.agpr_count:\s+(\d+)", line)
        if m:
            cur = {"agpr_count": int(m.group(1))}
            continue
        m = re.match(r"^    \.(name|vgpr_count|private_segment_fixed_size):\s+(\S+)", line)
        if m and cur is not None:
            key, val = m.groups()
            if key == "name":
                kernels[val] = cur
            else:
                cur[key] = int(val)
    return kernels


def closure(funcs, root):
    seen, stack = set(), [root]
    while stack:
        n = stack.pop()
        if n in seen or n not in funcs:
            continue
        seen.add(n)
        stack.extend(funcs[n]["callees"])
    return seen


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    build, out = sys.argv[1], sys.argv[2]
    files = sorted(glob.glob(os.path.join(build, "perf", "**", "*-hip-amdgcn-amd-amdhsa-*.s"),
                             recursive=True))
    if not files:
        sys.exit("no device .s files under %s/perf (was the build done with -save-temps?)" % build)

    entries = []  # (mangled, funcs, meta)
    for path in files:
        funcs, kernels = parse_asm(path)
        for name, meta in kernels.items():
            entries.append((name, funcs, meta))

    mangled = [e[0] for e in entries]
    demangled = subprocess.run(["c++filt"], input="\n".join(mangled) + "\n",
                               stdout=subprocess.PIPE, universal_newlines=True,
                               check=True).stdout.splitlines()

    rows = {}
    for (name, funcs, meta), dem in zip(entries, demangled):
        m = KERNEL_RE.search(dem)
        if not m:
            continue  # Kokkos housekeeping kernel (ViewValueFunctor, ...)
        kind, tname, opname = m.groups()
        if tname not in TYPE_NAMES:
            sys.exit("unknown kernel type %r in %s" % (tname, dem))
        key = (TYPE_NAMES[tname], opname.lower())
        if key in rows:
            sys.exit("duplicate kernel for %s" % (key,))
        reach = closure(funcs, name)
        if name not in funcs:
            sys.exit("kernel body not found for %s" % dem)
        rows[key] = (meta["vgpr_count"] + meta["agpr_count"], meta["private_segment_fixed_size"],
                     sum(funcs[n]["code"] for n in reach),
                     sum(funcs[n]["insns"] for n in reach),
                     sum(funcs[n]["calls"] for n in reach))

    expected = [(t, o.lower()) for _, t in REAL_TYPES for o in REAL_OPS] + \
               [(t, o.lower()) for _, t in COMPLEX_TYPES for o in COMPLEX_OPS]
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
