#!/usr/bin/env python3
"""Generate docs/PERF_TABLE.md from the latest perf/results/<arch>/ CSVs.

Usage: perf/gen_cost_table.py [--date YYYY-MM-DD] [--out docs/PERF_TABLE.md]
Performance only; makes no accuracy statement.
"""
import argparse
import csv
import datetime
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "perf" / "results"

GPUS = [  # (arch, name)
    ("sm_80", "A100"),
    ("sm_100", "B200"),
    ("sm_103", "GB300"),
    ("gfx90a", "MI250 (one GCD)"),
    ("gfx942", "MI300"),
]
TYPES = ["double", "float", "DD", "FF", "TF", "QF", "cDD", "cFF", "cTF", "cQF", "cdouble", "cfloat"]
SCIENCE_TYPES = ["double", "float", "DD", "FF", "TF", "QF"]


def vkey(p):
    m = re.search(r"_v(\d+(?:\.\d+)*)\.csv$", p.name)
    return tuple(int(x) for x in m.group(1).split(".")) if m else ()


def latest(arch, stem):
    files = sorted((RESULTS / arch).glob(f"{stem}_v*.csv"), key=vkey)
    return files[-1] if files else None


def rows(path):
    if path is None:
        return []
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def fnum(s, nd=2):
    return f"{float(s):.{nd}f}"


def ns(s):
    v = float(s)
    return f"{v:.2e}" if v < 0.01 else f"{v:.4g}"


def ratio(s):
    v = float(s)
    return f"{v:.3g}" if v < 100 else f"{v:.0f}"


def tag():
    for line in (ROOT / "vendor/xpmath/UPSTREAM.txt").read_text().splitlines():
        if line.startswith("tag:"):
            return line.split(":", 1)[1].strip()
    return "unknown"


def type_order(t):
    return TYPES.index(t) if t in TYPES else len(TYPES)


def gpu_section(arch, name):
    tp_path = latest(arch, "throughput")
    out = [f"## {name} ({arch})", ""]
    if tp_path is None:
        out += ["**NO DATA yet** (no P0.6 throughput baseline for this GPU).", ""]
        return out
    isa_path = latest(arch, "isa")
    sci_path = latest(arch, "science")
    sci_isa_path = latest(arch, "science_isa")
    srcs = [p.name for p in (tp_path, isa_path, sci_path, sci_isa_path) if p]
    out += [f"Sources (perf/results/{arch}/): " + ", ".join(srcs), ""]

    isa = {(r["type"], r["op"]): r for r in rows(isa_path)}
    tp = rows(tp_path)
    tp.sort(key=lambda r: (type_order(r["type"]), r["op"]))
    out += [
        "| type | op | ns/op | x double | x float | regs | stack B |",
        "|---|---|---:|---:|---:|---:|---:|",
    ]
    for r in tp:
        i = isa.get((r["type"], r["op"]))
        regs = i["regs"] if i else "n/a"
        stack = i["scratch_bytes"] if i else "n/a"
        out.append(
            f"| {r['type']} | {r['op']} | {ns(r['ns_per_op'])} | {ratio(r['ratio_double'])} "
            f"| {ratio(r['ratio_float'])} | {regs} | {stack} |"
        )
    out.append("")

    sci = {r["type"]: r for r in rows(sci_path)}
    sci_isa = {r["type"]: r for r in rows(sci_isa_path)}
    out += [
        "Science-shaped kernel (one chain of mixed ops per element):",
        "",
        "| type | ns/element | x double | x float | regs | stack B |",
        "|---|---:|---:|---:|---:|---:|",
    ]
    for t in SCIENCE_TYPES:
        s = sci.get(t)
        if s is None:
            continue
        si = sci_isa.get(t)
        out.append(
            f"| {t} | {ns(s['ns_per_element'])} | {ratio(s['ratio_double'])} "
            f"| {ratio(s['ratio_float'])} | {si['regs'] if si else 'n/a'} "
            f"| {si['scratch_bytes'] if si else 'n/a'} |"
        )
    out.append("")
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--date", default=datetime.date.today().isoformat())
    ap.add_argument("--out", default=str(ROOT / "docs" / "PERF_TABLE.md"))
    a = ap.parse_args()

    have = [n for arch, n in GPUS if latest(arch, "throughput")]
    lack = [n for arch, n in GPUS if not latest(arch, "throughput")]
    out = [
        "# Per-GPU cost table",
        "",
        f"xpmath tag: {tag()}   Generated: {a.date}   Generator: `perf/gen_cost_table.py` (do not hand-edit)",
        "",
        "**Performance only, no accuracy statement.** Accuracy lives in xpmath's `DOMAINS.md`.",
        "",
        "How to read it: `ns/op` is time per operation in a dependent chain (func rows: per call). "
        "`x double` and `x float` divide by the native type on the same GPU, so they are not comparable "
        "across GPUs (native double speed differs a lot); compare absolute `ns/op` across GPUs. "
        "`regs` and `stack B` come from the compiled ISA of the single-op kernel. "
        "Single timing sample per row; expect about 3% run-to-run noise.",
        "",
        "GPUs with data: " + ", ".join(have) + ". "
        + ("NO DATA yet: " + ", ".join(lack) + "." if lack else ""),
        "",
    ]
    for arch, name in GPUS:
        out += gpu_section(arch, name)
    Path(a.out).write_text("\n".join(out).rstrip("\n") + "\n")
    print(f"wrote {a.out}")


if __name__ == "__main__":
    sys.exit(main())
