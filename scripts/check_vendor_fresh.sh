#!/usr/bin/env bash
# Staleness guard for vendor/xpmath/.
#
# Re-fetches the tag recorded in vendor/xpmath/UPSTREAM.txt, diffs include/xp/
# and LICENSES/ against the vendored copies, and fails on any difference.
# Mirrors xpmath's domains_fresh idea: the committed vendor tree must be
# exactly what the recorded tag currently contains.
#
#   scripts/check_vendor_fresh.sh        # exit 0 if fresh, 1 if stale
#
# Needs git and network access to https://github.com/ReetBarik/xpmath.git.
# No build, no Kokkos.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
vendor="$root/vendor/xpmath"
upstream_txt="$vendor/UPSTREAM.txt"

if [ ! -f "$upstream_txt" ]; then
  echo "check_vendor_fresh: FAIL — $upstream_txt does not exist" >&2
  exit 1
fi

tag="$(awk -F': ' '/^tag:/{print $2; exit}' "$upstream_txt")"
recorded_sha="$(awk -F': ' '/^commit:/{print $2; exit}' "$upstream_txt")"
url="$(awk -F': ' '/^repository:/{print $2; exit}' "$upstream_txt")"

if [ -z "$tag" ] || [ -z "$recorded_sha" ] || [ -z "$url" ]; then
  echo "check_vendor_fresh: FAIL — could not parse tag/commit/repository from $upstream_txt" >&2
  cat "$upstream_txt" >&2
  exit 1
fi

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

echo "check_vendor_fresh: re-fetching $url at tag $tag ..."
if ! git clone --quiet --depth 1 --branch "$tag" "$url" "$tmp/xpmath"; then
  echo "check_vendor_fresh: FAIL — could not re-fetch tag $tag; fix the script or network, not the recorded SHA." >&2
  exit 2
fi

fetched_sha="$(git -C "$tmp/xpmath" rev-parse HEAD)"
if [ "$fetched_sha" != "$recorded_sha" ]; then
  echo "check_vendor_fresh: FAIL — tag $tag now points at $fetched_sha, UPSTREAM.txt records $recorded_sha." >&2
  echo "  Re-run scripts/sync_upstream.sh $tag and commit the result." >&2
  exit 1
fi

fail=0

if [ ! -d "$tmp/xpmath/include/xp" ]; then
  echo "check_vendor_fresh: FAIL — fetched tree has no include/xp/" >&2
  exit 1
fi
if [ ! -d "$vendor/xp" ]; then
  echo "check_vendor_fresh: FAIL — vendor/xpmath/xp/ missing" >&2
  exit 1
fi

if ! diff -ruN "$tmp/xpmath/include/xp" "$vendor/xp" > "$tmp/xp.diff"; then
  echo "check_vendor_fresh: FAIL — vendor/xpmath/xp/ drifts from $tag include/xp/" >&2
  head -80 "$tmp/xp.diff" >&2
  fail=1
fi

if [ ! -d "$tmp/xpmath/LICENSES" ]; then
  echo "check_vendor_fresh: FAIL — fetched tree has no LICENSES/" >&2
  exit 1
fi
if [ ! -d "$vendor/LICENSES" ]; then
  echo "check_vendor_fresh: FAIL — vendor/xpmath/LICENSES/ missing" >&2
  exit 1
fi

if ! diff -ruN "$tmp/xpmath/LICENSES" "$vendor/LICENSES" > "$tmp/lic.diff"; then
  echo "check_vendor_fresh: FAIL — vendor/xpmath/LICENSES/ drifts from $tag LICENSES/" >&2
  head -80 "$tmp/lic.diff" >&2
  fail=1
fi

if [ "$fail" -ne 0 ]; then
  echo "check_vendor_fresh: FAIL — vendor tree is stale or hand-edited." >&2
  echo "  Fix with: scripts/sync_upstream.sh $tag" >&2
  exit 1
fi

echo "check_vendor_fresh: PASS — vendor/xpmath matches $tag ($recorded_sha)"
exit 0
