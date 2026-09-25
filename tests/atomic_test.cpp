// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// K3: Kokkos::atomic_add bit-identity / lost-update gate for the eight wrappers.
//
// Serial execution space:
//   N threads each Kokkos::atomic_add a known value into one View element.
//   Result must equal the same left-fold performed serially through the core
//   xp:: type — bit-identity (no ulps, no tolerances).
//
// Exactly representable integers:
//   Accumulate integer values that every association yields identically.
//   Bit-exact in every space exercised. Separates lost updates from rounding
//   reorder; the general non-associative case cannot make that distinction.
//
// Device spaces (CUDA/HIP/SYCL), if ever exercised here: do NOT require
// bit-identity against the serial fold for the general (non-integer) case.
// Non-associative multi-word arithmetic plus nondeterministic atomic order
// only permit a finiteness / re-association envelope. This file currently
// runs Serial only; that distinction is stated so a future reader does not
// "fix" a device assertion into a false bit-identity gate. The
// exactly-representable-integer case remains bit-exact in every space.

#include <Kokkos_xpmath/Kokkos_xpmath.hpp>

#include <cstdio>
#include <cstdlib>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
      ++g_failures;                                                            \
    }                                                                          \
  } while (0)

// General scatter-add: N atomic_adds of a fixed non-integer value; Serial
// order matches a serial left-fold bit-for-bit.
template <class WrapT, class CoreT>
void check_atomic_serial_bitid(char const* label, int N, WrapT addend) {
  using exec_space = Kokkos::Serial;

  Kokkos::View<WrapT*, exec_space> cell("cell", 1);
  {
    auto h0 = Kokkos::create_mirror_view(cell);
    h0(0) = WrapT(0.0);
    Kokkos::deep_copy(cell, h0);
  }

  Kokkos::parallel_for(
      "atomic_add", Kokkos::RangePolicy<exec_space>(0, N),
      KOKKOS_LAMBDA(int) { Kokkos::atomic_add(&cell(0), addend); });
  Kokkos::fence();

  auto h = Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace(), cell);

  CoreT serial(0.0);
  CoreT const core_addend = static_cast<CoreT>(addend);
  for (int i = 0; i < N; ++i) {
    serial += core_addend;
  }

  CoreT const got = static_cast<CoreT>(h(0));
  if (!(got == serial)) {
    std::fprintf(stderr, "FAIL bit-identity %s N=%d\n", label, N);
    ++g_failures;
  }
}

// Exactly representable integers: any order gives the same answer. Bit-exact
// check that no updates were lost.
template <class WrapT, class CoreT>
void check_atomic_exact_int(char const* label, int N) {
  using exec_space = Kokkos::Serial;

  // addend = 1 (exact in every limb format); sum = N (exact for these N).
  WrapT const one(1.0);

  Kokkos::View<WrapT*, exec_space> cell("cell_int", 1);
  {
    auto h0 = Kokkos::create_mirror_view(cell);
    h0(0) = WrapT(0.0);
    Kokkos::deep_copy(cell, h0);
  }

  Kokkos::parallel_for(
      "atomic_add_int", Kokkos::RangePolicy<exec_space>(0, N),
      KOKKOS_LAMBDA(int) { Kokkos::atomic_add(&cell(0), one); });
  Kokkos::fence();

  auto h = Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace(), cell);

  CoreT const expect = CoreT(static_cast<double>(N));
  CoreT const got    = static_cast<CoreT>(h(0));
  if (!(got == expect)) {
    std::fprintf(stderr, "FAIL exact-int (lost update?) %s N=%d\n", label, N);
    ++g_failures;
  }
}

template <class WrapT, class CoreT>
void run_real(char const* name) {
  // Non-integer addend exercises the full expansion / renormalization path.
  WrapT const addend(0.1);
  static int const Ns[] = {1, 2, 128, 1024};
  for (int N : Ns) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s/serial", name);
    check_atomic_serial_bitid<WrapT, CoreT>(buf, N, addend);
    std::snprintf(buf, sizeof(buf), "%s/exact_int", name);
    check_atomic_exact_int<WrapT, CoreT>(buf, N);
  }
}

template <class WrapT, class CoreT, class RealWrapT>
void run_complex(char const* name) {
  // Componentwise: non-zero re and im.
  WrapT const addend(RealWrapT(0.1), RealWrapT(0.2));
  static int const Ns[] = {1, 2, 128, 1024};
  for (int N : Ns) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s/serial", name);
    check_atomic_serial_bitid<WrapT, CoreT>(buf, N, addend);
    std::snprintf(buf, sizeof(buf), "%s/exact_int", name);
    check_atomic_exact_int<WrapT, CoreT>(buf, N);
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  Kokkos::initialize(argc, argv);
  {
    run_real<Kokkos::Experimental::DoubleDouble, xp::DoubleDouble>("dd");
    run_real<Kokkos::Experimental::FloatFloat, xp::FloatFloat>("ff");
    run_real<Kokkos::Experimental::QuadFloat, xp::QuadFloat>("qf");
    run_real<Kokkos::Experimental::TripleFloat, xp::TripleFloat>("tf");

    run_complex<Kokkos::Experimental::DoubleDoubleComplex,
                xp::DoubleDoubleComplex, Kokkos::Experimental::DoubleDouble>(
        "ddc");
    run_complex<Kokkos::Experimental::FloatFloatComplex, xp::FloatFloatComplex,
                Kokkos::Experimental::FloatFloat>("ffc");
    run_complex<Kokkos::Experimental::QuadFloatComplex, xp::QuadFloatComplex,
                Kokkos::Experimental::QuadFloat>("qfc");
    run_complex<Kokkos::Experimental::TripleFloatComplex,
                xp::TripleFloatComplex, Kokkos::Experimental::TripleFloat>(
        "tfc");
  }
  Kokkos::finalize();

  if (g_failures != 0) {
    std::fprintf(stderr, "%d failure(s)\n", g_failures);
    return 1;
  }
  std::printf(
      "atomic_test: OK (8 types × {serial bit-id, exact-int} × N={1,2,128,1024})\n");
  return 0;
}
