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

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(xpmath_kokkos INTERFACE)
add_library(xpmath_kokkos::xpmath_kokkos ALIAS xpmath_kokkos)
target_include_directories(xpmath_kokkos INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor/xpmath>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(xpmath_kokkos INTERFACE Kokkos::kokkos)
target_compile_features(xpmath_kokkos INTERFACE cxx_std_17)

# ---------------------------------------------------------------------------
# Install + package-config (find_package(xpmath_kokkos))
# ---------------------------------------------------------------------------
# Consumers need both Kokkos_xpmath/ and xp/ on the include path. Install the
# wrappers under include/Kokkos_xpmath/ and the vendored core under include/xp/
# so #include <xp/...> matches the BUILD_INTERFACE layout.
install(DIRECTORY include/Kokkos_xpmath
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY vendor/xpmath/xp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES vendor/xpmath/UPSTREAM.txt
  DESTINATION ${CMAKE_INSTALL_DATADIR}/xpmath-kokkos
)
install(DIRECTORY vendor/xpmath/LICENSES
  DESTINATION ${CMAKE_INSTALL_DATADIR}/xpmath-kokkos
)

install(TARGETS xpmath_kokkos EXPORT xpmath_kokkosTargets)

install(EXPORT xpmath_kokkosTargets
  FILE xpmath_kokkosTargets.cmake
  NAMESPACE xpmath_kokkos::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xpmath_kokkos
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/xpmath_kokkosConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/xpmath_kokkosConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xpmath_kokkos
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/xpmath_kokkosConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/xpmath_kokkosConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/xpmath_kokkosConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xpmath_kokkos
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()

# Anti-rot: vendor/xpmath must match the tag recorded in UPSTREAM.txt.
add_test(
  NAME vendor_fresh
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_vendor_fresh.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(vendor_fresh PROPERTIES
  LABELS "host;vendor"
  TIMEOUT 120
)

# Per-header compile smokes (ctest targets).
set(_XPMATH_KOKKOS_SMOKES
  dd_math
  dd_complex
  ff_math
  ff_complex
  qf_math
  qf_complex
  tf_math
  tf_complex
)
foreach(_name IN LISTS _XPMATH_KOKKOS_SMOKES)
  add_executable(compile_smoke_${_name} tests/compile_smoke_${_name}.cpp)
  target_link_libraries(compile_smoke_${_name} PRIVATE xpmath_kokkos::xpmath_kokkos)
  add_test(NAME compile_smoke_${_name} COMMAND compile_smoke_${_name})
  set_tests_properties(compile_smoke_${_name} PROPERTIES LABELS "host;compile")
endforeach()

# K2: reduction_identity bit-identity (Serial). Suite = vendor_fresh + 8 smokes + this.
add_executable(reduction_test tests/reduction_test.cpp)
target_link_libraries(reduction_test PRIVATE xpmath_kokkos::xpmath_kokkos)
add_test(NAME reduction_test COMMAND reduction_test)
set_tests_properties(reduction_test PROPERTIES LABELS "host;reduction")

# K3: atomic_add CAS bit-identity + exact-integer lost-update gate (Serial).
add_executable(atomic_test tests/atomic_test.cpp)
target_link_libraries(atomic_test PRIVATE xpmath_kokkos::xpmath_kokkos)
add_test(NAME atomic_test COMMAND atomic_test)
set_tests_properties(atomic_test PROPERTIES LABELS "host;atomics")

# Gate extras from K1: build-only TUs (not ctest).
add_executable(compile_gate_all_forwards tests/compile_gate_all_forwards.cpp)
target_link_libraries(compile_gate_all_forwards PRIVATE xpmath_kokkos::xpmath_kokkos)

add_executable(compile_gate_real_imag tests/compile_gate_real_imag.cpp)
target_link_libraries(compile_gate_real_imag PRIVATE xpmath_kokkos::xpmath_kokkos)
