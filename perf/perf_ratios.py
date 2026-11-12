#!/usr/bin/env python3
"""Before/after timing ratios from interleaved perf runs (stdlib only, py3.6+).

Usage: python3 -I perf/perf_ratios.py --arch gfx90a --out ratios_gfx90a.csv \
           --before B1 B2 B3 --after A1 A2 A3

Each directory holds <arch>/throughput_*.csv and <arch>/science_*.csv as
written by the perf runner's --out-dir. Repeat i pairs before[i] with after[i].
Output columns: type,op,kind,before_ns,after_ns,ratio. before_ns/after_ns are
medians over repeats; ratio is the median of the per-repeat after/before
ratios. Science rows use op=science, kind=science. Timing only; no accuracy
claim is made here.
"""
import argparse
import csv
import glob
import os
import statistics
import sys


def _load(directory, arch, prefix, key_cols, val_col):
    paths = sorted(glob.glob(os.path.join(directory, arch, prefix + "_*.csv")))
    if len(paths) != 1:
        sys.exit("error: expected exactly one %s_*.csv in %s/%s, found %d"
                 % (prefix, directory, arch, len(paths)))
    out = {}
    with open(paths[0], newline="") as fh:
        for row in csv.DictReader(fh):
            out[tuple(row[c] for c in key_cols)] = float(row[val_col])
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--arch", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--before", nargs="+", required=True)
    ap.add_argument("--after", nargs="+", required=True)
    args = ap.parse_args()
    if len(args.before) != len(args.after):
        sys.exit("error: --before and --after need the same number of repeats")

    rows = []
    specs = (("throughput", ("type", "op", "kind"), "ns_per_op"),
             ("science", ("type",), "ns_per_element"))
    for prefix, key_cols, val_col in specs:
        before = [_load(d, args.arch, prefix, key_cols, val_col)
                  for d in args.before]
        after = [_load(d, args.arch, prefix, key_cols, val_col)
                 for d in args.after]
        keys = set(before[0])
        for m in before + after:
            if set(m) != keys:
                sys.exit("error: row set differs between runs (%s)" % prefix)
        for key in sorted(keys):
            b = [m[key] for m in before]
            a = [m[key] for m in after]
            ratio = statistics.median([x / y for x, y in zip(a, b)])
            if prefix == "science":
                ident = (key[0], "science", "science")
            else:
                ident = key
            rows.append(ident + (statistics.median(b), statistics.median(a),
                                 ratio))

    with open(args.out, "w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["type", "op", "kind", "before_ns", "after_ns", "ratio"])
        for r in rows:
            w.writerow(list(r[:3]) + ["%.4f" % r[3], "%.4f" % r[4],
                                      "%.4f" % r[5]])
    slow = [r for r in rows if r[5] > 1.03]
    print("wrote %s: %d rows, %d slower than 1.03x" % (args.out, len(rows),
                                                       len(slow)))


if __name__ == "__main__":
    main()
