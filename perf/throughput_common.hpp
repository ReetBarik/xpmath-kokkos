// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// Shared by throughput.cpp (driver) and tp_unit.cpp (per-type units). Kokkos
// only; the heavy xpmath headers stay out of the driver translation unit.

#pragma once

#include <Kokkos_Core.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace xpk_tp {

struct Config {
  int n = 1 << 21;       // elements per launch
  int table = 1 << 12;   // input table entries (power of two)
  int warmup = 2;
  int reps = 5;
  bool science = true;
};

// One timed kernel. `kind` is chain (add/mul/div), func, or science.
struct Row {
  std::string type, op, kind;
  int ops_per_elem = 0;
  int n = 0;
  double ms_median = 0, ms_min = 0, ms_max = 0;
  long nonfinite = 0;  // non-finite values in the sink output of the last run
  double ns_per_op() const { return ms_median * 1e6 / (static_cast<double>(n) * ops_per_elem); }
};

// 2 warm-up launches, then `reps` timed launches, each bracketed by fences.
template <class Launch>
void time_launches(const Config& cfg, Launch&& launch, Row& row) {
  Kokkos::fence();
  for (int i = 0; i < cfg.warmup; ++i) launch();
  Kokkos::fence();
  std::vector<double> ms;
  for (int i = 0; i < cfg.reps; ++i) {
    Kokkos::Timer timer;
    launch();
    Kokkos::fence();
    ms.push_back(timer.seconds() * 1e3);
  }
  std::sort(ms.begin(), ms.end());
  row.ms_min = ms.front();
  row.ms_max = ms.back();
  row.ms_median = ms[ms.size() / 2];
}

inline long count_nonfinite(const Kokkos::View<double*>& out) {
  auto h = Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace(), out);
  long bad = 0;
  for (std::size_t i = 0; i < h.extent(0); ++i) bad += std::isfinite(h(i)) ? 0 : 1;
  return bad;
}

}  // namespace xpk_tp
