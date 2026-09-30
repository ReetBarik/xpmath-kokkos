#!/usr/bin/env bash
# Copy this repository's vendored core and Kokkos wrappers into a Kokkos
# source tree. Header and license bytes are copied. The only edit of the
# Kokkos tree is an include-path and install registration appended to
# core/src/CMakeLists.txt, marked below. Wrapper text and xp header text
# are not rewritten.
#
# Usage:
#   scripts/materialize_kokkos_pr.sh <kokkos-source-tree>
#
# Prints the copied file list, one path per line, relative to the Kokkos tree.
set -euo pipefail

if [ "${1:-}" = "" ]; then
  echo "usage: scripts/materialize_kokkos_pr.sh <kokkos-source-tree>" >&2
  exit 2
fi

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
kokkos="$(cd "$1" && pwd)"

case "$kokkos" in
  "$root"|"$root"/*)
    echo "materialize_kokkos_pr: refuse — Kokkos tree must be outside this repository." >&2
    exit 1
    ;;
esac

if [ ! -f "$kokkos/core/src/CMakeLists.txt" ] || [ ! -d "$kokkos/tpls" ]; then
  echo "materialize_kokkos_pr: $kokkos does not look like a Kokkos source tree." >&2
  exit 1
fi

if [ ! -f "$root/vendor/xpmath/UPSTREAM.txt" ]; then
  echo "materialize_kokkos_pr: vendor/xpmath is missing. Sync a tag first." >&2
  exit 1
fi

rm -rf "$kokkos/tpls/xpmath" "$kokkos/core/src/Kokkos_xpmath"
mkdir -p "$kokkos/tpls/xpmath"
cp -a "$root/vendor/xpmath/xp" "$kokkos/tpls/xpmath/xp"
cp -a "$root/vendor/xpmath/LICENSES" "$kokkos/tpls/xpmath/LICENSES"
cp -a "$root/vendor/xpmath/UPSTREAM.txt" "$kokkos/tpls/xpmath/UPSTREAM.txt"
cp -a "$root/include/Kokkos_xpmath" "$kokkos/core/src/Kokkos_xpmath"

# Byte-copy check. A difference here means cp did not preserve the tree.
diff -rq "$root/vendor/xpmath/xp" "$kokkos/tpls/xpmath/xp" >/dev/null
diff -rq "$root/vendor/xpmath/LICENSES" "$kokkos/tpls/xpmath/LICENSES" >/dev/null
diff -q "$root/vendor/xpmath/UPSTREAM.txt" "$kokkos/tpls/xpmath/UPSTREAM.txt" >/dev/null
diff -rq "$root/include/Kokkos_xpmath" "$kokkos/core/src/Kokkos_xpmath" >/dev/null

marker="BEGIN xpmath materialize_kokkos_pr.sh"
cmake_lists="$kokkos/core/src/CMakeLists.txt"
if ! grep -q "$marker" "$cmake_lists"; then
  cat >>"$cmake_lists" <<'EOF'

# BEGIN xpmath materialize_kokkos_pr.sh
# Header and license files under tpls/xpmath and core/src/Kokkos_xpmath are
# byte copies from xpmath-kokkos. This block is the only Kokkos-tree edit:
# register the include path and install the vendored headers, the same kind
# of registration mdspan and desul already have in this file. No wrapper or
# xp source text is rewritten.
target_include_directories(
  kokkoscore PUBLIC $<BUILD_INTERFACE:${KOKKOS_SOURCE_DIR}/tpls/xpmath>
)
install(
  DIRECTORY "${KOKKOS_SOURCE_DIR}/tpls/xpmath/xp"
  DESTINATION ${KOKKOS_HEADER_DIR}
  FILES_MATCHING
  PATTERN "*.hpp"
)
install(
  DIRECTORY "${KOKKOS_SOURCE_DIR}/tpls/xpmath/LICENSES"
  DESTINATION ${KOKKOS_HEADER_DIR}/xpmath
)
install(
  FILES "${KOKKOS_SOURCE_DIR}/tpls/xpmath/UPSTREAM.txt"
  DESTINATION ${KOKKOS_HEADER_DIR}/xpmath
)
# END xpmath materialize_kokkos_pr.sh
EOF
fi

echo "materialize_kokkos_pr: byte copy into $kokkos"
echo "materialize_kokkos_pr: CMake registration is the only non-copy edit (core/src/CMakeLists.txt)"
(cd "$kokkos" && find tpls/xpmath core/src/Kokkos_xpmath -type f | sort)
