// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// Kokkos::Experimental compatibility wrapper. Numerics live under
// vendor/xpmath/xp/ at the recorded upstream tag and retain their upstream
// SPDX identifiers (see NOTICE.md). This file is only aliases and one-line
// Kokkos:: forwards — no new arithmetic.

// KOKKOS COMPATIBILITY WRAPPER for the standalone triple-float core.
//
// The implementation moved to include/xp/tf_math.hpp (namespace xp,
// zero Kokkos dependency). This file is what `#include <tf_math.hpp>` resolves
// to, and it keeps that API byte-for-byte: after including it,
// `Kokkos::Experimental::TripleFloat`, every free function that used to live
// in `Kokkos::Experimental`, and every `Kokkos::`-namespace math forwarder
// resolve. No consumer — tf_complex.hpp, the TF tests, the TF demos — needs a
// single edit. K1 completed the Kokkos:: one-line forwards to match dd/ff/qf.
//
// Licensing is unchanged: the algorithms descend from the QD 2.3.24 port
// (Hida-Li-Bailey, LBNL-BSD-License); see include/xp/tf_math.hpp for the full
// notice and docs/PORT_NOTES_TF.md for the k=3-specific modifications.
//
// NAMING (ratified via S2 naming memo + S3): xp:: = extended precision,
// companion to MxP (mixed precision). See include/xp/config.hpp for rationale.
// Every Kokkos-facing name below is an alias, so the rename touched only this
// file's right-hand sides.

#pragma once

#include <Kokkos_Core.hpp>

#include <xp/tf_math.hpp>

// ============================================================
// Re-exposure under namespace Kokkos::Experimental
// ============================================================
// Explicit using-declarations rather than `using namespace xp;`. Three
// reasons: (1) the Kokkos-visible API surface stays an auditable list rather
// than "whatever xp happens to declare"; (2) a using-DIRECTIVE participates
// in qualified lookup in a way that is easy to get subtly wrong when Kokkos
// itself declares same-named overloads in Kokkos::Experimental (sqrt, exp,
// ... for half_t), whereas a using-DECLARATION simply merges into that
// overload set; (3) it documents, for the S4 RFC, exactly what an upstream
// Kokkos would be adopting.
//
// The type alias is what makes `Kokkos::Experimental::TripleFloat` name the
// same type as `xp::TripleFloat` — not a distinct wrapper — so the two
// spellings are interchangeable in every signature, including across the
// tf_complex.hpp (future) boundary.
//
// Operators are deliberately absent: `tf + tf`, `1.0f * tf` and
// `os << tf` are found by ADL through the argument's real namespace (xp),
// so re-declaring them here would be redundant.
namespace Kokkos {
namespace Experimental {

// --- the type -----------------------------------------------------------
using TripleFloat = xp::TripleFloat;

// --- constants ----------------------------------------------------------
using xp::TripleFloat_e;
using xp::TripleFloat_euler_gamma;
using xp::TripleFloat_log10;
using xp::TripleFloat_log2;
using xp::TripleFloat_pi;
using xp::TripleFloat_sqrt2;

// --- primitive arithmetic -----------------------------------------------
using xp::add;
using xp::divide;
using xp::divide_scalar;
using xp::ieee_add;
using xp::mul_pwr2;
using xp::multiply;
using xp::multiply_scalar;
using xp::negate;
using xp::renorm;       // needed by tf_eft_test (future)
using xp::renorm_3;     // needed by tf_eft_test (future)
using xp::sloppy_add;
using xp::subtract;
using xp::tf_quick_two_sum;  // needed by tf_eft_test (future)
using xp::tf_two_prod;       // needed by tf_eft_test, tf_fma_guard_test (future)
using xp::tf_two_sqr;        // needed by tf_fma_guard_test (future)
using xp::tf_two_sum;        // needed by tf_eft_test, tf_fma_guard_test (future)

// --- basic math ---------------------------------------------------------
using xp::abs;
using xp::pow_int;
using xp::round_to_nearest_int;
using xp::sqr;
using xp::sqrt;

// --- exp / log family ---------------------------------------------------
using xp::exp;
using xp::exp10;
using xp::exp2;
using xp::expm1;
using xp::log;
using xp::log10;
using xp::log1p;
using xp::log2;
using xp::pow;

// --- trig ---------------------------------------------------------------
using xp::acos;
using xp::angle;
using xp::asin;
using xp::atan;
using xp::atan2;
using xp::cos;
using xp::sin;
using xp::sincos;
using xp::tan;

// --- hyperbolic ---------------------------------------------------------
using xp::acosh;
using xp::asinh;
using xp::atanh;
using xp::cosh;
using xp::sinh;
using xp::sinhcosh;
using xp::tanh;

// --- rounding / data ops ------------------------------------------------
using xp::ceil;
using xp::copysign;
using xp::fdim;
using xp::floor;
using xp::fma;
using xp::fmax;
using xp::fmin;
using xp::fmod;
using xp::hypot;
using xp::remainder;
using xp::round;
using xp::trunc;

}  // namespace Experimental
}  // namespace Kokkos

// ============================================================
// Re-exposure under namespace Kokkos (T0.4/T2.0)
// ============================================================
// Mirrors the dd/ff/qf one-line Kokkos:: forwards so user code can call
// Kokkos::exp(tf) identically to Kokkos::exp(double). The 158d618 wrapper used
// bare `using xp::fn;` here; that is completed to the sibling KOKKOS_INLINE_FUNCTION
// pattern (adoption ergonomics / plan K1).
// NOTE: add/subtract/multiply/divide are deliberately NOT forwarded here — those
// are reached via operators and explicit ADL, not as Kokkos::add etc.
namespace Kokkos {
// clang-format off
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat abs(Experimental::TripleFloat x)   { return Experimental::abs(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat sqrt(Experimental::TripleFloat x)  { return Experimental::sqrt(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat exp(Experimental::TripleFloat x)   { return Experimental::exp(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat exp2(Experimental::TripleFloat x)  { return Experimental::exp2(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat exp10(Experimental::TripleFloat x) { return Experimental::exp10(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat expm1(Experimental::TripleFloat x) { return Experimental::expm1(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat log(Experimental::TripleFloat x)   { return Experimental::log(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat log2(Experimental::TripleFloat x)  { return Experimental::log2(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat log10(Experimental::TripleFloat x) { return Experimental::log10(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat log1p(Experimental::TripleFloat x) { return Experimental::log1p(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat sin(Experimental::TripleFloat x)   { return Experimental::sin(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat cos(Experimental::TripleFloat x)  { return Experimental::cos(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat tan(Experimental::TripleFloat x)   { return Experimental::tan(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat asin(Experimental::TripleFloat x)  { return Experimental::asin(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat acos(Experimental::TripleFloat x)  { return Experimental::acos(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat atan(Experimental::TripleFloat x)  { return Experimental::atan(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat atan2(Experimental::TripleFloat y, Experimental::TripleFloat x) { return Experimental::atan2(y, x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat sinh(Experimental::TripleFloat x)  { return Experimental::sinh(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat cosh(Experimental::TripleFloat x)  { return Experimental::cosh(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat tanh(Experimental::TripleFloat x)  { return Experimental::tanh(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat asinh(Experimental::TripleFloat x) { return Experimental::asinh(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat acosh(Experimental::TripleFloat x) { return Experimental::acosh(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat atanh(Experimental::TripleFloat x) { return Experimental::atanh(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat pow(Experimental::TripleFloat a, Experimental::TripleFloat b) { return Experimental::pow(a, b); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat hypot(Experimental::TripleFloat a, Experimental::TripleFloat b) { return Experimental::hypot(a, b); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat fmod(Experimental::TripleFloat a, Experimental::TripleFloat b) { return Experimental::fmod(a, b); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat remainder(Experimental::TripleFloat a, Experimental::TripleFloat b) { return Experimental::remainder(a, b); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat copysign(Experimental::TripleFloat a, Experimental::TripleFloat b) { return Experimental::copysign(a, b); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat fmax(Experimental::TripleFloat a, Experimental::TripleFloat b) { return Experimental::fmax(a, b); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat fmin(Experimental::TripleFloat a, Experimental::TripleFloat b) { return Experimental::fmin(a, b); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat fdim(Experimental::TripleFloat a, Experimental::TripleFloat b) { return Experimental::fdim(a, b); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat fma(Experimental::TripleFloat a, Experimental::TripleFloat b, Experimental::TripleFloat c) { return Experimental::fma(a, b, c); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat ceil(Experimental::TripleFloat x)  { return Experimental::ceil(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat floor(Experimental::TripleFloat x) { return Experimental::floor(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat round(Experimental::TripleFloat x) { return Experimental::round(x); }
KOKKOS_INLINE_FUNCTION Experimental::TripleFloat trunc(Experimental::TripleFloat x) { return Experimental::trunc(x); }
// clang-format on
}  // namespace Kokkos
