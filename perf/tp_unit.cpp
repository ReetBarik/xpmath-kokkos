// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// Throughput units (GPU_PERF_PLAN P0.3). Compiled 12 times, once per (kind,
// type), like unit.cpp, so the translation units build in parallel and each
// gets its own device assembly. A complex unit also times the science kernel
// for its (real, complex) pair.

#include "throughput_kernels.hpp"

#ifndef XPK_KIND
#error "XPK_KIND, XPK_TYPE_ID and XPK_UNIT_FN must be defined by perf/CMakeLists.txt"
#endif

namespace {
using namespace xpk_perf;
#if XPK_KIND == 0
#if XPK_TYPE_ID == 0
using T = double;
constexpr const char* kName = "double";
#elif XPK_TYPE_ID == 1
using T = float;
constexpr const char* kName = "float";
#elif XPK_TYPE_ID == 2
using T = DD;
constexpr const char* kName = "DD";
#elif XPK_TYPE_ID == 3
using T = FF;
constexpr const char* kName = "FF";
#elif XPK_TYPE_ID == 4
using T = TF;
constexpr const char* kName = "TF";
#elif XPK_TYPE_ID == 5
using T = QF;
constexpr const char* kName = "QF";
#elif defined(XPMATH_HAS_TIERS) && XPK_TYPE_ID == 6
using T = DD_fast;
constexpr const char* kName = "DD_fast";
#elif defined(XPMATH_HAS_TIERS) && XPK_TYPE_ID == 7
using T = FF_fast;
constexpr const char* kName = "FF_fast";
#elif defined(XPMATH_HAS_TIERS) && XPK_TYPE_ID == 8
using T = TF_fast;
constexpr const char* kName = "TF_fast";
#elif defined(XPMATH_HAS_TIERS) && XPK_TYPE_ID == 9
using T = QF_fast;
constexpr const char* kName = "QF_fast";
#else
#error "bad XPK_TYPE_ID"
#endif
#else
#if XPK_TYPE_ID == 0
using T = CD;
constexpr const char* kName = "cdouble";
constexpr const char* kRealName = "double";
#elif XPK_TYPE_ID == 1
using T = CF;
constexpr const char* kName = "cfloat";
constexpr const char* kRealName = "float";
#elif XPK_TYPE_ID == 2
using T = DDC;
constexpr const char* kName = "cDD";
constexpr const char* kRealName = "DD";
#elif XPK_TYPE_ID == 3
using T = FFC;
constexpr const char* kName = "cFF";
constexpr const char* kRealName = "FF";
#elif XPK_TYPE_ID == 4
using T = TFC;
constexpr const char* kName = "cTF";
constexpr const char* kRealName = "TF";
#elif XPK_TYPE_ID == 5
using T = QFC;
constexpr const char* kName = "cQF";
constexpr const char* kRealName = "QF";
#elif defined(XPMATH_HAS_TIERS) && XPK_TYPE_ID == 6
using T = DDC_fast;
constexpr const char* kName = "cDD_fast";
constexpr const char* kRealName = "DD_fast";
#elif defined(XPMATH_HAS_TIERS) && XPK_TYPE_ID == 7
using T = FFC_fast;
constexpr const char* kName = "cFF_fast";
constexpr const char* kRealName = "FF_fast";
#elif defined(XPMATH_HAS_TIERS) && XPK_TYPE_ID == 8
using T = TFC_fast;
constexpr const char* kName = "cTF_fast";
constexpr const char* kRealName = "TF_fast";
#elif defined(XPMATH_HAS_TIERS) && XPK_TYPE_ID == 9
using T = QFC_fast;
constexpr const char* kName = "cQF_fast";
constexpr const char* kRealName = "QF_fast";
#else
#error "bad XPK_TYPE_ID"
#endif
#endif
}  // namespace

void XPK_UNIT_FN(const xpk_tp::Config& cfg, std::vector<xpk_tp::Row>& rows) {
#if XPK_KIND == 0
  run_ops<T>(cfg, kName, rows, RealOps{});
#else
  run_ops<T>(cfg, kName, rows, ComplexOps{});
  if (cfg.science) run_science<ComplexTraits<T>::Real, T>(cfg, kRealName, rows);
#endif
}
