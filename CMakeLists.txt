cmake_minimum_required(VERSION 3.16)

# All new code in this repository is Apache-2.0 WITH LLVM-exception.
# Vendored xpmath under vendor/xpmath/ keeps its upstream licenses — see NOTICE.md.
project(xpmath-kokkos VERSION 0.1.0 LANGUAGES CXX)

# Kokkos ≥5.1 installs are often built as C++20; this project stays C++17 on
# purpose. Those two numbers must remain different.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Kokkos 5.1 REQUIRED CONFIG)

add_library(xpmath_kokkos INTERFACE)
add_library(xpmath_kokkos::xpmath_kokkos ALIAS xpmath_kokkos)
target_include_directories(xpmath_kokkos INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/xpmath>
)
target_link_libraries(xpmath_kokkos INTERFACE Kokkos::kokkos)
target_compile_features(xpmath_kokkos INTERFACE cxx_std_17)

enable_testing()

# Anti-rot: vendor/xpmath must match the tag recorded in UPSTREAM.txt.
# Mirrors xpmath's domains_fresh — no build artifacts required.
add_test(
  NAME vendor_fresh
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_vendor_fresh.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(vendor_fresh PROPERTIES
  LABELS "host;vendor"
  TIMEOUT 120
)

# K1 fills include/Kokkos_xpmath/ and tests/. examples/ filled later.
