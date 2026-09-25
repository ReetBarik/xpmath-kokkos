// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// Kokkos::Experimental compatibility wrapper. Numerics live under
// vendor/xpmath/xp/ at the recorded upstream tag and retain their upstream
// SPDX identifiers (see NOTICE.md). This file is only aliases and one-line
// Kokkos:: forwards — no new arithmetic.
//
// Kokkos::complex<DoubleDouble> does not instantiate (Kokkos 5.1
// static_assert on std::is_floating_point). The supported spelling is
// the standalone struct aliased below. See docs/COMPLEX_INTEROP.md.

// KOKKOS COMPATIBILITY WRAPPER for the standalone double-double complex core.
//
// The implementation moved to include/xp/dd_complex.hpp (namespace xp,
// zero Kokkos dependency). This file is what `#include <dd_complex.hpp>` has
// always resolved to, and it keeps that API byte-for-byte: after including
// it, `Kokkos::Experimental::DoubleDoubleComplex`, every free function that
// used to live in `Kokkos::Experimental`, and every `Kokkos::`-namespace
// math forwarder resolve exactly as before. No consumer — the DD complex
// tests, the DD demo — needs a single edit.
//
// Ported from DDFUN v04:
//   https://www.davidhbailey.com/dhbsoftware/ddfun-v04.tar.gz
//   Original author: David H. Bailey (LBNL retired / UC Davis)
//   Original license: DHB-License (modified BSD-3-Clause with §3
//     grant-back clause). Full text: LICENSES/LicenseRef-DHB-License.txt
//     or https://www.davidhbailey.com/dhbsoftware/DHB-License.txt.
//
// NAMING (ratified via S2 naming memo + S3): xp:: = extended precision,
// companion to MxP (mixed precision). See include/xp/config.hpp for rationale.
// Every Kokkos-facing name below is an alias, so the rename touched only this
// file's right-hand sides.

#pragma once

#include <Kokkos_Core.hpp>

#include <Kokkos_xpmath/dd_math.hpp>
#include <xp/dd_complex.hpp>

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
// The type alias is what makes `Kokkos::Experimental::DoubleDoubleComplex`
// name the same type as `xp::DoubleDoubleComplex` — not a distinct wrapper —
// so the two spellings are interchangeable in every signature.
//
// Operators are deliberately absent: `ddc + ddc`, `dd * ddc` and
// `os << ddc` are found by ADL through the argument's real namespace (xp),
// so re-declaring them here would be redundant.
namespace Kokkos {
namespace Experimental {

// --- the type -----------------------------------------------------------
using DoubleDoubleComplex = xp::DoubleDoubleComplex;

// --- complex operations -------------------------------------------------
using xp::abs;
using xp::acos;
using xp::acosh;
using xp::asin;
using xp::asinh;
using xp::atan;
using xp::atanh;
using xp::conj;
using xp::cos;
using xp::cosh;
using xp::exp;
using xp::log;
using xp::log10;
using xp::polar;
using xp::pow;
using xp::sin;
using xp::sinh;
using xp::sqrt;
using xp::tan;
using xp::tanh;

}  // namespace Experimental
}  // namespace Kokkos

// ============================================================
// Re-exposure under namespace Kokkos (T0.4)
// ============================================================
// Mirror of dd_math.hpp: so Kokkos::exp(ddc) works identically to
// Kokkos::exp(Kokkos::complex<double>). One-line forwards. Arithmetic operators
// are reached directly / via ADL and are not re-exposed here.
namespace Kokkos {
// clang-format off
KOKKOS_INLINE_FUNCTION Experimental::DoubleDouble        abs(Experimental::DoubleDoubleComplex z)   { return Experimental::abs(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex conj(Experimental::DoubleDoubleComplex z)  { return Experimental::conj(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex sqrt(Experimental::DoubleDoubleComplex z)  { return Experimental::sqrt(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex exp(Experimental::DoubleDoubleComplex z)   { return Experimental::exp(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex log(Experimental::DoubleDoubleComplex z)   { return Experimental::log(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex log10(Experimental::DoubleDoubleComplex z) { return Experimental::log10(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex sin(Experimental::DoubleDoubleComplex z)   { return Experimental::sin(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex cos(Experimental::DoubleDoubleComplex z)   { return Experimental::cos(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex tan(Experimental::DoubleDoubleComplex z)   { return Experimental::tan(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex asin(Experimental::DoubleDoubleComplex z)  { return Experimental::asin(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex acos(Experimental::DoubleDoubleComplex z)  { return Experimental::acos(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex atan(Experimental::DoubleDoubleComplex z)  { return Experimental::atan(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex sinh(Experimental::DoubleDoubleComplex z)  { return Experimental::sinh(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex cosh(Experimental::DoubleDoubleComplex z)  { return Experimental::cosh(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex tanh(Experimental::DoubleDoubleComplex z)  { return Experimental::tanh(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex asinh(Experimental::DoubleDoubleComplex z) { return Experimental::asinh(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex acosh(Experimental::DoubleDoubleComplex z) { return Experimental::acosh(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex atanh(Experimental::DoubleDoubleComplex z) { return Experimental::atanh(z); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex pow(Experimental::DoubleDoubleComplex z, Experimental::DoubleDoubleComplex w) { return Experimental::pow(z, w); }
// Adoption ergonomics: Kokkos::real / Kokkos::imag spelling (no host complex128 overloads).
KOKKOS_INLINE_FUNCTION Experimental::DoubleDouble real(Experimental::DoubleDoubleComplex z) { return z.real(); }
KOKKOS_INLINE_FUNCTION Experimental::DoubleDouble imag(Experimental::DoubleDoubleComplex z) { return z.imag(); }
// clang-format on
}  // namespace Kokkos

// ============================================================
// Kokkos::reduction_identity — DoubleDoubleComplex
// ============================================================
// sum/prod only; max/min omitted (same convention as Kokkos::complex).
namespace Kokkos {
template <>
struct reduction_identity<Experimental::DoubleDoubleComplex> {
  KOKKOS_FORCEINLINE_FUNCTION static Experimental::DoubleDoubleComplex sum() {
    return Experimental::DoubleDoubleComplex(0.0);
  }
  KOKKOS_FORCEINLINE_FUNCTION static Experimental::DoubleDoubleComplex prod() {
    return Experimental::DoubleDoubleComplex(1.0);
  }
};
}  // namespace Kokkos

// ============================================================
// Kokkos::atomic_add — DoubleDoubleComplex (componentwise on re/im)
// ============================================================
// Each component uses the real-type CAS over its full expansion (see
// dd_math.hpp / impl/atomic_cas_add.hpp). Determinism caveat applies per
// component: order-dependent last-limb differences are not a defect.
namespace Kokkos {
KOKKOS_INLINE_FUNCTION void atomic_add(
    Experimental::DoubleDoubleComplex* dest,
    Experimental::DoubleDoubleComplex const& val) {
  Kokkos::atomic_add(&dest->re, val.re);
  Kokkos::atomic_add(&dest->im, val.im);
}
KOKKOS_INLINE_FUNCTION Experimental::DoubleDoubleComplex atomic_fetch_add(
    Experimental::DoubleDoubleComplex* dest,
    Experimental::DoubleDoubleComplex const& val) {
  Experimental::DoubleDoubleComplex old;
  old.re = Kokkos::atomic_fetch_add(&dest->re, val.re);
  old.im = Kokkos::atomic_fetch_add(&dest->im, val.im);
  return old;
}
}  // namespace Kokkos
