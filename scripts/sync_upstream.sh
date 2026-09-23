#!/usr/bin/env bash
# Vendor a named xpmath tag into vendor/xpmath/.
#
# Usage:
#   scripts/sync_upstream.sh <tag>
#
# Clones https://github.com/ReetBarik/xpmath at <tag>, copies include/xp/ and
# LICENSES/ into vendor/xpmath/, and rewrites vendor/xpmath/UPSTREAM.txt.
# Refuses to run when vendor/xpmath already has committed content with local
# modifications — vendored files are never hand-edited.
set -euo pipefail

if [ "${1:-}" = "" ]; then
  echo "usage: scripts/sync_upstream.sh <tag>" >&2
  exit 2
fi

tag="$1"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
vendor="$root/vendor/xpmath"
upstream_url="https://github.com/ReetBarik/xpmath.git"

cd "$root"

# Refuse dirty vendor/ once it has been committed. First sync (nothing tracked
# under vendor/xpmath yet) is allowed so the tree can be populated.
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  if git ls-files --error-unmatch vendor/xpmath/UPSTREAM.txt >/dev/null 2>&1; then
    if [ -n "$(git status --porcelain -- vendor/xpmath)" ]; then
      echo "sync_upstream: refuse — vendor/xpmath is dirty." >&2
      echo "  Commit or restore vendor/ before re-syncing. Never hand-edit vendored files." >&2
      git status --porcelain -- vendor/xpmath >&2
      exit 1
    fi
  fi
fi

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

echo "sync_upstream: cloning $upstream_url at tag $tag ..."
git clone --quiet --depth 1 --branch "$tag" "$upstream_url" "$tmp/xpmath"

# Annotated tags peel to the commit; record the full SHA of the tree we copied.
full_sha="$(git -C "$tmp/xpmath" rev-parse HEAD)"
utc="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

if [ ! -d "$tmp/xpmath/include/xp" ]; then
  echo "sync_upstream: FAIL — $tag has no include/xp/" >&2
  exit 1
fi
if [ ! -d "$tmp/xpmath/LICENSES" ]; then
  echo "sync_upstream: FAIL — $tag has no LICENSES/" >&2
  exit 1
fi

mkdir -p "$vendor"
rm -rf "$vendor/xp" "$vendor/LICENSES"
cp -a "$tmp/xpmath/include/xp" "$vendor/xp"
cp -a "$tmp/xpmath/LICENSES" "$vendor/LICENSES"

cat > "$vendor/UPSTREAM.txt" <<EOF
repository: $upstream_url
tag: $tag
commit: $full_sha
synced_utc: $utc
EOF

echo "sync_upstream: PASS — vendored $tag ($full_sha) at $utc"
echo "  xp/      -> vendor/xpmath/xp/"
echo "  LICENSES -> vendor/xpmath/LICENSES/"
echo "  record   -> vendor/xpmath/UPSTREAM.txt"
