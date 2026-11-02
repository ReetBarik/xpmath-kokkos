# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
#
# Perf harness (GPU_PERF_PLAN P0.1). Included from the top-level
# CMakeLists.txt only when -DXPMATH_KOKKOS_BUILD_PERF=ON. It registers NO
# ctest targets: CI asserts exactly 13.

set(XPMATH_KOKKOS_PERF_XP_DIR "" CACHE PATH
  "Local xpmath checkout whose include/ replaces vendor/xpmath for the perf harness")

if(XPMATH_KOKKOS_PERF_XP_DIR)
  get_filename_component(_xpk_xp_root "${XPMATH_KOKKOS_PERF_XP_DIR}" ABSOLUTE)
  set(_xpk_xp_inc "${_xpk_xp_root}/include")
  if(NOT EXISTS "${_xpk_xp_inc}/xp/dd_math.hpp")
    message(FATAL_ERROR
      "XPMATH_KOKKOS_PERF_XP_DIR=${_xpk_xp_root}: no include/xp/dd_math.hpp")
  endif()
  execute_process(
    COMMAND git -C "${_xpk_xp_root}" describe --tags --always --dirty
    OUTPUT_VARIABLE _xpk_xp_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _xpk_git_rc)
  if(NOT _xpk_git_rc EQUAL 0 OR _xpk_xp_version STREQUAL "")
    set(_xpk_xp_version "local")
  endif()
else()
  set(_xpk_xp_inc "${PROJECT_SOURCE_DIR}/vendor/xpmath")
  file(STRINGS "${PROJECT_SOURCE_DIR}/vendor/xpmath/UPSTREAM.txt" _xpk_tag_line
       REGEX "^tag: ")
  string(REGEX REPLACE "^tag: *" "" _xpk_xp_version "${_xpk_tag_line}")
endif()
message(STATUS "xpk_perf: xpmath headers from ${_xpk_xp_inc} (${_xpk_xp_version})")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/xp_version.txt" "${_xpk_xp_version}\n")

# One object library per (kind, type) so units compile in parallel and each
# gets its own assembly file.
set(_xpk_units)
foreach(_kind r c)
  if(_kind STREQUAL "r")
    set(_kind_id 0)
  else()
    set(_kind_id 1)
  endif()
  foreach(_id RANGE 0 5)
    set(_name xpk_unit_${_kind}${_id})
    add_library(${_name} OBJECT unit.cpp)
    target_include_directories(${_name} PRIVATE
      "${PROJECT_SOURCE_DIR}/include"
      "${_xpk_xp_inc}")
    target_compile_definitions(${_name} PRIVATE
      XPK_KIND=${_kind_id}
      XPK_TYPE_ID=${_id}
      XPK_UNIT_FN=xpk_run_unit_${_kind}${_id}
      XPMATH_ENABLE_DIAGNOSTICS=0)
    target_link_libraries(${_name} PRIVATE Kokkos::kokkos)
    if(Kokkos_ENABLE_HIP)
      # Keep device assembly next to each object for isa_metrics.sh.
      target_compile_options(${_name} PRIVATE -save-temps=obj)
    endif()
    list(APPEND _xpk_units $<TARGET_OBJECTS:${_name}>)
  endforeach()
endforeach()

add_executable(xpk_perf_isa perf_main.cpp ${_xpk_units})
target_link_libraries(xpk_perf_isa PRIVATE Kokkos::kokkos)
