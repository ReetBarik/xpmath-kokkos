// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC

// Gate: all eight types + every forwarded Kokkos:: / Experimental name from the
// recovered 158d618 surface (TF Kokkos:: forwards completed in K1). Compile-only;
// not a ctest target — keeps the suite at exactly 9 tests.
#include <Kokkos_xpmath/Kokkos_xpmath.hpp>

int main() {
  using namespace Kokkos::Experimental;

  DoubleDouble v_DoubleDouble{}; (void)v_DoubleDouble;
  FloatFloat v_FloatFloat{}; (void)v_FloatFloat;
  QuadFloat v_QuadFloat{}; (void)v_QuadFloat;
  TripleFloat v_TripleFloat{}; (void)v_TripleFloat;
  DoubleDoubleComplex v_DoubleDoubleComplex{}; (void)v_DoubleDoubleComplex;
  FloatFloatComplex v_FloatFloatComplex{}; (void)v_FloatFloatComplex;
  QuadFloatComplex v_QuadFloatComplex{}; (void)v_QuadFloatComplex;
  TripleFloatComplex v_TripleFloatComplex{}; (void)v_TripleFloatComplex;

  // Name every Experimental using-declaration (odr-use via function pointers / addresses where possible).
  // Kokkos:: math forwards (real)
  { auto r = Kokkos::abs(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::sqrt(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::exp(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::exp2(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::exp10(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::expm1(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::log(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::log2(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::log10(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::log1p(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::sin(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::cos(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::tan(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::asin(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::acos(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::atan(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::atan2(DoubleDouble{}, DoubleDouble{}); (void)r; }
  { auto r = Kokkos::sinh(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::cosh(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::tanh(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::asinh(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::acosh(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::atanh(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::pow(DoubleDouble{}, DoubleDouble{}); (void)r; }
  { auto r = Kokkos::hypot(DoubleDouble{}, DoubleDouble{}); (void)r; }
  { auto r = Kokkos::fmod(DoubleDouble{}, DoubleDouble{}); (void)r; }
  { auto r = Kokkos::remainder(DoubleDouble{}, DoubleDouble{}); (void)r; }
  { auto r = Kokkos::copysign(DoubleDouble{}, DoubleDouble{}); (void)r; }
  { auto r = Kokkos::fmax(DoubleDouble{}, DoubleDouble{}); (void)r; }
  { auto r = Kokkos::fmin(DoubleDouble{}, DoubleDouble{}); (void)r; }
  { auto r = Kokkos::fdim(DoubleDouble{}, DoubleDouble{}); (void)r; }
  { auto r = Kokkos::fma(DoubleDouble{}, DoubleDouble{}, DoubleDouble{}); (void)r; }
  { auto r = Kokkos::ceil(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::floor(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::round(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::trunc(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::erf(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::erfc(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::tgamma(DoubleDouble{}); (void)r; }
  { auto r = Kokkos::abs(FloatFloat{}); (void)r; }
  { auto r = Kokkos::sqrt(FloatFloat{}); (void)r; }
  { auto r = Kokkos::exp(FloatFloat{}); (void)r; }
  { auto r = Kokkos::exp2(FloatFloat{}); (void)r; }
  { auto r = Kokkos::exp10(FloatFloat{}); (void)r; }
  { auto r = Kokkos::expm1(FloatFloat{}); (void)r; }
  { auto r = Kokkos::log(FloatFloat{}); (void)r; }
  { auto r = Kokkos::log2(FloatFloat{}); (void)r; }
  { auto r = Kokkos::log10(FloatFloat{}); (void)r; }
  { auto r = Kokkos::log1p(FloatFloat{}); (void)r; }
  { auto r = Kokkos::sin(FloatFloat{}); (void)r; }
  { auto r = Kokkos::cos(FloatFloat{}); (void)r; }
  { auto r = Kokkos::tan(FloatFloat{}); (void)r; }
  { auto r = Kokkos::asin(FloatFloat{}); (void)r; }
  { auto r = Kokkos::acos(FloatFloat{}); (void)r; }
  { auto r = Kokkos::atan(FloatFloat{}); (void)r; }
  { auto r = Kokkos::atan2(FloatFloat{}, FloatFloat{}); (void)r; }
  { auto r = Kokkos::sinh(FloatFloat{}); (void)r; }
  { auto r = Kokkos::cosh(FloatFloat{}); (void)r; }
  { auto r = Kokkos::tanh(FloatFloat{}); (void)r; }
  { auto r = Kokkos::asinh(FloatFloat{}); (void)r; }
  { auto r = Kokkos::acosh(FloatFloat{}); (void)r; }
  { auto r = Kokkos::atanh(FloatFloat{}); (void)r; }
  { auto r = Kokkos::pow(FloatFloat{}, FloatFloat{}); (void)r; }
  { auto r = Kokkos::hypot(FloatFloat{}, FloatFloat{}); (void)r; }
  { auto r = Kokkos::fmod(FloatFloat{}, FloatFloat{}); (void)r; }
  { auto r = Kokkos::remainder(FloatFloat{}, FloatFloat{}); (void)r; }
  { auto r = Kokkos::copysign(FloatFloat{}, FloatFloat{}); (void)r; }
  { auto r = Kokkos::fmax(FloatFloat{}, FloatFloat{}); (void)r; }
  { auto r = Kokkos::fmin(FloatFloat{}, FloatFloat{}); (void)r; }
  { auto r = Kokkos::fdim(FloatFloat{}, FloatFloat{}); (void)r; }
  { auto r = Kokkos::fma(FloatFloat{}, FloatFloat{}, FloatFloat{}); (void)r; }
  { auto r = Kokkos::ceil(FloatFloat{}); (void)r; }
  { auto r = Kokkos::floor(FloatFloat{}); (void)r; }
  { auto r = Kokkos::round(FloatFloat{}); (void)r; }
  { auto r = Kokkos::trunc(FloatFloat{}); (void)r; }
  { auto r = Kokkos::erf(FloatFloat{}); (void)r; }
  { auto r = Kokkos::erfc(FloatFloat{}); (void)r; }
  { auto r = Kokkos::tgamma(FloatFloat{}); (void)r; }
  { auto r = Kokkos::abs(QuadFloat{}); (void)r; }
  { auto r = Kokkos::sqrt(QuadFloat{}); (void)r; }
  { auto r = Kokkos::exp(QuadFloat{}); (void)r; }
  { auto r = Kokkos::exp2(QuadFloat{}); (void)r; }
  { auto r = Kokkos::exp10(QuadFloat{}); (void)r; }
  { auto r = Kokkos::expm1(QuadFloat{}); (void)r; }
  { auto r = Kokkos::log(QuadFloat{}); (void)r; }
  { auto r = Kokkos::log2(QuadFloat{}); (void)r; }
  { auto r = Kokkos::log10(QuadFloat{}); (void)r; }
  { auto r = Kokkos::log1p(QuadFloat{}); (void)r; }
  { auto r = Kokkos::sin(QuadFloat{}); (void)r; }
  { auto r = Kokkos::cos(QuadFloat{}); (void)r; }
  { auto r = Kokkos::tan(QuadFloat{}); (void)r; }
  { auto r = Kokkos::asin(QuadFloat{}); (void)r; }
  { auto r = Kokkos::acos(QuadFloat{}); (void)r; }
  { auto r = Kokkos::atan(QuadFloat{}); (void)r; }
  { auto r = Kokkos::atan2(QuadFloat{}, QuadFloat{}); (void)r; }
  { auto r = Kokkos::sinh(QuadFloat{}); (void)r; }
  { auto r = Kokkos::cosh(QuadFloat{}); (void)r; }
  { auto r = Kokkos::tanh(QuadFloat{}); (void)r; }
  { auto r = Kokkos::asinh(QuadFloat{}); (void)r; }
  { auto r = Kokkos::acosh(QuadFloat{}); (void)r; }
  { auto r = Kokkos::atanh(QuadFloat{}); (void)r; }
  { auto r = Kokkos::pow(QuadFloat{}, QuadFloat{}); (void)r; }
  { auto r = Kokkos::hypot(QuadFloat{}, QuadFloat{}); (void)r; }
  { auto r = Kokkos::fmod(QuadFloat{}, QuadFloat{}); (void)r; }
  { auto r = Kokkos::remainder(QuadFloat{}, QuadFloat{}); (void)r; }
  { auto r = Kokkos::copysign(QuadFloat{}, QuadFloat{}); (void)r; }
  { auto r = Kokkos::fmax(QuadFloat{}, QuadFloat{}); (void)r; }
  { auto r = Kokkos::fmin(QuadFloat{}, QuadFloat{}); (void)r; }
  { auto r = Kokkos::fdim(QuadFloat{}, QuadFloat{}); (void)r; }
  { auto r = Kokkos::fma(QuadFloat{}, QuadFloat{}, QuadFloat{}); (void)r; }
  { auto r = Kokkos::ceil(QuadFloat{}); (void)r; }
  { auto r = Kokkos::floor(QuadFloat{}); (void)r; }
  { auto r = Kokkos::round(QuadFloat{}); (void)r; }
  { auto r = Kokkos::trunc(QuadFloat{}); (void)r; }
  { auto r = Kokkos::abs(TripleFloat{}); (void)r; }
  { auto r = Kokkos::sqrt(TripleFloat{}); (void)r; }
  { auto r = Kokkos::exp(TripleFloat{}); (void)r; }
  { auto r = Kokkos::exp2(TripleFloat{}); (void)r; }
  { auto r = Kokkos::exp10(TripleFloat{}); (void)r; }
  { auto r = Kokkos::expm1(TripleFloat{}); (void)r; }
  { auto r = Kokkos::log(TripleFloat{}); (void)r; }
  { auto r = Kokkos::log2(TripleFloat{}); (void)r; }
  { auto r = Kokkos::log10(TripleFloat{}); (void)r; }
  { auto r = Kokkos::log1p(TripleFloat{}); (void)r; }
  { auto r = Kokkos::sin(TripleFloat{}); (void)r; }
  { auto r = Kokkos::cos(TripleFloat{}); (void)r; }
  { auto r = Kokkos::tan(TripleFloat{}); (void)r; }
  { auto r = Kokkos::asin(TripleFloat{}); (void)r; }
  { auto r = Kokkos::acos(TripleFloat{}); (void)r; }
  { auto r = Kokkos::atan(TripleFloat{}); (void)r; }
  { auto r = Kokkos::atan2(TripleFloat{}, TripleFloat{}); (void)r; }
  { auto r = Kokkos::sinh(TripleFloat{}); (void)r; }
  { auto r = Kokkos::cosh(TripleFloat{}); (void)r; }
  { auto r = Kokkos::tanh(TripleFloat{}); (void)r; }
  { auto r = Kokkos::asinh(TripleFloat{}); (void)r; }
  { auto r = Kokkos::acosh(TripleFloat{}); (void)r; }
  { auto r = Kokkos::atanh(TripleFloat{}); (void)r; }
  { auto r = Kokkos::pow(TripleFloat{}, TripleFloat{}); (void)r; }
  { auto r = Kokkos::hypot(TripleFloat{}, TripleFloat{}); (void)r; }
  { auto r = Kokkos::fmod(TripleFloat{}, TripleFloat{}); (void)r; }
  { auto r = Kokkos::remainder(TripleFloat{}, TripleFloat{}); (void)r; }
  { auto r = Kokkos::copysign(TripleFloat{}, TripleFloat{}); (void)r; }
  { auto r = Kokkos::fmax(TripleFloat{}, TripleFloat{}); (void)r; }
  { auto r = Kokkos::fmin(TripleFloat{}, TripleFloat{}); (void)r; }
  { auto r = Kokkos::fdim(TripleFloat{}, TripleFloat{}); (void)r; }
  { auto r = Kokkos::fma(TripleFloat{}, TripleFloat{}, TripleFloat{}); (void)r; }
  { auto r = Kokkos::ceil(TripleFloat{}); (void)r; }
  { auto r = Kokkos::floor(TripleFloat{}); (void)r; }
  { auto r = Kokkos::round(TripleFloat{}); (void)r; }
  { auto r = Kokkos::trunc(TripleFloat{}); (void)r; }
  // Kokkos:: math forwards (complex) + real/imag
  { auto r = Kokkos::abs(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::conj(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::sqrt(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::exp(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::log(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::log10(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::sin(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::cos(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::tan(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::asin(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::acos(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::atan(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::sinh(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::cosh(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::tanh(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::asinh(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::acosh(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::atanh(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::pow(DoubleDoubleComplex{}, DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::real(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::imag(DoubleDoubleComplex{}); (void)r; }
  { auto r = Kokkos::abs(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::conj(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::sqrt(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::exp(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::log(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::log10(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::sin(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::cos(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::tan(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::asin(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::acos(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::atan(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::sinh(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::cosh(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::tanh(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::asinh(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::acosh(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::atanh(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::pow(FloatFloatComplex{}, FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::real(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::imag(FloatFloatComplex{}); (void)r; }
  { auto r = Kokkos::abs(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::norm(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::arg(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::conj(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::sqrt(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::exp(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::log(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::log10(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::sin(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::cos(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::tan(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::asin(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::acos(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::atan(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::sinh(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::cosh(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::tanh(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::asinh(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::acosh(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::atanh(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::pow(QuadFloatComplex{}, QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::real(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::imag(QuadFloatComplex{}); (void)r; }
  { auto r = Kokkos::abs(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::norm(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::arg(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::conj(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::sqrt(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::exp(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::log(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::log10(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::sin(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::cos(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::tan(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::asin(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::acos(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::atan(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::sinh(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::cosh(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::tanh(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::asinh(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::acosh(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::atanh(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::pow(TripleFloatComplex{}, TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::real(TripleFloatComplex{}); (void)r; }
  { auto r = Kokkos::imag(TripleFloatComplex{}); (void)r; }

  // Experimental using-declarations: reference each name in an unevaluated/odr-safe way
  { using Kokkos::Experimental::DoubleDouble_e; }
  { using Kokkos::Experimental::DoubleDouble_euler_gamma; }
  { using Kokkos::Experimental::DoubleDouble_log10; }
  { using Kokkos::Experimental::DoubleDouble_log2; }
  { using Kokkos::Experimental::DoubleDouble_pi; }
  { using Kokkos::Experimental::DoubleDouble_sqrt2; }
  { using Kokkos::Experimental::add; }
  { using Kokkos::Experimental::divide; }
  { using Kokkos::Experimental::divide_scalar; }
  { using Kokkos::Experimental::multiply; }
  { using Kokkos::Experimental::multiply_scalar; }
  { using Kokkos::Experimental::negate; }
  { using Kokkos::Experimental::subtract; }
  { using Kokkos::Experimental::two_prod; }
  { using Kokkos::Experimental::abs; }
  { using Kokkos::Experimental::pow_int; }
  { using Kokkos::Experimental::round_to_nearest_int; }
  { using Kokkos::Experimental::sqrt; }
  { using Kokkos::Experimental::exp; }
  { using Kokkos::Experimental::exp10; }
  { using Kokkos::Experimental::exp2; }
  { using Kokkos::Experimental::expm1; }
  { using Kokkos::Experimental::log; }
  { using Kokkos::Experimental::log10; }
  { using Kokkos::Experimental::log1p; }
  { using Kokkos::Experimental::log2; }
  { using Kokkos::Experimental::acos; }
  { using Kokkos::Experimental::angle; }
  { using Kokkos::Experimental::asin; }
  { using Kokkos::Experimental::atan; }
  { using Kokkos::Experimental::atan2; }
  { using Kokkos::Experimental::cos; }
  { using Kokkos::Experimental::sin; }
  { using Kokkos::Experimental::sincos; }
  { using Kokkos::Experimental::tan; }
  { using Kokkos::Experimental::acosh; }
  { using Kokkos::Experimental::asinh; }
  { using Kokkos::Experimental::atanh; }
  { using Kokkos::Experimental::cosh; }
  { using Kokkos::Experimental::sinh; }
  { using Kokkos::Experimental::sinhcosh; }
  { using Kokkos::Experimental::tanh; }
  { using Kokkos::Experimental::copysign; }
  { using Kokkos::Experimental::fdim; }
  { using Kokkos::Experimental::fma; }
  { using Kokkos::Experimental::fmax; }
  { using Kokkos::Experimental::fmin; }
  { using Kokkos::Experimental::fmod; }
  { using Kokkos::Experimental::hypot; }
  { using Kokkos::Experimental::pow; }
  { using Kokkos::Experimental::remainder; }
  { using Kokkos::Experimental::ceil; }
  { using Kokkos::Experimental::floor; }
  { using Kokkos::Experimental::round; }
  { using Kokkos::Experimental::trunc; }
  { using Kokkos::Experimental::bessel_j0; }
  { using Kokkos::Experimental::bessel_j1; }
  { using Kokkos::Experimental::bessel_jn; }
  { using Kokkos::Experimental::bessel_y0; }
  { using Kokkos::Experimental::bessel_y1; }
  { using Kokkos::Experimental::bessel_yn; }
  { using Kokkos::Experimental::erf; }
  { using Kokkos::Experimental::erfc; }
  { using Kokkos::Experimental::erfc_asymptotic_sum; }
  { using Kokkos::Experimental::expint; }
  { using Kokkos::Experimental::incgamma; }
  { using Kokkos::Experimental::tgamma; }
  { using Kokkos::Experimental::zeta; }
  { using Kokkos::Experimental::FloatFloat_e; }
  { using Kokkos::Experimental::FloatFloat_euler_gamma; }
  { using Kokkos::Experimental::FloatFloat_log10; }
  { using Kokkos::Experimental::FloatFloat_log2; }
  { using Kokkos::Experimental::FloatFloat_pi; }
  { using Kokkos::Experimental::FloatFloat_sqrt2; }
  { using Kokkos::Experimental::add; }
  { using Kokkos::Experimental::divide; }
  { using Kokkos::Experimental::divide_scalar; }
  { using Kokkos::Experimental::multiply; }
  { using Kokkos::Experimental::multiply_scalar; }
  { using Kokkos::Experimental::negate; }
  { using Kokkos::Experimental::subtract; }
  { using Kokkos::Experimental::two_prod; }
  { using Kokkos::Experimental::abs; }
  { using Kokkos::Experimental::pow_int; }
  { using Kokkos::Experimental::round_to_nearest_int; }
  { using Kokkos::Experimental::sqrt; }
  { using Kokkos::Experimental::exp; }
  { using Kokkos::Experimental::exp10; }
  { using Kokkos::Experimental::exp2; }
  { using Kokkos::Experimental::expm1; }
  { using Kokkos::Experimental::log; }
  { using Kokkos::Experimental::log10; }
  { using Kokkos::Experimental::log1p; }
  { using Kokkos::Experimental::log2; }
  { using Kokkos::Experimental::acos; }
  { using Kokkos::Experimental::angle; }
  { using Kokkos::Experimental::asin; }
  { using Kokkos::Experimental::atan; }
  { using Kokkos::Experimental::atan2; }
  { using Kokkos::Experimental::cos; }
  { using Kokkos::Experimental::sin; }
  { using Kokkos::Experimental::sincos; }
  { using Kokkos::Experimental::tan; }
  { using Kokkos::Experimental::acosh; }
  { using Kokkos::Experimental::asinh; }
  { using Kokkos::Experimental::atanh; }
  { using Kokkos::Experimental::cosh; }
  { using Kokkos::Experimental::sinh; }
  { using Kokkos::Experimental::sinhcosh; }
  { using Kokkos::Experimental::tanh; }
  { using Kokkos::Experimental::copysign; }
  { using Kokkos::Experimental::fdim; }
  { using Kokkos::Experimental::fma; }
  { using Kokkos::Experimental::fmax; }
  { using Kokkos::Experimental::fmin; }
  { using Kokkos::Experimental::fmod; }
  { using Kokkos::Experimental::hypot; }
  { using Kokkos::Experimental::pow; }
  { using Kokkos::Experimental::remainder; }
  { using Kokkos::Experimental::ceil; }
  { using Kokkos::Experimental::floor; }
  { using Kokkos::Experimental::round; }
  { using Kokkos::Experimental::trunc; }
  { using Kokkos::Experimental::erf; }
  { using Kokkos::Experimental::erfc; }
  { using Kokkos::Experimental::erfc_asymptotic_sum; }
  { using Kokkos::Experimental::incgamma; }
  { using Kokkos::Experimental::tgamma; }
  { using Kokkos::Experimental::QuadFloat_e; }
  { using Kokkos::Experimental::QuadFloat_euler_gamma; }
  { using Kokkos::Experimental::QuadFloat_log10; }
  { using Kokkos::Experimental::QuadFloat_log2; }
  { using Kokkos::Experimental::QuadFloat_pi; }
  { using Kokkos::Experimental::QuadFloat_sqrt2; }
  { using Kokkos::Experimental::add; }
  { using Kokkos::Experimental::divide; }
  { using Kokkos::Experimental::divide_accurate; }
  { using Kokkos::Experimental::divide_scalar; }
  { using Kokkos::Experimental::ieee_add; }
  { using Kokkos::Experimental::mul_pwr2; }
  { using Kokkos::Experimental::multiply; }
  { using Kokkos::Experimental::multiply_scalar; }
  { using Kokkos::Experimental::negate; }
  { using Kokkos::Experimental::qf_quick_two_sum; }
  { using Kokkos::Experimental::qf_two_prod; }
  { using Kokkos::Experimental::qf_two_sqr; }
  { using Kokkos::Experimental::qf_two_sum; }
  { using Kokkos::Experimental::renorm; }
  { using Kokkos::Experimental::renorm_4; }
  { using Kokkos::Experimental::sloppy_add; }
  { using Kokkos::Experimental::subtract; }
  { using Kokkos::Experimental::abs; }
  { using Kokkos::Experimental::pow_int; }
  { using Kokkos::Experimental::round_to_nearest_int; }
  { using Kokkos::Experimental::sqr; }
  { using Kokkos::Experimental::sqrt; }
  { using Kokkos::Experimental::exp; }
  { using Kokkos::Experimental::exp10; }
  { using Kokkos::Experimental::exp2; }
  { using Kokkos::Experimental::expm1; }
  { using Kokkos::Experimental::log; }
  { using Kokkos::Experimental::log10; }
  { using Kokkos::Experimental::log1p; }
  { using Kokkos::Experimental::log2; }
  { using Kokkos::Experimental::acos; }
  { using Kokkos::Experimental::angle; }
  { using Kokkos::Experimental::asin; }
  { using Kokkos::Experimental::atan; }
  { using Kokkos::Experimental::atan2; }
  { using Kokkos::Experimental::cos; }
  { using Kokkos::Experimental::sin; }
  { using Kokkos::Experimental::sincos; }
  { using Kokkos::Experimental::tan; }
  { using Kokkos::Experimental::acosh; }
  { using Kokkos::Experimental::asinh; }
  { using Kokkos::Experimental::atanh; }
  { using Kokkos::Experimental::cosh; }
  { using Kokkos::Experimental::sinh; }
  { using Kokkos::Experimental::sinhcosh; }
  { using Kokkos::Experimental::tanh; }
  { using Kokkos::Experimental::copysign; }
  { using Kokkos::Experimental::fdim; }
  { using Kokkos::Experimental::fma; }
  { using Kokkos::Experimental::fmax; }
  { using Kokkos::Experimental::fmin; }
  { using Kokkos::Experimental::fmod; }
  { using Kokkos::Experimental::hypot; }
  { using Kokkos::Experimental::pow; }
  { using Kokkos::Experimental::remainder; }
  { using Kokkos::Experimental::ceil; }
  { using Kokkos::Experimental::floor; }
  { using Kokkos::Experimental::round; }
  { using Kokkos::Experimental::trunc; }
  { using Kokkos::Experimental::TripleFloat_e; }
  { using Kokkos::Experimental::TripleFloat_euler_gamma; }
  { using Kokkos::Experimental::TripleFloat_log10; }
  { using Kokkos::Experimental::TripleFloat_log2; }
  { using Kokkos::Experimental::TripleFloat_pi; }
  { using Kokkos::Experimental::TripleFloat_sqrt2; }
  { using Kokkos::Experimental::add; }
  { using Kokkos::Experimental::divide; }
  { using Kokkos::Experimental::divide_scalar; }
  { using Kokkos::Experimental::ieee_add; }
  { using Kokkos::Experimental::mul_pwr2; }
  { using Kokkos::Experimental::multiply; }
  { using Kokkos::Experimental::multiply_scalar; }
  { using Kokkos::Experimental::negate; }
  { using Kokkos::Experimental::renorm; }
  { using Kokkos::Experimental::renorm_3; }
  { using Kokkos::Experimental::sloppy_add; }
  { using Kokkos::Experimental::subtract; }
  { using Kokkos::Experimental::tf_quick_two_sum; }
  { using Kokkos::Experimental::tf_two_prod; }
  { using Kokkos::Experimental::tf_two_sqr; }
  { using Kokkos::Experimental::tf_two_sum; }
  { using Kokkos::Experimental::abs; }
  { using Kokkos::Experimental::pow_int; }
  { using Kokkos::Experimental::round_to_nearest_int; }
  { using Kokkos::Experimental::sqr; }
  { using Kokkos::Experimental::sqrt; }
  { using Kokkos::Experimental::exp; }
  { using Kokkos::Experimental::exp10; }
  { using Kokkos::Experimental::exp2; }
  { using Kokkos::Experimental::expm1; }
  { using Kokkos::Experimental::log; }
  { using Kokkos::Experimental::log10; }
  { using Kokkos::Experimental::log1p; }
  { using Kokkos::Experimental::log2; }
  { using Kokkos::Experimental::pow; }
  { using Kokkos::Experimental::acos; }
  { using Kokkos::Experimental::angle; }
  { using Kokkos::Experimental::asin; }
  { using Kokkos::Experimental::atan; }
  { using Kokkos::Experimental::atan2; }
  { using Kokkos::Experimental::cos; }
  { using Kokkos::Experimental::sin; }
  { using Kokkos::Experimental::sincos; }
  { using Kokkos::Experimental::tan; }
  { using Kokkos::Experimental::acosh; }
  { using Kokkos::Experimental::asinh; }
  { using Kokkos::Experimental::atanh; }
  { using Kokkos::Experimental::cosh; }
  { using Kokkos::Experimental::sinh; }
  { using Kokkos::Experimental::sinhcosh; }
  { using Kokkos::Experimental::tanh; }
  { using Kokkos::Experimental::ceil; }
  { using Kokkos::Experimental::copysign; }
  { using Kokkos::Experimental::fdim; }
  { using Kokkos::Experimental::floor; }
  { using Kokkos::Experimental::fma; }
  { using Kokkos::Experimental::fmax; }
  { using Kokkos::Experimental::fmin; }
  { using Kokkos::Experimental::fmod; }
  { using Kokkos::Experimental::hypot; }
  { using Kokkos::Experimental::remainder; }
  { using Kokkos::Experimental::round; }
  { using Kokkos::Experimental::trunc; }
  { using Kokkos::Experimental::abs; }
  { using Kokkos::Experimental::acos; }
  { using Kokkos::Experimental::acosh; }
  { using Kokkos::Experimental::asin; }
  { using Kokkos::Experimental::asinh; }
  { using Kokkos::Experimental::atan; }
  { using Kokkos::Experimental::atanh; }
  { using Kokkos::Experimental::conj; }
  { using Kokkos::Experimental::cos; }
  { using Kokkos::Experimental::cosh; }
  { using Kokkos::Experimental::exp; }
  { using Kokkos::Experimental::log; }
  { using Kokkos::Experimental::log10; }
  { using Kokkos::Experimental::polar; }
  { using Kokkos::Experimental::pow; }
  { using Kokkos::Experimental::sin; }
  { using Kokkos::Experimental::sinh; }
  { using Kokkos::Experimental::sqrt; }
  { using Kokkos::Experimental::tan; }
  { using Kokkos::Experimental::tanh; }
  { using Kokkos::Experimental::abs; }
  { using Kokkos::Experimental::acos; }
  { using Kokkos::Experimental::acosh; }
  { using Kokkos::Experimental::asin; }
  { using Kokkos::Experimental::asinh; }
  { using Kokkos::Experimental::atan; }
  { using Kokkos::Experimental::atanh; }
  { using Kokkos::Experimental::conj; }
  { using Kokkos::Experimental::cos; }
  { using Kokkos::Experimental::cosh; }
  { using Kokkos::Experimental::exp; }
  { using Kokkos::Experimental::log; }
  { using Kokkos::Experimental::log10; }
  { using Kokkos::Experimental::polar; }
  { using Kokkos::Experimental::pow; }
  { using Kokkos::Experimental::sin; }
  { using Kokkos::Experimental::sinh; }
  { using Kokkos::Experimental::sqrt; }
  { using Kokkos::Experimental::tan; }
  { using Kokkos::Experimental::tanh; }
  { using Kokkos::Experimental::abs; }
  { using Kokkos::Experimental::acos; }
  { using Kokkos::Experimental::acosh; }
  { using Kokkos::Experimental::arg; }
  { using Kokkos::Experimental::asin; }
  { using Kokkos::Experimental::asinh; }
  { using Kokkos::Experimental::atan; }
  { using Kokkos::Experimental::atanh; }
  { using Kokkos::Experimental::conj; }
  { using Kokkos::Experimental::cos; }
  { using Kokkos::Experimental::cosh; }
  { using Kokkos::Experimental::exp; }
  { using Kokkos::Experimental::log; }
  { using Kokkos::Experimental::log10; }
  { using Kokkos::Experimental::norm; }
  { using Kokkos::Experimental::polar; }
  { using Kokkos::Experimental::pow; }
  { using Kokkos::Experimental::sin; }
  { using Kokkos::Experimental::sinh; }
  { using Kokkos::Experimental::sqrt; }
  { using Kokkos::Experimental::tan; }
  { using Kokkos::Experimental::tanh; }
  { using Kokkos::Experimental::abs; }
  { using Kokkos::Experimental::acos; }
  { using Kokkos::Experimental::acosh; }
  { using Kokkos::Experimental::arg; }
  { using Kokkos::Experimental::asin; }
  { using Kokkos::Experimental::asinh; }
  { using Kokkos::Experimental::atan; }
  { using Kokkos::Experimental::atanh; }
  { using Kokkos::Experimental::conj; }
  { using Kokkos::Experimental::cos; }
  { using Kokkos::Experimental::cosh; }
  { using Kokkos::Experimental::exp; }
  { using Kokkos::Experimental::log; }
  { using Kokkos::Experimental::log10; }
  { using Kokkos::Experimental::norm; }
  { using Kokkos::Experimental::polar; }
  { using Kokkos::Experimental::pow; }
  { using Kokkos::Experimental::sin; }
  { using Kokkos::Experimental::sinh; }
  { using Kokkos::Experimental::sqrt; }
  { using Kokkos::Experimental::tan; }
  { using Kokkos::Experimental::tanh; }
  return 0;
}
