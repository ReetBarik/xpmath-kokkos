// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// One translation unit per (kind, type). CMake compiles this file twelve
// times with XPK_KIND (0 real, 1 complex), XPK_TYPE_ID and XPK_UNIT_FN set,
// so the device compiler can build the units in parallel and each unit's
// assembly is a separate file.

#include "kernels.hpp"

#ifndef XPK_KIND
#error "XPK_KIND, XPK_TYPE_ID and XPK_UNIT_FN must be defined by perf/CMakeLists.txt"
#endif

namespace {
#if XPK_KIND == 0
#if XPK_TYPE_ID == 0
using T = double;
#elif XPK_TYPE_ID == 1
using T = float;
#elif XPK_TYPE_ID == 2
using T = xpk_perf::DD;
#elif XPK_TYPE_ID == 3
using T = xpk_perf::FF;
#elif XPK_TYPE_ID == 4
using T = xpk_perf::TF;
#elif XPK_TYPE_ID == 5
using T = xpk_perf::QF;
#else
#error "bad XPK_TYPE_ID"
#endif
#else
#if XPK_TYPE_ID == 0
using T = xpk_perf::CD;
#elif XPK_TYPE_ID == 1
using T = xpk_perf::CF;
#elif XPK_TYPE_ID == 2
using T = xpk_perf::DDC;
#elif XPK_TYPE_ID == 3
using T = xpk_perf::FFC;
#elif XPK_TYPE_ID == 4
using T = xpk_perf::TFC;
#elif XPK_TYPE_ID == 5
using T = xpk_perf::QFC;
#else
#error "bad XPK_TYPE_ID"
#endif
#endif
}  // namespace

void XPK_UNIT_FN(int n) {
#if XPK_KIND == 0
  xpk_perf::run_real<T>(n);
#else
  xpk_perf::run_complex<T>(n);
#endif
}
