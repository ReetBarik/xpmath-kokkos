// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// Science-shaped kernel (GPU_PERF_PLAN P0.3): a synthetic stand-in for a
// loop-integral leaf. It does not depend on QCDLoop. Its purpose is to show
// occupancy effects that single-op kernels hide: 24 complex values stay live
// across a fixed chain of 200 operations (complex mul/add/div/log/sqrt, real
// atan2, a data-dependent sign choice), so register pressure and spills
// decide the speed, not any one op.
//
// Boundedness: every value starts in the unit disk and every block maps
// values of modulus <= 1.6 to values of modulus <= 1.6 (sqrt of a product,
// division by something with modulus >= 2.4, log of something in [1.4, 4.2]),
// so no type leaves its range. The runner counts non-finite results anyway.
//
// Speed only: no oracle, no ulps, no tolerances.

#pragma once

#include "limbs.hpp"

#include <cstddef>
#include <utility>

namespace xpk_perf {

template <class R, class Z>
struct ScienceKernel {
  static constexpr int nvals = 24;
  static constexpr int nblocks = 20;
  static constexpr int ops_per_element = 200;  // 10 per block

  Inputs in;  // in.n is the table size, a power of two
  Kokkos::View<double*> out;

  template <int M>
  KOKKOS_FORCEINLINE_FUNCTION double col(int j) const {
    if constexpr (M == 0) return in.a(j);
    else if constexpr (M == 1) return in.b(j);
    else if constexpr (M == 2) return in.c(j);
    else return in.d(j);
  }

  template <int K>
  KOKKOS_FORCEINLINE_FUNCTION Z load_one(int i) const {
    const int j = (i + 97 * K) & (in.n - 1);
    return ComplexTraits<Z>::make(col<K % 4>(j), col<(K + 1) % 4>(j), in.t(j));
  }

  template <std::size_t... K>
  KOKKOS_FORCEINLINE_FUNCTION void load_all(Z (&z)[nvals], int i, std::index_sequence<K...>) const {
    ((z[K] = load_one<static_cast<int>(K)>(i)), ...);
  }

  template <int B>
  static KOKKOS_FORCEINLINE_FUNCTION void block(Z (&z)[nvals], const Z& four, const Z& c28,
                                                const R& sixteenth) {
    constexpr int ia = (5 * B + 1) % nvals;
    constexpr int ib = (7 * B + 2) % nvals;
    constexpr int ic = (11 * B + 3) % nvals;
    constexpr int id = (13 * B + 5) % nvals;
    constexpr int ix = (7 * B + 11) % nvals;
    const Z t0 = z[ia] * z[ib];
    const Z t1 = fwd::sqrt(t0);
    const Z t2 = t1 + z[ic];
    const Z den = z[id] + four;
    const Z t3 = t2 / den;
    const Z t4 = fwd::log(t3 + c28);
    R at = fwd::atan2(im_of(t2), re_of(t2)) * sixteenth;
    if (lead(im_of(t3)) < 0.0) at = -at;  // branch-cut-style sign choice
    z[ix] = Z(re_of(t4), im_of(t4) + at);
  }

  template <std::size_t... B>
  static KOKKOS_FORCEINLINE_FUNCTION void chain(Z (&z)[nvals], const Z& four, const Z& c28,
                                                const R& sixteenth, std::index_sequence<B...>) {
    (block<static_cast<int>(B)>(z, four, c28, sixteenth), ...);
  }

  template <std::size_t... K>
  static KOKKOS_FORCEINLINE_FUNCTION double reduce(const Z (&z)[nvals], std::index_sequence<K...>) {
    double acc = 0.0;
    ((acc += sink(z[K])), ...);
    return acc;
  }

  KOKKOS_INLINE_FUNCTION void operator()(const int i) const {
    Z z[nvals];
    load_all(z, i, std::make_index_sequence<nvals>{});
    const Z four = ComplexTraits<Z>::make(4.0, 0.0, 0.0);
    const Z c28 = ComplexTraits<Z>::make(2.8, 0.0, 0.0);
    const R sixteenth = RealTraits<R>::make(0.0625, 0.0);
    chain(z, four, c28, sixteenth, std::make_index_sequence<nblocks>{});
    out(i) = reduce(z, std::make_index_sequence<nvals>{});
  }
};

}  // namespace xpk_perf
