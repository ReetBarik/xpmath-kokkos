#!/usr/bin/env bash
# Governing-rule guard. See the no-oracle-guard job in .github/workflows/ci.yml.
#
# Comment stripping is deliberate. A comment that explains the rule names the
# forbidden tokens; naming them is not a dependency, and this check must not
# fail on that explanation. The same exemption covers the Markdown files that
# state the rule in prose (they have no comment syntax): README.md, CLAUDE.md,
# and docs/KOKKOS_LAYER_PLAN_STATUS.md. This script is not scanned either,
# because it contains the search pattern.
#
# "Test source" for the ulp/digits scan is tests/**/*.{cpp,hpp,h,cc,cxx,cu,hip}.
# tests/data/ is the vendored grid (an operation column is named ulp). That is
# data, not source, and it is not an accuracy verdict.
#
# validation/ is scanned for mpfr, mpc_, and __float128. The quadmath token is
# not applied there: committed logs and the A100 launch script record a host
# install directory whose name contains that substring.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "check_no_oracle: FAIL — not a git checkout" >&2
  exit 1
fi

exec python3 - "$root" <<'PY'
import re
import subprocess
import sys
from pathlib import Path

root = Path(sys.argv[1])
code_pat = re.compile(r"mpfr|mpc_|__float128|quadmath")
test_pat = re.compile(r"ulp|digits")
test_src = {".cpp", ".hpp", ".h", ".cc", ".cxx", ".cu", ".hip"}
exempt = {
    "scripts/check_no_oracle.sh",
    "README.md",
    "CLAUDE.md",
    "docs/KOKKOS_LAYER_PLAN_STATUS.md",
}

def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    out = []
    for line in text.splitlines():
        if re.match(r"^[ \t]*#", line):
            continue
        line = re.sub(r"//.*$", "", line)
        out.append(line)
    return "\n".join(out)

files = subprocess.check_output(["git", "ls-files", "-z"], cwd=root)
paths = [p.decode() for p in files.split(b"\0") if p]
fail = 0
scanned = 0
for rel in paths:
    if rel.startswith("vendor/") or rel in exempt:
        continue
    path = root / rel
    if not path.is_file():
        continue
    raw = path.read_bytes()
    if b"\0" in raw:
        continue
    text = strip_comments(raw.decode("utf-8", errors="replace"))
    scanned += 1
    in_validation = rel.startswith("validation/")
    for i, line in enumerate(text.splitlines(), 1):
        m = code_pat.search(line)
        if m and not (in_validation and m.group(0) == "quadmath"):
            print(f"::error file={rel},line={i}::{m.group(0)} outside a comment")
            print(f"{rel}:{i}:{line.strip()[:200]}")
            fail = 1
    if rel.startswith("tests/") and Path(rel).suffix in test_src:
        for i, line in enumerate(text.splitlines(), 1):
            m = test_pat.search(line)
            if m:
                print(f"::error file={rel},line={i}::test source contains {m.group(0)}")
                print(f"{rel}:{i}:{line.strip()[:200]}")
                fail = 1

if scanned == 0:
    print("::error::no files scanned — wrong cwd or empty checkout")
    sys.exit(1)
if fail:
    print("check_no_oracle: FAIL")
    sys.exit(1)
print(f"check_no_oracle: PASS ({scanned} files, vendor/ exempt, comments stripped)")
PY
