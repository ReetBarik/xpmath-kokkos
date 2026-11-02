// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// Runs every kernel once. Exists so the harness links into one executable
// and so a GPU run can confirm every kernel launches. No timing and no
// result checking here; throughput is P0.3.

#include <Kokkos_Core.hpp>

#include <cstdio>
#include <cstdlib>

void xpk_run_unit_r0(int);
void xpk_run_unit_r1(int);
void xpk_run_unit_r2(int);
void xpk_run_unit_r3(int);
void xpk_run_unit_r4(int);
void xpk_run_unit_r5(int);
void xpk_run_unit_c0(int);
void xpk_run_unit_c1(int);
void xpk_run_unit_c2(int);
void xpk_run_unit_c3(int);
void xpk_run_unit_c4(int);
void xpk_run_unit_c5(int);

int main(int argc, char** argv) {
  Kokkos::initialize(argc, argv);
  {
    const int n = 1 << 14;
    xpk_run_unit_r0(n);
    xpk_run_unit_r1(n);
    xpk_run_unit_r2(n);
    xpk_run_unit_r3(n);
    xpk_run_unit_r4(n);
    xpk_run_unit_r5(n);
    xpk_run_unit_c0(n);
    xpk_run_unit_c1(n);
    xpk_run_unit_c2(n);
    xpk_run_unit_c3(n);
    xpk_run_unit_c4(n);
    xpk_run_unit_c5(n);
    Kokkos::fence();
    std::printf("xpk_perf: launched 90 kernels (n=%d)\n", n);
  }
  Kokkos::finalize();
  return EXIT_SUCCESS;
}
