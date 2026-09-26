#!/usr/bin/env bash
# Compile one translation unit per wrapper header for DEVICE_SPACE.
# Compile only: the binaries are not executed (no GPU).
# DEVICE_SPACE is Cuda or HIP. KOKKOS_PREFIX is an installed Kokkos 5.1 tree.
set -euo pipefail

: "${DEVICE_SPACE:?DEVICE_SPACE is required (Cuda or HIP)}"
: "${KOKKOS_PREFIX:?KOKKOS_PREFIX is required}"

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

if [ "${DEVICE_SPACE}" = "Cuda" ]; then
  cxx="${KOKKOS_PREFIX}/bin/nvcc_wrapper"
else
  cxx="${CXX:-hipcc}"
fi

cat > "${work}/CMakeLists.txt" <<EOF
cmake_minimum_required(VERSION 3.22)
project(device_smoke LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Kokkos 5.1 REQUIRED CONFIG)
EOF

# header kind type ctor-args
rows=(
  "dd_math real DoubleDouble 0.5"
  "ff_math real FloatFloat 0.5"
  "qf_math real QuadFloat 0.5"
  "tf_math real TripleFloat 0.5"
  "dd_complex complex DoubleDoubleComplex 0.25,0.125"
  "ff_complex complex FloatFloatComplex 0.25f,0.125f"
  "qf_complex complex QuadFloatComplex 0.25f,0.125f"
  "tf_complex complex TripleFloatComplex 0.25f,0.125f"
)

expected=8
if [ "${#rows[@]}" -ne "${expected}" ]; then
  echo "::error::expected ${expected} device TUs, found ${#rows[@]}"
  exit 1
fi

for row in "${rows[@]}"; do
  read -r hdr kind type args <<<"${row}"
  src="${work}/${hdr}.cpp"
  if [ "${kind}" = "real" ]; then
    cat > "${src}" <<EOF
#include <Kokkos_Core.hpp>
#include <Kokkos_xpmath/${hdr}.hpp>
int main() {
  Kokkos::initialize();
  Kokkos::parallel_for(
      Kokkos::RangePolicy<Kokkos::${DEVICE_SPACE}>(0, 1),
      KOKKOS_LAMBDA(int) {
        Kokkos::Experimental::${type} x(${args});
        auto y = Kokkos::sqrt(Kokkos::exp(x));
        (void)y;
      });
  Kokkos::finalize();
}
EOF
  else
    cat > "${src}" <<EOF
#include <Kokkos_Core.hpp>
#include <Kokkos_xpmath/${hdr}.hpp>
int main() {
  Kokkos::initialize();
  Kokkos::parallel_for(
      Kokkos::RangePolicy<Kokkos::${DEVICE_SPACE}>(0, 1),
      KOKKOS_LAMBDA(int) {
        Kokkos::Experimental::${type} z(${args});
        z = Kokkos::exp(z);
        z.re = Kokkos::real(z);
        z.im = Kokkos::imag(z);
        (void)z;
      });
  Kokkos::finalize();
}
EOF
  fi
  cat >> "${work}/CMakeLists.txt" <<EOF
add_executable(tu_${hdr} ${hdr}.cpp)
target_link_libraries(tu_${hdr} PRIVATE Kokkos::kokkos)
target_include_directories(tu_${hdr} PRIVATE
  "${root}/include" "${root}/vendor/xpmath")
EOF
done

echo "=== configuring ${DEVICE_SPACE} device TUs (${expected} headers) ==="
cmake -S "${work}" -B "${work}/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_CXX_COMPILER="${cxx}" \
  -DCMAKE_PREFIX_PATH="${KOKKOS_PREFIX}"

for row in "${rows[@]}"; do
  read -r hdr _kind _type _args <<<"${row}"
  echo "=== ${DEVICE_SPACE} compile ${hdr} ==="
  cmake --build "${work}/build" --target "tu_${hdr}" -j"$(nproc)"
done

echo "device TUs: ${expected} compiled for ${DEVICE_SPACE} (not executed)"
