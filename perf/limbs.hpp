// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// Limb access for the throughput runner and science kernel (GPU_PERF_PLAN
// P0.3). `sink` folds every limb of a value into one double, so a kernel can
// store a single double per element without letting the compiler drop the
// low-order work. `lead` is the leading limb, used only to steer branches.
// Nothing here judges accuracy.

#pragma once

#include "kernels.hpp"

#include <type_traits>

namespace xpk_perf {

template <class T>
struct IsComplex : std::false_type {};
template <>
struct IsComplex<CD> : std::true_type {};
template <>
struct IsComplex<CF> : std::true_type {};
template <>
struct IsComplex<DDC> : std::true_type {};
template <>
struct IsComplex<FFC> : std::true_type {};
template <>
struct IsComplex<TFC> : std::true_type {};
template <>
struct IsComplex<QFC> : std::true_type {};

KOKKOS_INLINE_FUNCTION double sink(double x) { return x; }
KOKKOS_INLINE_FUNCTION double sink(float x) { return static_cast<double>(x); }
KOKKOS_INLINE_FUNCTION double sink(const DD& x) { return x.hi + x.lo; }
KOKKOS_INLINE_FUNCTION double sink(const FF& x) {
  return static_cast<double>(x.hi) + static_cast<double>(x.lo);
}
KOKKOS_INLINE_FUNCTION double sink(const TF& x) {
  return static_cast<double>(x.f0) + static_cast<double>(x.f1) + static_cast<double>(x.f2);
}
KOKKOS_INLINE_FUNCTION double sink(const QF& x) {
  return (static_cast<double>(x.f0) + static_cast<double>(x.f1)) +
         (static_cast<double>(x.f2) + static_cast<double>(x.f3));
}
KOKKOS_INLINE_FUNCTION double sink(const CD& z) { return z.real() + z.imag(); }
KOKKOS_INLINE_FUNCTION double sink(const CF& z) {
  return static_cast<double>(z.real()) + static_cast<double>(z.imag());
}
template <class Z, class = std::enable_if_t<IsComplex<Z>::value && !std::is_same_v<Z, CD> &&
                                            !std::is_same_v<Z, CF>>>
KOKKOS_INLINE_FUNCTION double sink(const Z& z) {
  return sink(z.re) + sink(z.im);
}

KOKKOS_INLINE_FUNCTION double lead(double x) { return x; }
KOKKOS_INLINE_FUNCTION double lead(float x) { return static_cast<double>(x); }
KOKKOS_INLINE_FUNCTION double lead(const DD& x) { return x.hi; }
KOKKOS_INLINE_FUNCTION double lead(const FF& x) { return static_cast<double>(x.hi); }
KOKKOS_INLINE_FUNCTION double lead(const TF& x) { return static_cast<double>(x.f0); }
KOKKOS_INLINE_FUNCTION double lead(const QF& x) { return static_cast<double>(x.f0); }

KOKKOS_INLINE_FUNCTION double re_of(const CD& z) { return z.real(); }
KOKKOS_INLINE_FUNCTION double im_of(const CD& z) { return z.imag(); }
KOKKOS_INLINE_FUNCTION float re_of(const CF& z) { return z.real(); }
KOKKOS_INLINE_FUNCTION float im_of(const CF& z) { return z.imag(); }
template <class Z, class = std::enable_if_t<IsComplex<Z>::value && !std::is_same_v<Z, CD> &&
                                            !std::is_same_v<Z, CF>>>
KOKKOS_INLINE_FUNCTION auto re_of(const Z& z) { return z.re; }
template <class Z, class = std::enable_if_t<IsComplex<Z>::value && !std::is_same_v<Z, CD> &&
                                            !std::is_same_v<Z, CF>>>
KOKKOS_INLINE_FUNCTION auto im_of(const Z& z) { return z.im; }

}  // namespace xpk_perf
