// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// Adoption example: swap precision by changing two aliases; keep calling
// Kokkos::exp / Kokkos::sqrt / … the same way. This is not a timing demo and
// does not print accuracy columns. Bit-identity lives in tests/; this TU only
// shows the call-site spelling.
//
// ---------------------------------------------------------------------------
// Precision is selected by aliases. The three common spellings side by side:
//
//   // Kokkos::complex<double> (what this replaces)
//   using real_t    = double;
//   using complex_t = Kokkos::complex<real_t>;
//
//   // host-only Kokkos + libquadmath (what a quadmath user would have written)
//   using real_t    = __float128;
//   using complex_t = Kokkos::complex<__float128>;   // or __complex128
//
//   // this repository (supported spelling — K4)
//   using real_t    = Kokkos::Experimental::DoubleDouble;
//   using complex_t = Kokkos::Experimental::DoubleDoubleComplex;
//
// Kokkos::complex<DoubleDouble> does not instantiate; see docs/COMPLEX_INTEROP.md.
//
// Three call-site differences that do not survive the swap:
//   (1) no conversion to or from std::complex<double>;
//   (2) real() / imag() return a copy — writes go to z.re / z.im;
//   (3) DoubleDoubleComplex is 32 bytes, not the 16 of Kokkos::complex<double>.
// ---------------------------------------------------------------------------

#include <Kokkos_Core.hpp>
#include <Kokkos_xpmath/dd_complex.hpp>

#include <cstdio>

using real_t    = Kokkos::Experimental::DoubleDouble;
using complex_t = Kokkos::Experimental::DoubleDoubleComplex;

int main(int argc, char* argv[]) {
  Kokkos::initialize(argc, argv);
  {
    Kokkos::View<real_t*> r("r", 1);
    Kokkos::View<complex_t*> z("z", 1);

    Kokkos::parallel_for(
        "type_swap_kernel", 1, KOKKOS_LAMBDA(int) {
          real_t x(0.5);
          r(0) = Kokkos::sqrt(Kokkos::exp(x));

          complex_t c(real_t(0.25), real_t(0.125));
          // Complex op through the same Kokkos:: spelling.
          c = Kokkos::exp(c);
          // Writes must target .re / .im (real()/imag() return a copy).
          c.re = Kokkos::real(c);
          c.im = Kokkos::imag(c);
          z(0) = c;
        });
    Kokkos::fence();

    // Touch host mirrors so the Views cannot be DCE'd. No accuracy columns.
    auto r_h = Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace{}, r);
    auto z_h = Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace{}, z);
    (void)r_h;
    (void)z_h;

    static_assert(sizeof(complex_t) == 32,
                  "DoubleDoubleComplex is two DoubleDoubles (32 bytes)");
    std::printf("type_swap_smoke: OK (sizeof(complex_t)=%zu)\n",
                sizeof(complex_t));
  }
  Kokkos::finalize();
  return 0;
}
