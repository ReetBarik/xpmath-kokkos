// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// Wrapper-side evaluators for K5 bit-identity. Parallel to sweep_ops_core.hpp
// (copied from xpmath v0.2.0 scripts/sweep_ops.hpp) but every math call goes
// through Kokkos:: / Kokkos::Experimental:: instead of xp::.
//
// Arithmetic operators are ADL on the same underlying type (the Experimental
// aliases name xp:: types). polar has no Kokkos:: forward — use Experimental.
//
// No oracle, no ulps, no host-quad-type.

#pragma once

#include <Kokkos_xpmath/Kokkos_xpmath.hpp>

#include "sweep_ops_core.hpp"  // op enums R_*/C_* and kReal/kComplex tables

namespace xpsweep {

template <class S, int ID>
XPMATH_INLINE_FUNCTION S eval_real_wrap_ct(const S& a, const S& b, const S& c) {
  if constexpr (ID == R_Add)            return a + b;
  else if constexpr (ID == R_Sub)       return a - b;
  else if constexpr (ID == R_Mul)       return a * b;
  else if constexpr (ID == R_Div)       return a / b;
  else if constexpr (ID == R_Sqrt)      return Kokkos::sqrt(a);
  else if constexpr (ID == R_Abs)       return Kokkos::abs(a);
  else if constexpr (ID == R_Exp)       return Kokkos::exp(a);
  else if constexpr (ID == R_Log)       return Kokkos::log(a);
  else if constexpr (ID == R_Exp2)      return Kokkos::exp2(a);
  else if constexpr (ID == R_Exp10)     return Kokkos::exp10(a);
  else if constexpr (ID == R_Expm1)     return Kokkos::expm1(a);
  else if constexpr (ID == R_Log2)      return Kokkos::log2(a);
  else if constexpr (ID == R_Log10)     return Kokkos::log10(a);
  else if constexpr (ID == R_Log1p)     return Kokkos::log1p(a);
  else if constexpr (ID == R_Sin)       return Kokkos::sin(a);
  else if constexpr (ID == R_Cos)       return Kokkos::cos(a);
  else if constexpr (ID == R_Tan)       return Kokkos::tan(a);
  else if constexpr (ID == R_Asin)      return Kokkos::asin(a);
  else if constexpr (ID == R_Acos)      return Kokkos::acos(a);
  else if constexpr (ID == R_Atan)      return Kokkos::atan(a);
  else if constexpr (ID == R_Sinh)      return Kokkos::sinh(a);
  else if constexpr (ID == R_Cosh)      return Kokkos::cosh(a);
  else if constexpr (ID == R_Tanh)      return Kokkos::tanh(a);
  else if constexpr (ID == R_Acosh)     return Kokkos::acosh(a);
  else if constexpr (ID == R_Asinh)     return Kokkos::asinh(a);
  else if constexpr (ID == R_Atanh)     return Kokkos::atanh(a);
  else if constexpr (ID == R_Pow)       return Kokkos::pow(a, b);
  else if constexpr (ID == R_Hypot)     return Kokkos::hypot(a, b);
  else if constexpr (ID == R_Fmod)      return Kokkos::fmod(a, b);
  else if constexpr (ID == R_Remainder) return Kokkos::remainder(a, b);
  else if constexpr (ID == R_Copysign)  return Kokkos::copysign(a, b);
  else if constexpr (ID == R_Fmax)      return Kokkos::fmax(a, b);
  else if constexpr (ID == R_Fmin)      return Kokkos::fmin(a, b);
  else if constexpr (ID == R_Fdim)      return Kokkos::fdim(a, b);
  else if constexpr (ID == R_Fma)       return Kokkos::fma(a, b, c);
  else if constexpr (ID == R_Ceil)      return Kokkos::ceil(a);
  else if constexpr (ID == R_Floor)     return Kokkos::floor(a);
  else if constexpr (ID == R_Round)     return Kokkos::round(a);
  else if constexpr (ID == R_Trunc)     return Kokkos::trunc(a);
  else {
    static_assert(ID != ID, "xpsweep::eval_real_wrap_ct: unhandled ID");
    return a;
  }
}

template <class S, class Z, int ID>
XPMATH_INLINE_FUNCTION Z eval_complex_wrap_ct(const Z& a, const Z& b, bool& is_real) {
  is_real = false;
  if constexpr (ID == C_Add)        return a + b;
  else if constexpr (ID == C_Sub)   return a - b;
  else if constexpr (ID == C_Mul)   return a * b;
  else if constexpr (ID == C_Div)   return a / b;
  else if constexpr (ID == C_Abs) {
    is_real = true;
    return Z(Kokkos::abs(a), S(0.0));
  }
  else if constexpr (ID == C_Conj)  return Kokkos::conj(a);
  else if constexpr (ID == C_Sqrt)  return Kokkos::sqrt(a);
  else if constexpr (ID == C_Exp)   return Kokkos::exp(a);
  else if constexpr (ID == C_Log)   return Kokkos::log(a);
  else if constexpr (ID == C_Log10) return Kokkos::log10(a);
  else if constexpr (ID == C_Sin)   return Kokkos::sin(a);
  else if constexpr (ID == C_Cos)   return Kokkos::cos(a);
  else if constexpr (ID == C_Tan)   return Kokkos::tan(a);
  else if constexpr (ID == C_Asin)  return Kokkos::asin(a);
  else if constexpr (ID == C_Acos)  return Kokkos::acos(a);
  else if constexpr (ID == C_Atan)  return Kokkos::atan(a);
  else if constexpr (ID == C_Sinh)  return Kokkos::sinh(a);
  else if constexpr (ID == C_Cosh)  return Kokkos::cosh(a);
  else if constexpr (ID == C_Tanh)  return Kokkos::tanh(a);
  else if constexpr (ID == C_Asinh) return Kokkos::asinh(a);
  else if constexpr (ID == C_Acosh) return Kokkos::acosh(a);
  else if constexpr (ID == C_Atanh) return Kokkos::atanh(a);
  else if constexpr (ID == C_Pow)   return Kokkos::pow(a, b);
  else if constexpr (ID == C_Polar) return Kokkos::Experimental::polar(a.re, a.im);
  else {
    static_assert(ID != ID, "xpsweep::eval_complex_wrap_ct: unhandled ID");
    return a;
  }
}
}  // namespace xpsweep
