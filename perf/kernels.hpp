// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// Perf harness kernels (GPU_PERF_PLAN P0.1). One templated kernel per op,
// instantiated for six real and six complex types from the same source.
//
// This header measures speed only. It never judges accuracy: no oracle, no
// ulps, no tolerances. Every call goes through the Kokkos:: forwards so the
// wrapper layer is part of what is measured.

#pragma once

#include <Kokkos_Core.hpp>
#include <Kokkos_Complex.hpp>
#include <Kokkos_xpmath/Kokkos_xpmath.hpp>

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <tuple>
#include <type_traits>
#include <utility>

namespace xpk_perf {

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------
using DD = Kokkos::Experimental::DoubleDouble;
using FF = Kokkos::Experimental::FloatFloat;
using TF = Kokkos::Experimental::TripleFloat;
using QF = Kokkos::Experimental::QuadFloat;
using DDC = Kokkos::Experimental::DoubleDoubleComplex;
using FFC = Kokkos::Experimental::FloatFloatComplex;
using TFC = Kokkos::Experimental::TripleFloatComplex;
using QFC = Kokkos::Experimental::QuadFloatComplex;
using CD = Kokkos::complex<double>;
using CF = Kokkos::complex<float>;

// Fast tier (xpmath P3.0b). Present only when the xpmath headers in use define
// XPMATH_HAS_TIERS, so the harness still builds against xpmath 4d597fd, which
// has none. The fast types forward to strict until a P3 task overrides a
// function; the wrapper layer is not involved (task R.1 adds it), so these
// kernels name the xp types directly and call through the Kokkos:: forwards.
#if defined(XPMATH_HAS_TIERS)
using DD_fast = xp::DoubleDoubleFast;
using FF_fast = xp::FloatFloatFast;
using TF_fast = xp::TripleFloatFast;
using QF_fast = xp::QuadFloatFast;
using DDC_fast = xp::DoubleDoubleComplexFast;
using FFC_fast = xp::FloatFloatComplexFast;
using TFC_fast = xp::TripleFloatComplexFast;
using QFC_fast = xp::QuadFloatComplexFast;
#endif

// make(x, t): build a T from the double x. `t` in [-1,1) only gives DD a
// non-trivial low word; FF/TF/QF take their limbs from x's exact expansion.
template <class T>
struct RealTraits {
  static constexpr double exp_max = 80.0;  // float-range types overflow past ~88
  static KOKKOS_INLINE_FUNCTION T make(double x, double) { return T(x); }
};
template <>
struct RealTraits<double> {
  static constexpr double exp_max = 700.0;
  static KOKKOS_INLINE_FUNCTION double make(double x, double) { return x; }
};
template <>
struct RealTraits<float> {
  static constexpr double exp_max = 80.0;
  static KOKKOS_INLINE_FUNCTION float make(double x, double) { return static_cast<float>(x); }
};
template <>
struct RealTraits<DD> {
  static constexpr double exp_max = 700.0;
  static KOKKOS_INLINE_FUNCTION DD make(double x, double t) {
    return DD(x, x * 0x1p-55 * t);
  }
};

template <class Z>
struct ComplexTraits;
template <>
struct ComplexTraits<CD> {
  using Real = double;
  static constexpr double exp_max = 700.0;
  static KOKKOS_INLINE_FUNCTION CD make(double re, double im, double) { return CD(re, im); }
};
template <>
struct ComplexTraits<CF> {
  using Real = float;
  static constexpr double exp_max = 80.0;
  static KOKKOS_INLINE_FUNCTION CF make(double re, double im, double) {
    return CF(static_cast<float>(re), static_cast<float>(im));
  }
};
#define XPK_XP_COMPLEX_TRAITS(Z, R)                                              \
  template <>                                                                    \
  struct ComplexTraits<Z> {                                                      \
    using Real = R;                                                              \
    static constexpr double exp_max = RealTraits<R>::exp_max;                    \
    static KOKKOS_INLINE_FUNCTION Z make(double re, double im, double t) {       \
      return Z(RealTraits<R>::make(re, t), RealTraits<R>::make(im, t));          \
    }                                                                            \
  }
XPK_XP_COMPLEX_TRAITS(DDC, DD);
XPK_XP_COMPLEX_TRAITS(FFC, FF);
XPK_XP_COMPLEX_TRAITS(TFC, TF);
XPK_XP_COMPLEX_TRAITS(QFC, QF);
#if defined(XPMATH_HAS_TIERS)
template <>
struct RealTraits<DD_fast> {
  static constexpr double exp_max = 700.0;
  static KOKKOS_INLINE_FUNCTION DD_fast make(double x, double t) {
    return DD_fast(x, x * 0x1p-55 * t);
  }
};
XPK_XP_COMPLEX_TRAITS(DDC_fast, DD_fast);
XPK_XP_COMPLEX_TRAITS(FFC_fast, FF_fast);
XPK_XP_COMPLEX_TRAITS(TFC_fast, TF_fast);
XPK_XP_COMPLEX_TRAITS(QFC_fast, QF_fast);
#endif
#undef XPK_XP_COMPLEX_TRAITS

// ---------------------------------------------------------------------------
// Ops. Tag names appear verbatim in the demangled kernel symbols, and
// isa_metrics_hip.py reads them back; keep them stable.
// ---------------------------------------------------------------------------
namespace op {
struct Add {};
struct Mul {};
struct Div {};
struct Sqrt {};
struct Exp {};
struct Log {};
struct Sin {};
struct Atan2 {};
struct Abs {};
}  // namespace op

// Math dispatch for apply(): wrappers and builtins go through Kokkos::, the
// fast-tier xp types (which have no Kokkos:: forwards) through their xp:: overloads.
namespace fwd {
using Kokkos::abs;
using Kokkos::atan2;
using Kokkos::exp;
using Kokkos::log;
using Kokkos::sin;
using Kokkos::sqrt;
#if defined(XPMATH_HAS_TIERS)
template <class T>
struct is_fast : std::false_type {};
template <> struct is_fast<DD_fast> : std::true_type {};
template <> struct is_fast<FF_fast> : std::true_type {};
template <> struct is_fast<TF_fast> : std::true_type {};
template <> struct is_fast<QF_fast> : std::true_type {};
template <> struct is_fast<DDC_fast> : std::true_type {};
template <> struct is_fast<FFC_fast> : std::true_type {};
template <> struct is_fast<TFC_fast> : std::true_type {};
template <> struct is_fast<QFC_fast> : std::true_type {};
#define XPK_FAST_FWD1(name)                                                   \
  template <class T, std::enable_if_t<is_fast<T>::value, int> = 0>            \
  KOKKOS_INLINE_FUNCTION auto name(const T& a) { return xp::name(a); }
XPK_FAST_FWD1(abs)
XPK_FAST_FWD1(exp)
XPK_FAST_FWD1(log)
XPK_FAST_FWD1(sin)
XPK_FAST_FWD1(sqrt)
#undef XPK_FAST_FWD1
template <class T, std::enable_if_t<is_fast<T>::value, int> = 0>
KOKKOS_INLINE_FUNCTION T atan2(const T& a, const T& b) { return xp::atan2(a, b); }
#endif
}  // namespace fwd

template <class T>
KOKKOS_INLINE_FUNCTION auto apply(op::Add, const T& a, const T& b) { return a + b; }
template <class T>
KOKKOS_INLINE_FUNCTION auto apply(op::Mul, const T& a, const T& b) { return a * b; }
template <class T>
KOKKOS_INLINE_FUNCTION auto apply(op::Div, const T& a, const T& b) { return a / b; }
template <class T>
KOKKOS_INLINE_FUNCTION auto apply(op::Sqrt, const T& a, const T&) { return fwd::sqrt(a); }
template <class T>
KOKKOS_INLINE_FUNCTION auto apply(op::Exp, const T& a, const T&) { return fwd::exp(a); }
template <class T>
KOKKOS_INLINE_FUNCTION auto apply(op::Log, const T& a, const T&) { return fwd::log(a); }
template <class T>
KOKKOS_INLINE_FUNCTION auto apply(op::Sin, const T& a, const T&) { return fwd::sin(a); }
template <class T>
KOKKOS_INLINE_FUNCTION auto apply(op::Atan2, const T& a, const T& b) { return fwd::atan2(a, b); }
template <class T>
KOKKOS_INLINE_FUNCTION auto apply(op::Abs, const T& a, const T&) { return fwd::abs(a); }

using RealOps = std::tuple<op::Add, op::Mul, op::Div, op::Sqrt, op::Exp, op::Log, op::Sin, op::Atan2>;
using ComplexOps = std::tuple<op::Add, op::Mul, op::Div, op::Abs, op::Sqrt, op::Log, op::Exp>;

// ---------------------------------------------------------------------------
// Inputs: deterministic, built on the host, copied to the device. The
// kernels read plain doubles, so input generation is never in the kernel
// ISA and there is no generator kernel to filter out.
// ---------------------------------------------------------------------------
struct Inputs {
  int n = 0;
  Kokkos::View<double*> a, b, c, d, t;  // a,b: first operand (re,im); c,d: second
};

enum class Dom {
  Arith,    // magnitude log-uniform in [0.5, 1e3], random sign (nonzero: safe divisor)
  Pos,      // log-uniform in [1e-3, 1e3]; positive (sqrt, log)
  Exp,      // uniform in [-exp_max, exp_max]
  Trig,     // uniform in [-100, 100]
  Atan2,    // uniform in [-10, 10], magnitude >= 0.1
  CArith,   // complex: components magnitude log-uniform in [0.5, 100], random sign
  CExp,     // complex: re uniform in [-exp_max, exp_max], im uniform in [-100, 100]
};

namespace detail {
inline std::uint64_t splitmix64(std::uint64_t& s) {
  std::uint64_t z = (s += 0x9E3779B97F4A7C15ull);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}
inline double unit(std::uint64_t& s) {  // [0,1)
  return static_cast<double>(splitmix64(s) >> 11) * 0x1p-53;
}
inline double log_uniform(std::uint64_t& s, double lo, double hi) {
  return std::exp(std::log(lo) + unit(s) * (std::log(hi) - std::log(lo)));
}
inline double signed_log_uniform(std::uint64_t& s, double lo, double hi) {
  const double m = log_uniform(s, lo, hi);
  return (splitmix64(s) & 1u) ? m : -m;
}
inline double uniform(std::uint64_t& s, double lo, double hi) { return lo + unit(s) * (hi - lo); }
}  // namespace detail

inline Inputs make_inputs(int n, Dom dom, double exp_max, std::uint64_t seed = 0x5851F42D4C957F2Dull) {
  Inputs in;
  in.n = n;
  in.a = Kokkos::View<double*>(Kokkos::view_alloc("perf_a", Kokkos::WithoutInitializing), n);
  in.b = Kokkos::View<double*>(Kokkos::view_alloc("perf_b", Kokkos::WithoutInitializing), n);
  in.c = Kokkos::View<double*>(Kokkos::view_alloc("perf_c", Kokkos::WithoutInitializing), n);
  in.d = Kokkos::View<double*>(Kokkos::view_alloc("perf_d", Kokkos::WithoutInitializing), n);
  in.t = Kokkos::View<double*>(Kokkos::view_alloc("perf_t", Kokkos::WithoutInitializing), n);
  auto ha = Kokkos::create_mirror_view(in.a);
  auto hb = Kokkos::create_mirror_view(in.b);
  auto hc = Kokkos::create_mirror_view(in.c);
  auto hd = Kokkos::create_mirror_view(in.d);
  auto ht = Kokkos::create_mirror_view(in.t);
  std::uint64_t s = seed;
  for (int i = 0; i < n; ++i) {
    double v[4];
    switch (dom) {
      case Dom::Arith:
        for (double& x : v) x = detail::signed_log_uniform(s, 0.5, 1e3);
        break;
      case Dom::Pos:
        for (double& x : v) x = detail::log_uniform(s, 1e-3, 1e3);
        break;
      case Dom::Exp:
        for (double& x : v) x = detail::uniform(s, -exp_max, exp_max);
        break;
      case Dom::Trig:
        for (double& x : v) x = detail::uniform(s, -100.0, 100.0);
        break;
      case Dom::Atan2:
        for (double& x : v) x = detail::signed_log_uniform(s, 0.1, 10.0);
        break;
      case Dom::CArith:
        for (double& x : v) x = detail::signed_log_uniform(s, 0.5, 100.0);
        break;
      case Dom::CExp:
        v[0] = detail::uniform(s, -exp_max, exp_max);
        v[1] = detail::uniform(s, -100.0, 100.0);
        v[2] = detail::uniform(s, -exp_max, exp_max);
        v[3] = detail::uniform(s, -100.0, 100.0);
        break;
    }
    ha(i) = v[0];
    hb(i) = v[1];
    hc(i) = v[2];
    hd(i) = v[3];
    ht(i) = detail::uniform(s, -1.0, 1.0);
  }
  Kokkos::deep_copy(in.a, ha);
  Kokkos::deep_copy(in.b, hb);
  Kokkos::deep_copy(in.c, hc);
  Kokkos::deep_copy(in.d, hd);
  Kokkos::deep_copy(in.t, ht);
  return in;
}

template <class Op>
constexpr Dom real_domain() {
  if constexpr (std::is_same_v<Op, op::Sqrt> || std::is_same_v<Op, op::Log>) return Dom::Pos;
  else if constexpr (std::is_same_v<Op, op::Exp>) return Dom::Exp;
  else if constexpr (std::is_same_v<Op, op::Sin>) return Dom::Trig;
  else if constexpr (std::is_same_v<Op, op::Atan2>) return Dom::Atan2;
  else return Dom::Arith;
}
template <class Op>
constexpr Dom complex_domain() {
  if constexpr (std::is_same_v<Op, op::Exp>) return Dom::CExp;
  else return Dom::CArith;
}

// ---------------------------------------------------------------------------
// Kernels. LaunchBounds<256,1> is QCDLoop's validated launch policy.
// ---------------------------------------------------------------------------
using Policy = Kokkos::RangePolicy<Kokkos::LaunchBounds<256, 1>>;

template <class T, class Op>
struct RealKernel {
  using Result = decltype(apply(Op{}, std::declval<T>(), std::declval<T>()));
  Inputs in;
  Kokkos::View<Result*> out;

  KOKKOS_INLINE_FUNCTION void operator()(const int i) const {
    const T x = RealTraits<T>::make(in.a(i), in.t(i));
    const T y = RealTraits<T>::make(in.b(i), in.t(i));
    out(i) = apply(Op{}, x, y);
  }
};

template <class Z, class Op>
struct ComplexKernel {
  using Real = typename ComplexTraits<Z>::Real;
  using Result = decltype(apply(Op{}, std::declval<Z>(), std::declval<Z>()));
  Inputs in;
  Kokkos::View<Result*> out;

  KOKKOS_INLINE_FUNCTION void operator()(const int i) const {
    const Z x = ComplexTraits<Z>::make(in.a(i), in.b(i), in.t(i));
    const Z y = ComplexTraits<Z>::make(in.c(i), in.d(i), in.t(i));
    out(i) = apply(Op{}, x, y);
  }
};

// Launch one kernel; the result View is returned so the caller keeps it alive
// (and so nothing is dead-code eliminated).
template <class T, class Op>
auto launch_real(const Inputs& in) {
  RealKernel<T, Op> k{in, {}};
  k.out = Kokkos::View<typename RealKernel<T, Op>::Result*>(
      Kokkos::view_alloc(Kokkos::WithoutInitializing, "perf_out"), in.n);
  Kokkos::parallel_for("xpk_real", Policy(0, in.n), k);
  return k.out;
}

template <class Z, class Op>
auto launch_complex(const Inputs& in) {
  ComplexKernel<Z, Op> k{in, {}};
  k.out = Kokkos::View<typename ComplexKernel<Z, Op>::Result*>(
      Kokkos::view_alloc(Kokkos::WithoutInitializing, "perf_out"), in.n);
  Kokkos::parallel_for("xpk_complex", Policy(0, in.n), k);
  return k.out;
}

// Run every op for one type. Instantiates every kernel for T.
template <class T, class... Ops>
void run_real_ops(int n, std::tuple<Ops...>) {
  (void)std::initializer_list<int>{
      (launch_real<T, Ops>(make_inputs(n, real_domain<Ops>(), RealTraits<T>::exp_max)), 0)...};
}
template <class Z, class... Ops>
void run_complex_ops(int n, std::tuple<Ops...>) {
  (void)std::initializer_list<int>{
      (launch_complex<Z, Ops>(make_inputs(n, complex_domain<Ops>(), ComplexTraits<Z>::exp_max)), 0)...};
}

template <class T>
void run_real(int n) { run_real_ops<T>(n, RealOps{}); }
template <class Z>
void run_complex(int n) { run_complex_ops<Z>(n, ComplexOps{}); }

}  // namespace xpk_perf
