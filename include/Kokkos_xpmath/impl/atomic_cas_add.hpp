// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// ============================================================
// Kokkos::atomic_add for multi-word xpmath types — CAS helper
// ============================================================
//
// DETERMINISM (read this before filing a bug):
// Atomic accumulation into a non-associative multi-word type is
// order-dependent. Concurrent atomic_add into the same location may produce
// last-limb differences across runs. That is a property of the operation, not
// a defect in the CAS loop or the core arithmetic. Prefer bit-identity checks
// only under a deterministic execution space (Serial) or on exactly
// representable integer sums where every association agrees.
//
// Implementation:
//   Compare-and-swap over a byte-equal integer view of the whole expansion.
//   The integer view is obtained with std::memcpy into a POD of uint32_t limbs
//   (never by reading a float/double glvalue as an integer, which would violate
//   strict aliasing). The CAS itself is Kokkos::atomic_compare_exchange on
//   that POD: hardware CAS when desul considers the width lock-free, otherwise
//   desul's address-locked CAS (needed for 12-byte TripleFloat, and for
//   8/16-byte types whose storage alignment is below the native atomic align).
//
// Complex types are handled componentwise by the public overloads (atomic_add
// on re, then on im), each of which uses this helper on the real expansion.

#pragma once

#include <Kokkos_Core.hpp>

#include <cstdint>
#include <cstring>

namespace Kokkos {
namespace Impl {
namespace xpmath_atomic {

// Byte-identical integer view of an N-byte object, as N/4 little-endian
// uint32_t words. Size and alignment match the multi-word float/double
// expansions (align 4 or 8); we deliberately do not over-align to 16 so a
// View element of QF/FF is a valid object to CAS through this type.
template <std::size_t NBytes>
struct AtomicWord {
  static_assert(NBytes % sizeof(std::uint32_t) == 0,
                "xpmath atomic word size must be a multiple of 4");
  static constexpr std::size_t NWords = NBytes / sizeof(std::uint32_t);
  std::uint32_t w[NWords];

  KOKKOS_FORCEINLINE_FUNCTION bool operator==(AtomicWord const& o) const {
    for (std::size_t i = 0; i < NWords; ++i) {
      if (w[i] != o.w[i]) return false;
    }
    return true;
  }
  KOKKOS_FORCEINLINE_FUNCTION bool operator!=(AtomicWord const& o) const {
    return !(*this == o);
  }
};

template <class T>
KOKKOS_FORCEINLINE_FUNCTION AtomicWord<sizeof(T)> to_bits(T const& v) {
  AtomicWord<sizeof(T)> bits{};
  // memcpy: type-pun without a strict-aliasing violation.
  std::memcpy(&bits, &v, sizeof(T));
  return bits;
}

template <class T>
KOKKOS_FORCEINLINE_FUNCTION T from_bits(AtomicWord<sizeof(T)> const& bits) {
  T v;
  std::memcpy(&v, &bits, sizeof(T));
  return v;
}

// CAS loop: read bits → add in T → CAS bits. Returns the value present before
// this thread's successful update (atomic_fetch_add convention).
template <class T>
KOKKOS_INLINE_FUNCTION T atomic_fetch_add_cas(T* dest, T const& val) {
  // T has user-defined copy/assign in the core headers, so it is not
  // std::is_trivially_copyable, but its object representation is a plain
  // float/double limb array — memcpy of sizeof(T) bytes is the intended,
  // strict-aliasing-safe bit transfer (same bytes desul would CAS).
  using Bits = AtomicWord<sizeof(T)>;
  static_assert(sizeof(Bits) == sizeof(T), "atomic word size mismatch");

  // Storage overlay: dest is a T; CAS operates on the same bytes as Bits.
  // Obtaining *values* always goes through memcpy (to_bits/from_bits); the
  // pointer cast exists only so Kokkos/desul can CAS those bytes.
  Bits* const bdest =
      reinterpret_cast<Bits*>(reinterpret_cast<void*>(dest));

  Bits assumed = to_bits(*dest);
  Bits oldv    = assumed;
  for (;;) {
    assumed     = oldv;
    T const cur = from_bits<T>(assumed);
    Bits const desired = to_bits(cur + val);
    oldv = Kokkos::atomic_compare_exchange(bdest, assumed, desired);
    if (oldv == assumed) {
      return cur;
    }
  }
}

template <class T>
KOKKOS_INLINE_FUNCTION void atomic_add_cas(T* dest, T const& val) {
  (void)atomic_fetch_add_cas(dest, val);
}

}  // namespace xpmath_atomic
}  // namespace Impl
}  // namespace Kokkos
